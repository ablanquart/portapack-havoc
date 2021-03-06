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

#ifndef __UI_RECORD_VIEW_H__
#define __UI_RECORD_VIEW_H__

#include "ui_widget.hpp"

#include "capture_thread.hpp"
#include "signal.hpp"

#include "bitmap.hpp"

#include <cstddef>
#include <string>
#include <memory>

namespace ui {

class RecordView : public View {
public:
	enum FileType {
		RawS16 = 2,
		WAV = 3,
	};

	RecordView(
		const Rect parent_rect,
		std::string filename_stem_pattern,
		FileType file_type,
		const size_t buffer_size_k,
		const size_t buffer_count_k
	);
	~RecordView();

	void focus() override;

	void set_sampling_rate(const size_t new_sampling_rate) {
		if( new_sampling_rate != sampling_rate ) {
			stop();
			sampling_rate = new_sampling_rate;
		}
	}

	void start();
	void stop();

	bool is_active() const;

private:
	void toggle();
	void write_metadata_file(const std::string& filename);

	void on_tick_second();

	const std::string filename_stem_pattern;
	const FileType file_type;
	const size_t buffer_size_k;
	const size_t buffer_count_k;
	size_t sampling_rate { 0 };
	SignalToken signal_token_tick_second;

	ImageButton button_record {
		{ 0 * 8, 0 * 16, 2 * 8, 1 * 16 },
		&bitmap_record,
		Color::red(),
		Color::black()
	};

	Text text_record_filename {
		{ 3 * 8, 0 * 16, 8 * 8, 16 },
		"",
	};

	Text text_record_dropped {
		{ 16 * 8, 0 * 16, 3 * 8, 16 },
		"",
	};

	std::unique_ptr<CaptureThread> capture_thread;
};

} /* namespace ui */

#endif/*__UI_RECORD_VIEW_H__*/
