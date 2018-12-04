// bakeinflash_impl.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation code for the bakeinflash SWF player library.


#ifndef BAKEINFLASH_IMPL_H
#define BAKEINFLASH_IMPL_H


#include "bakeinflash/bakeinflash.h"
#include "bakeinflash/bakeinflash_action.h"
#include "bakeinflash/bakeinflash_types.h"
#include "bakeinflash/bakeinflash_log.h"
#include "bakeinflash/bakeinflash_movie_def.h"
#include "bakeinflash/bakeinflash_character.h"
#include "bakeinflash/bakeinflash_render.h"
#include <assert.h>
#include "base/container.h"
#include "base/utility.h"
#include <stdarg.h>


namespace jpeg { struct input; }


namespace bakeinflash
{
	struct action_buffer;
	struct bitmap_character_def;
	struct bitmap_info;
	struct character;
	struct character_def;
	struct display_info;
	struct execute_tag;
	struct font;
	struct root;
	struct movie_definition_sub;

	struct stream;
	struct swf_event;

	struct swf_event
	{
		// NOTE: DO NOT USE THESE AS VALUE TYPES IN AN
		// array<>!  They cannot be moved!  The private
		// operator=(const swf_event&) should help guard
		// against that.

		event_id	m_event;
		action_buffer	m_method;

		swf_event() {}

	private:

		// DON'T USE THESE
		swf_event(const swf_event& s) { assert(0); }
		void	operator=(const swf_event& s) { assert(0); }
	};


	// For characters that don't store unusual state in their instances.
	struct generic_character : public character
	{
		smart_ptr<character_def>	m_def;
		rgba* m_rgba;		// set_rgba

		generic_character(character_def* def, character* parent, int id);
		~generic_character();

		virtual character_def* get_character_def();
		virtual void	display();
		virtual bool get_topmost_mouse_entity( character * &te, float x, float y);
		virtual rgba* get_rgba();
		virtual void set_rgba(const rgba& color);
		virtual void load_bitmaps(array < smart_ptr<bitmap_info> >* bitmap_hash);
	};


	struct bitmap_character_def : public character_def
	{
		bitmap_character_def(movie_definition* rdef, bitmap_info* bi);
		bitmap_character_def(movie_definition* rdef, Uint8* tag, int tag_length, int tag_type);
		bitmap_character_def(movie_definition* rdef, Uint8* tag, int tag_length, int tag_type, jpeg::input* jpegloader);
		virtual ~bitmap_character_def();

		virtual bitmap_info*	get_bitmap_info();

		// Return true if the specified point is on the interior of our shape.
		// Incoming coords are local coords.
		bool	point_test_local(float x, float y);

		virtual void get_bound(rect* bound);
		virtual void	display(character* ch);
		void clear_bitmap_info() 
		{
			if (m_tag)
			{
				m_bitmap_info = NULL; 
			}
		}

		protected:
			smart_ptr<bitmap_info>	m_bitmap_info_keep;
			smart_ptr<bitmap_info>	m_bitmap_info;
			Uint8* m_tag;
			int m_tag_length;
			int m_tag_type;
			jpeg::input* m_jpegloader;
	};

	// Execute tags include things that control the operation of
	// the movie.  Essentially, these are the events associated
	// with a frame.
	struct execute_tag
	{
		virtual ~execute_tag() {}
		virtual void	execute(character* m) {}
		virtual void	execute_now(character* m) { execute(m); }
		virtual void	execute_state(character* m) {}
		virtual void	execute_state_reverse(character* m, int frame) { execute_state(m); }
		virtual bool	is_remove_tag() const { return false; }
		virtual bool	is_action_tag() const { return false; }
		virtual uint32	get_depth_id_of_replace_or_add_tag() const { return static_cast<uint32>(-1); }
	};

	//
	// Loader callbacks.
	//

	// Register a loader function for a certain tag type.  Most
	// standard tags are handled within bakeinflash.  Host apps might want
	// to call this in order to handle special tag types.
	void	register_tag_loader(int tag_type, loader_function lf);


	// Tag loader functions.
	void	null_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	set_background_color_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	jpeg_tables_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_bits_jpeg_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_bits_jpeg2_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_bits_jpeg3_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_shape_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_shape_morph_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_font_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_font_info_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_text_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_edit_text_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	place_object_2_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_bits_lossless_2_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	sprite_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	end_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	remove_object_2_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	do_action_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	button_character_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	frame_label_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	export_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	import_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_sound_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	start_sound_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	button_sound_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	do_init_action_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_video_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	video_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	sound_stream_head_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	sound_stream_block_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_file_attribute_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_font_alignzones(stream* in, int tag_type, movie_definition_sub* m);
	void	define_csm_textsetting_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_metadata_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_enable_debugger_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_tabindex_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_abc_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	symbol_class_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_scene_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_font_name(stream* in, int tag_type, movie_definition_sub* m);

}	// end namespace bakeinflash


#endif // bakeinflash_IMPL_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
