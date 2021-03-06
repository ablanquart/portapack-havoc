/*
 * Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __STREAM_INPUT_H__
#define __STREAM_INPUT_H__

#include "message.hpp"
#include "fifo.hpp"

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

#include <cstdint>
#include <cstddef>
#include <memory>

class StreamInput {
public:
	StreamInput(CaptureConfig* const config) :
		config { config },
		K { config->write_size_log2 + config->buffer_count_log2 },
		event_bytes_mask { (1UL << config->write_size_log2) - 1 },
		data { std::make_unique<uint8_t[]>(1UL << K) },
		fifo { data.get(), K }
	{
		config->fifo = &fifo;
	}

	size_t write(const void* const data, const size_t length) {
		const auto written = fifo.in(reinterpret_cast<const uint8_t*>(data), length);

		const auto last_bytes_written = bytes_written;
		bytes_written += written;
		if( (bytes_written & event_bytes_mask) < (last_bytes_written & event_bytes_mask) ) {
			creg::m4txevent::assert();
		}
		config->baseband_bytes_received += length;
		config->baseband_bytes_dropped = config->baseband_bytes_received - bytes_written;

		return written;
	}

private:
	CaptureConfig* const config;
	const size_t K;
	const uint64_t event_bytes_mask;
	uint64_t bytes_written = 0;
	std::unique_ptr<uint8_t[]> data;
	FIFO<uint8_t> fifo;
};

#endif/*__STREAM_INPUT_H__*/
