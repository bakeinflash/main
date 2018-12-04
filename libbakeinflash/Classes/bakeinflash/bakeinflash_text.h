// bakeinflash_text.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Code for the text tags.


//TODO: csm_textsetting() is common for text_def & edit_text_def
//	therefore make new base class for text_def & edit_text_def
//	and move csm_textsetting() to it.

#include "base/utf8.h"
#include "base/utility.h"
#include "bakeinflash/bakeinflash_impl.h"
#include "bakeinflash/bakeinflash_shape.h"
#include "bakeinflash/bakeinflash_stream.h"
#include "bakeinflash/bakeinflash_log.h"
#include "bakeinflash/bakeinflash_font.h"
#include "bakeinflash/bakeinflash_render.h"
#include "bakeinflash/bakeinflash_as_classes/as_textformat.h"
#include "bakeinflash/bakeinflash_root.h"
#include "bakeinflash/bakeinflash_movie_def.h"

#if ANDROID == 1
#include <jni.h>
#endif

#ifdef WINPHONE
ref class myWindowsTextBox sealed
{
	internal:
		myWindowsTextBox(void* textfield);

		void setFocus(bool hasFocus);
		void setText(Platform::String^ str);
		property bool HasFocus
		{
			bool get() { return m_hasFocus; }
		}

		void OnTextChanged(Windows::Phone::UI::Core::KeyboardInputBuffer^ sender, Windows::Phone::UI::Core::CoreTextChangedEventArgs^ args);

		Windows::Phone::UI::Core::KeyboardInputBuffer^ m_inputBuffer;
		bool m_hasFocus;
		void* m_textfield;
};
#endif

namespace bakeinflash
{


	void	as_global_textfield_ctor(const fn_call& fn);

	// Helper struct.
	struct text_style
	{
		int	m_font_id;
		mutable font*	m_font;
		mutable font*   m_lastfont;
		rgba	m_color;
		float	m_x_offset;
		float	m_y_offset;
		float	m_text_height;
		bool	m_has_x_offset;
		bool	m_has_y_offset;
		float m_scale;
		float m_leading;

		text_style() :
			m_font_id(-1),
			m_font(NULL),
			m_lastfont(NULL),
			m_x_offset(0),
			m_y_offset(0),
			m_text_height(1.0f),
			m_has_x_offset(false),
			m_has_y_offset(false),
			m_scale(0),
			m_leading(0)
		{
		}

		void	resolve_font(movie_definition_sub* root_def) const
		{
			if (m_font == NULL)
			{
				assert(m_font_id >= 0);

				m_font = root_def->get_font(m_font_id);
				if (m_font == NULL)
				{
					myprintf("error: text style with undefined font; font_id = %d\n", m_font_id);
				}
			}
		}
	};


	// Helper structs.

	struct text_glyph_record
	{
		text_style	m_style;
		array<glyph>	m_glyphs;

		void	read(stream* in, int glyph_count, int glyph_bits, int advance_bits)
		{
			m_glyphs.resize(glyph_count);
			for (int i = 0; i < glyph_count; i++)
			{
				m_glyphs[i].m_glyph_index = in->read_uint(glyph_bits);
				m_glyphs[i].m_glyph_advance = (float) in->read_sint(advance_bits);
			}
		}
	};

	//
	// text_character_def
	//

	struct text_character_def : public character_def
	{
		movie_definition_sub*	m_root_def;
		rect	m_rect;
		matrix	m_matrix;
		array<text_glyph_record>	m_text_glyph_records;
		smart_ptr<bitmap_info> m_text_rendered;

		// Flash 8
		bool m_use_flashtype;
		int m_grid_fit;
		float m_thickness;
		float m_sharpness;

		text_character_def(movie_definition_sub* root_def);
		virtual ~text_character_def();

		void	read(stream* in, int tag_type, movie_definition_sub* m);
		void	display(character* inst);
		void	csm_textsetting(stream* in, int tag_type);

		virtual void get_bound(rect* bound);
	};

	//
	// edit_text_character_def
	//

	struct edit_text_character_def : public character_def
	// A definition for a text display character, whose text can
	// be changed at runtime (by script or host).
	{
		movie_definition_sub*	m_root_def;
		rect			m_rect;
		tu_string		m_var_name;
		bool			m_word_wrap;
		bool			m_multiline;
		bool			m_password;	// show asterisks instead of actual characters
		bool			m_readonly;
		bool			m_auto_size;	// resize our bound to fit the text
		bool			m_no_select;
		bool			m_border;	// forces white background and black border -- silly, but sometimes used
		bool			m_html;
		bool			m_use_outlines;	// when true, use specified SWF internal font.  Otherwise, renderer picks a default font
		int				m_font_id;
		font*			m_font;
		float			m_text_height;
		rgba			m_color;
		Uint32		m_max_length;

		enum alignment
		{
			ALIGN_LEFT = 0,
			ALIGN_RIGHT,
			ALIGN_CENTER,
			ALIGN_JUSTIFY	// probably don't need to implement...
		};
		alignment	m_alignment;

		float			m_left_margin;	// extra space between box border and text
		float			m_right_margin;
		float			m_indent;	// how much to indent the first line of multiline text
		float			m_leading;	// extra space between lines (in addition to default font line spacing)
		tu_string	m_default_text;

		// Flash 8
		bool			m_use_flashtype;
		int				m_grid_fit;
		float			m_thickness;
		float			m_sharpness;

		edit_text_character_def(int width, int height);
		edit_text_character_def(movie_definition_sub* root_def);
		~edit_text_character_def();

		character*	create_character_instance(character* parent, int id);
		void	read(stream* in, int tag_type, movie_definition_sub* m);
		void	csm_textsetting(stream* in, int tag_type);

		virtual void get_bound(rect* bound) {	*bound = m_rect; }
	};

	//
	// edit_text_character
	//

	struct edit_text_character : public character
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_EDIT_TEXT };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return character::is(class_id);
		}

		smart_ptr<edit_text_character_def>	m_def;
		tu_string	m_text;
		bool m_has_focus;
		bool m_password;
		int m_cursor;

		// instance specific
		rgba m_color;
		float m_text_height;
		smart_ptr<font> m_font;
		edit_text_character_def::alignment	m_alignment;
		float	m_left_margin;
		float	m_right_margin;
		float	m_indent;
		float	m_leading;
		rgba m_background_color;
		matrix m_world_matrix;	// current world matrix, for dynamic scaling
		tu_string m_var_name;

		smart_ptr<bitmap_info> m_text_rendered;	// prerendered text
    tu_string m_keyboardType;

#ifdef ANDROID
		jobject m_ios_field;
#else
		#ifdef WINPHONE
			myWindowsTextBox^ m_ios_field;
		#else
			void* m_ios_field;
		#endif
#endif

		edit_text_character(character* parent, edit_text_character_def* def, int id);
		~edit_text_character();

		virtual character_def* get_character_def() { return m_def.get();	}
		void reset_format(as_textformat* tf);

		const char *type_of() { return "edittext";}

		void display();
		virtual bool on_event(const event_id& id);
		virtual bool can_handle_mouse_event();
		virtual bool get_topmost_mouse_entity( character * &te, float x, float y);
		const tu_string&	get_var_name() const;
		void set_var_name(const tu_string& name);
//		void	reset_bounding_box(float x, float y);
		void	set_text_value(const tu_string& new_text);
		virtual const char*	to_string();
		virtual void	set_visible(bool visible);
		bool	set_member(const tu_string& name, const as_value& val);
		bool	get_member(const tu_string& name, as_value* val);
		void	align_line(edit_text_character_def::alignment align, int last_line_start_record, float x);

//		void	format_text();
//		void	format_plain_text(const tu_string& text, text_glyph_record& rec);

		bool	format_html_text(tu_string* plain_text);
		const char* html_paragraph(const char* p, tu_string* plain_text);
		const char* html_font(const char* p, tu_string* plain_text);
		const char* html_text(const char* p, tu_string* plain_text);

		virtual void advance_actions();
		float get_text_height();
		void	set_text(const tu_string& new_text);
		void html_to_plain();

		// extension.. internal keyboard manager
		void kbd_set_visible(bool val);
    void resetKeyboardType();
	};

}	// end namespace bakeinflash


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
