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
 * \file      bitstream.c
 * \author    Emeric Grange <emeric.grange@gmail.com>
 * \date      2012
 */

// C standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// C POSIX libraries
#include <unistd.h>
#if ENABLE_LARGEFILE
    #include <sys/types.h>
#endif

// minivideo headers
#include "minitraces.h"

#include "bitstream.h"

/* ************************************************************************** */

/*!
 * \brief Init a new bitstream.
 * \param *video A pointer to a VideoFile_t structure, containing every informations available about the current video file.
 * \param *bitstream_map A pointer to a bitstreamMap_t structure, containing informations about video payload datas.
 * \return *bitstr A pointer to our newly allocated bitstream structure.
 *
 * Warning: this implementation only support file size up to 2GiB.
 *
 * If no bitstream_map is available, it mean we have continuous video data,
 * starting at byte offset 0 and until the end of file.
 * Otherwise, an available bitstream_map structure mean that we have encapsulated
 * video data, and we must bufferize data sample by sample.
 */
Bitstream_t *init_bitstream(VideoFile_t *video, BitstreamMap_t *bitstream_map)
{
    TRACE_INFO(BITS, "<b> " BLUE "init_bitstream()\n" RESET);
    Bitstream_t *bitstr = NULL;

    if (video == NULL ||
        video->file_pointer == NULL)
    {
        TRACE_ERROR(BITS, "<b> Unable to use VideoFile_t structure!\n");
    }
    else
    {
        // Bitstream structure allocation
        bitstr = (Bitstream_t*)calloc(1, sizeof(Bitstream_t));

        if (bitstr == NULL)
        {
            TRACE_ERROR(BITS, "<b> Unable to calloc bitstream unit!\n");
        }
        else
        {
            // Bitstream structure initialization
            bitstr->bitstream_file = video->file_pointer;

            // Use a bitstream_map if available
            if (bitstream_map != NULL)
                bitstr->bitstream_map = bitstream_map;
            else
                bitstr->bitstream_map = NULL;

            bitstr->bitstream_size = video->file_size;
            bitstr->bitstream_offset = 0;
            bitstr->bitstream_sample_index = 0;

            bitstr->buffer = NULL;
            bitstr->buffer_size = BITSTREAM_BUFFER_SIZE;
            bitstr->buffer_offset = 0;

            // Bitstream buffer allocation
            //FIXME use realloc(bitstr->buffer, bitstr->buffer_size + 4) to prevent some cases of invalid 32 bits reads near the end of the buffer
            //FIXME use realloc(bitstr->buffer, bitstr->buffer_size + 8) to prevent some cases of invalid 64 bits reads near the end of the buffer
            if ((bitstr->buffer = (uint8_t*)calloc(bitstr->buffer_size, sizeof(uint8_t))) == NULL)
            {
                TRACE_ERROR(BITS, "<b> Unable to calloc the bitstream buffer!\n");
                free(bitstr);
                bitstr = NULL;
            }
            else
            {
                // Initial buffer filling, only if we do not use a bitstream_map
                if (bitstream_map == NULL)
                {
                    // Rewind file pointer
                    rewind(bitstr->bitstream_file);

                    // Feed bitstream buffer
                    if (buffer_feed_dynamic(bitstr, -1) == FAILURE)
                    {
                        free(bitstr->buffer);
                        bitstr->buffer = NULL;
                        free(bitstr);
                        bitstr = NULL;
                    }
                }
            }
        }
    }

    if (bitstr != NULL)
    {
        TRACE_1(BITS, "<b> Bitstream initialization success\n");
    }
    else
    {
        TRACE_ERROR(BITS, "<b> Bitstream initialization FAILED!\n");
    }

    // Return the bitstream structure pointer
    return bitstr;
}

/* ************************************************************************** */

/*!
 * \brief Feed the bitstream buffer with fresh data.
 * \param *bitstr The bitstream to freed.
 * \return 1 if success, 0 otherwise.
 *
 * This function is only used by the H.264 video decoder.
 */
int buffer_feed_next_sample(Bitstream_t *bitstr)
{
    TRACE_INFO(BITS, "<b> " BLUE "buffer_feed_next_sample()\n" RESET);
    int retcode = SUCCESS;

    // Print stats?
    //TRACE_1(BITS, "<b> Status before reallocation:\n");
    //bitstream_print_stats(bitstr);

    // Check for premature end of file
    if (bitstr->bitstream_sample_index == bitstr->bitstream_map->sample_count)
    {
        if ((bitstr->buffer_size - bitstr->buffer_offset) < 8)
        {
            TRACE_ERROR(BITS, "<b> Fatal error: premature end of file reached!\n");
            retcode = FAILURE;
            exit(EXIT_FAILURE);
        }
        else
        {
            retcode = SUCCESS;
        }
    }

    if (retcode == SUCCESS)
    {
        // Reset parameters
        bitstr->buffer_offset = 0;
        bitstr->buffer_discarded_bytes = 0;

        // Read sample parameters
        bitstr->bitstream_offset = bitstr->bitstream_map->sample_offset[bitstr->bitstream_sample_index];
        bitstr->buffer_size = bitstr->bitstream_map->sample_size[bitstr->bitstream_sample_index];

        // Check bitstream_map->sample validity
        if (bitstr->bitstream_sample_index > bitstr->bitstream_map->sample_count ||
            bitstr->bitstream_offset <= 0 ||
            bitstr->buffer_size == 0)
        {
            TRACE_ERROR(BITS, "<b> Corrupted bitstream_map->sample[i] values!\n");
            retcode = FAILURE;
        }
        else
        {
            // Realloc buffer
            //FIXME use realloc(bitstr->buffer, bitstr->buffer_size + 4) to prevent some cases of invalid 32 bits reads near the end of the buffer
            //FIXME use realloc(bitstr->buffer, bitstr->buffer_size + 8) to prevent some cases of invalid 64 bits reads near the end of the buffer
            bitstr->buffer = realloc(bitstr->buffer, bitstr->buffer_size);

            if (bitstr->buffer == NULL)
            {
                TRACE_ERROR(BITS, "<b> Unable to realloc bitstream buffer!\n");
                retcode = FAILURE;
            }
            else
            {
                // Move file pointer
                if (fseek(bitstr->bitstream_file, bitstr->bitstream_offset, SEEK_SET) != 0)
                {
                    TRACE_ERROR(BITS, "<b> Unable to seek through the input file!\n");
                    retcode = FAILURE;
                }
                else
                {
                    // Feed buffer
                    if (fread(bitstr->buffer, sizeof(uint8_t), bitstr->buffer_size, bitstr->bitstream_file) != bitstr->buffer_size)
                    {
                        TRACE_ERROR(BITS, "<b> Unable to read from the input file!\n");
                        retcode = FAILURE;
                    }
                    else
                    {
                        TRACE_1(BITS, "<b> Bitstream buffer reallocation succeed!\n");
                        retcode = SUCCESS;
                    }
                }

                // Print stats?
                //TRACE_1(BITS, "<b> Status after reallocation:\n");
                //bitstream_print_stats(bitstr);

                // Update sample position
                bitstr->bitstream_sample_index++;
            }
        }
    }

    return retcode;
}

/* ************************************************************************** */

/*!
 * \brief Feed the bitstream buffer with fresh data.
 * \param *bitstr The bitstream to freed.
 * \param new_bitstream_offset The byte offset of the data we want to bufferize.
 * \return 1 if success, 0 otherwise.
 *
 * If the bitstream_offset is a negative value, just load the data following the
 * data from the current buffer.
 * Otherwise, load data starting at the given new_bitstream_offset.
 */
int buffer_feed_dynamic(Bitstream_t *bitstr, int64_t new_bitstream_offset)
{
    TRACE_INFO(BITS, "<b> " BLUE "buffer_feed_dynamic()\n" RESET);
    int retcode = FAILURE;

    // Print stats?
    //TRACE_1(BITS, "<b> Status before reallocation:\n");
    //bitstream_print_stats(bitstr);

    // Update current offset into the bitstream
    if (new_bitstream_offset >= 0)
    {
        bitstr->bitstream_offset = new_bitstream_offset;
        bitstr->buffer_offset = 0;
    }
    else
    {
        bitstr->bitstream_offset += (int64_t)(bitstr->buffer_offset/8) + bitstr->buffer_discarded_bytes;
        bitstr->buffer_offset %= 8;
    }

    // Check for premature end of file
    if (bitstr->bitstream_offset >= bitstr->bitstream_size)
    {
        TRACE_ERROR(BITS, "<b> Fatal error: premature end of file reached!\n");
        retcode = FAILURE;
        exit(EXIT_FAILURE);
    }
    else
    {
        // Reset buffer size (necessary if some data have been dynamically removed from previous buffer)
        bitstr->buffer_size = BITSTREAM_BUFFER_SIZE;
        bitstr->buffer_discarded_bytes = 0;

        // Resize buffer if end of file is almost reached
        if ((bitstr->bitstream_offset + bitstr->buffer_size) > bitstr->bitstream_size)
        {
            unsigned int buffer_size_saved = bitstr->buffer_size;
            bitstr->buffer_size = (unsigned int)(bitstr->bitstream_size - bitstr->bitstream_offset);
            bitstr->buffer = realloc(bitstr->buffer, bitstr->buffer_size);

            if (bitstr->buffer == NULL)
            {
                TRACE_ERROR(BITS, "<b> Unable to realloc bitstream buffer!\n");
                retcode = FAILURE;
            }
            else
            {
                TRACE_1(BITS, "<b> Bitstream buffer resized from %uB to %uB\n", buffer_size_saved, bitstr->buffer_size);
            }
        }

        // Move file pointer
        if (fseek(bitstr->bitstream_file, bitstr->bitstream_offset, SEEK_SET) != 0)
        {
            TRACE_ERROR(BITS, "<b> Unable to seek through the input file!\n");
            retcode = FAILURE;
        }
        else
        {
            // Feed buffer
            if (fread(bitstr->buffer, sizeof(uint8_t), bitstr->buffer_size, bitstr->bitstream_file) != bitstr->buffer_size)
            {
                TRACE_ERROR(BITS, "<b> Unable to read from the input file!\n");
                retcode = FAILURE;
            }
            else
            {
                TRACE_1(BITS, "<b> Bitstream buffer feeded!\n");
                retcode = SUCCESS;
            }

            // Print stats?
            //TRACE_1(BITS, "<b> Status after reallocation:\n");
            //bitstream_print_stats(bitstr);
        }
    }

    return retcode;
}

/* ************************************************************************** */

/*!
 * \brief Destroy a bitstream and it's buffer.
 * \param **bitstr_ptr The bitstream to freed.
 *
 * This function do not freed VideoFile_t structure.
 */
void free_bitstream(Bitstream_t **bitstr_ptr)
{
    TRACE_INFO(BITS, "<b> " BLUE "free_bitstream()\n" RESET);

    if (*bitstr_ptr != NULL)
    {
        if ((*bitstr_ptr)->buffer != NULL)
        {
            free((*bitstr_ptr)->buffer);
            (*bitstr_ptr)->buffer = NULL;

            TRACE_1(BITS, "<b> Bitstream buffer freed\n");
        }

        {
            free(*bitstr_ptr);
            *bitstr_ptr = NULL;

            TRACE_1(BITS, "<b> Bitstream freed\n");
        }
    }
}

/* ************************************************************************** */
/* ************************************************************************** */

/*!
 * \brief Read 1 bit from a bitstream, return it then move the bitstream position.
 * \param *bitstr The bitstream to read.
 * \return bit The bit read from the bitstream.
 *
 * First step is to check if reading 1 bit is possible, then if there is 1 bit
 * left in the bitstream buffer.
 */
uint32_t read_bit(Bitstream_t *bitstr)
{
    TRACE_3(BITS, "<b> " BLUE "read_bit()" RESET " starting at bit offset %i\n", bitstream_get_absolute_bit_offset(bitstr));
    uint32_t bit = 0;
    uint32_t bp = 7 - (uint32_t)(bitstr->buffer_offset % 8); // back padding, in bit
    uint32_t start_byte = (uint32_t)(bitstr->buffer_offset / 8);

    // Fill new data into the bitstream buffer, if needed
    ////////////////////////////////////////////////////////////////////////

    if (start_byte > bitstr->buffer_size)
    {
        if (buffer_feed_dynamic(bitstr, -1) == FAILURE)
        {
            return FAILURE;
        }

        bp = 7 - (uint32_t)(bitstr->buffer_offset % 8);
        start_byte = (uint32_t)(bitstr->buffer_offset / 8);
    }

    // Read one bit
    ////////////////////////////////////////////////////////////////////////

    bit = (uint32_t)(bitstr->buffer[start_byte] >> bp);
    bit &= 0x01;

    // Update bit offset
    bitstr->buffer_offset++;

    // Return result
    ////////////////////////////////////////////////////////////////////////

    TRACE_2(BITS, "<b>   bit     : %i\n", bit);

    return bit;
}

/* ************************************************************************** */

/*!
 * \brief Read n bit(s) from a bitstream, return it then move the bitstream position.
 * \param *bitstr The bitstream to read.
 * \param n The number of bits to read, up to 32.
 * \return bits The content read from the bitstream.
 *
 * First step is to check if reading n bits is possible, then if there is n bits
 * left in the bitstream buffer.
 */
uint32_t read_bits(Bitstream_t *bitstr, const unsigned int n)
{
    TRACE_3(BITS, "<b> " BLUE "read_bits(%u)" RESET " starting at bit offset %i\n", n, bitstream_get_absolute_bit_offset(bitstr));

    uint32_t bits = 0;
    uint32_t fp = (uint32_t)(bitstr->buffer_offset % 8); // front padding, in bit
    uint32_t byte_current = (uint32_t)floor(bitstr->buffer_offset / 8.0);
    uint32_t tbr = (uint32_t)ceil((n + fp) / 8.0);
    uint32_t tbr_current = tbr;

    // Debug traces
    ////////////////////////////////////////////////////////////////////////

#if ENABLE_DEBUG
    TRACE_2(BITS, "<b>   n       : %u\n", n);
    TRACE_2(BITS, "<b>   fp      : %u\n", fp);
    TRACE_2(BITS, "<b>   bo      : %u\n", bitstr->buffer_offset);
    TRACE_2(BITS, "<b>   start   : %u\n", byte_current);
    TRACE_2(BITS, "<b>   tbr     : %u\n", tbr);

    // Check if we can read n bits
    ////////////////////////////////////////////////////////////////////////

    if (n == 0)
    {
        TRACE_WARNING(BITS, "This function cannot read 0 bits!\n");
        return FAILURE;
    }
    else if (n > 32)
    {
        TRACE_WARNING(BITS, "You want to read %i bits, but this function can only read up to 32 bits!\n", n);
        return FAILURE;
    } else
#endif /* ENABLE_DEBUG */

    if (n == 1)
    {
        return read_bit(bitstr);
    }

    // Fill new data into the bitstream buffer, if needed
    ////////////////////////////////////////////////////////////////////////

    if ((byte_current + tbr) > bitstr->buffer_size)
    {
        if (buffer_feed_dynamic(bitstr, -1) == FAILURE)
        {
            return FAILURE;
        }

        fp = (uint32_t)(bitstr->buffer_offset % 8); // front padding, in bit
        byte_current = (uint32_t)floor(bitstr->buffer_offset / 8.0);
        tbr = (uint32_t)ceil((n + fp) / 8.0);
        tbr_current = tbr;
    }

    // Read
    ////////////////////////////////////////////////////////////////////////

    if (fp > 0)
    {
        // Read un-aligned bits
        bits += bitstr->buffer[byte_current++];
        bits &= 0xFF >> fp;
        tbr_current--;

        while (tbr_current > 0)
        {
            if (tbr > 4 &&
                tbr_current == 1)
            {
                bits <<= fp;
                bits += bitstr->buffer[byte_current++] & (0xFF >> fp);
            }
            else
            {
                bits <<= 8;
                bits += bitstr->buffer[byte_current++];
            }

            tbr_current--;
        }

        bits >>= ((tbr*8) - n - fp);
    }
    else
    {
        // Read aligned bits
        while (tbr_current > 0)
        {
            bits <<= 8;
            bits += bitstr->buffer[byte_current++];
            tbr_current--;
        }

        bits >>= (32 - n) % 8;
    }

    // Update bit offset
    bitstr->buffer_offset += n;

    // Return result
    ////////////////////////////////////////////////////////////////////////

    TRACE_2(BITS, "<b>   content = 0d%u\n", bits);
    TRACE_2(BITS, "<b>   content = 0x%08X\n", bits);

    return bits;
}

/* ************************************************************************** */

/*!
 * \brief Read n bit(s) from a bitstream, return it then move the bitstream position.
 * \param *bitstr The bitstream to read.
 * \param n The number of bits to read, up to 64.
 * \return bits The content read from the bitstream.
 *
 * First step is to check if reading n bits is possible, then if there is n bits
 * left in the bitstream buffer.
 */
uint64_t read_bits_64(Bitstream_t *bitstr, const unsigned int n)
{
    TRACE_3(BITS, "<b> " BLUE "read_bits_64(%u)" RESET " starting at bit offset %i\n", n, bitstream_get_absolute_bit_offset(bitstr));

    uint64_t bits = 0;
    uint32_t fp = (uint32_t)(bitstr->buffer_offset % 8); // front padding, in bit
    uint32_t byte_current = (uint32_t)floor(bitstr->buffer_offset / 8.0);
    uint32_t tbr = (uint32_t)ceil((n + fp) / 8.0);
    uint32_t tbr_current = tbr;

    // Debug traces
    ////////////////////////////////////////////////////////////////////////

#if ENABLE_DEBUG
    TRACE_2(BITS, "<b>   n       : %u\n", n);
    TRACE_2(BITS, "<b>   fp      : %u\n", fp);
    TRACE_2(BITS, "<b>   bo      : %u\n", bitstr->buffer_offset);
    TRACE_2(BITS, "<b>   start   : %u\n", byte_current);
    TRACE_2(BITS, "<b>   tbr     : %u\n", tbr);

    // Check if we can read n bits
    ////////////////////////////////////////////////////////////////////////

    if (n == 0)
    {
        TRACE_WARNING(BITS, "This function cannot read 0 bits!\n");
        return FAILURE;
    }
    else if (n > 64)
    {
        TRACE_WARNING(BITS, "You want to read %i bits, but this function can only read up to 64 bits!\n", n);
        return FAILURE;
    } else
#endif /* ENABLE_DEBUG */

    if (n == 1)
    {
        return read_bit(bitstr);
    }

    // Fill new data into the bitstream buffer, if needed
    ////////////////////////////////////////////////////////////////////////

    if ((byte_current + tbr) > bitstr->buffer_size)
    {
        if (buffer_feed_dynamic(bitstr, -1) == FAILURE)
        {
            return FAILURE;
        }

        fp = (uint32_t)(bitstr->buffer_offset % 8); // front padding, in bit
        byte_current = (uint32_t)floor(bitstr->buffer_offset / 8.0);
        tbr = (uint32_t)ceil((n + fp) / 8.0);
        tbr_current = tbr;
    }

    // Read
    ////////////////////////////////////////////////////////////////////////

    if (fp > 0)
    {
        // Read un-aligned bits
        bits += bitstr->buffer[byte_current++];
        bits &= 0xFF >> fp;
        tbr_current--;

        while (tbr_current > 0)
        {
            if (tbr > 4 &&
                tbr_current == 1)
            {
                bits <<= fp;
                bits += bitstr->buffer[byte_current++] & (0xFF >> fp);
            }
            else
            {
                bits <<= 8;
                bits += bitstr->buffer[byte_current++];
            }

            tbr_current--;
        }

        bits >>= ((tbr*8) - n - fp);
    }
    else
    {
        // Read aligned bits
        while (tbr_current > 0)
        {
            bits <<= 8;
            bits += bitstr->buffer[byte_current++];
            tbr_current--;
        }

        bits >>= (64 - n) % 8;
    }

    // Update bit offset
    bitstr->buffer_offset += n;

    // Return result
    ////////////////////////////////////////////////////////////////////////

    TRACE_2(BITS, "<b>   content = 0d%LLu\n", bits);
    TRACE_2(BITS, "<b>   content = 0x%16X\n", bits);

    return bits;
}

/* ************************************************************************** */
/* ************************************************************************** */

/*!
 * \brief Read 1 byte from a bitstream, return it, then move the bitstream position.
 * \param *bitstr The bitstream to read.
 * \return bit The bit read from the bitstream.
 *
 * First step is to check if reading one byte is possible, if there is 8 bits
 * left in the bitstream buffer, then move the bitstream offset.
 */
uint32_t read_byte_aligned(Bitstream_t *bitstr)
{
    TRACE_3(BITS, "<b> " BLUE "read_byte_aligned()" RESET " starting at bit offset %i\n", bitstream_get_absolute_bit_offset(bitstr));
    uint32_t byte = 0;

    // Fill new data into the bitstream buffer, if needed
    ////////////////////////////////////////////////////////////////////////

    if ((bitstr->buffer_offset + 8) > (bitstr->buffer_size * 8))
    {
        if (buffer_feed_dynamic(bitstr, -1) == FAILURE)
        {
            return FAILURE;
        }
    }

    // Check byte alignment
    ////////////////////////////////////////////////////////////////////////

#if ENABLE_DEBUG
    if ((bitstr->buffer_offset % 8) != 0)
    {
        TRACE_ERROR(BITS, "<b> " BLUE "read_byte_aligned() on unaligned offset" RESET " at current byte offset %i + %i bit(s)\n", bitstream_get_absolute_byte_offset(bitstr), bitstream_get_absolute_bit_offset(bitstr)%8);
        return FAILURE;
    }
#endif /* ENABLE_DEBUG */

    // Read one byte
    ////////////////////////////////////////////////////////////////////////

    byte = (uint32_t)(bitstr->buffer[bitstr->buffer_offset / 8]);

    // Update bit offset
    bitstr->buffer_offset += 8;

    // Return result
    ////////////////////////////////////////////////////////////////////////

    TRACE_2(BITS, "<b>   byte    : %02X\n", byte);

    return byte;
}

/* ************************************************************************** */

/*!
 * \brief Read one byte from a bitstream and return it, but DO NOT move the bitstream position.
 * \param *bitstr The bitstream to read.
 * \return bit The bit read from the bitstream.
 *
 * This function is basically the same as read_byte_aligned() but it DO NOT move
 * the buffer position after reading one byte.
 */
uint32_t next_byte_aligned(Bitstream_t *bitstr)
{
    TRACE_3(BITS, "<b> " BLUE "next_byte_aligned()" RESET " starting at bit offset %i\n", bitstream_get_absolute_bit_offset(bitstr));
    uint32_t byte = 0;

    // Fill new data into the bitstream buffer, if needed
    ////////////////////////////////////////////////////////////////////////

    if ((bitstr->buffer_offset + 8) > (bitstr->buffer_size * 8))
    {
        if (buffer_feed_dynamic(bitstr, -1) == FAILURE)
        {
            return FAILURE;
        }
    }

    // Check byte alignment
    ////////////////////////////////////////////////////////////////////////

#if ENABLE_DEBUG
    if ((bitstr->buffer_offset % 8) != 0)
    {
        TRACE_ERROR(BITS, "<b> " BLUE "read_byte_aligned() on unaligned offset" RESET " at current byte offset %i + %i bit(s)\n", bitstream_get_absolute_byte_offset(bitstr), bitstream_get_absolute_bit_offset(bitstr)%8);
        return FAILURE;
    }
#endif /* ENABLE_DEBUG */

    // Read one byte
    ////////////////////////////////////////////////////////////////////////

    byte = (uint32_t)(bitstr->buffer[bitstr->buffer_offset / 8]);

    // Return result
    ////////////////////////////////////////////////////////////////////////

    TRACE_2(BITS, "<b>   byte    : %02X\n", byte);

    return byte;
}

/* ************************************************************************** */
/* ************************************************************************** */

/*!
 * \brief Read one bit from a bitstream and return it, but DO NOT move the bitstream position.
 * \param *bitstr The bitstream to read.
 * \return bit The bit read from the bitstream.
 *
 * This function is basically the same as read_bit() but it DO NOT move the
 * buffer position after reading one bit.
 */
uint32_t next_bit(Bitstream_t *bitstr)
{
    TRACE_3(BITS, "<b> " BLUE "next_bit()" RESET " starting at bit offset %i\n", bitstream_get_absolute_bit_offset(bitstr));

    uint32_t bit = 0;
    uint32_t bp = 7 - (uint32_t)(bitstr->buffer_offset % 8); // back padding, in bit
    uint32_t start_byte = (uint32_t)(bitstr->buffer_offset / 8);

    // Fill new data into the bitstream buffer, if needed
    ////////////////////////////////////////////////////////////////////////

    if (start_byte > bitstr->buffer_size)
    {
        if (buffer_feed_dynamic(bitstr, -1) == FAILURE)
        {
            return FAILURE;
        }

        bp = 7 - (uint32_t)(bitstr->buffer_offset % 8);
        start_byte = (uint32_t)(bitstr->buffer_offset / 8);
    }

    // Read one bit
    ////////////////////////////////////////////////////////////////////////

    bit = (uint32_t)(bitstr->buffer[start_byte] >> bp);
    bit &= 0x01;

    // Return result
    ////////////////////////////////////////////////////////////////////////

    TRACE_2(BITS, "<b>   bit     = %i\n", bit);

    return bit;
}

/* ************************************************************************** */

/*!
 * \brief Read n bit(s) from a bitstream and return it, but DO NOT move the bitstream position.
 * \param *bitstr The bitstream to read.
 * \param n The number of bit(s) to read. Up to 32 bits.
 * \return bits The content read from the bitstream.
 *
 * This function is basically the same as read_bits() but it DO NOT move the
 * buffer position after reading n bits.
 */
uint32_t next_bits(Bitstream_t *bitstr, const unsigned int n)
{
    TRACE_3(BITS, "<b> " BLUE "next_bits(%u)" RESET " starting at bit offset %i\n", n, bitstream_get_absolute_bit_offset(bitstr));

    uint32_t bits = 0;
    uint32_t fp = (uint32_t)(bitstr->buffer_offset % 8); // front padding, in bit
    uint32_t byte_current = (uint32_t)floor(bitstr->buffer_offset / 8.0);
    uint32_t tbr = (uint32_t)ceil((n + fp) / 8.0);
    uint32_t tbr_current = tbr;

    // Debug traces
    ////////////////////////////////////////////////////////////////////////

#if ENABLE_DEBUG
    TRACE_2(BITS, "<b>   n       : %u\n", n);
    TRACE_2(BITS, "<b>   fp      : %u\n", fp);
    TRACE_2(BITS, "<b>   bo      : %u\n", bitstr->buffer_offset);
    TRACE_2(BITS, "<b>   start   : %u\n", byte_current);
    TRACE_2(BITS, "<b>   tbr     : %u\n", tbr);

    // Check if we can read n bits
    ////////////////////////////////////////////////////////////////////////

    if (n == 0)
    {
        TRACE_WARNING(BITS, "This function cannot read 0 bits!\n");
        return FAILURE;
    }
    else if (n > 32)
    {
        TRACE_WARNING(BITS, "You want to read %i bits, but this function can only read up to 32 bits!\n", n);
        return FAILURE;
    }
    else
#endif /* ENABLE_DEBUG */

    if (n == 1)
    {
        return next_bit(bitstr);
    }

    // Fill new data into the bitstream buffer, if needed
    ////////////////////////////////////////////////////////////////////////

    if ((byte_current + tbr) > bitstr->buffer_size)
    {
        if (buffer_feed_dynamic(bitstr, -1) == FAILURE)
        {
            return FAILURE;
        }

        fp = (uint32_t)(bitstr->buffer_offset % 8); // front padding, in bit
        byte_current = (uint32_t)floor(bitstr->buffer_offset / 8.0);
        tbr = (uint32_t)ceil((n + fp) / 8.0);
        tbr_current = tbr;
    }

    // Read
    ////////////////////////////////////////////////////////////////////////

    if (fp > 0)
    {
        // Read un-aligned bits
        bits += bitstr->buffer[byte_current++];
        bits &= 0xFF >> fp;
        tbr_current--;

        while (tbr_current > 0)
        {
            if (tbr > 4 &&
                tbr_current == 1)
            {
                bits <<= fp;
                bits += bitstr->buffer[byte_current++] & (0xFF >> fp);
            }
            else
            {
                bits <<= 8;
                bits += bitstr->buffer[byte_current++];
            }

            tbr_current--;
        }

        bits >>= ((tbr*8) - n - fp);
    }
    else
    {
        // Read aligned bits
        while (tbr_current > 0)
        {
            bits <<= 8;
            bits += bitstr->buffer[byte_current];
            byte_current++;
            tbr_current--;
        }

        bits >>= (32 - n) % 8;
    }

    // Return result
    ////////////////////////////////////////////////////////////////////////

    TRACE_2(BITS, "<b>   content = 0d%u\n", bits);
    TRACE_2(BITS, "<b>   content = 0x%08X\n", bits);

    return bits;
}

/* ************************************************************************** */
/* ************************************************************************** */

/*!
 * \brief Skip n bits in a bitstream.
 * \param *bitstr The bitstream to use.
 * \param n The number of bits to skip.
 * \return 1 if success, 0 otherwise.
 *
 * If n is bigger than the buffer size, we do a jump directly to the offset we
 * want. That will trigger a buffer refresh.
 */
int skip_bits(Bitstream_t *bitstr, const unsigned int n)
{
    int retcode = FAILURE;

    // Load next data if needed
    if ((bitstr->buffer_offset + n) > (bitstr->buffer_size * 8))
    {
        if (n > bitstr->buffer_size)
        {
            // Must reload previous data and go directly to the offset we want
            int64_t new_offset = (int64_t)(bitstr->bitstream_offset*8 + bitstr->buffer_offset + n);
            retcode = bitstream_goto_offset(bitstr, new_offset);
        }
        else
        {
            // Refresh buffer
            retcode = buffer_feed_dynamic(bitstr, -1);

            // Skip n bits
            bitstr->buffer_offset += n;
        }
    }
    else
    {
        // Skip n bits
        bitstr->buffer_offset += n;
        retcode = SUCCESS;
    }

#if ENABLE_DEBUG
    if (retcode == SUCCESS)
    {
        TRACE_2(BITS, "<b> " BLUE "skip_bits(%u)" RESET " SUCCESS\n", n);
    }
    else
    {
        TRACE_ERROR(BITS, "<b> " BLUE "skip_bits(%u)" RESET " Cannot skip bits at bit offset\n", n, bitstr->buffer_offset);
    }
#endif /* ENABLE_DEBUG */

    return retcode;
}

/* ************************************************************************** */

/*!
 * \brief Rewind n bits in a bitstream.
 * \param *bitstr The bitstream to use.
 * \param n The number of bits to rewind.
 * \return 1 if success, 0 otherwise.
 *
 * If rewinding is impossible due to the current buffer_offset being smaller
 * than n, we do a jump directly to the offset we want. That will trigger a
 * buffer refresh.
 */
int rewind_bits(Bitstream_t *bitstr, const unsigned int n)
{
    int retcode = FAILURE;

    // Check if rewinding inside the buffer is possible
    if (n < bitstr->buffer_offset)
    {
        // Rewind n bits
        bitstr->buffer_offset -= n;
        retcode = SUCCESS;
    }
    else
    {
        // Must reload previous data and go directly to the offset we want
        int64_t new_offset = (int64_t)(bitstr->bitstream_offset*8 + bitstr->buffer_offset - n);
        retcode = bitstream_goto_offset(bitstr, new_offset);
    }

#if ENABLE_DEBUG
    if (retcode == SUCCESS)
    {
        TRACE_2(BITS, "<b> " BLUE "rewind_bits(%u)" RESET " SUCCESS\n", n);
    }
    else
    {
        TRACE_ERROR(BITS, "<b> " BLUE "rewind_bits(%u)" RESET " Cannot rewind bits at bit offset %u\n", n, bitstr->buffer_offset);
    }
#endif /* ENABLE_DEBUG */

    return retcode;
}

/* ************************************************************************** */
/* ************************************************************************** */

/*!
 * \brief Return the absolute byte offset into the bitstream.
 * \param *bitstr The bitstream to use.
 * \return The absolute byte offset into the bitstream.
 */
int64_t bitstream_get_absolute_byte_offset(Bitstream_t *bitstr)
{
    return (int64_t)(bitstr->bitstream_offset + bitstr->buffer_offset/8 + bitstr->buffer_discarded_bytes);
}

/* ************************************************************************** */

/*!
 * \brief Return the absolute bit offset into the bitstream.
 * \param *bitstr The bitstream to use.
 * \return The absolute bit offset into the bitstream.
 *
 * Be careful of integer overflow if file is more than 134217728 GiB?
 */
int64_t bitstream_get_absolute_bit_offset(Bitstream_t *bitstr)
{
    return (int64_t)(bitstr->bitstream_offset*8 + bitstr->buffer_offset + bitstr->buffer_discarded_bytes*8);
}

/* ************************************************************************** */

/*!
 * \brief Go to the n byte of a file, if possible.
 * \param *bitstr The bitstream to use.
 * \param n The position to go to, in byte.
 * \return 1 if success, 0 otherwise.
 *
 * Note that it is NOT possible to jump to the last byte of the bitstream,
 * because it would cause a buffer reload, and incidentally a "premature end
 * of file" error.
 */
int bitstream_goto_offset(Bitstream_t *bitstr, const int64_t n)
{
    int retcode = FAILURE;

    if (n > 0 && n < bitstr->bitstream_size)
    {
        retcode = buffer_feed_dynamic(bitstr, n);
    }
    else
    {
        retcode = FAILURE;
    }

#if ENABLE_DEBUG
    if (retcode == SUCCESS)
    {
        TRACE_2(BITS, "<b> " BLUE "bitstream_goto_offset(%lli)" RESET " Success\n", n);
    }
    else
    {
        TRACE_ERROR(BITS, "<b> " BLUE "bitstream_goto_offset(%lli)" RESET " Cannot jump outside bitstream boundaries!\n", n);
    }
#endif /* ENABLE_DEBUG */

    return retcode;
}

/* ************************************************************************** */
