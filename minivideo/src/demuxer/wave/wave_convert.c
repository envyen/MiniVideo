/*!
 * COPYRIGHT (C) 2016 Emeric Grange - All Rights Reserved
 *
 * This file is part of MiniVideo.
 *
 * MiniVideo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MiniVideo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with MiniVideo.  If not, see <http://www.gnu.org/licenses/>.
 *
 * \file      wave_convert.c
 * \author    Emeric Grange <emeric.grange@gmail.com>
 * \date      2016
 */

// minivideo headers
#include "wave.h"
#include "wave_struct.h"
#include "../riff/riff.h"
#include "../riff/riff_struct.h"
#include "../xml_mapper.h"
#include "../../utils.h"
#include "../../bitstream.h"
#include "../../bitstream_utils.h"
#include "../../typedef.h"
#include "../../twocc.h"
#include "../../fourcc.h"
#include "../../minitraces.h"

// C standard libraries
#include <stdio.h>
#include <string.h>

/* ************************************************************************** */

int wave_indexer_initmap(MediaFile_t *media, wave_t *wave)
{
    int retcode = SUCCESS;
    uint64_t pcm_samples_count = 0;

    // Init a bitstreamMap_t for each wave track
    if (wave->fmt.wFormatTag == WAVE_FORMAT_MS_PCM ||
        wave->fmt.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
        pcm_samples_count = (wave->data.datasSize) / (wave->fmt.nChannels * (wave->fmt.wBitsPerSample / 8));
        if (pcm_samples_count > UINT32_MAX)
            pcm_samples_count = UINT32_MAX;

        retcode = init_bitstream_map(&media->tracks_audio[0], (uint32_t)pcm_samples_count);
    }
    else
    {
        retcode = init_bitstream_map(&media->tracks_audio[0], 1);
    }

    if (retcode == SUCCESS)
    {
        BitstreamMap_t *track = media->tracks_audio[media->tracks_audio_count];

        track->stream_type  = stream_AUDIO;
        track->stream_codec = getCodecFromTwoCC(wave->fmt.wFormatTag);

        if (wave->fmt.wFormatTag == WAVE_FORMAT_MS_PCM ||
            wave->fmt.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        {
            track->stream_codec = CODEC_LPCM;

            if (wave->fact.dwSampleLength)
            {
                track->stream_size = wave->fact.dwSampleLength * (wave->fmt.wBitsPerSample*8) * wave->fmt.nChannels;

                if (track->stream_size != (uint64_t)wave->data.datasSize)
                    TRACE_WARNING(WAV, "track->stream_size != wave->data.datasSize (%d vs %d)",
                                  track->stream_size, wave->data.datasSize);

                if (wave->fmt.nSamplesPerSec)
                    track->duration_ms = wave->fact.dwSampleLength * (1000.0 / (double)(wave->fmt.nSamplesPerSec));
            }
            else
            {
                track->stream_size = wave->data.datasSize; // may not be necessary

                if (wave->fmt.wBitsPerSample)
                {
                    track->duration_ms = wave->fmt.nSamplesPerSec * (wave->fmt.wBitsPerSample/8) * wave->fmt.nChannels;
                    //track->duration = wave->fmt.nSamplesPerSec / (wave->fmt.nSamplesPerSec * wave->fmt.nChannels * (wave->fmt.wBitsPerSample/8));
                }
            }

            track->bitrate = wave->fmt.nSamplesPerSec * wave->fmt.wBitsPerSample * wave->fmt.nChannels;
            track->bitrate_mode = BITRATE_CBR;

            // PCM specific metadatas
            track->pcm_sample_format = 0;
            track->pcm_sample_size = 0;
            track->pcm_sample_endianness = 0;
        }

        // backup computations
        {
            if (track->duration_ms == 0 && wave->fmt.nAvgBytesPerSec)
            {
                track->duration_ms = ((double)wave->data.datasSize / (double)wave->fmt.nAvgBytesPerSec) * 1000.0;
            }

            if (track->stream_size == 0)
            {
                track->stream_size = wave->data.datasSize;
            }
        }

        track->channel_count = wave->fmt.nChannels;
        track->sampling_rate = wave->fmt.nSamplesPerSec;
        track->bit_per_sample = wave->fmt.wBitsPerSample;

        // SAMPLES
        if (track->stream_codec == CODEC_LPCM)
        {
            track->sample_alignment = true;

            uint64_t sid = 0;
            uint32_t pcm_frame_size = media->tracks_audio[0]->channel_count * (media->tracks_audio[0]->bit_per_sample / 8);
            double pcm_frame_tick_ns = (1000000.0 / (double)track->sampling_rate);

            for (int64_t i = 0; i < wave->data.datasSize; i += pcm_frame_size)
            {
                // Set PCM frame into the bitstream_map
                sid = media->tracks_audio[0]->sample_count;
                if (sid < pcm_samples_count)
                {
                    media->tracks_audio[0]->sample_type[sid] = sample_AUDIO;
                    media->tracks_audio[0]->sample_size[sid] = pcm_frame_size;
                    media->tracks_audio[0]->sample_offset[sid] = wave->data.datasOffset + i;
                    media->tracks_audio[0]->sample_pts[sid] = (int64_t)(sid * pcm_frame_tick_ns);
                    media->tracks_audio[0]->sample_dts[sid] = 0;
                    media->tracks_audio[0]->sample_count++;
                }
            }

            track->duration_ms = (int64_t)((double)media->tracks_audio[0]->sample_count * (1000.0 / (double)track->sampling_rate));
        }
        else
        {
            track->sample_alignment = false;
            track->sample_count = track->frame_count_idr = 1;
            track->bitrate_mode = BITRATE_UNKNOWN;

            track->sample_type[0] = sample_RAW;
            track->sample_size[0] = wave->data.datasSize;
            track->sample_offset[0] = wave->data.datasOffset;
            track->sample_pts[0] = 0;
            track->sample_dts[0] = 0;
        }
    }

    return retcode;
}

/* ************************************************************************** */

int wave_indexer(Bitstream_t *bitstr, MediaFile_t *media, wave_t *wave)
{
    TRACE_INFO(WAV, BLD_GREEN "wave_indexer()" CLR_RESET);
    int retcode = SUCCESS;

    // Convert index into a bitstream map
    retcode = wave_indexer_initmap(media, wave);

    if (retcode == SUCCESS && media->tracks_audio[0])
    {
        media->tracks_audio_count = 1;
        media->duration = media->tracks_audio[0]->duration_ms;
    }

    return retcode;
}

/* ************************************************************************** */
