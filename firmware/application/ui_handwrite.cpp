/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#include "ui_handwrite.hpp"

#include "ch.h"

#include "ff.h"
#include "portapack.hpp"
#include "event_m0.hpp"
#include "hackrf_hal.hpp"
#include "portapack_shared_memory.hpp"

#include <cstring>

using namespace portapack;

namespace ui {
	
void HandWriteView::paint(Painter& painter) {
	_painter = &painter;
}

HandWriteView::HandWriteView(
	NavigationView& nav,
	char txt[],
	uint8_t max_len
) {
	const char special_chars[5] = {'\'', '.', '?', '!', '='}; 
	_max_len = max_len;
	
	// Handwriting alphabet definition here
	handwriting = &handwriting_unistroke;
	
	txtidx = strlen(txt);
	memcpy(txtinput, txt, max_len + 1);

	add_children({ {
		&text_input,
		&button_case,
		&button_ok
	} });

	const auto button_fn = [this](Button& button) {
		this->on_button(button);
	};

	size_t n = 0;
	for(auto& button : num_buttons) {
		add_child(&button);
		button.on_select = button_fn;
		button.set_parent_rect({
			static_cast<Coord>(n * 24),
			static_cast<Coord>(236),
			24, 28
		});
		const std::string label {
			(char)(n + 0x30)
		};
		button.set_text(label);
		button.id = n + 0x30;
		n++;
	}
	
	n = 0;
	for(auto& button : special_buttons) {
		add_child(&button);
		button.on_select = button_fn;
		button.set_parent_rect({
			static_cast<Coord>(50 + n * 24),
			static_cast<Coord>(270),
			24, 28
		});
		const std::string label {
			(char)(special_chars[n])
		};
		button.set_text(label);
		button.id = special_chars[n];
		n++;
	}
	
	button_case.on_select = [this, &nav](Button&) {
		if (_lowercase == true) {
			_lowercase = false;
			button_case.set_text("LC");
		} else {
			_lowercase = true;
			button_case.set_text("UC");
		}
	};

	button_ok.on_select = [this, &nav, txt, max_len](Button&) {
		memcpy(txt, txtinput, max_len + 1);
		on_changed(this->value());
		nav.pop();
	};

	update_text();
}

bool HandWriteView::on_touch(const TouchEvent event) {
	if (event.type == ui::TouchEvent::Type::Start) {
		stroke_index = 0;
		move_wait = 3;
		tracing = true;
	}
	if (event.type == ui::TouchEvent::Type::End) {
		tracing = false;
		guess_letter();
	}
	if (event.type == ui::TouchEvent::Type::Move) {
		if (tracing) {
			current_pos = event.point;
		}
	}
	return true;
}

void HandWriteView::clear_zone(Color color, bool flash) {
	display.fill_rectangle(
		{{0, 32}, {240, 216}},
		color
	);
	if (flash) {
		flash_timer = 4;
	} else {
		_painter->draw_rectangle(
			{{0, 32}, {80, 216}},
			Color::grey()
		);
		_painter->draw_rectangle(
			{{80, 32}, {80, 216}},
			Color::grey()
		);
		_painter->draw_rectangle(
			{{160, 32}, {80, 216}},
			Color::grey()
		);
		_painter->draw_rectangle(
			{{0, 104}, {240, 72}},
			Color::grey()
		);
	}
}

void HandWriteView::guess_letter() {
	uint8_t symbol, match, count, stroke_idx, stroke_data;
	Condition cond;
	Direction dir;
	bool matched;
	
	// Letter guessing
	if (stroke_index) {
		for (symbol = 0; symbol < handwriting->letter_count; symbol++) {
			count = handwriting->letter[symbol].count;
			matched = false;
			if (count) {
				// We have a count match to do
				if ((count == 1) && (stroke_index == 1)) matched = true;
				if ((count == 2) && (stroke_index == 2)) matched = true;
				if ((count == 3) && (stroke_index > 2)) matched = true;
			} else {
				matched = true;
			}
			if (matched) {
				for (match = 0; match < 3; match++) {
					cond = handwriting->letter[symbol].match[match].cond;
					dir = handwriting->letter[symbol].match[match].dir;
					if ((cond != cond_empty) && (dir != dir_empty)) {
						if (cond == last) {
							if (stroke_index)
								stroke_idx = stroke_index - 1;
							else
								stroke_idx = 0;
						} else if (cond == stroke_a)
							stroke_idx = 0;
						else if (cond == stroke_b)
							stroke_idx = 1;
						else if (cond == stroke_c)
							stroke_idx = 2;
						else
							stroke_idx = 3;
						if (stroke_idx >= stroke_index) break;
						stroke_data = stroke_list[stroke_idx];
						if ((dir & 0xF0) == 0xF0) {
							if ((dir & 0x0F) != (stroke_data & 0x0F)) break;
						} else if ((dir & 0x0F) == 0x0F) {
							if ((dir & 0xF0) != (stroke_data & 0xF0)) break;
						} else {
							if (dir != stroke_data) break;
						}
					}
				}
				if (match == 3)
					break;
				else
					matched = false;
			}
		}
		if (matched) {
			if (symbol) {
				if (_lowercase)
					char_add('a' + symbol - 1);
				else
					char_add('A' + symbol - 1);
				clear_zone(Color::green(), true);	// Green flash
			} else {
				if (txtidx) {
					txtinput[--txtidx] = 0;				// Erase
					clear_zone(Color::yellow(), true);	// Yellow flash
				} else {
					clear_zone(Color::red(), true);		// Red flash
				}
			}
		} else {
			clear_zone(Color::red(), true);		// Red flash
		}
	} else {
		// Short tap is space
		txtinput[txtidx++] = ' ';
		clear_zone(Color::green(), true);		// Green flash
	}
	update_text();
	stroke_index = 0;
}

void HandWriteView::add_stroke(uint8_t dir) {
	if (stroke_index < 8) {
		stroke_list[stroke_index] = dir;
		stroke_index++;
	} else {
		guess_letter();
	}
}

void HandWriteView::sample_pen() {
	int16_t diff_x, diff_y;
	uint8_t dir, dir_ud, dir_lr, stroke_prev;
	
	// Blink cursor
	if (!(sample_skip & 15)) {
		Point cursor_pos;
		
		cursor_pos.x = text_input.screen_rect().pos.x + (txtidx * 8);
		cursor_pos.y = text_input.screen_rect().pos.y + 16 - 4;
		
		if (cursor) {
			display.fill_rectangle(
				{cursor_pos, {text_input.screen_rect().size.w - cursor_pos.x, 4}},
				Color::black()
			);
		} else {
			display.fill_rectangle(
				{cursor_pos, {8, 4}},
				Color::white()
			);
		}
		cursor = !cursor;	
	}
	
	if (flash_timer) {
		if (flash_timer == 1) clear_zone(Color::black(), false);
		flash_timer--;
	}
	
	if (!(sample_skip & 1)) {
		if (tracing) {
			if (move_wait) {
				move_wait--;	// ~100ms delay to get rid of jitter from touch start
			} else {
				diff_x = current_pos.x - last_pos.x;
				diff_y = current_pos.y - last_pos.y;

				if (current_pos.y <= 240) {
					display.fill_rectangle(
						{{current_pos.x, current_pos.y}, {4, 4}},
						Color::grey()
					);
				}
				
				dir = 0;				// UL by default
				if (abs(diff_x) > 7) {
					if (diff_x > 0)
						dir |= 0x01;	// R
				} else {
					dir |= 0x02;		// ?
				}
				if (abs(diff_y) > 7) {
					if (diff_y > 0)
						dir |= 0x10;	// D
				} else {
					dir |= 0x20;		// ?
				}
				
				// Need at least two identical directions to validate stroke
				if ((dir & 0x11) == (dir_prev & 0x11))
					dir_cnt++;
				else
					dir_cnt = 0;
				dir_prev = dir;

				if (dir_cnt > 1) {
					dir_cnt = 0;
					if (stroke_index) {
						if ((stroke_list[stroke_index - 1] != dir) && (dir != 0x22)) {
							// Register stroke if different from last one
							dir_ud = (dir & 0xF0);
							dir_lr = (dir & 0x0F);
							stroke_prev = stroke_list[stroke_index - 1];
							if (dir_ud == 0x20) {
								// LR changed
								if ((stroke_prev & 0x0F) != dir_lr) add_stroke(dir);
							} else if (dir_lr == 0x02) {
								// UD changed
								if ((stroke_prev & 0xF0) != dir_ud) add_stroke(dir);
							} else {
								// Add direction
								if (((stroke_prev & 0xF0) == 0x20) && (dir_ud != 0x20)) {
									// Add UD
									if ((stroke_prev & 0x0F) == dir_lr) {
										// Replace totally
										stroke_list[stroke_index - 1] = dir;
									} else if (dir_lr == 0x02) {
										// Merge UD
										stroke_list[stroke_index - 1] = dir_ud | (stroke_prev & 0x0F);
									} else {
										add_stroke(dir);
									}
								} else if (((stroke_prev & 0x0F) == 0x02) && (dir_lr != 0x02)) {
									// Add LR
									if ((stroke_prev & 0xF0) == dir_ud) {
										// Replace totally
										stroke_list[stroke_index - 1] = dir;
									} else if (dir_ud == 0x20) {
										// Merge LR
										stroke_list[stroke_index - 1] = dir_lr | (stroke_prev & 0xF0);
									} else {
										add_stroke(dir);
									}
								} else {
									add_stroke(dir);
								}
							}
						}
					} else {
						// Register first stroke
						if (dir != 0x22) add_stroke(dir);
					}
				}
			}
		
			last_pos = current_pos;
		}
	}
	
	sample_skip++;
}

void HandWriteView::on_show() {
	// Use screen refresh rate as sampling frequency
	EventDispatcher::message_map().unregister_handler(Message::ID::DisplayFrameSync);
	EventDispatcher::message_map().register_handler(Message::ID::DisplayFrameSync,
		[this](const Message* const) {
			sample_pen();
		}
	);
	clear_zone(Color::black(), false);
}

void HandWriteView::on_hide() {
	EventDispatcher::message_map().unregister_handler(Message::ID::DisplayFrameSync);
}

char * HandWriteView::value() {
	return txtinput;
}

void HandWriteView::on_button(Button& button) {
	char_add(button.id);
	update_text();
}

void HandWriteView::char_add(const char c) {
	if (txtidx < _max_len) {
		txtinput[txtidx] = c;
		txtidx++;
	}
}

void HandWriteView::update_text() {
	text_input.set(txtinput);
}

}
