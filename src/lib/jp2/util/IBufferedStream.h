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
 */

#pragma once

namespace grk {

struct IBufferedStream {

	virtual ~IBufferedStream() {
	}

	// low level write methods
	virtual bool write_byte(uint8_t value)=0;
	virtual bool write_short(uint16_t value) = 0;
	virtual bool write_24(uint32_t value)=0;
	virtual bool write_int(uint32_t value) = 0;

	/**
	 * Write bytes to the stream.
	 * @param		p_buffer	pointer to the data buffer to be written.
	 * @param		p_size		number of bytes to write.
     *
	 * @return		the number of bytes written, or -1 if an error occurred.
	 */
	virtual size_t write_bytes(const uint8_t *p_buffer, size_t p_size)= 0;

	/**
	 * Flush write stream to disk
	 * @return		true if the data could be flushed, otherwise false.
	 */
	virtual bool flush()= 0;

	/**
	 * Skip bytes in stream, forward or reverse
	 * @param		p_size		the number of bytes to skip.
     *
	 * @return		true if successful, otherwise false.
	 */
	virtual bool skip(int64_t p_size)= 0;

	/**
	 * Tell byte offset in stream (similar to ftell).
	 * @return		current position of the stream.
	 */
	virtual uint64_t tell(void)= 0;

	/**
	 * Get number of bytes left before end of the stream
	 * @return		Number of bytes left.
	 */
	virtual uint64_t get_number_byte_left(void)= 0;

	/**
	 * Seek to absolute offset in stream.
	 * @param		offset		absolute offset in stream

	 * @return		true if successful, otherwise false.
	 */
	virtual bool seek(uint64_t offset)= 0;

	/**
	 * Check if stream is seekable. (A stdin/stdout stream
	 * is not seekable).
	 *
	 * @return	 true if stream is seekable, otherwise false
	 */
	virtual bool has_seek()= 0;

};

}
