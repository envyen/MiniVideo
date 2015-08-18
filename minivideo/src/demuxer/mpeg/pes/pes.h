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
 * \file      pes.h
 * \author    Emeric Grange <emeric.grange@gmail.com>
 * \date      2012
 */

#ifndef PARSER_MPEG_PES_H
#define PARSER_MPEG_PES_H

// minivideo headers
#include "../../../bitstream.h"
#include "pes_struct.h"

/* ************************************************************************** */

#define MARKER_BIT \
    if (read_bit(bitstr) != 1) { \
        TRACE_ERROR(MP4, "wrong 'marker_bit' (M1)\n"); \
        return FAILURE; \
    }

/* ************************************************************************** */

int parse_pes(Bitstream_t *bitstr, PesPacket_t *packet);
void print_pes(PesPacket_t *packet);

int parse_pes_padding(Bitstream_t *bitstr, PesPacket_t *packet);

int skip_pes(Bitstream_t *bitstr, PesPacket_t *pes);
int skip_pes_data(Bitstream_t *bitstr, PesPacket_t *pes);

/* ************************************************************************** */
#endif // PARSER_MPEG_PES_H
