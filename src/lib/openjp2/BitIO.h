/*
*    Copyright (C) 2016-2017 Grok Image Compression Inc.
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

#include "IBitIO.h"
#include "IGrokStream.h"

namespace grk {

/*
Bit input/output
*/
class BitIO : public IBitIO {

public:

	BitIO(uint8_t *bp, uint64_t len, bool isEncoder);
	BitIO(IGrokStream* stream, bool isEncoder);

	/*
	Number of bytes written.
	@return the number of bytes written
	*/
	size_t numbytes();

	/*
	Write bits
	@param v Value of bits
	@param n Number of bits to write
	*/
	bool write( uint32_t v, uint32_t n);
	/*
	Read bits
	@param n Number of bits to read
	@return the corresponding read number
	*/
	bool read(uint32_t* bits, uint32_t n);
	/*
	Flush bits
	@return true if successful, returns false otherwise
	*/
	bool flush();
	/*
	Passes the ending bits (coming from flushing)
	@return true if successful, returns false otherwise
	*/
	bool inalign();

	void simulateOutput(bool doSimulate) { sim_out = doSimulate; }

private:

	/* pointer to the start of the buffer */
	uint8_t *start;

	size_t offset;
	size_t buf_len;

	/* temporary place where each byte is read or written */
	uint8_t buf;
	/* coder : number of bits free to write. decoder : number of bits read */
	uint8_t ct;

	size_t total_bytes;

	bool sim_out;

	bool is_encoder;

	IGrokStream* stream;

	/*
	Write a bit
	@param bio BIO handle
	@param b Bit to write (0 or 1)
	*/
	bool putbit( uint8_t b);
	/*
	Read a bit
	@param bio BIO handle
	@return the read bit
	*/
	bool getbit(uint32_t* bits, uint8_t pos);
	/*
	Write a byte
	@param bio BIO handle
	@return true if successful, returns false otherwise
	*/
	bool byteout();

	/*
	Write a byte
	@param bio BIO handle
	@return true if successful, returns false otherwise
	*/
	bool byteout_stream();
	/*
	Read a byte
	@param bio BIO handle
	@return true if successful, returns false otherwise
	*/
	bool bytein();


};


}
	
