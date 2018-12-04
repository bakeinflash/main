// bakeinflash_sprite.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation code for the bakeinflash SWF player library.


#ifndef BAKEINFLASH_SPRITE_H
#define BAKEINFLASH_SPRITE_H


#include "bakeinflash/bakeinflash.h"
#include "bakeinflash/bakeinflash_action.h"
#include "bakeinflash/bakeinflash_as_classes/as_mcloader.h"
#include "bakeinflash/bakeinflash_dlist.h"
#include "bakeinflash/bakeinflash_log.h"
#include "bakeinflash/bakeinflash_movie_def.h"
#include "bakeinflash/bakeinflash_root.h"
#include "bakeinflash/bakeinflash_sprite_def.h"
#include "bakeinflash/bakeinflash_types.h"
#include "bakeinflash/bakeinflash_canvas.h"

#include "base/container.h"
#include "base/utility.h"
#include <stdarg.h>

namespace bakeinflash
{

	struct as_bitmapdata;

	struct sprite_instance : public character
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_SPRITE };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return character::is(class_id);
		}

		smart_ptr<movie_definition_sub>	m_def;
		root*	m_root;

		display_list	m_display_list;
		array<action_buffer*>	m_action_list;

		play_state	m_play_state;
		int		m_current_frame;
		int m_target_frame;
		array<bool>	m_init_actions_executed;	// a bit-array class would be ideal for this

		as_environment	m_as_environment;

		enum mouse_state
		{
			UP = 0,
			DOWN,
			OVER
		};
		mouse_state m_mouse_state;
		bool m_enabled;
		bool m_on_event_load_called;

		// for setMask
		weak_ptr<sprite_instance> m_mask_clip;

		// AS3
		hash<int, smart_ptr<as_function> >* m_script;	// <frame, script>
		string_hash< weak_ptr<as_function> >* m_as3listener;	// <event, handler>
		int m_last_visited_frame;	


		// for goto_frame() 
		bool m_do_advance;

		// for advance_ctor
		bool m_do_ctor;

		// for bitmap caching
		array < smart_ptr<bitmap_info> >*	m_bitmap_hash;

		// for scroller
		rect m_scrollbox_bounds;		// global coords in pixels
		array<float> m_scroll_way;
		int m_scroll_mode;		// 00=hori/, 01=vert, 10=hori+nopgaes, 11=vert+nopages

		sprite_instance(movie_definition_sub* def, root* r, character* parent, int id);
		virtual ~sprite_instance();

		virtual character_def* get_character_def() { return m_def.get();	}
		virtual bool has_keypress_event();
		inline root*	get_root() { return m_root; }

		// used in loadMovieClip()
		void	set_root(root* mroot) { m_root = mroot; }
		uint32	get_file_bytes() const;
		uint32	get_loaded_bytes() const;

		 character*	get_root_movie() const { return m_root->get_root_movie(); }
		movie_definition*	get_movie_definition() { return m_def.get(); }

		virtual void get_bound(rect* bound);

		virtual int	get_current_frame() const { return m_current_frame; }
		virtual int	get_target_frame() const { return m_target_frame; }		// for goto frame
		virtual int	get_frame_count() const { return m_def->get_frame_count(); }
		virtual int get_loading_frame() const { return m_def->get_loading_frame(); }

		character* add_empty_movieclip(const char* name, int depth);

		 void	set_play_state(play_state s);
		play_state	get_play_state() const;

		character*	get_character(int character_id)
		{
			//			return m_def->get_character_def(character_id);
			// @@ TODO -- look through our dlist for a match
			if (character_id >= m_display_list.size())
			{
				return NULL;
			}

			return m_display_list.get_character(character_id);
		}

		float	get_pixel_scale() const { return m_root->get_pixel_scale(); }

		virtual void	get_mouse_state(int* x, int* y, int* buttons)
		{
			m_root->get_mouse_state(x, y, buttons);
		}

		void	set_background_color(const rgba& color)
		{
			// apply only for root movie
			character* mroot = get_root_movie();
			if (this == mroot)
			{
				m_root->set_background_color(color);
			}
		}

//		virtual bool	has_looped() const { return m_has_looped; }

		inline int	transition(int a, int b) const
			// Combine the flags to avoid a conditional. It would be faster with a macro.
		{
			return (a << 2) | b;
		}

		virtual bool can_handle_mouse_event();
		virtual bool get_topmost_mouse_entity( character * &te, float x, float y);
		virtual void advance_dlist();
		virtual void advance_actions();
		virtual void advance_ctor();

		virtual void	this_alive();
		void	execute_frame_tags(int frame, bool state_only = false);
		void	execute_frame_tags_reverse(int frame);
		execute_tag*	find_previous_replace_or_add_tag(int frame, int depth, int id);
		void	execute_remove_tags(int frame);
		void	do_actions();
		virtual void do_actions(const array<action_buffer*>& action_list);
		bool	goto_frame(const tu_string& target_frame);
		bool	goto_frame(int target_frame_number);
		bool	goto_labeled_frame(const tu_string& label);

		void	display();

		character*	add_display_object( Uint16 character_id, const tu_string& name,
			const array<swf_event*>& event_handlers, int depth, bool replace_if_depth_is_occupied,
			const cxform& color_transform, const matrix& matrix, float ratio, Uint16 clip_depth, Uint8 blend_mode);
		void	move_display_object( int depth, bool use_cxform, const cxform& color_xform, bool use_matrix,
			const matrix& mat, float ratio, Uint16 clip_depth, Uint8 blend_mode);
		void	replace_display_object( Uint16 character_id, const tu_string& name, int depth, bool use_cxform,
			const cxform& color_transform, bool use_matrix, const matrix& mat, float ratio, Uint16 clip_depth, Uint8 blend_mode);
		void	replace_display_object( character* ch, const tu_string& name, int depth, bool use_cxform,
			const cxform& color_transform, bool use_matrix, const matrix& mat, float ratio, Uint16 clip_depth, Uint8 blend_mode);

		void	remove_display_object(int depth, int id);
		void	remove_display_object(const tu_string& name);
		void	remove_display_object(character* ch);
		void	clear_display_objects();

		virtual void replace_me(movie_definition*	md);
		virtual void replace_me(character_def*	def);

		void	add_action_buffer(action_buffer* a);
		int	get_id_at_depth(int depth);
		int	get_highest_depth();

		//
		// ActionScript support
		//

		virtual void	set_variable(const char* path_to_var, const char* new_value);
//		virtual const char*	get_variable(const char* path_to_var) const;

		virtual bool	set_member(const tu_string& name, const as_value& val);
		virtual bool	get_member(const tu_string& name, as_value* val);
		virtual void	call_frame_actions(const as_value& frame_spec);
		virtual void	stop_drag();
		character*	clone_display_object(const tu_string& newname, int depth);
		virtual bool	on_event(const event_id& id);
		virtual tu_string	call_method(const char* method_name, as_value * arguments, int argument_count );
		bool	hit_test(character* target);
		virtual bool	hit_test(double x, double y, bool shape_flag);
		sprite_instance* attach_movie(const tu_string& id, const tu_string name, int depth);

		void local_to_global(as_object* pt);
		void global_to_local(as_object* pt);

		character* create_text_field(const char* name, int depth, int x, int y, int width, int height);

		virtual void clear_refs(hash<as_object*, bool>* visited_objects, as_object* this_ptr);
		virtual as_environment*	get_environment() { return &m_as_environment; }
		virtual void dump(int tabs = 0);

		virtual character_def*	find_exported_resource(const tu_string& symbol);

		// drawing API
		canvas* get_canvas();

		virtual const char*	type_of() { return "movieclip"; }
		virtual	void enumerate(as_environment* env);
		virtual	void enumerate(as_array* a);
		virtual	bool is_enabled() const;

		// extension
		virtual void load_bitmaps(array < smart_ptr<bitmap_info> >* bitmap_hash);

		virtual rgba* get_rgba();
		virtual void set_rgba(const rgba& color);
		virtual const tu_string&	to_tu_string();

		void print();
		virtual image::rgba* render();

		// AS3
		void add_script(int frame, as_function* func);
		void add_event_listener(const tu_string& eventname, as_function* func);
		void remove_event_listener(const tu_string& eventname, as_function* func);
		void sprite_dispatch_event(const fn_call& fn);
		virtual as_function* get_class_constructor(character* ch);

		// for scroller
		void scroll_start(float speed);
		void scroll();

		void attach_bitmapdata(as_bitmapdata* bd, int depth);

	};
}

#endif
