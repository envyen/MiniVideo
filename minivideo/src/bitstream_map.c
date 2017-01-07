/*!
 * COPYRIGHT (C) 2012 Emeric Grange - All Rights Reserved
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
 * \file      bitstream_map.c
 * \author    Emeric Grange <emeric.grange@gmail.com>
 * \date      2012
 */

// minivideo headers
#include "bitstream.h"
#include "avcodecs.h"
#include "fourcc.h"
#include "minitraces.h"

// C standard libraries
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ************************************************************************** */

/*!
 * \brief Initialize a bitstream_map structure with a fixed number of entries.
 * \param bitstream_map: The address of the pointer to the bitstreamMap_t structure to initialize.
 * \param entries: The number of sample to init into the bitstreamMap_t structure.
 * \return 1 if succeed, 0 otherwise.
 *
 * Everything inside the bitstreamMap_t structure is set to 0, even the number
 * of entries (sample_count).
 */
int init_bitstream_map(BitstreamMap_t **bitstream_map, uint32_t entries)
{
    TRACE_INFO(DEMUX, "<b> " BLD_BLUE "init_bitstream_map()" CLR_RESET);
    int retcode = SUCCESS;

    if (*bitstream_map != NULL)
    {
        TRACE_ERROR(DEMUX, "<b> Unable to alloc a new bitstream_map: not null!");
        retcode = FAILURE;
    }
    else
    {
        if (entries == 0)
        {
            TRACE_ERROR(DEMUX, "<b> Unable to allocate a new bitstream_map: no entries to allocate!");
            retcode = FAILURE;
        }
        else
        {
            *bitstream_map = (BitstreamMap_t*)calloc(1, sizeof(BitstreamMap_t));

            if (*bitstream_map == NULL)
            {
                TRACE_ERROR(DEMUX, "<b> Unable to allocate a new bitstream_map!");
                retcode = FAILURE;
            }
            else
            {
                (*bitstream_map)->sample_type = (uint32_t*)calloc(entries, sizeof(uint32_t));
                (*bitstream_map)->sample_size = (uint32_t*)calloc(entries, sizeof(uint32_t));
                (*bitstream_map)->sample_offset = (int64_t*)calloc(entries, sizeof(int64_t));
                (*bitstream_map)->sample_pts = (int64_t*)calloc(entries, sizeof(int64_t));
                (*bitstream_map)->sample_dts = (int64_t*)calloc(entries, sizeof(int64_t));

                if ((*bitstream_map)->sample_type == NULL ||
                    (*bitstream_map)->sample_size == NULL ||
                    (*bitstream_map)->sample_offset == NULL ||
                    (*bitstream_map)->sample_pts == NULL ||
                    (*bitstream_map)->sample_dts == NULL)
                {
                    TRACE_ERROR(DEMUX, "<b> Unable to alloc bitstream_map > sample_type / sample_size / sample_offset / sample_timecode!");

                    if ((*bitstream_map)->sample_type != NULL)
                        free((*bitstream_map)->sample_type);
                    if ((*bitstream_map)->sample_size != NULL)
                        free((*bitstream_map)->sample_size);
                    if ((*bitstream_map)->sample_offset != NULL)
                        free((*bitstream_map)->sample_offset);
                    if ((*bitstream_map)->sample_pts != NULL)
                        free((*bitstream_map)->sample_pts);
                    if ((*bitstream_map)->sample_dts != NULL)
                        free((*bitstream_map)->sample_dts);

                    free(*bitstream_map);
                    *bitstream_map = NULL;
                    retcode = FAILURE;
                }
            }
        }
    }

    return retcode;
}

/* ************************************************************************** */

/*!
 * \brief Destroy a bitstream_map structure.
 * \param *bitstream_map A pointer to a *bitstreamMap_t structure.
 */
void free_bitstream_map(BitstreamMap_t **bitstream_map)
{
    if ((*bitstream_map) != NULL)
    {
        TRACE_INFO(DEMUX, "<b> " BLD_BLUE "free_bitstream_map()" CLR_RESET);

        // Textual metadatas
        free((*bitstream_map)->stream_encoder);
        (*bitstream_map)->stream_encoder = NULL;
        free((*bitstream_map)->track_title);
        (*bitstream_map)->track_title = NULL;
        free((*bitstream_map)->track_languagecode);
        (*bitstream_map)->track_languagecode = NULL;
        free((*bitstream_map)->subtitles_name);
        (*bitstream_map)->subtitles_name = NULL;

        // Sample tables
        free((*bitstream_map)->sample_type);
        (*bitstream_map)->sample_type = NULL;

        free((*bitstream_map)->sample_size);
        (*bitstream_map)->sample_size = NULL;

        free((*bitstream_map)->sample_offset);
        (*bitstream_map)->sample_offset = NULL;

        free((*bitstream_map)->sample_pts);
        (*bitstream_map)->sample_pts = NULL;

        free((*bitstream_map)->sample_dts);
        (*bitstream_map)->sample_dts = NULL;


        free(*bitstream_map);
        *bitstream_map = NULL;

        TRACE_1(DEMUX, "<b> Bitstream_map freed");
    }
}

/* ************************************************************************** */
/* ************************************************************************** */

/*!
 * \brief Print the content of a bitstreamMap_t structure.
 * \param bitstream_map docme.
 */
void print_bitstream_map(BitstreamMap_t *bitstream_map)
{
#if ENABLE_DEBUG

    if (bitstream_map == NULL)
    {
        TRACE_ERROR(DEMUX, "Invalid bitstream_map structure!");
    }
    else
    {
        TRACE_INFO(DEMUX, BLD_GREEN "print_bitstream_map()" CLR_RESET);

        if (bitstream_map->stream_type == stream_VIDEO &&
            bitstream_map->sample_count > 0)
        {
            TRACE_INFO(DEMUX, "Elementary stream type > VIDEO");
        }
        else if (bitstream_map->stream_type == stream_AUDIO &&
                 bitstream_map->sample_count > 0)
        {
            TRACE_INFO(DEMUX, "Elementary stream type > AUDIO");
        }
        else
        {
            TRACE_WARNING(DEMUX, "Unknown elementary stream type!");
        }

        TRACE_1(DEMUX, "Track codec:     '%s'", getCodecString(bitstream_map->stream_type, bitstream_map->stream_codec, true));

        TRACE_INFO(DEMUX, "> samples alignment: %i", bitstream_map->sample_alignment);
        TRACE_INFO(DEMUX, "> samples count    : %i", bitstream_map->sample_count);
        TRACE_INFO(DEMUX, "> IDR samples count: %i", bitstream_map->frame_count_idr);

        if (bitstream_map->sample_count > 0)
        {
            TRACE_1(DEMUX, "SAMPLES");
            for (unsigned  i = 0; i < bitstream_map->sample_count; i++)
            {
                TRACE_1(DEMUX, "> sample_type      : %i", bitstream_map->sample_type[i]);
                TRACE_1(DEMUX, "  | sample_offset  : %i", bitstream_map->sample_offset[i]);
                TRACE_1(DEMUX, "  | sample_size    : %i", bitstream_map->sample_size[i]);
                TRACE_1(DEMUX, "  | sample_timecode: %i", bitstream_map->sample_pts[i]);
            }
        }
    }

#endif // ENABLE_DEBUG
}

/* ************************************************************************** */
/* ************************************************************************** */

static void computeSamplesDatasTrack(BitstreamMap_t *track)
{
    if (track)
    {
        uint64_t totalbytes = 0;
        bool cbr = true;
        int64_t frameinterval = 0;
        bool cfr = true;
        unsigned j = 0;

        if (track->sample_alignment)
        {
            track->frame_count = track->sample_count;
        }
        if (track->stream_intracoded)
        {
            track->frame_count_idr = track->frame_count;
        }

        // Audio frame duration
        if (track->stream_type == stream_AUDIO)
        {
            if (track->sample_dts && track->sample_count >= 2)
            {
                track->frame_duration = track->sample_dts[1] - track->sample_dts[0];
                track->frame_duration /= 1000; // ns to  ms
            }
        }
        // Video frame duration
        if (track->stream_type == stream_VIDEO && track->frame_duration == 0 && track->framerate != 0)
        {
            track->frame_duration = 1000.0 / track->framerate;
        }

        // Video frame interval
        if (track->stream_type == stream_VIDEO)
        {
            // FIXME this is not reliable whenever using B frames
            if (track->sample_dts && track->sample_count >= 2)
                frameinterval = track->sample_dts[1] - track->sample_dts[0];
        }

        // Iterate on each sample
        for (j = 0; j < track->sample_count; j++)
        {
            totalbytes += track->sample_size[j];

            if (track->sample_size[j] > (track->sample_size[10] + 1) || track->sample_size[j] < (track->sample_size[10] - 1))
            {
                cbr = false; // TODO find a reference // TODO not use TAGS
            }

            if (track->sample_alignment == false)
            {
                if (track->sample_pts[j] || track->sample_dts[j])
                    track->frame_count++;
            }
/*
            if (j > 0)
            {
                // FIXME this is not reliable whenever using B frames
                if ((t->sample_dts[j] - t->sample_dts[j - 1]) != frameinterval)
                {
                    cfr = false;
                    TRACE_ERROR(DEMUX, "dts F: %lli != %lli (j: %u)", (t->sample_dts[j] - t->sample_dts[j - 1]), frameinterval, j);
                    TRACE_ERROR(DEMUX, "dts F: %lli !=  %lli ", (t->sample_pts[j] - t->sample_pts[j - 1]), frameinterval);
                }
            }
*/
        }

        // Set bitrate mode
        if (track->bitrate_mode == BITRATE_UNKNOWN)
        {
            if (cbr == true)
            {
                track->bitrate_mode = BITRATE_CBR;
            }
            else
            {
                // TODO check if we have AVBR / CVBR ?
                track->bitrate_mode = BITRATE_VBR;
            }
        }
/*
        // Set framerate mode
        if (t->framerate_mode == FRAMERATE_UNKNOWN)
        {
            if (cfr == true)
            {
                t->framerate_mode = FRAMERATE_CFR;
            }
            else
            {
                t->framerate_mode = FRAMERATE_VFR;
            }
        }
*/
        // Set stream size
        if (track->stream_size == 0)
        {
            track->stream_size = totalbytes;
        }

        // Set stream duration
        if (track->duration_ms == 0)
        {
            track->duration_ms = (double)track->frame_count * track->frame_duration;
        }

        // Set gross bitrate value (in bps)
        if (track->bitrate == 0 && track->duration_ms != 0)
        {
            track->bitrate = (unsigned int)round(((double)track->stream_size / (double)(track->duration_ms)));
            track->bitrate *= 1000; // ms to s
            track->bitrate *= 8; // B to b
        }
    }
}

/* ************************************************************************** */
/* ************************************************************************** */

/*!
 * \brief PCM sample size hack
 *
 * PCM sample size can be recomputed if the informations gathered from the
 * containers seems wrong (like the sample size). This will also trigger a new
 * bitrate computation.
 */
bool computePCMSettings(BitstreamMap_t *track)
{
    bool retcode = SUCCESS;
    uint32_t sample_size_cbr = track->channel_count * (track->bit_per_sample / 8);

    // First, check if the hack is needed
    if (track->sample_count > 0 && track->sample_size[0] != sample_size_cbr)
    {
        TRACE_ERROR(DEMUX, BLD_GREEN "computePCMSettings()" CLR_RESET);

        track->sample_per_frames = 1;
        track->stream_size = track->sample_count * sample_size_cbr;
        track->bitrate = 0; // reset bitrate

        for (unsigned i = 0; i < track->sample_count; i++)
        {
            track->sample_size[i] = sample_size_cbr;
        }
    }

    return retcode;
}

bool computeCodecs(MediaFile_t *media)
{
    TRACE_INFO(DEMUX, BLD_GREEN "computeCodecs()" CLR_RESET);
    bool retcode = SUCCESS;

    for (unsigned i = 0; i < media->tracks_video_count; i++)
    {
        if (media->tracks_video[i] && media->tracks_video[i]->stream_codec == CODEC_UNKNOWN)
        {
             media->tracks_video[i]->stream_codec = getCodecFromFourCC(media->tracks_video[i]->stream_fcc);
        }
    }

    for (unsigned i = 0; i < media->tracks_audio_count; i++)
    {
        if (media->tracks_audio[i] && media->tracks_audio[i]->stream_codec == CODEC_UNKNOWN)
        {
            media->tracks_audio[i]->stream_codec = getCodecFromFourCC(media->tracks_audio[i]->stream_fcc);
        }

        // PCM hack
        if (media->tracks_audio[i]->stream_codec == CODEC_LPCM ||
            media->tracks_audio[i]->stream_codec == CODEC_LogPCM ||
            media->tracks_audio[i]->stream_codec == CODEC_DPCM ||
            media->tracks_audio[i]->stream_codec == CODEC_ADPCM)
        {
            computePCMSettings(media->tracks_audio[i]);
        }
    }

    return retcode;
}

/* ************************************************************************** */

bool computeAspectRatios(MediaFile_t *media)
{
    TRACE_INFO(DEMUX, BLD_GREEN "computeAspectRatios()" CLR_RESET);
    bool retcode = SUCCESS;

    for (unsigned i = 0; i < media->tracks_video_count; i++)
    {
        BitstreamMap_t *t = media->tracks_video[i];
        if (t)
        {
            // First pass on PAR (if set by the container)
             if (t->pixel_aspect_ratio_h && t->pixel_aspect_ratio_v)
             {
                 t->pixel_aspect_ratio = (double)t->pixel_aspect_ratio_h / (double)t->pixel_aspect_ratio_v;
             }
             else
             {
                 t->pixel_aspect_ratio = 1.0;
                 t->pixel_aspect_ratio_h = 1;
                 t->pixel_aspect_ratio_v = 1;
             }

             if (t->video_aspect_ratio_h && t->video_aspect_ratio_v)
             {
                 // First pass on PAR (if set by the container)
                 t->video_aspect_ratio = (double)t->video_aspect_ratio_h / (double)t->video_aspect_ratio_v;
             }
             else if (t->width && t->height)
             {
                 // First pass on PAR (if computer from video resolution)
                 t->video_aspect_ratio = (double)t->width / (double)t->height,
                 t->video_aspect_ratio_h = t->width;
                 t->video_aspect_ratio_v = t->height;
             }

             // Compute display aspect ratio
             if (t->display_aspect_ratio_h && t->display_aspect_ratio_v)
             {
                 t->display_aspect_ratio = (double)t->display_aspect_ratio_h / (double)t->display_aspect_ratio_v;
             }
             else
             {
                 if (t->pixel_aspect_ratio != 1.0)
                 {
                     t->display_aspect_ratio = t->video_aspect_ratio * t->pixel_aspect_ratio;
                 }
                 else
                 {
                     t->display_aspect_ratio = t->video_aspect_ratio;
                 }
             }

             // Second pass on PAR
             if (t->pixel_aspect_ratio == 1.0 &&
                 (t->video_aspect_ratio != t->display_aspect_ratio))
             {
                 //
             }
        }
    }

    for (unsigned i = 0; i < media->tracks_audio_count; i++)
    {
        //
    }

    return retcode;
}

/* ************************************************************************** */

bool computeSamplesDatas(MediaFile_t *media)
{
    TRACE_INFO(DEMUX, BLD_GREEN "computeSamplesDatas()" CLR_RESET);
    bool retcode = SUCCESS;

    for (unsigned i = 0; i < media->tracks_video_count; i++)
    {
        if (media->tracks_video[i])
        {
            computeSamplesDatasTrack(media->tracks_video[i]);
        }
    }

    for (unsigned i = 0; i < media->tracks_audio_count; i++)
    {
        if (media->tracks_audio[i])
        {
            computeSamplesDatasTrack(media->tracks_audio[i]);
        }
    }

    return retcode;
}

/* ************************************************************************** */

uint64_t computeTrackMemory(BitstreamMap_t *t)
{
    uint64_t mem = 0;

    if (t)
    {
        mem += sizeof(*t);

        if (t->stream_encoder) mem += strlen(t->stream_encoder);
        if (t->track_title) mem += strlen(t->track_title);
        if (t->track_languagecode) mem += strlen(t->track_languagecode);
        if (t->subtitles_name) mem += strlen(t->subtitles_name);

        mem += t->sample_count * (4 + 4 + 8 + 8 + 8);
    }
    TRACE_1(DEMUX, "track(x): %u B\n", mem);

    return mem;
}

bool computeMediaMemory(MediaFile_t *media)
{
    TRACE_INFO(DEMUX, BLD_GREEN "computeMediaMemory()" CLR_RESET);
    bool retcode = SUCCESS;

    uint64_t mem = 0;

    mem += sizeof(*media);
    if (media->creation_app)
    {
        mem += strlen(media->creation_app);
    }

    for (unsigned i = 0; i < media->tracks_video_count; i++)
    {
        mem += computeTrackMemory(media->tracks_video[i]);
    }

    for (unsigned i = 0; i < media->tracks_audio_count; i++)
    {
        mem += computeTrackMemory(media->tracks_audio[i]);
    }

    for (unsigned i = 0; i < media->tracks_subtitles_count; i++)
    {
        mem += computeTrackMemory(media->tracks_subt[i]);
    }

    for (unsigned i = 0; i < media->tracks_others_count; i++)
    {
        mem += computeTrackMemory(media->tracks_others[i]);
    }

    media->parsingMemory = mem;
    TRACE_INFO(DEMUX, "media parsing memory: %u B\n", mem);

    return retcode;
}

/* ************************************************************************** */
/* ************************************************************************** */
