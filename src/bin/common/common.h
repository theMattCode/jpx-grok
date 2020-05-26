/*
 *    Copyright (C) 2016-2020 Grok Image Compression Inc.
 *
 *    This source code is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *    This source code is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *    This source code incorporates work covered by the following copyright and
 *    permission notice:
 *
 * The copyright in this software is being made available under the 2-clauses
 * BSD License, included below. This software may be subject to other third
 * party and contributor rights, including patent rights, and no such rights
 * are granted under this license.
 *
 * Copyright (c) 2002-2014, Universite catholique de Louvain (UCL), Belgium
 * Copyright (c) 2002-2014, Professor Benoit Macq
 * Copyright (c) 2001-2003, David Janssens
 * Copyright (c) 2002-2003, Yannick Verschueren
 * Copyright (c) 2003-2007, Francois-Olivier Devaux
 * Copyright (c) 2003-2014, Antonin Descampe
 * Copyright (c) 2005, Herve Drolon, FreeImage Team
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "spdlog/spdlog.h"

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cassert>
#include "grok.h"
#include <algorithm>
using namespace std;

namespace grk {

template <typename ... Args>
void log(grk_msg_callback msg_handler, void *l_data, char const * const format, Args & ... args) noexcept
{
    const int message_size = 512;
	if ((format != nullptr)) {
		char message[message_size];
		memset(message, 0, message_size);
		vsnprintf(message, message_size, format, args...);
		msg_handler(message, l_data);
	}
}


const GRK_SUPPORTED_FILE_FMT supportedStdoutFileFormats[] = { GRK_BMP_FMT,
		GRK_PNG_FMT, GRK_RAW_FMT, GRK_RAWL_FMT, GRK_JPG_FMT };

const size_t maxICCProfileBufferLen = 10000000;

int batch_sleep(int val);

struct grk_dircnt {
	char *filename_buf;
	char **filename;
};

struct grk_img_fol {
	char *imgdirpath;
	const char *out_format;
	bool set_imgdir;
	bool set_out_format;
};

int parse_DA_values(char *inArg, uint32_t *DA_x0, uint32_t *DA_y0,
		uint32_t *DA_x1, uint32_t *DA_y1);

bool safe_fclose(FILE *fd);
bool useStdio(const char *filename);
bool supportedStdioFormat(GRK_SUPPORTED_FILE_FMT format);
bool grok_set_binary_mode(FILE* file);
bool jpeg2000_file_format(const char *fname, GRK_SUPPORTED_FILE_FMT *fmt);
int get_file_format(const char *filename);
const char* get_path_separator();
char* get_file_name(char *name);
uint32_t get_num_images(char *imgdirpath);
char* actual_path(const char *outfile);

// swap endian for 16 bit integer
template<typename T> inline T swap(T x) {
	return (T) ((x >> 8) | ((x & 0x00ff) << 8));
}
// specialization for 32 bit unsigned
template<> inline uint32_t swap(uint32_t x) {
	return (uint32_t) ((x >> 24) | ((x & 0x00ff0000) >> 8)
			| ((x & 0x0000ff00) << 8) | ((x & 0x000000ff) << 24));
}
// no-op specialization for 8 bit
template<> inline uint8_t swap(uint8_t x) {
	return x;
}
// no-op specialization for 8 bit
template<> inline int8_t swap(int8_t x) {
	return x;
}
template<typename T> inline T endian(T x, bool big_endian) {

#ifdef GROK_BIG_ENDIAN
	if (!big_endian)
	   return swap<T>(x);
#else
	if (big_endian)
		return swap<T>(x);
#endif
	return x;
}

template<typename T> inline bool writeBytes(T val, T *buf, T **outPtr,
		size_t *outCount, size_t len, bool big_endian, FILE *out) {
	if (*outCount >= len)
		return false;
	*(*outPtr)++ = grk::endian<T>(val, big_endian);
	(*outCount)++;
	if (*outCount == len) {
		size_t res = fwrite(buf, sizeof(T), len, out);
		if (res != len)
			return false;
		*outCount = 0;
		*outPtr = buf;
	}
	return true;
}

template<typename T> inline bool readBytes(FILE *fp, grk_image *image,
		size_t area) {

	if (!fp || !image)
		return false;

	assert(image->numcomps <= 4);

	uint64_t i = 0;
	uint64_t index = 0;
	uint32_t compno = 0;
	uint64_t totalSize = area * image->numcomps;
	const uint64_t chunkSize = 4096 * 4;
	T chunk[chunkSize];
	while (i < totalSize) {
		uint64_t toRead = min(chunkSize, (uint64_t) (totalSize - i));
		size_t bytesRead = fread(chunk, sizeof(T), toRead, fp);
		if (!bytesRead)
			break;
		T *chunkPtr = chunk;
		for (size_t ct = 0; ct < bytesRead; ++ct) {
			image->comps[compno++].data[index] = grk::endian<T>(*chunkPtr++,
					true);
			if (compno == image->numcomps) {
				compno = 0;
				index++;
			}
		}
		i += bytesRead;
	}
	if (i != totalSize) {
		spdlog::error("bytes read ({}) are less than expected number of bytes ({})", i, totalSize);
		return false;
	}

	return true;
}

uint32_t uint_adds(uint32_t a, uint32_t b);
bool all_components_sanity_check(grk_image *image);
bool isSubsampled(grk_image *  image);

int population_count(uint32_t val);
int count_leading_zeros(uint32_t val);
int count_trailing_zeros(uint32_t val);

}
