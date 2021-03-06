/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#include "ui_setup.hpp"
#include "string_format.hpp"
#include "portapack_persistent_memory.hpp"
#include "ui_font_fixed_8x16.hpp"

#include "lpc43xx_cpp.hpp"
using namespace lpc43xx;

#include "portapack.hpp"
using portapack::receiver_model;
using namespace portapack;

namespace ui {

SetDateTimeView::SetDateTimeView(
	NavigationView& nav
) {
	button_ok.on_select = [&nav, this](Button&){
		const auto model = this->form_collect();
		const rtc::RTC new_datetime {
			model.year, model.month, model.day,
			model.hour, model.minute, model.second
		};
		rtcSetTime(&RTCD1, &new_datetime);
		nav.pop();
	},

	button_cancel.on_select = [&nav](Button&){
		nav.pop();
	},

	add_children({ {
		&text_title,
		&field_year,
		&text_slash1,
		&field_month,
		&text_slash2,
		&field_day,
		&field_hour,
		&text_colon1,
		&field_minute,
		&text_colon2,
		&field_second,
		&text_format,
		&button_ok,
		&button_cancel,
	} });

	rtc::RTC datetime;
	rtcGetTime(&RTCD1, &datetime);
	SetDateTimeModel model {
		datetime.year(),
		datetime.month(),
		datetime.day(),
		datetime.hour(),
		datetime.minute(),
		datetime.second()
	};

	form_init(model);
}

void SetDateTimeView::focus() {
	button_cancel.focus();
}

void SetDateTimeView::form_init(const SetDateTimeModel& model) {
	field_year.set_value(model.year);
	field_month.set_value(model.month);
	field_day.set_value(model.day);
	field_hour.set_value(model.hour);
	field_minute.set_value(model.minute);
	field_second.set_value(model.second);
}

SetDateTimeModel SetDateTimeView::form_collect() {
	return {
		.year = static_cast<uint16_t>(field_year.value()),
		.month = static_cast<uint8_t>(field_month.value()),
		.day = static_cast<uint8_t>(field_day.value()),
		.hour = static_cast<uint8_t>(field_hour.value()),
		.minute = static_cast<uint8_t>(field_minute.value()),
		.second = static_cast<uint8_t>(field_second.value())
	};
}

SetFrequencyCorrectionView::SetFrequencyCorrectionView(
	NavigationView& nav
) {
	button_ok.on_select = [&nav, this](Button&){
		const auto model = this->form_collect();
		portapack::persistent_memory::set_correction_ppb(model.ppm * 1000);
		nav.pop();
	},

	button_cancel.on_select = [&nav](Button&){
		nav.pop();
	},

	add_children({ {
		&text_title,
		&field_ppm,
		&text_ppm,
		&button_ok,
		&button_cancel,
	} });

	SetFrequencyCorrectionModel model {
		static_cast<int8_t>(portapack::persistent_memory::correction_ppb() / 1000)
	};

	form_init(model);
}

void SetFrequencyCorrectionView::focus() {
	button_cancel.focus();
}

void SetFrequencyCorrectionView::form_init(const SetFrequencyCorrectionModel& model) {
	field_ppm.set_value(model.ppm);
}

SetFrequencyCorrectionModel SetFrequencyCorrectionView::form_collect() {
	return {
		.ppm = static_cast<int8_t>(field_ppm.value()),
	};
}

AntennaBiasSetupView::AntennaBiasSetupView(NavigationView& nav) {
	add_children({ {
		&text_title,
		&text_description_1,
		&text_description_2,
		&text_description_3,
		&text_description_4,
		&options_bias,
		&button_done,
	} });

	options_bias.set_by_value(receiver_model.antenna_bias() ? 1 : 0);
	options_bias.on_change = [this](size_t, OptionsField::value_t v) {
		receiver_model.set_antenna_bias(v);
	};

	button_done.on_select = [&nav](Button&){ nav.pop(); };
}

void AntennaBiasSetupView::focus() {
	button_done.focus();
}

SetTouchCalibView::SetTouchCalibView(NavigationView& nav) {
	add_children({{
		&text_title,
		&text_debugx,
		&text_debugy,
		&button_ok
		}});

	button_ok.on_select = [&nav](Button&){ nav.pop(); };
}

void SetTouchCalibView::focus() {
	button_ok.focus();
}

bool SetTouchCalibView::on_touch(const TouchEvent event) {
	if (event.type == ui::TouchEvent::Type::Start) {
		text_debugx.set(to_string_dec_uint(round(event.point.x), 4));
		text_debugy.set(to_string_dec_uint(round(event.point.y), 4));
	}
	return true;
}

SetPlayDeadView::SetPlayDeadView(NavigationView& nav) {
	add_children({{
		&text_sequence,
		&button_enter,
		&button_cancel
		}});

	button_enter.on_select = [this,&nav](Button&){
		if (entermode == false) {
			sequence = 0;
			memset(sequence_txt,0,11);
			text_sequence.set("");
			keycount = 0;
			entermode = true;
			button_cancel.hidden(true);
		} else {
			persistent_memory::set_playdead_sequence(sequence);
			nav.pop();
		}
	};
	button_enter.on_dir = [this,&nav](Button&, KeyEvent key){
		if ((entermode == true) && (keycount < 10)) {
			key_code = static_cast<std::underlying_type<KeyEvent>::type>(key);
			if (key_code == 0)
				sequence_txt[keycount] = 'R';
			else if (key_code == 1)
				sequence_txt[keycount] = 'L';
			else if (key_code == 2)
				sequence_txt[keycount] = 'D';
			else if (key_code == 3)
				sequence_txt[keycount] = 'U';
			text_sequence.set(sequence_txt);
			sequence = (sequence<<3) | key_code;
			keycount++;
			
		}
	};
	button_cancel.on_select = [&nav](Button&){ nav.pop(); };
}

void SetPlayDeadView::focus() {
	button_enter.focus();
}

SetUIView::SetUIView(NavigationView& nav) {
	uint32_t ui_config;
	
	add_children({{
		&checkbox_showsplash,
		&checkbox_bloff,
		&options_bloff,
		&button_ok
	}});
	
	ui_config = portapack::persistent_memory::ui_config();
	
	if (ui_config & 1) checkbox_showsplash.set_value(true);
	if (ui_config & 2) checkbox_bloff.set_value(true);
	options_bloff.set_selected_index((ui_config >> 5) & 7);

	button_ok.on_select = [&nav,this](Button&){
		uint32_t ui_config = 0;
		if (checkbox_showsplash.value() == true) ui_config |= 1;
		if (checkbox_bloff.value() == true) ui_config |= 2;
		ui_config |= (portapack::persistent_memory::ui_config_textentry() << 2);
		ui_config |= (options_bloff.selected_index() << 5);
		
		portapack::persistent_memory::set_ui_config(ui_config);
		nav.pop();
	};
}

void SetUIView::focus() {
	button_ok.focus();
}

void ModInfoView::on_show() {
	if (modules_nb) update_infos(0);
}

void ModInfoView::update_infos(uint8_t modn) {
	char info_str[27];
	char ch;
	uint8_t c;
	Point pos = { 0, 0 };
	Rect rect = { { 16, 144 }, { 208, 144 } };
	
	info_str[0] = 0;
	strcat(info_str, module_list[modn].name);
	text_namestr.set(info_str);
	
	info_str[0] = 0;
	strcat(info_str, to_string_dec_uint(module_list[modn].size).c_str());
	strcat(info_str, " bytes");
	text_sizestr.set(info_str);
	
	info_str[0] = 0;
	for (c = 0; c < 8; c++)
		strcat(info_str, to_string_hex(module_list[modn].md5[c], 2).c_str());
	text_md5_a.set(info_str);

	info_str[0] = 0;
	for (c = 8; c < 16; c++)
		strcat(info_str, to_string_hex(module_list[modn].md5[c], 2).c_str());
	text_md5_b.set(info_str);
	
	// TODO: Use ui_console
	display.fill_rectangle(rect, Color::black());
	
	const Font& font = font::fixed_8x16;
	const auto line_height = font.line_height();
	c = 0;
	while((ch = module_list[modn].description[c++])) {
		const auto glyph = font.glyph(ch);
		const auto advance = glyph.advance();
		if((pos.x + advance.x) > rect.width()) {
			pos.x = 0;
			pos.y += line_height;
		}
		const Point pos_glyph {
			static_cast<Coord>(rect.pos.x + pos.x),
			static_cast<Coord>(rect.pos.y + pos.y)
		};
		display.draw_glyph(pos_glyph, glyph, Color::white(), Color::black());
		pos.x += advance.x;
	}
}

ModInfoView::ModInfoView(NavigationView& nav) {
	const char magic[4] = {'P', 'P', 'M', ' '};
	UINT bw;
	uint8_t i;
	char read_buf[16];
	char info_str[25];
	FILINFO modinfo;
	FIL modfile;
	DIR rootdir;
	FRESULT res;
	uint8_t c;
	
	using option_t = std::pair<std::string, int32_t>;
	using options_t = std::vector<option_t>;
	option_t opt;
	options_t opts;
	
	static constexpr Style style_orange {
		.font = font::fixed_8x16,
		.background = Color::black(),
		.foreground = Color::orange(),
	};
	
	add_children({{
		&text_modcount,
		&text_name,
		&text_namestr,
		&text_size,
		&text_sizestr,
		&text_md5,
		&text_md5_a,
		&text_md5_b,
		&button_ok
	}});
	
	text_name.set_style(&style_orange);
	text_size.set_style(&style_orange);
	text_md5.set_style(&style_orange);

	// TODO: Find a way to merge this with m4_load_image() in m4_startup.cpp
	
	// Scan SD card root directory for files starting with the magic bytes
	c = 0;
	if (f_opendir(&rootdir, "/") == FR_OK) {
		for (;;) {
			res = f_readdir(&rootdir, &modinfo);
			if (res != FR_OK || modinfo.fname[0] == 0) break;	// Reached last file, abort
			// Only care about files with .bin extension
			if ((!(modinfo.fattrib & AM_DIR)) && (modinfo.fname[9] == 'B') && (modinfo.fname[10] == 'I') && (modinfo.fname[11] == 'N')) {
				f_open(&modfile, modinfo.fname, FA_OPEN_EXISTING | FA_READ);
				// Magic bytes check
				f_read(&modfile, &read_buf, 4, &bw);
				for (i = 0; i < 4; i++)
					if (read_buf[i] != magic[i]) break;
				if (i == 4) {
					memcpy(&module_list[c].filename, modinfo.fname, 8);
					module_list[c].filename[8] = 0;
					
					f_lseek(&modfile, 4);
					f_read(&modfile, &module_list[c].version, 2, &bw);
					f_lseek(&modfile, 6);
					f_read(&modfile, &module_list[c].size, 4, &bw);
					f_lseek(&modfile, 10);
					f_read(&modfile, &module_list[c].name, 16, &bw);
					f_lseek(&modfile, 26);
					f_read(&modfile, &module_list[c].md5, 16, &bw);
					f_lseek(&modfile, 42);
					f_read(&modfile, &module_list[c].description, 214, &bw);
					f_lseek(&modfile, 256);
					
					// Sanitize
					module_list[c].name[15] = 0;
					module_list[c].description[213] = 0;
					
					memcpy(info_str, module_list[c].filename, 16);
					strcat(info_str, "(V");
					strcat(info_str, to_string_dec_uint(module_list[c].version, 4, '0').c_str());
					strcat(info_str, ")");
					while(strlen(info_str) < 24)
						strcat(info_str, " ");
					
					opt = std::make_pair(info_str, c);
					opts.insert(opts.end(), opt);
					
					c++;
				}
				f_close(&modfile);
			}
			if (c == 8) break;
		}
	}
	f_closedir(&rootdir);
	
	modules_nb = c;

	if (modules_nb) {
		strcpy(info_str, "Found ");
		strcat(info_str, to_string_dec_uint(modules_nb, 1).c_str());
		strcat(info_str, " module(s)");
		
		text_modcount.set(info_str);
		option_modules.set_options(opts);
		
		add_child(&option_modules);
		
		option_modules.on_change = [this](size_t n, OptionsField::value_t v) {
			(void)n;
			update_infos(v);
		};
	} else {
		strcpy(info_str, "No modules found");
		text_modcount.set(info_str);
	}

	button_ok.on_select = [&nav,this](Button&){
		nav.pop();
	};
}

void ModInfoView::focus() {
	if (modules_nb)
		option_modules.focus();
	else
		button_ok.focus();
}

SetupMenuView::SetupMenuView(NavigationView& nav) {
	add_items<7>({ {
		{ "UI", ui::Color::white(), [&nav](){ nav.push<SetUIView>(); } },
		{ "SD card modules", ui::Color::white(), [&nav](){ nav.push<ModInfoView>(); } },
		{ "Date/Time", ui::Color::white(), [&nav](){ nav.push<SetDateTimeView>(); } },
		{ "Frequency correction", ui::Color::white(), [&nav](){ nav.push<SetFrequencyCorrectionView>(); } },
		{ "Antenna Bias Voltage", ui::Color::white(), [&nav](){ nav.push<AntennaBiasSetupView>(); } },		
		{ "Touch screen", ui::Color::white(),     [&nav](){ nav.push<SetTouchCalibView>(); } },
		{ "Play dead", ui::Color::red(), [&nav](){ nav.push<SetPlayDeadView>(); } }
	} });
	on_left = [&nav](){ nav.pop(); };
}

} /* namespace ui */
