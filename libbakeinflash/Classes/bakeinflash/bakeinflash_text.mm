// bakeinflash_text.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Code for the text tags.


//TODO: csm_textsetting() is common for text_def & edit_text_def
//	therefore make new base class for text_def & edit_text_def
//	and move csm_textsetting() to it.

#include "bakeinflash/bakeinflash_text.h"
#include "bakeinflash/bakeinflash_sprite.h"
#include "bakeinflash/bakeinflash_log.h"
#include "base/tu_file.h"

extern bool s_has_virtual_keyboard;

#if ANDROID == 1
	extern JNIEnv*  s_jenv;
	extern float s_scale;
	extern int s_x0;
	extern int s_y0;
	static float s_retina = 1;
	extern tu_string s_pkgName;
#endif

#if iOS == 1
  #import <UIKit/UIView.h>
  #import <UIKit/UITextField.h>
  #import <QuartzCore/QuartzCore.h>
  #import "bakeinflash/bakeinflash_text_.h"

  @interface EAGLView : UIView <UITextFieldDelegate> @end
  extern EAGLView* s_view;
  extern float s_scale;
  extern float s_retina;
  extern int s_x0;
  extern int s_y0;
#endif

#ifdef WINPHONE

	#include "renderer.h"

	using namespace Windows::Foundation;
	using namespace Windows::UI::Core;
	using namespace Windows::Phone::UI::Core;
	using namespace Windows::System;
	using namespace Platform;

	extern renderer^ m_renderer;

	myWindowsTextBox::myWindowsTextBox(void* textfield) :
		m_textfield(textfield)
	{
		m_inputBuffer = ref new KeyboardInputBuffer();
		m_inputBuffer->TextChanged += ref new TypedEventHandler<KeyboardInputBuffer^, CoreTextChangedEventArgs^>(this, &myWindowsTextBox::OnTextChanged);
	}

	void myWindowsTextBox::setText(Platform::String^ str)
	{
		m_inputBuffer->Text = str;
	}

	void myWindowsTextBox::setFocus(bool hasFocus)
	{
		if (m_hasFocus == hasFocus)
		{
			return;
		}

		Windows::UI::Core::CoreWindow^ w = m_renderer->getMainWindow();
		m_hasFocus = hasFocus;
		if (m_hasFocus)
		{
			bakeinflash::edit_text_character* et = (bakeinflash::edit_text_character*) m_textfield;

			const size_t cSize = strlen(et->m_text.c_str()) + 1;
			wchar_t* wc = new wchar_t[cSize];
			mbstowcs (wc, et->m_text.c_str(), cSize);

			String^ s = ref new String(wc);
			setText(s);

			m_inputBuffer->InputScope = Windows::Phone::UI::Core::CoreInputScope::Url;
			w->KeyboardInputBuffer = m_inputBuffer;
			w->IsKeyboardInputEnabled = true;
		}
		else
		{
			w->IsKeyboardInputEnabled = false;
		}
	}

	void myWindowsTextBox::OnTextChanged(KeyboardInputBuffer^ sender, CoreTextChangedEventArgs^ args)
	{
		tu_string s;
		tu_string::encode_utf8_from_wchar(&s, m_inputBuffer->Text->Data());
		//bakeinflash::myprintf("OnTextChanged: '%s'\n", s.c_str());

		bakeinflash::edit_text_character* et = (bakeinflash::edit_text_character*) m_textfield;
		et->set_text(s);

		// move to end
		et->m_cursor = s.size() + 1;
	}

#endif

#ifndef WINPHONE
extern int s_real_fps;
#endif

namespace bakeinflash
{
	// use safe container
	static array<int> s_bakeinflash_key_to_ascii;
	static array<int> s_bakeinflash_key_to_ascii_shifted;

	static void initialize_key_to_ascii()
	{
		if (s_bakeinflash_key_to_ascii.size() > 0)
		{
			return;
		}

		s_bakeinflash_key_to_ascii.resize(key::KEYCOUNT);
		s_bakeinflash_key_to_ascii_shifted.resize(key::KEYCOUNT);

		// the letters
		for (int i = 0; i < s_bakeinflash_key_to_ascii.size(); i++)
		{
			if (i >= key::A && i <= key::Z)
			{
				s_bakeinflash_key_to_ascii[i]         = 'a' + ( i - key::A );
				s_bakeinflash_key_to_ascii_shifted[i] = 'A' + ( i - key::A );
			}
			else
			{
				s_bakeinflash_key_to_ascii[i]         = 0;
				s_bakeinflash_key_to_ascii_shifted[i] = 0;
			}
		}

		// numbers
		s_bakeinflash_key_to_ascii_shifted[key::KP_0] = s_bakeinflash_key_to_ascii[key::KP_0] = s_bakeinflash_key_to_ascii[key::_0] = '0'; s_bakeinflash_key_to_ascii_shifted[key::_0] = ')';
		s_bakeinflash_key_to_ascii_shifted[key::KP_1] = s_bakeinflash_key_to_ascii[key::KP_1] = s_bakeinflash_key_to_ascii[key::_1] = '1'; s_bakeinflash_key_to_ascii_shifted[key::_1] = '!';
		s_bakeinflash_key_to_ascii_shifted[key::KP_2] = s_bakeinflash_key_to_ascii[key::KP_2] = s_bakeinflash_key_to_ascii[key::_2] = '2'; s_bakeinflash_key_to_ascii_shifted[key::_2] = '@';
		s_bakeinflash_key_to_ascii_shifted[key::KP_3] = s_bakeinflash_key_to_ascii[key::KP_3] = s_bakeinflash_key_to_ascii[key::_3] = '3'; s_bakeinflash_key_to_ascii_shifted[key::_3] = '#';
		s_bakeinflash_key_to_ascii_shifted[key::KP_4] = s_bakeinflash_key_to_ascii[key::KP_4] = s_bakeinflash_key_to_ascii[key::_4] = '4'; s_bakeinflash_key_to_ascii_shifted[key::_4] = '$';
		s_bakeinflash_key_to_ascii_shifted[key::KP_5] = s_bakeinflash_key_to_ascii[key::KP_5] = s_bakeinflash_key_to_ascii[key::_5] = '5'; s_bakeinflash_key_to_ascii_shifted[key::_5] = '%';
		s_bakeinflash_key_to_ascii_shifted[key::KP_6] = s_bakeinflash_key_to_ascii[key::KP_6] = s_bakeinflash_key_to_ascii[key::_6] = '6'; s_bakeinflash_key_to_ascii_shifted[key::_6] = '^';
		s_bakeinflash_key_to_ascii_shifted[key::KP_7] = s_bakeinflash_key_to_ascii[key::KP_7] = s_bakeinflash_key_to_ascii[key::_7] = '7'; s_bakeinflash_key_to_ascii_shifted[key::_7] = '&';
		s_bakeinflash_key_to_ascii_shifted[key::KP_8] = s_bakeinflash_key_to_ascii[key::KP_8] = s_bakeinflash_key_to_ascii[key::_8] = '8'; s_bakeinflash_key_to_ascii_shifted[key::_8] = '*';
		s_bakeinflash_key_to_ascii_shifted[key::KP_9] = s_bakeinflash_key_to_ascii[key::KP_9] = s_bakeinflash_key_to_ascii[key::_9] = '9'; s_bakeinflash_key_to_ascii_shifted[key::_9] = '(';

		// number pad others
		s_bakeinflash_key_to_ascii_shifted[key::KP_MULTIPLY] = s_bakeinflash_key_to_ascii[key::KP_MULTIPLY] = '*';
		s_bakeinflash_key_to_ascii_shifted[key::KP_ADD]      = s_bakeinflash_key_to_ascii[key::KP_ADD]      = '+';
		s_bakeinflash_key_to_ascii_shifted[key::KP_SUBTRACT] = s_bakeinflash_key_to_ascii[key::KP_SUBTRACT] = '-';
		s_bakeinflash_key_to_ascii_shifted[key::KP_DECIMAL]  = s_bakeinflash_key_to_ascii[key::KP_DECIMAL]  = '.';
		s_bakeinflash_key_to_ascii_shifted[key::KP_DIVIDE]   = s_bakeinflash_key_to_ascii[key::KP_DIVIDE]   = '/';

		// the rest...
		s_bakeinflash_key_to_ascii_shifted[key::SPACE] =  s_bakeinflash_key_to_ascii[key::SPACE]                     = ' ';
		s_bakeinflash_key_to_ascii[key::SEMICOLON]     = ';'; s_bakeinflash_key_to_ascii_shifted[key::SEMICOLON]     = ':';
		s_bakeinflash_key_to_ascii[key::EQUALS]        = '='; s_bakeinflash_key_to_ascii_shifted[key::EQUALS]        = '+';
		s_bakeinflash_key_to_ascii[key::MINUS]         = '-'; s_bakeinflash_key_to_ascii_shifted[key::MINUS]         = '_';
		s_bakeinflash_key_to_ascii[key::SLASH]         = '/'; s_bakeinflash_key_to_ascii_shifted[key::SLASH]         = '?';
		s_bakeinflash_key_to_ascii[key::BACKTICK]      = '`'; s_bakeinflash_key_to_ascii_shifted[key::BACKTICK]      = '~';
		s_bakeinflash_key_to_ascii[key::LEFT_BRACKET]  = '['; s_bakeinflash_key_to_ascii_shifted[key::LEFT_BRACKET]  = '{';
		s_bakeinflash_key_to_ascii[key::BACKSLASH]     = '\\';s_bakeinflash_key_to_ascii_shifted[key::BACKSLASH]     = '|';
		s_bakeinflash_key_to_ascii[key::RIGHT_BRACKET] = ']'; s_bakeinflash_key_to_ascii_shifted[key::RIGHT_BRACKET] = '}';
		s_bakeinflash_key_to_ascii[key::QUOTE]        = '\''; s_bakeinflash_key_to_ascii_shifted[key::QUOTE]         = '\"';
		s_bakeinflash_key_to_ascii[key::COMMA]         = ','; s_bakeinflash_key_to_ascii_shifted[key::COMMA]         = '<';
		s_bakeinflash_key_to_ascii[key::PERIOD]        = '.'; s_bakeinflash_key_to_ascii_shifted[key::PERIOD]        = '>';
		s_bakeinflash_key_to_ascii[key::ENTER]        = '\n'; s_bakeinflash_key_to_ascii_shifted[key::ENTER]        = '\n';

		// rus
		s_bakeinflash_key_to_ascii[key::RUS_a1]     = 0xD0B0; s_bakeinflash_key_to_ascii_shifted[key::RUS_1]     = 0xD090;
		s_bakeinflash_key_to_ascii[key::RUS_a2]     = 0xD0B1; s_bakeinflash_key_to_ascii_shifted[key::RUS_2]     = 0xD091;
		s_bakeinflash_key_to_ascii[key::RUS_a3]     = 0xD0B2; s_bakeinflash_key_to_ascii_shifted[key::RUS_3]     = 0xD092;
		s_bakeinflash_key_to_ascii[key::RUS_a4]     = 0xD0B3; s_bakeinflash_key_to_ascii_shifted[key::RUS_4]     = 0xD093;
		s_bakeinflash_key_to_ascii[key::RUS_a5]     = 0xD0B4; s_bakeinflash_key_to_ascii_shifted[key::RUS_5]     = 0xD094;
		s_bakeinflash_key_to_ascii[key::RUS_a6]     = 0xD0B5; s_bakeinflash_key_to_ascii_shifted[key::RUS_6]     = 0xD095;
		s_bakeinflash_key_to_ascii[key::RUS_a7]     = 0xD0B6; s_bakeinflash_key_to_ascii_shifted[key::RUS_7]     = 0xD096;
		s_bakeinflash_key_to_ascii[key::RUS_a8]     = 0xD0B7; s_bakeinflash_key_to_ascii_shifted[key::RUS_8]     = 0xD097;
		s_bakeinflash_key_to_ascii[key::RUS_a9]     = 0xD0B8; s_bakeinflash_key_to_ascii_shifted[key::RUS_9]     = 0xD098;
		s_bakeinflash_key_to_ascii[key::RUS_a10]    = 0xD0B9; s_bakeinflash_key_to_ascii_shifted[key::RUS_10]     = 0xD099;
		s_bakeinflash_key_to_ascii[key::RUS_a11]    = 0xD0BA; s_bakeinflash_key_to_ascii_shifted[key::RUS_11]     = 0xD09A;
		s_bakeinflash_key_to_ascii[key::RUS_a12]    = 0xD0BB; s_bakeinflash_key_to_ascii_shifted[key::RUS_12]     = 0xD09B;
		s_bakeinflash_key_to_ascii[key::RUS_a13]    = 0xD0BC; s_bakeinflash_key_to_ascii_shifted[key::RUS_13]     = 0xD09C;
		s_bakeinflash_key_to_ascii[key::RUS_a14]    = 0xD0BD; s_bakeinflash_key_to_ascii_shifted[key::RUS_14]     = 0xD09D;
		s_bakeinflash_key_to_ascii[key::RUS_a15]    = 0xD0BE; s_bakeinflash_key_to_ascii_shifted[key::RUS_15]     = 0xD09E;
		s_bakeinflash_key_to_ascii[key::RUS_a16]    = 0xD0BF; s_bakeinflash_key_to_ascii_shifted[key::RUS_16]     = 0xD09F;
		s_bakeinflash_key_to_ascii[key::RUS_a17]    = 0xD180; s_bakeinflash_key_to_ascii_shifted[key::RUS_17]     = 0xD0A0;
		s_bakeinflash_key_to_ascii[key::RUS_a18]    = 0xD181; s_bakeinflash_key_to_ascii_shifted[key::RUS_18]     = 0xD0A1;
		s_bakeinflash_key_to_ascii[key::RUS_a19]    = 0xD182; s_bakeinflash_key_to_ascii_shifted[key::RUS_19]     = 0xD0A2;
		s_bakeinflash_key_to_ascii[key::RUS_a20]    = 0xD183; s_bakeinflash_key_to_ascii_shifted[key::RUS_20]     = 0xD0A3;
		s_bakeinflash_key_to_ascii[key::RUS_a21]    = 0xD184; s_bakeinflash_key_to_ascii_shifted[key::RUS_21]     = 0xD0A4;
		s_bakeinflash_key_to_ascii[key::RUS_a22]    = 0xD185; s_bakeinflash_key_to_ascii_shifted[key::RUS_22]     = 0xD0A5;
		s_bakeinflash_key_to_ascii[key::RUS_a23]    = 0xD186; s_bakeinflash_key_to_ascii_shifted[key::RUS_23]     = 0xD0A6;
		s_bakeinflash_key_to_ascii[key::RUS_a24]    = 0xD187; s_bakeinflash_key_to_ascii_shifted[key::RUS_24]     = 0xD0A7;
		s_bakeinflash_key_to_ascii[key::RUS_a25]    = 0xD188; s_bakeinflash_key_to_ascii_shifted[key::RUS_25]     = 0xD0A8;
		s_bakeinflash_key_to_ascii[key::RUS_a26]    = 0xD189; s_bakeinflash_key_to_ascii_shifted[key::RUS_26]     = 0xD0A9;
		s_bakeinflash_key_to_ascii[key::RUS_a27]    = 0xD18A; s_bakeinflash_key_to_ascii_shifted[key::RUS_27]     = 0xD0AA;
		s_bakeinflash_key_to_ascii[key::RUS_a28]    = 0xD18B; s_bakeinflash_key_to_ascii_shifted[key::RUS_28]     = 0xD0AB;
		s_bakeinflash_key_to_ascii[key::RUS_a29]    = 0xD18C; s_bakeinflash_key_to_ascii_shifted[key::RUS_29]     = 0xD0AC;
		s_bakeinflash_key_to_ascii[key::RUS_a30]    = 0xD18D; s_bakeinflash_key_to_ascii_shifted[key::RUS_30]     = 0xD0AD;
		s_bakeinflash_key_to_ascii[key::RUS_a31]    = 0xD18E; s_bakeinflash_key_to_ascii_shifted[key::RUS_31]     = 0xD0AE;
		s_bakeinflash_key_to_ascii[key::RUS_a32]    = 0xD18F; s_bakeinflash_key_to_ascii_shifted[key::RUS_32]     = 0xD0AF;
	}

	void	as_global_textfield_ctor(const fn_call& fn)
	// Constructor for ActionScript class XMLSocket
	{
		edit_text_character_def* empty_text_def = new edit_text_character_def(0, 0);
		character* ch = new edit_text_character(NULL, empty_text_def, 0);
		fn.result->set_as_object(ch);
	}

	text_character_def::text_character_def(movie_definition_sub* root_def) :
		m_root_def(root_def), 
		m_use_flashtype(false),
		m_grid_fit(0), 
		m_thickness(0.0f),
		m_sharpness(0.0f)
	{
		assert(m_root_def);
	}

	text_character_def::~text_character_def()
	{
	}

	void	text_character_def::read(stream* in, int tag_type, movie_definition_sub* m)
	{
		assert(m != NULL);
		assert(tag_type == 11 || tag_type == 33);

		m_rect.read(in);
		m_matrix.read(in);

		int	glyph_bits = in->read_u8();
		int	advance_bits = in->read_u8();

		IF_VERBOSE_PARSE(myprintf("begin text records\n"));

		bool	last_record_was_style_change = false;

		text_style	style;
		for (;;)
		{
			int	first_byte = in->read_u8();

			if (first_byte == 0)
			{
				// This is the end of the text records.
				IF_VERBOSE_PARSE(myprintf("end text records\n"));
				break;
			}

			// Style changes and glyph records just alternate.
			// (Contrary to what most SWF references say!)
			if (last_record_was_style_change == false)
			{
				// This is a style change.

				last_record_was_style_change = true;

				bool	has_font = (first_byte >> 3) & 1;
				bool	has_color = (first_byte >> 2) & 1;
				bool	has_y_offset = (first_byte >> 1) & 1;
				bool	has_x_offset = (first_byte >> 0) & 1;

				IF_VERBOSE_PARSE(myprintf("  text style change\n"));

				if (has_font)
				{
					Uint16	font_id = in->read_u16();
					style.m_font_id = font_id;
					IF_VERBOSE_PARSE(myprintf("  has_font: font id = %d\n", font_id));
				}
				if (has_color)
				{
					if (tag_type == 11)
					{
						style.m_color.read_rgb(in);
					}
					else
					{
						assert(tag_type == 33);
						style.m_color.read_rgba(in);
					}
					IF_VERBOSE_PARSE(myprintf("  has_color\n"));
				}
				if (has_x_offset)
				{
					style.m_has_x_offset = true;
					style.m_x_offset = in->read_s16();
					IF_VERBOSE_PARSE(myprintf("  has_x_offset = %g\n", style.m_x_offset));
				}
				else
				{
					style.m_has_x_offset = false;
					style.m_x_offset = 0.0f;
				}
				if (has_y_offset)
				{
					style.m_has_y_offset = true;
					style.m_y_offset = in->read_s16();
					IF_VERBOSE_PARSE(myprintf("  has_y_offset = %g\n", style.m_y_offset));
				}
				else
				{
					style.m_has_y_offset = false;
					style.m_y_offset = 0.0f;
				}
				if (has_font)
				{
					style.m_text_height = in->read_u16();
					IF_VERBOSE_PARSE(myprintf("  text_height = %g\n", style.m_text_height));
				}
			}
			else
			{
				// Read the glyph record.

				last_record_was_style_change = false;

				int	glyph_count = first_byte;

				m_text_glyph_records.resize(m_text_glyph_records.size() + 1);
				m_text_glyph_records.back().m_style = style;
				m_text_glyph_records.back().read(in, glyph_count, glyph_bits, advance_bits);

				IF_VERBOSE_PARSE(myprintf("  glyph_records: count = %d\n", glyph_count));
			}
		}
	}

	void	text_character_def::display(character* inst)
	// Draw the string.
	{
		if (m_text_glyph_records.size() < 1)
		{
			return;
		}

		matrix  mat;
		inst->get_world_matrix(&mat);

		const text_glyph_record&	rec0 = m_text_glyph_records[0];
		if (m_text_rendered == NULL)
		{
			rect bound;
			get_bound(&bound);

			float xscale = mat.get_x_scale();
			float yscale = mat.get_y_scale();
		//	float scale = fmin(xscale, yscale);

			matrix m;
			m.set_scale_rotation(xscale, yscale, 0);
			m.transform(&bound);

			array<int> xleading;
			array<int> yleading;

			int w = (int) ceilf(TWIPS_TO_PIXELS(bound.width()));
			int h = (int) ceilf(TWIPS_TO_PIXELS(bound.height()));

			glyph_provider* fp = get_glyph_provider();
			assert(fp);

			int fontsize = 0;
			font*	fnt = NULL;
			tu_string str;
			for (int i = 0; i < m_text_glyph_records.size(); i++)
			{
				const text_glyph_record&	rec = m_text_glyph_records[i];
				if (fnt == NULL)
				{
					rec.m_style.resolve_font(m_root_def);
					fnt = rec.m_style.m_font;
					assert(fnt);

					fontsize = (int) (rec.m_style.m_text_height / 20.0f * yscale);
				}

				for (int j = 0; j < rec.m_glyphs.size(); j++)
				{
					Uint16	code = fnt->get_code_by_index(rec.m_glyphs[j].m_glyph_index);
					str.append_wide_char(code);
				}

				float xl = rec.m_style.m_has_x_offset ? rec.m_style.m_x_offset + m_matrix.m_[0][2]	- m_rect.m_x_min : 0;
				xl *= xscale;
				xleading.push_back((int) TWIPS_TO_PIXELS(xl));

				float yl = rec.m_style.m_has_y_offset ? rec.m_style.m_y_offset + m_matrix.m_[1][2]	- m_rect.m_y_min : 0;
				yl *= yscale;
				yleading.push_back((int) TWIPS_TO_PIXELS(yl));

				if (i < m_text_glyph_records.size() - 1)
				{
					str += '\n';
				}
			}
			//printf("%s", str.c_str());

			int alignment = 0;
			int vert_advance = yleading.size() > 1 ? yleading[1] - yleading[0] : 0;
			image::alpha* im = fp->render_string(str, alignment, fnt->get_name(),	
				fnt->is_bold(), fnt->is_italic(), fontsize, xleading, yleading, vert_advance, w, h, true, -1, xscale / yscale, yscale);
			m_text_rendered = render::create_bitmap_info(im);

			// keep bound's ratio
			m_text_rendered->set_xratio(w / (float) p2(w));
			m_text_rendered->set_yratio(h / (float) p2(h));
		}

		if (m_text_rendered != NULL)
		{
			rect uv_bounds(0, m_text_rendered->get_xratio(), 0, m_text_rendered->get_yratio());

			cxform	cx;
			inst->get_world_cxform(&cx);
			rgba	transformed_color = cx.transform(rec0.m_style.m_color);

			rect bound;
			get_bound(&bound);
			render::draw_bitmap(mat, m_text_rendered, bound, uv_bounds, transformed_color);
		}

	}

	void	text_character_def::csm_textsetting(stream* in, int tag_type)
	{
		assert(tag_type == 74);	// Flash 8

		m_use_flashtype = in->read_uint(2) == 0 ? false : true;

		// 0 = Do not use grid fitting. AlignmentZones and LCD sub-pixel information
		// will not be used.
		// 1 = Pixel grid fit. Only supported for left-aligned dynamic text.
		// This setting provides the ultimate in FlashType readability, with
		// crisp letters aligned to pixels.
		// 2 = Sub-pixel grid fit. Align letters to the 1/3 pixel used by LCD monitors.
		// Can also improve quality for CRT output.
		m_grid_fit = in->read_uint(3);

		in->read_uint(3); // reserved;

		m_thickness = in->read_fixed();
		m_sharpness = in->read_fixed();

		in->read_u8();	// reserved

		IF_VERBOSE_PARSE(
										 myprintf("reading CSMTextSetting tag\n");
										 myprintf("	m_use_flashtype = %s\n", m_use_flashtype ? "true" : "false");
										 myprintf("	m_grid_fit = %d\n", m_grid_fit);
										 myprintf("	m_thickness = %f\n", m_thickness);
										 myprintf("	m_sharpness = %f\n", m_sharpness);
										 );

	}

	void text_character_def::get_bound(rect* bound)
	{
		*bound = m_rect;
	}

	void	define_text_loader(stream* in, int tag_type, movie_definition_sub* m)
	// Read a DefineText tag.
	{
		assert(tag_type == 11 || tag_type == 33);

		Uint16	character_id = in->read_u16();

		text_character_def*	ch = new text_character_def(m);
		IF_VERBOSE_PARSE(myprintf("text_character, id = %d\n", character_id));
		ch->read(in, tag_type, m);

		// IF_VERBOSE_PARSE(print some stuff);

		m->add_character(character_id, ch);
	}


	//
	// edit_text_character_def
	//

	// creates empty dynamic text field
	edit_text_character_def::edit_text_character_def(int width, int height) :
	m_root_def(NULL),
	m_word_wrap(false),
	m_multiline(false),
	m_password(false),
	m_readonly(false),
	m_auto_size(false),
	m_no_select(false),
	m_border(false),
	m_html(false),
	m_use_outlines(false),
	m_font_id(-1),
	m_font(NULL),
	m_text_height(PIXELS_TO_TWIPS(12)),	//	default 12
	m_max_length(0),
	m_alignment(ALIGN_LEFT),
	m_left_margin(0.0f),
	m_right_margin(0.0f),
	m_indent(0.0f),
	m_leading(0.0f),
	m_use_flashtype(false),
	m_grid_fit(0),
	m_thickness(0.0f),
	m_sharpness(0.0f)
	{
		m_rect.m_x_min = 0;
		m_rect.m_y_min = 0;
		m_rect.m_x_max = PIXELS_TO_TWIPS(width);
		m_rect.m_y_max = PIXELS_TO_TWIPS(height);

		m_color.set(0, 0, 0, 255);
		m_font = new font();
	}

	edit_text_character_def::edit_text_character_def(movie_definition_sub* root_def) :
		m_root_def(root_def),
		m_word_wrap(false),
		m_multiline(false),
		m_password(false),
		m_readonly(false),
		m_auto_size(false),
		m_no_select(false),
		m_border(false),
		m_html(false),
		m_use_outlines(false),
		m_font_id(-1),
		m_font(NULL),
		m_text_height(PIXELS_TO_TWIPS(12)),	//	default 12
		m_max_length(0),
		m_alignment(ALIGN_LEFT),
		m_left_margin(0.0f),
		m_right_margin(0.0f),
		m_indent(0.0f),
		m_leading(0.0f),
		m_use_flashtype(false),
		m_grid_fit(0),
		m_thickness(0.0f),
		m_sharpness(0.0f)
	{
		assert(m_root_def);
		m_color.set(0, 0, 0, 255);
	}

	edit_text_character_def::~edit_text_character_def()
	{
	}

	void	edit_text_character_def::read(stream* in, int tag_type, movie_definition_sub* m)
	{
		assert(m != NULL);
		assert(tag_type == 37);

		m_rect.read(in);

		in->align();
		bool	has_text = in->read_uint(1) ? true : false;
		m_word_wrap = in->read_uint(1) ? true : false;
		m_multiline = in->read_uint(1) ? true : false;
		m_password = in->read_uint(1) ? true : false;
		m_readonly = in->read_uint(1) ? true : false;
		bool	has_color = in->read_uint(1) ? true : false;
		bool	has_max_length = in->read_uint(1) ? true : false;
		bool	has_font = in->read_uint(1) ? true : false;

		in->read_uint(1);	// reserved
		m_auto_size = in->read_uint(1) ? true : false;
		bool	has_layout = in->read_uint(1) ? true : false;
		m_no_select = in->read_uint(1) ? true : false;
		m_border = in->read_uint(1) ? true : false;
		in->read_uint(1);	// reserved
		m_html = in->read_uint(1) ? true : false;
		m_use_outlines = in->read_uint(1) ? true : false;

		if (has_font)
		{
			m_font_id = in->read_u16();
			m_text_height = (float) in->read_u16();
		}

		if (has_color)
		{
			m_color.read_rgba(in);
		}

		if (has_max_length)
		{
			m_max_length = in->read_u16();
		}

		if (has_layout)
		{
			m_alignment = (alignment) in->read_u8();
			m_left_margin = (float) in->read_u16();
			m_right_margin = (float) in->read_u16();
			m_indent = (float) in->read_s16();
			m_leading = (float) in->read_s16();
		}

		in->read_string(&m_var_name);

		if (has_text)
		{
			in->read_string(&m_default_text);
		}

		IF_VERBOSE_PARSE(myprintf("edit_text_char, varname = %s, text = %s\n",
														 m_var_name.c_str(), m_default_text.c_str()));
	}

	void	edit_text_character_def::csm_textsetting(stream* in, int tag_type)
	{
		assert(tag_type == 74);	// Flash 8
		m_use_flashtype = in->read_uint(2) == 0 ? false : true;

		// 0 = Do not use grid fitting. AlignmentZones and LCD sub-pixel information
		// will not be used.
		// 1 = Pixel grid fit. Only supported for left-aligned dynamic text.
		// This setting provides the ultimate in FlashType readability, with
		// crisp letters aligned to pixels.
		// 2 = Sub-pixel grid fit. Align letters to the 1/3 pixel used by LCD monitors.
		// Can also improve quality for CRT output.
		m_grid_fit = in->read_uint(3);

		in->read_uint(3); // reserved;

		m_thickness = in->read_fixed();
		m_sharpness = in->read_fixed();

		in->read_u8();	// reserved

		IF_VERBOSE_PARSE(
										 myprintf("reading CSMTextSetting tag\n");
										 myprintf("	m_use_flashtype = %s\n", m_use_flashtype ? "true" : "false");
										 myprintf("	m_grid_fit = %d\n", m_grid_fit);
										 myprintf("	m_thickness = %f\n", m_thickness);
										 myprintf("	m_sharpness = %f\n", m_sharpness);
										 );

	}

	//
	// edit_text_character
	//

	void	variable_getter(const fn_call& fn)
	{
		edit_text_character* et = cast_to<edit_text_character>(fn.this_ptr);
		assert(et);
		fn.result->set_tu_string(et->get_var_name());
	}

	void	variable_setter(const fn_call& fn)
	{
		edit_text_character* et = cast_to<edit_text_character>(fn.this_ptr);
		assert(et);
		const tu_string& vname = fn.arg(0).to_string();
		et->set_var_name(vname);
	}

	void	placeholder_getter(const fn_call& fn)
	{
		edit_text_character* et = cast_to<edit_text_character>(fn.this_ptr);
		assert(et);
		tu_string ph;
#if iOS == 1
    myTextField *textField = (myTextField*) et->m_ios_field;
    if (textField)
    {
      ph = [textField.placeholder UTF8String];
    }
#endif
		fn.result->set_tu_string(ph);
	}
	
	void	placeholder_setter(const fn_call& fn)
	{
		edit_text_character* et = cast_to<edit_text_character>(fn.this_ptr);
		assert(et);
#if iOS == 1
    myTextField *textField = (myTextField*) et->m_ios_field;
    if (textField)
    {
      textField.placeholder = [NSString stringWithUTF8String: (char*) fn.arg(0).to_string()];
    }
#endif

	}
	
  void	set_keyboardType(const fn_call& fn)
  {
    edit_text_character* et = cast_to<edit_text_character>(fn.this_ptr);
    if (et && fn.nargs > 0)
    {
      et->m_keyboardType = fn.arg(0).to_tu_string();
      et->resetKeyboardType();
    }
  }
  
	void	set_textformat(const fn_call& fn)
	{
		edit_text_character* et = cast_to<edit_text_character>(fn.this_ptr);
		assert(et);
		if (fn.nargs == 1)
		{
			as_textformat* tf = cast_to<as_textformat>(fn.arg(0).to_object());
			et->reset_format(tf);
		}
	}

	void	get_textHeight(const fn_call& fn)
	{
		edit_text_character* et = cast_to<edit_text_character>(fn.this_ptr);
		if (et)
		{
			fn.result->set_double(et->get_text_height());
		}
	}

	const tu_string&	edit_text_character::get_var_name() const
	{
		return m_var_name;
	}

  
  void edit_text_character::resetKeyboardType()
  {
#if iOS == 1
    int kbd;
    // Displays a keyboard which can enter ASCII characters, non-ASCII keyboards remain active
    if (m_keyboardType == "ASCIICapable")
    {
      kbd = UIKeyboardTypeASCIICapable;
    }
    else
      // Numbers and assorted punctuation.
      if (m_keyboardType == "NumbersAndPunctuation")
      {
        kbd = UIKeyboardTypeNumbersAndPunctuation;
      }
      else
        // A type optimized for URL entry (shows . / .com prominently).
        if (m_keyboardType == "URL")
        {
          kbd = UIKeyboardTypeURL;
        }
        else
          // A number pad (0-9). Suitable for PIN entry.
          if (m_keyboardType == "NumberPad")
          {
            kbd = UIKeyboardTypeNumberPad;
          }
          else
            // A phone pad (1-9, *, 0, #, with letters under the numbers).
            if (m_keyboardType == "PhonePad")
            {
              kbd = UIKeyboardTypePhonePad;
            }
            else
              // A type optimized for entering a person's name or phone number.
              if (m_keyboardType == "NamePhonePad")
              {
                kbd = UIKeyboardTypeNamePhonePad;
              }
              else
                // A type optimized for entering a person's name or phone number.
                if (m_keyboardType == "EmailAddress")
                {
                  kbd = UIKeyboardTypeEmailAddress;
                }
                else
                  if (m_keyboardType == "DecimalPad")
                  {
                    kbd = UIKeyboardTypeDecimalPad;
                  }
                  else
                  {
                    kbd = UIKeyboardTypeDefault;
                  }
    myTextField *textField = (myTextField*) m_ios_field;
    if (textField)
    {
      [textField setKeyboardType :kbd];
    }
#endif
  }
  
  void edit_text_character::set_var_name(const tu_string& name)
	{
		m_var_name = name;

		// then reset VAR / TEXT_FIELD value
		set_text_value(to_string());
	}

	character*	edit_text_character_def::create_character_instance(character* parent, int id)
	{
		if (m_font == NULL)
		{
			// Resolve the font, if possible.
			if (m_root_def)
			{
				m_font = m_root_def->get_font(m_font_id);
				if (m_font == NULL)
				{
					myprintf("error: text style with undefined font; font_id = %d\n", m_font_id);
				}
			}
		}

		edit_text_character*	ch = new edit_text_character(parent, this, id);
		// instanciate_registered_class(ch);	//TODO: test it

		return ch;
	}


	void	define_edit_text_loader(stream* in, int tag_type, movie_definition_sub* m)
	// Read a DefineText tag.
	{
		assert(tag_type == 37);

		Uint16	character_id = in->read_u16();

		edit_text_character_def*	ch = new edit_text_character_def(m);
		IF_VERBOSE_PARSE(myprintf("edit_text_char, id = %d\n", character_id));
		ch->read(in, tag_type, m);

		m->add_character(character_id, ch);
	}

	bool edit_text_character::get_topmost_mouse_entity( character * &te, float x, float y)
	{
		if (get_visible() == false)
		{
			return false;
		}

		const matrix&  m = get_matrix();

		point   p;
		m.transform_by_inverse(&p, point(x, y));

		if (m_def->m_rect.point_test(p.m_x, p.m_y))
		{
			te = this;
			return true;
		}

		return false;
	}

	const char*	edit_text_character::to_string()
	{
		// get text from VAR
		const tu_string& var = get_var_name();
		if (var.size() > 0)
		{
			as_object* ch = get_parent();	// target, default is parent
			tu_string path;

			// hack
#ifndef WINPHONE
			if (var == "FPS")
			{
				set_text(tu_string(s_real_fps));
				return m_text.c_str();
			}
#endif

			tu_string varname(var);
			if (as_environment::parse_path(var, &path, &varname))
			{
				ch = ch->find_target(path.c_str());
			}

			if (ch)
			{
				as_value val;
				if (ch->get_member(varname, &val) && val.to_object() != this)
				{
					if (val.to_tu_string() != m_text)
					{
						set_text(val.to_tu_string().c_str());
					}
				}
			}
		}

		return m_text.c_str();
	}


	bool	edit_text_character::set_member(const tu_string& name, const as_value& val)
	// We have a "text" member.
	{
		// first try text field properties
		as_standard_member	std_member = get_standard_member(name);
		switch (std_member)
		{
			default:
				break;

			case M_VISIBLE:
			{
				set_visible(val.to_bool());
				break;
			}

			case M_PASSWORD:
			{
				m_password = val.to_bool();
				break;
			}

			case M_TEXT:
			{
				set_text_value(val.to_tu_string());
				break;
			}

			case M_TEXTCOLOR:
			{
				// The arg is 0xRRGGBB format.
				rgba color(val.to_number());
				m_color.m_r = color.m_r;
				m_color.m_g = color.m_g;
				m_color.m_b = color.m_b;
				m_color.m_a = color.m_a;
				//format_text();
				break;
			}

			case M_BORDER:
				m_def->m_border = val.to_bool();
				// format_text();
				break;

			case M_MULTILINE:
				m_def->m_multiline = val.to_bool();
				// format_text();
				break;

			case M_WORDWRAP:
				m_def->m_word_wrap = val.to_bool();
				// format_text();
				break;

			case M_TYPE:
				// Specifies the type of text field.
				// There are two values: "dynamic", which specifies a dynamic text field
				// that cannot be edited by the user, and "input",
				// which specifies an input text field.
				if (val.to_tu_string() == "input")
				{
					m_def->m_readonly = false;
				}
				else
					if (val.to_tu_string() == "dynamic")
					{
						m_def->m_readonly = true;
					}
					else
					{
						// myprintf("not input &  dynamic");
					}
				break;

			case M_BACKGROUNDCOLOR:
				m_background_color = rgba(val.to_number());
				// format_text();
				break;
		}

		return character::set_member(name, val);
	}

	void edit_text_character::set_visible(bool visible)
	{
		m_visible = visible; 
#if iOS == 1
    myTextField *textField = (myTextField*) m_ios_field;
    if (textField)
    {
      textField.hidden = visible ? NO : YES;
      if (visible == false)
      {
        [textField resignFirstResponder];
      }
      
    }
#endif
	}

	bool	edit_text_character::get_member(const tu_string& name, as_value* val)
	{
		// first try text field properties
		as_standard_member	std_member = get_standard_member(name);
		switch (std_member)
		{
			default:
				break;

			case M_PASSWORD:
			{
				val->set_bool( m_password );
			}break;

			case M_TEXT:
				// hack
				//html_to_plain();

				val->set_tu_string(m_text);
				return true;

			case M_TEXTCOLOR:
				// Return color in 0xRRGGBB format
				val->set_int((m_color.m_r << 16) + (m_color.m_g << 8) + m_color.m_b);
				return true;

			case M_TEXTWIDTH:
				// Return the width, in pixels, of the text as laid out.
				// (I.e. the actual text content, not our defined
				// bounding box.)
				//
				// In local coords.  Verified against Macromedia Flash.

				// fixme
				// val->set_double(TWIPS_TO_PIXELS(m_text_bounding_box.width()));
				return true;

			case M_BORDER:
				val->set_bool(m_def->m_border);
				return true;

			case M_MULTILINE:
				val->set_bool(m_def->m_multiline);
				return true;

			case M_WORDWRAP:
				val->set_bool(m_def->m_word_wrap);
				return true;

			case M_TYPE:
				val->set_tu_string(m_def->m_readonly ? "dynamic" : "input");
				return true;

			case M_BACKGROUNDCOLOR:
				val->set_int(m_background_color.m_r << 16 | m_background_color.m_g << 8 |
										 m_background_color.m_b);
				return true;

		}

		return character::get_member(name, val);
	}

	bool edit_text_character::can_handle_mouse_event()
	{
		return m_def->m_readonly == false;
	}

	edit_text_character::edit_text_character(character* parent, edit_text_character_def* def, int id)	:
		character(parent, id),
		m_def(def),
		m_has_focus(false),
		m_password(def->m_password),
		m_cursor(0),
		m_ios_field(NULL)
	{
		assert(parent);
		assert(m_def != NULL);

		// make sure the ascii table is initialized
		initialize_key_to_ascii();

		// defaults
		m_color = m_def->m_color;
		m_text_height = m_def->m_text_height;
		m_font = m_def->m_font;
		m_alignment = m_def->m_alignment;
		m_left_margin = m_def->m_left_margin;
		m_right_margin = m_def->m_right_margin;
		m_indent = m_def->m_indent;
		m_leading = m_def->m_leading;
		m_background_color.set(255, 255, 255, 255);

		builtin_member("setTextFormat", set_textformat);
		builtin_member("variable", as_value(variable_getter, variable_setter));
		builtin_member("placeholder", as_value(placeholder_getter, placeholder_setter));
		builtin_member("textHeight", as_value(get_textHeight, as_value()));
		builtin_member("keyboardType", as_value(as_value(), set_keyboardType));
    
		m_var_name = m_def->m_var_name;

		// first set default text value
		set_text(m_def->m_default_text.c_str());

		// then reset VAR / TEXT_FIELD value
		set_text_value(to_string());

//		m_dummy_style.push_back(fill_style());
//		reset_bounding_box(0, 0);
	}

	edit_text_character::~edit_text_character()
	{
		// on_event(event_id::KILLFOCUS) will be executed
		// during remove_display_object()
		if (m_ios_field == NULL)
		{
			return;
		}

#if ANDROID == 1
		if (s_has_virtual_keyboard == false)
		{
			jclass cls = s_jenv->FindClass(s_pkgName + tu_string("myEditText"));
			jmethodID mid = s_jenv->GetMethodID(cls, "remove", "()V");
			s_jenv->CallVoidMethod(m_ios_field, mid);
			s_jenv->DeleteGlobalRef((jobject) m_ios_field);
		}
#endif

#if iOS == 1
    myTextField *textField = (myTextField*) m_ios_field;
    [textField removeFromSuperview];
    [textField release];
#endif

#ifdef WINPHONE
		m_ios_field = nullptr;
#endif
	}

	void edit_text_character::reset_format(as_textformat* tf)
	{
		if (tf == NULL)
		{
			myprintf("NULL textFormat\n");
			return;
		}

		as_value val;

		if (tf->get_member("leftMargin", &val))
		{
			m_left_margin = PIXELS_TO_TWIPS(val.to_float());
		}

		if (tf->get_member("indent", &val))
		{
			m_indent = PIXELS_TO_TWIPS(val.to_float());
		}

		if (tf->get_member("rightMargin", &val))
		{
			m_right_margin = PIXELS_TO_TWIPS(val.to_float());
		}

		if (tf->get_member("leading", &val))
		{
			m_leading = PIXELS_TO_TWIPS(val.to_float());
		}

		if (tf->get_member("color", &val))
		{
			int rgb = val.to_int();
			Uint8 r = (rgb >> 16) & 0xFF;
			Uint8 g = (rgb >> 8) & 0xFF;
			Uint8 b = rgb & 0xFF;
			m_color.set(r, g, b, 255);
		}

		if (tf->get_member("size", &val))
		{
			m_text_height = PIXELS_TO_TWIPS(val.to_float());
		}

		if (tf->get_member("align", &val))
		{
			if (val.to_tu_string() == "left")
			{
				m_alignment = edit_text_character_def::ALIGN_LEFT;
			}
			else
				if (val.to_tu_string() == "center")
				{
					m_alignment = edit_text_character_def::ALIGN_CENTER;
				}
				else
					if (val.to_tu_string() == "right")
					{
						m_alignment = edit_text_character_def::ALIGN_RIGHT;
					}
					else
						if (val.to_tu_string() == "justify")
						{
							m_alignment = edit_text_character_def::ALIGN_JUSTIFY;
						}
		}

		tu_string fontname = m_font->get_name();
		if (tf->get_member("font", &val))
		{
			fontname = val.to_tu_string();
		}

		bool bold = m_font->is_bold();
		if (tf->get_member("bold", &val))
		{
			bold = val.to_bool();
		}

		bool italic  = m_font->is_italic();
		if (tf->get_member("italic", &val))
		{
			italic = val.to_bool();
		}

		if (italic  != m_font->is_italic() ||
				bold  != m_font->is_bold() ||
				fontname  != m_font->get_name())
		{
			// try to find embedded font
			character_def* res = find_exported_resource(fontname);
			font* embedded_font = NULL;
			if (res)
			{
				embedded_font = cast_to<font>(res);
			}

			if (embedded_font)
			{
				// we have embedded font
				m_font = cast_to<font>(embedded_font);
			}
			else
			{
				m_font = new font();
			}
			m_font->set_bold(bold);
			m_font->set_italic(italic);
			m_font->set_name(fontname);
		}

		m_text_rendered = NULL;
	}

	void edit_text_character::display()
	{
#ifndef WINPHONE
		if (m_ios_field)
		{
			return;
		}
#endif

		// if world matrix was updated we should reformat a text
		matrix  mat;
		get_world_matrix(&mat);
		if (mat != m_world_matrix)
		{
			m_world_matrix = mat;
			m_text_rendered = NULL;
		}

		// Show white background + black bounding box.
		if (m_def->m_border == true)
		{
			point   coords[4];
			coords[0] = m_def->m_rect.get_corner(0);
			coords[1] = m_def->m_rect.get_corner(1);
			coords[2] = m_def->m_rect.get_corner(3);
			coords[3] = m_def->m_rect.get_corner(2);
			coord_component icoords[18] =
			{
				// strip (fill in)
				(coord_component) coords[0].m_x, (coord_component) coords[0].m_y,
				(coord_component) coords[1].m_x, (coord_component) coords[1].m_y,
				(coord_component) coords[2].m_x, (coord_component) coords[2].m_y,
				(coord_component) coords[3].m_x, (coord_component) coords[3].m_y,

				// outline
				(coord_component) coords[0].m_x, (coord_component) coords[0].m_y,
				(coord_component) coords[1].m_x, (coord_component) coords[1].m_y,
				(coord_component) coords[3].m_x, (coord_component) coords[3].m_y,
				(coord_component) coords[2].m_x, (coord_component) coords[2].m_y,
				(coord_component) coords[0].m_x, (coord_component) coords[0].m_y,
			};

			render::set_matrix(mat);
			render::fill_style_color(0,	m_background_color);
			render::draw_mesh_strip(&icoords[0], 4);

			render::line_style_color(rgba(0,0,0,255));
			render::line_style_width(0);
			render::draw_line_strip(&icoords[8], 5);
		}

		character_def* def = get_character_def();
		if (m_text_rendered == NULL)
		{

			// try as HTML
			//myprintf("html text: %s\n", m_text.c_str());
			tu_string plain_text;
			if (format_html_text(&plain_text))
			{
				m_text = plain_text;
			}

			rect bound;
			def->get_bound(&bound);

			float xscale = mat.get_x_scale();
			float yscale = mat.get_y_scale();
			//float scale = fmax(xscale, yscale);

			matrix m;
			m.set_scale_rotation(xscale, yscale, 0);
			m.transform(&bound);

			int w = (int) ceilf(TWIPS_TO_PIXELS(bound.width()));
			int h = (int) ceilf(TWIPS_TO_PIXELS(bound.height()));

			glyph_provider* fp = get_glyph_provider();
			assert(fp);

			int fontsize = (int) ceilf(m_text_height * yscale / 20.0f);

			array<int> xleading;
			xleading.push_back((int) TWIPS_TO_PIXELS(m_indent * xscale));

			array<int> yleading;
			int vert_advance = (int) TWIPS_TO_PIXELS(m_leading * yscale);

			image::alpha* im = NULL;
			int cursor = m_has_focus ? m_cursor : -1;
			bool multiline = true; // m_def->m_multiline
			if (m_password)
			{
				tu_string str;
				for (Uint32 i = 0; i < m_text.size(); i++)
				{
					str += '*';
				}
				im = fp->render_string(str, m_alignment, m_font->get_name(),
					m_font->is_bold(), m_font->is_italic(), fontsize, xleading, yleading, vert_advance, w, h, multiline, cursor, xscale / yscale, yscale);
			}
			else
			{
				im = fp->render_string(m_text, m_alignment, m_font->get_name(),
					m_font->is_bold(), m_font->is_italic(), fontsize, xleading, yleading, vert_advance, w, h, multiline, cursor, xscale / yscale, yscale);
			}

			if (im != NULL)
			{
				m_text_rendered = render::create_bitmap_info(im);

				// keep bound's ratio
				m_text_rendered->set_xratio(w / (float) p2(w));
				m_text_rendered->set_yratio(h / (float) p2(h));
			}
		}

		if (m_text_rendered != NULL)
		{
			rect uv_bounds(0, m_text_rendered->get_xratio(), 0, m_text_rendered->get_yratio());

			cxform	cx;
			get_world_cxform(&cx);
			rgba	transformed_color = cx.transform(m_color);

			rect bound;
			def->get_bound(&bound);
			render::draw_bitmap(mat, m_text_rendered, bound, uv_bounds, transformed_color);
		}

		if (m_has_focus)
		{
#ifdef WINPHONE
			if (m_ios_field != nullptr)
			{
				m_ios_field->setFocus(true);
			}
#endif
		}
	}

	// extension
	// internal keyboard manager
	// kbd will be placed on highest depth in _root and its name will be '_kbd_'
	void edit_text_character::kbd_set_visible(bool visible)
	{
		if (s_has_virtual_keyboard)
		{
			// try embedded kbd 
			const char* kbd_clip_name = "_kbd_";
			sprite_instance* mroot = cast_to<sprite_instance>(get_root_movie());
			sprite_instance* kbd = cast_to<sprite_instance>(mroot->m_display_list.get_character_by_name(kbd_clip_name));
			if (kbd == NULL)
			{
				tu_string finame = get_workdir();
				finame += "kbd.swf";
				int highest_depth = mroot->get_highest_depth();
				kbd = cast_to<sprite_instance>(mroot->add_empty_movieclip(kbd_clip_name, highest_depth));
				mroot->get_environment()->load_file(finame, kbd, 0);
				myprintf("*** using virtual keyboard\n");
			}

			as_value main;
			if (kbd && kbd->get_member("main", &main))
			{
				as_object* obj = main.to_object();
				if (obj)
				{
					obj->set_member("focus", this);		// active field
					obj->set_member("visible", visible);
				}
			}
		}
	}

	bool edit_text_character::on_event(const event_id& id)
	{
		if (m_def->m_readonly == true)
		{
			return false;
		}

		switch (id.m_id)
		{
			case event_id::SETFOCUS:
			{
				get_root()->set_active_entity(this);
				if (m_has_focus == false)
				{
					// Invoke user defined event handler
					as_value function;
					if (get_member("onSetFocus", &function))
					{
						as_environment env;
						env.push(as_value());	// oldfocus, TODO
						bakeinflash::call_method(function, &env, this, 1, env.get_top_index());
					}

					get_root()->m_keypress_listener.add(this);
					m_has_focus = true;
					m_cursor = m_text.utf8_length();
					m_text_rendered = NULL;		// redraw the text

					kbd_set_visible(true);
				}
#ifdef iOS
				myTextField *textField = (myTextField*) m_ios_field;
				if (textField)
				{					
					[textField becomeFirstResponder]; 
				}
#endif
				break;
			}

			case event_id::KILLFOCUS:
			{
				if (m_has_focus == true)
				{
					// Invoke user defined event handler
					as_value function;
					if (get_member("onKillFocus", &function))
					{
						as_environment env;
						env.push(as_value());	// newfocus, TODO
						bakeinflash::call_method(function, &env, this, 1, env.get_top_index());
					}

					m_has_focus = false;
					m_text_rendered = NULL;		// redraw the text
					get_root()->m_keypress_listener.remove(this);
				}

			//	kbd_set_visible(false);

#ifdef iOS
				myTextField *textField = (myTextField*) m_ios_field;
				if (textField)
				{					
					[textField resignFirstResponder]; 
				}
#endif

#ifdef WINPHONE
				if (m_ios_field != nullptr)
				{					
					m_ios_field->setFocus(false);
				}
#endif

				break;
			}

			case event_id::KEY_PRESS:
			{
				// may be m_text is changed in ActionScript
				int len = m_text.utf8_length();
				m_cursor = imin(m_cursor, len);
				switch (id.m_key_code)
				{
					case key::BACKSPACE:
						if (m_cursor > 0)
						{
							// move to unicode symbol
							const char* ch = m_text.c_str();
							int k = 0;
							for (int i = 0; i < m_cursor - 1; i++)
							{
								k += (*ch & 0x80) ? 2 : 1;
								ch += (*ch & 0x80) ? 2 : 1;
							}
							m_text.erase(k, (*ch & 0x80) ? 2 : 1);
							m_cursor--;
							set_text_value(m_text);
						}
						break;

					case key::DELETEKEY:
						if (m_cursor < len)
						{
							// move to unicode symbol
							const char* ch = m_text.c_str();
							int k = 0;
							for (int i = 0; i < m_cursor; i++)
							{
								k += (*ch & 0x80) ? 2 : 1;
								ch += (*ch & 0x80) ? 2 : 1;
							}
							m_text.erase(k, (*ch & 0x80) ? 2 : 1);
							//m_cursor--;
							set_text_value(m_text);
						}
						break;

					case key::HOME:
					case key::PGUP:
					case key::UP:
						m_cursor = 0;
						break;

					case key::END:
					case key::PGDN:
					case key::DOWN:
						m_cursor = len;
						break;

					case key::LEFT:
						m_cursor = m_cursor > 0 ? m_cursor - 1 : 0;
						break;

					case key::RIGHT:
						m_cursor = m_cursor < len - 1 ? m_cursor + 1 : len;
						break;

					default:
					{
						// sanity check
						if (get_root()->m_shift_key_state == true && s_bakeinflash_key_to_ascii_shifted.size() <= id.m_key_code)
						{
							break;
						}
						if (get_root()->m_shift_key_state == false && s_bakeinflash_key_to_ascii.size() <= id.m_key_code)
						{
							break;
						}

						// move to unicode symbol
						const char* ch = m_text.c_str();
						int k = 0;
						for (int i = 0; i < m_cursor; i++)
						{
							k += (*ch & 0x80) ? 2 : 1;
							ch += (*ch & 0x80) ? 2 : 1;
						}

						Uint16 print_char = (get_root()->m_shift_key_state) ? s_bakeinflash_key_to_ascii_shifted[id.m_key_code] : s_bakeinflash_key_to_ascii[id.m_key_code];
						if (print_char == 0)
						{
							break;
						}

						if (print_char > 256)
						{
							m_text.insert(k, (char) (print_char >> 8));
							m_text.insert(k + 1, (char) (print_char & 0xFF));
						}
						else
						{
							m_text.insert(k, (char) print_char);
						}
						m_cursor++;
						set_text_value(m_text);
						break;
					}
				}
				m_text_rendered = NULL;		// redraw the text
			}

			default:
				return false;
		}
		return true;
	}

//	void	edit_text_character::reset_bounding_box(float x, float y)
	// Reset our text bounding box to the given point.
//	{
//		m_text_bounding_box.m_x_min = x;
//		m_text_bounding_box.m_x_max = x;
//		m_text_bounding_box.m_y_min = y;
//		m_text_bounding_box.m_y_max = y;
//	}

	void edit_text_character::advance_actions()
	{
		to_string();

		// input field ?
		if (m_def->m_readonly)
		{
			return;
		}

#ifdef WINPHONE

		if (m_ios_field == nullptr)
		{
			m_ios_field = ref new myWindowsTextBox(this);
		}

#endif

#ifdef iOS
    myTextField *textField = (myTextField*) m_ios_field;
    if (textField == NULL)
    {
      // get actual size of characters in pixels
      matrix m;
      get_world_matrix(&m);
      
      float xscale = m.get_x_scale() * s_scale / s_retina;
      float yscale = m.get_y_scale() * s_scale / s_retina;
      int fontsize = TWIPS_TO_PIXELS(yscale * m_text_height);
      
      int x = s_x0 / s_retina + TWIPS_TO_PIXELS(m.m_[0][2] * s_scale / s_retina);
      int y = s_y0 / s_retina + TWIPS_TO_PIXELS(m.m_[1][2] * s_scale / s_retina);
      //	printf("x0=%d y0=%df scale=%f retina=%f xsacle=%f yscale=%f\n", s_x0, s_y0, s_scale, s_retina, xscale, yscale);
      int w = TWIPS_TO_PIXELS(get_width() * xscale);
      int h = TWIPS_TO_PIXELS(get_height() * yscale);
      
      myTextField *textField = [[myTextField alloc] initWithFrame:CGRectMake(x, y, w, h)];
      m_ios_field = textField;
      textField.parent = this;
      
      textField.autocapitalizationType = UITextAutocapitalizationTypeNone;
      textField.returnKeyType = UIReturnKeyDefault;
      //			[textField setFont:[UIFont fontWithName:[NSString stringWithUTF8String: m_font->get_name().c_str()] size:fontsize]];
      [textField setFont:[UIFont systemFontOfSize:fontsize]];
      textField.textColor = [UIColor colorWithRed:m_def->m_color.m_r/255.0 green:m_def->m_color.m_g/255.0 blue:m_def->m_color.m_b/255.0 alpha:m_def->m_color.m_a/255.0];
      textField.backgroundColor = [UIColor colorWithRed:m_background_color.m_r/255.0 green:m_background_color.m_g/255.0 blue:m_background_color.m_b/255.0 alpha:(m_def->m_border ? m_background_color.m_a/255.0:0)];
      // textField.layer.cornerRadius = 10 * s_scale / s_retina;
      
      switch (m_alignment)
      {
        case edit_text_character_def::ALIGN_LEFT:
          textField.textAlignment = UITextAlignmentLeft;
          break;
        case edit_text_character_def::ALIGN_RIGHT:
          textField.textAlignment = UITextAlignmentRight;
          break;
        default:
          textField.textAlignment = UITextAlignmentCenter;
          break;
      }
      
      textField.enabled = m_def->m_readonly ? NO:YES;
      textField.clearsOnBeginEditing = NO;
      textField.secureTextEntry = m_password;
      textField.delegate = s_view;
      
      resetKeyboardType();
      
      [s_view addSubview:textField];
      
			//html_to_plain();

      // first set default text value, if was no set_member
      set_text(m_text.size() == 0 ? m_def->m_default_text.c_str() : m_text.c_str());
      
      // then reset VAR / TEXT_FIELD value
      set_text_value(to_string());
      
    }
    else
    {
      // reset pos
      
      // get actual size of characters in pixels
      matrix m;
      get_world_matrix(&m);
      
      float xscale = m.get_x_scale() * s_scale / s_retina;
      float yscale = m.get_y_scale() * s_scale / s_retina;
      int fontsize = TWIPS_TO_PIXELS(yscale * m_text_height);
      
      int x = s_x0 / s_retina + TWIPS_TO_PIXELS(m.m_[0][2] * s_scale / s_retina);
      int y = s_y0 / s_retina + TWIPS_TO_PIXELS(m.m_[1][2] * s_scale / s_retina);
      //	printf("x0=%d y0=%df scale=%f retina=%f xsacle=%f yscale=%f\n", s_x0, s_y0, s_scale, s_retina, xscale, yscale);
      int w = TWIPS_TO_PIXELS(get_width() * xscale);
      int h = TWIPS_TO_PIXELS(get_height() * yscale);
      
      CGRect rect = textField.frame;
      rect.origin.x = x;
      rect.origin.y = y;
      textField.frame = rect;
      
    }
    to_string();
    m_text = [textField.text UTF8String];
    set_visible(m_visible);
    
#endif

#ifdef ANDROID
		if (s_has_virtual_keyboard)
		{
			return;
		}

		if (m_ios_field == NULL)
		{
			// get actual size of characters in pixels
			matrix m;
			get_world_matrix(&m);

      float xscale = m.get_x_scale() * s_scale / s_retina;
      float yscale = m.get_y_scale() * s_scale / s_retina;
      int fontsize = TWIPS_TO_PIXELS(yscale * m_text_height);

			int x = (int) (s_x0 / s_retina + TWIPS_TO_PIXELS(m.m_[0][2] * s_scale / s_retina));
			int y = (int) (s_y0 / s_retina + TWIPS_TO_PIXELS(m.m_[1][2] * s_scale / s_retina));
			int w = (int) TWIPS_TO_PIXELS(get_width() * xscale);
			int h = (int) TWIPS_TO_PIXELS(get_height() * yscale);

			int textColor = int(m_def->m_color.m_a << 24 | m_def->m_color.m_r << 16 | m_def->m_color.m_g << 8 | m_def->m_color.m_b);
			int backgroundColor = m_background_color.m_a << 24 | m_background_color.m_r << 16 | m_background_color.m_g << 8 | m_background_color.m_b;

			int align = 0;
			switch (m_alignment)
			{
				case edit_text_character_def::ALIGN_LEFT:
					align = 3; //s_jenv->CallVoidMethod(m_ios_field, mid, 3);	// hack, android.view.Gravity.LEFT
					break;
				case edit_text_character_def::ALIGN_RIGHT:
					align = 5; //s_jenv->CallVoidMethod(m_ios_field, mid, 5);
					break;
				default:
					align = 17; //s_jenv->CallVoidMethod(m_ios_field, mid, 17);
					break;
			}

			int style = 0;		// normal
			if (m_font->is_italic() && !m_font->is_bold())
			{
				style = 2;	// italic
			}
			else
			if (m_font->is_bold() && !m_font->is_italic())
			{
				style = 1; //bold
			}
			else
			if (m_font->is_bold() && m_font->is_italic())
			{
				style = 3; //bold+italic
			}

			const char* font_name = m_font->get_name().c_str();
			jstring jsFont = s_jenv->NewStringUTF(font_name);

			int bg_color = m_def->m_border ? backgroundColor : 0x00FFFFFF;

			// new myEditText();
			// public myEditText(int w, int h, int x, int y, int fontsize, float scale, int textColor, int bgColor, boolean enabled, int align, String font, int style)
			jclass cls = s_jenv->FindClass(s_pkgName + tu_string("myEditText"));
			jmethodID mid = s_jenv->GetMethodID(cls, "<init>", "(IIIIIIIZILjava/lang/String;I)V");
			m_ios_field = s_jenv->NewObject(cls, mid, w, h, x, y, fontsize, textColor, bg_color, !m_def->m_readonly,	align, jsFont, style);
			m_ios_field = s_jenv->NewGlobalRef(m_ios_field);

			// hack
			//html_to_plain();

			// first set default text value, if was no set_member
			set_text(m_text.size() == 0 ? m_def->m_default_text.c_str() : m_text.c_str());

			// then reset VAR / TEXT_FIELD value
			set_text_value(to_string());
		}

		to_string();

		// get value and set visibility
		jclass cls = s_jenv->FindClass(s_pkgName + tu_string("myEditText"));
		jmethodID mid = s_jenv->GetMethodID(cls, "getValue", "(I)Ljava/lang/String;");
		jstring js = (jstring) s_jenv->CallObjectMethod(m_ios_field, mid, get_visible() ? 0 : 8);		// 8=gone
		m_text = s_jenv->GetStringUTFChars(js, NULL);

#endif
	}

	// hack
	void	edit_text_character::html_to_plain()
	{
		if (m_def->m_html)
		{
			// try as HTML
			//myprintf("html text: %s\n", m_text.c_str());
			tu_string plain_text;
			format_html_text(&plain_text);
			m_text = plain_text;
			m_def->m_html = false;
			m_def->m_default_text = m_text;
		}
	}

	void	edit_text_character::set_text(const tu_string& new_text)
	// Local. Set our text to the given string.
	{
//		if (m_text == new_text)
//		{
//			return;
//		}

		m_text = new_text;
		html_to_plain();		// hack

		if (m_def->m_max_length > 0	&& m_text.size() > m_def->m_max_length)
		{
			m_text.resize(m_def->m_max_length, true);
		}
		m_text_rendered = NULL;

#ifdef iOS
		if (m_ios_field)
		{
			myTextField *textField = (myTextField*) m_ios_field;
			textField.text = [NSString stringWithUTF8String: (char*) m_text.c_str()];
		}
#endif

#ifdef ANDROID
		if (m_ios_field && (s_has_virtual_keyboard == false))
		{
			jclass cls = s_jenv->FindClass(s_pkgName + tu_string("myEditText"));
			jmethodID mid = s_jenv->GetMethodID(cls, "setText", "(Ljava/lang/CharSequence;)V");
			jstring js = s_jenv->NewStringUTF(m_text.c_str());
			s_jenv->CallVoidMethod(m_ios_field, mid, js);
		}
#endif

#ifdef WINPHONE
//		WCHAR buffer[512];
//		swprintf_s(buffer, 512, L"%s", m_text.size() == 0 ? m_def->m_default_text.c_str() : m_text.c_str());
//		String^ s = ref new String(buffer);
//		m_ios_field->setText(s);
#endif
	}

	void	edit_text_character::set_text_value(const tu_string& new_text)
	// Set our text to the given string.
	{
		// set our text
		set_text(new_text);

		// set VAR
		if (get_var_name().size() > 0)
		{
			as_object* ch = get_parent();	// target, default is parent
			tu_string path;
			tu_string var = get_var_name();
			if (as_environment::parse_path(get_var_name(), &path, &var))
			{
				ch = ch->find_target(path.c_str());
			}

			if (ch)
			{
				ch->set_member(var, new_text.c_str());
			}
		}
	}


	// @@ WIDTH_FUDGE is a total fudge to make it match the Flash player!  Maybe
	// we have a bug?
#define WIDTH_FUDGE 80.0f


	void	edit_text_character::align_line(edit_text_character_def::alignment align, int last_line_start_record, float x)
	// Does LEFT/CENTER/RIGHT alignment on the records in
	// m_text_glyph_records[], starting with
	// last_line_start_record and going through the end of
	// m_text_glyph_records.
	{
		float	extra_space = (m_def->m_rect.width() - m_right_margin) - x - WIDTH_FUDGE;

		// extra_space may be < 0
		// assert(extra_space >= 0.0f);

		float	shift_right = 0.0f;

		if (align == edit_text_character_def::ALIGN_LEFT)
		{
			// Nothing to do; already aligned left.
			return;
		}
		else if (align == edit_text_character_def::ALIGN_CENTER)
		{
			// Distribute the space evenly on both sides.
			shift_right = extra_space / 2;
		}
		else if (align == edit_text_character_def::ALIGN_RIGHT)
		{
			// Shift all the way to the right.
			shift_right = extra_space;
		}

		// Shift the beginnings of the records on this line.
//		for (int i = last_line_start_record; i < m_text_glyph_records.size(); i++)
//		{
//			text_glyph_record&	rec = m_text_glyph_records[i];

//			if (rec.m_style.m_has_x_offset)
//			{
//				rec.m_style.m_x_offset += shift_right;
//			}
//		}

		// This fix the cursor position with centered texts
		// Thanks Francois Guibert
		//m_xcursor += shift_right;
	}

	// Convert the characters in m_text into a series of
	// text_glyph_records to be rendered.
#if 0
	void	edit_text_character::format_text()
	{
#if iOS == 1
    myTextField *textField = (myTextField*) m_ios_field;
    if (textField)
    {
      textField.textColor = [UIColor colorWithRed:m_color.m_r/255.0 green:m_color.m_g/255.0 blue:m_color.m_b/255.0 alpha:m_color.m_a/255.0];
      textField.backgroundColor = [UIColor colorWithRed:m_background_color.m_r/255.0 green:m_background_color.m_g/255.0 blue:m_background_color.m_b/255.0 alpha:(m_def->m_border ? m_background_color.m_a/255.0:0)];
    }
#endif
    
		if (m_font == NULL)
		{
			return;
		}

//		m_text_glyph_records.resize(0);

		text_glyph_record rec;	// holds current glyph

		rec.m_style.m_scale =  m_text_height / 1024.0f;	// the EM square is 1024 x 1024
		if (m_font->is_define_font3())
		{
			// Flash 8: All the EMSquare coordinates are multiplied by 20 at export,
			// allowing fractional resolution to 1/20 of a unit.
			rec.m_style.m_scale /= 20.0f;
		}

		float font_leading = m_font->get_leading();
		float font_descent = m_font->get_descent();
		rec.m_style.m_leading = m_leading;
		rec.m_style.m_leading += font_leading * rec.m_style.m_scale;

		rec.m_style.m_font = m_font.get();
		rec.m_style.m_color = m_color;

		rec.m_style.m_x_offset = fmax(0, m_left_margin + m_indent);
		rec.m_style.m_x_offset += font_leading * rec.m_style.m_scale;

//		rec.m_style.m_y_offset = m_text_height + (font_leading - font_descent) * rec.m_style.m_scale;
		rec.m_style.m_y_offset = m_text_height; // + font_descent * rec.m_style.m_scale;

		rec.m_style.m_text_height = m_text_height;

		rec.m_style.m_has_x_offset = true;
		rec.m_style.m_has_y_offset = true;

		m_x = rec.m_style.m_x_offset;
		m_y = rec.m_style.m_y_offset;

		// hack
		//html_to_plain();

		// use default glypth record
		format_plain_text(m_text, rec);
	}
#endif

	// HTML content
	// <p> ... </p> Defines a paragraph.
	//		The attribute align may be present, with value left, right, or center.
	// <br> Inserts a line break.
	// <a> ... </a> Defines a hyperlink.
	// <font> ... </font> Defines a span of text that uses a given font.
	//		? face, which specifies a font name
	//		? size, which is specified in twips, and may include a leading ?+? or ?-? for relative sizes
	//		? color, which is specified as a #RRGGBB hex triplet
	// <b> ... </b> Defines a span of bold text.
	// <i> ... </i> Defines a span of italic text.
	// <u> ... </u> Defines a span of underlined text.
	// <li> ... </li> Defines a bulleted paragraph.
	// <textformat> ... </textformat>
	// <tab> Inserts a tab character
	//
	// sample:
	// <p align="left"><font face="Times New Roman" size="74" color="#cccccc"
	// letterSpacing="0.000000" kerning="0">aaa</font></p>
	// <p align="left"><font face="Courier" size="74" color="#ff0000"
	// letterSpacing="0.000000" kerning="0">bbb</font></p>

	// paragraph parser
	const char*	edit_text_character::html_paragraph(const char* p, tu_string* plain_text)
	{
		//		myprintf("html paragraph\n");
		const char* pend = p + strlen(p);
		while (p <= pend)
		{
			if (strncmp(p, "<p", 2) == 0)
			{
				html_paragraph(p + 3, plain_text);
			}
			else
				if (strncmp(p, "</p>", 4) == 0 && *(p + 4) != 0)
				{
					*plain_text += '\n';
					return p + 4;
				}
				else
					if (strncmp(p, "align=", 6) == 0)
					{
						p += 7;
						if (strncmp(p, "left", 4) == 0)
						{
							m_alignment = edit_text_character_def::ALIGN_LEFT;
							p += 5;
						}
						else
							if (strncmp(p, "center", 6) == 0)
							{
								m_alignment = edit_text_character_def::ALIGN_CENTER;
								p += 7;
							}
							else
								if (strncmp(p, "right", 5) == 0)
								{
									m_alignment = edit_text_character_def::ALIGN_RIGHT;
									p += 6;
								}
								else
								{
									assert(0);	// invalid align
								}

						assert(*p == '>');
						p++;
					}
					else
						if (strncmp(p, "<font ", 5) == 0)
						{
							p = html_font(p + 6, plain_text);
						}
						else
						{
							p++;
						}
		}

		//assert(0);
		return NULL;
	}

	// text parser
	const char*	edit_text_character::html_text(const char* p, tu_string* plain_text)
	{
		// keep old values
		bool saved_bold = m_font->is_bold();
		bool saved_italic = m_font->is_italic();

		tu_string text;

		const char* pend = p + strlen(p);
		while (true)
		{
			assert(p <= pend);

			if (strncmp(p, "<b>", 3) == 0)
			{
				p += 3;
			//	rec.m_style.m_font->set_bold(true);
			}
			else
				if (strncmp(p, "<i>", 3) == 0)
				{
					p += 3;
				//	rec.m_style.m_font->set_italic(true);
				}
				else
					if (strncmp(p, "</i>", 4) == 0)
					{
						*plain_text += text;
						text = "";
						p += 4;
					//	rec.m_style.m_font->set_italic(false);
					}
					else
						if (strncmp(p, "</b>", 4) == 0)
						{
							*plain_text += text;
							text = "";
							p += 4;
						//	rec.m_style.m_font->set_bold(false);
						}
						else
							if (*p == '<')
							{
								*plain_text += text;
								text = "";
								return p;
							}
							else
							{
								if (strncmp(p, "&nbsp;", 5) == 0)
								{
									text += " ";
									p += 6;
								}
								else
								{
									// plain text
									text += *p;
									p++;
								}
							}
		}

		return p;
	}

	// font parser
	const char*	edit_text_character::html_font(const char* p, tu_string* plain_text)
	{
		// Start a new record with a new style
//		if (rec.m_glyphs.size() > 0)
//		{
//			m_text_glyph_records.push_back(rec);
//			rec.m_glyphs.resize(0);
//			rec.m_style.m_x_offset = m_x;
//			rec.m_style.m_y_offset = m_y;
//		}

		const char* pend = p + strlen(p);
		while (true)
		{
			assert(p <= pend);

			if (*p == ' ')
			{
				p++;
			}
			else
				if (*p == '>')	// begin of text
				{
					p = html_text(p + 1, plain_text);
				}
				else
					if (strncmp(p, "</font>", 7) == 0)
					{
					//	if (rec.m_style.m_lastfont != NULL)
					//	{
					//		rec.m_style.m_font = rec.m_style.m_lastfont;
					//		rec.m_style.m_lastfont = NULL;
					//	}
						return p + 7;
					}
					else
						if (strncmp(p, "face=", 5) == 0)
						{
							p +=6;
							const char* end = strchr(p, '"');
							assert(end);
							int len = int(end - p);
							tu_string face(p, len);
							p += len + 1;

							movie_def_impl* md = cast_to<movie_def_impl>(m_def->m_root_def);
							assert(md);
							font* f = md->find_font(face.c_str());
							if (f)
							{
								//rec.m_style.m_lastfont = rec.m_style.m_font;
								//rec.m_style.m_font = f;
							}
							else
							{
								myprintf("html_font: can't find font, face = '%s'\n", face.c_str());
							}

						}
						else
							if (strncmp(p, "size=", 5) == 0)
							{
								p += 6;
								const char* end = strchr(p, '"');
								int len = int(end - p);
								tu_string size(p, len);
								p += len + 1;

								// reset size
								double res;
								string_to_number(&res, size.c_str());
								//rec.m_style.m_text_height = static_cast<float>(PIXELS_TO_TWIPS(res));

								// reset advance
								//rec.m_style.m_scale =  rec.m_style.m_text_height / 1024.0f;
								if (m_font->is_define_font3())
								{
								//	rec.m_style.m_scale /= 20.0f;
								}
							}
							else
								if (strncmp(p, "color=", 6) == 0)
								{
									p += 7;
									assert(*p == '#');
									++p;

									Uint32 rgb = strtol(p, 0, 16);
									//rec.m_style.m_color.m_r = rgb >> 16;
								//	rec.m_style.m_color.m_g = (rgb >> 8) & 0xFF;
								//	rec.m_style.m_color.m_b = rgb & 0xFF;

									p += 7;
								}
								else
									if (strncmp(p, "letterSpacing=", 14) == 0)
									{
										p += 15;
										const char* end = strchr(p, '"');
										int len = int(end - p);
										tu_string letterSpacing(p, len);
										p += len + 1;
									}
									else
										if (strncmp(p, "kerning=", 8) == 0)
										{
											p += 9;
											const char* end = strchr(p, '"');
											int len = int(end - p);
											tu_string letterSpacing(p, len);
											p += len + 1;
										}
										else
											if (strncmp(p, "<font ", 5) == 0)
											{
												p = html_font(p + 6, plain_text);
											}
											else if (strncmp(p, "<a ", 3) == 0)
											{
												p += 3;
												myprintf("html_font: Define a hyperlink is not implemented yet\n");

												// skip
												for (; strncmp(p, "</a>", 4); p++) {}
												p += 4;
											}
											else
											if (strncmp(p, "<sbr ", 5) == 0)
											{
												p += 5;
												myprintf("html_font: SBR is not implemented yet\n");

												// skip
												for (; strncmp(p, "/>", 2); p++) {}
												p += 2;
											}
											else
											{
												p = html_text(p, plain_text);
												//myprintf("html_font: unknown keyword %s\n", p); assert(0);
											}
		}

		assert(0);
		return NULL;
	}

	bool	edit_text_character::format_html_text(tu_string* plain_text)
	{
		//		myprintf("%s\n", m_text.c_str());
		const char* p = m_text.c_str();

		if (strncmp(p, "<p", 2) != 0)
		{
			// it is not html text
			return false;
		}

		do
		{
			p = html_paragraph(p + 3, plain_text);
		}
		while (p != NULL && strncmp(p, "<p", 2) == 0);

		return true;
	}

	float edit_text_character::get_text_height()
	{
/*		if (m_text_glyph_records.size() > 0)
		{
			text_glyph_record&	rec = m_text_glyph_records[0];
			float h = TWIPS_TO_PIXELS(rec.m_style.m_text_height);
			h *= m_text_glyph_records.size();
			
			matrix	mat;
			get_world_matrix(&mat);
			point p(0, h);
			point res;
			mat.transform_vector(&res, p);

			return res.m_y;
		}*/
		return 0;
	}


}	// end namespace bakeinflash


//
//
//
#ifdef iOS
@implementation myTextField
@synthesize parent;


- (int) getMaxLength
{
  bakeinflash::edit_text_character* obj = bakeinflash::cast_to<bakeinflash::edit_text_character>((bakeinflash::edit_text_character*) parent);
  bakeinflash::as_value val(-1);
  if (obj)
  {
    obj->get_member("inputLimit", &val);
  }
  return val.to_int();
}

- (void) onFocus
{
  bakeinflash::edit_text_character* obj = bakeinflash::cast_to<bakeinflash::edit_text_character>((bakeinflash::edit_text_character*) parent);
  
  // Invoke user defined event handler
  bakeinflash::as_value function;
  if (obj)
  {
    bakeinflash::get_root()->set_active_entity(obj);
   	if (obj->get_member("onSetFocus", &function))
    {
      bakeinflash::as_environment env;
      env.push(bakeinflash::as_value());	// oldfocus, TODO
      bakeinflash::call_method(function, &env, obj, 1, env.get_top_index());
    }
  }
}

@end

#endif

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
