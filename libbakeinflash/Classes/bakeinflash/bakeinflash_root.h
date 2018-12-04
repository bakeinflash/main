// bakeinflash_root.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation code for the bakeinflash SWF player library.


#ifndef BAKEINFLASH_ROOT_H
#define BAKEINFLASH_ROOT_H


#include "bakeinflash/bakeinflash.h"
#include "bakeinflash/bakeinflash_action.h"
#include "bakeinflash/bakeinflash_types.h"
#include "bakeinflash/bakeinflash_log.h"
#include "bakeinflash/bakeinflash_character.h"
#include "bakeinflash/bakeinflash_mutex.h"
#include "bakeinflash/bakeinflash_listener.h"
#include <assert.h>
#include "base/container.h"
#include "base/utility.h"
#include "base/smart_ptr.h" 
#include <stdarg.h>

#include "bakeinflash/bakeinflash_as_classes/as_array.h"

namespace bakeinflash
{
	tu_mutex& bakeinflash_engine_mutex();

	struct movie_def_impl;

	//
	// Helper to generate mouse events, given mouse state & history.
	//

	struct mouse_button_state
	{
		smart_ptr<character> m_active_entity;	// entity that currently owns the mouse pointer
		smart_ptr<character> m_topmost_entity;	// what's underneath the mouse right now

		bool	m_mouse_button_state_last;		// previous state of mouse button
		bool	m_mouse_button_state_current;		// current state of mouse button

		bool	m_mouse_inside_entity_last;	// whether mouse was inside the active_entity last frame
		int		m_x;
		int		m_y;
		int		m_x_last;
		int		m_y_last;

		mouse_button_state() :
			m_mouse_button_state_last(0),
			m_mouse_button_state_current(0),
			m_mouse_inside_entity_last(false),
			m_x_last(0),
			m_y_last(0)
		{
		}
	};

	//
	// root
	//
	// Global, shared root state for a movie and all its characters.
	//
	struct root : public ref_counted
	{
		smart_ptr<movie_def_impl>	m_def;
		smart_ptr<character>	m_movie;
		int			m_viewport_x0, m_viewport_y0, m_viewport_width, m_viewport_height;
		float		m_pixel_scale;

		rgba		m_background_color;
		int			m_mouse_x, m_mouse_y, m_mouse_buttons;
		void*		m_userdata;
		character::drag_state	m_drag_state;	// @@ fold this into m_mouse_button_state?
		mouse_button_state m_mouse_button_state;
		bool		m_on_event_load_called;
		bool        m_shift_key_state;

		smart_ptr<character> m_current_active_entity;
		float	m_time_remainder;
		float m_frame_time;

		// listeners
		listener m_keypress_listener;
		listener m_listener;
		listener m_mouse_listener;

		bool m_advance_ctor;
		tu_string m_infile;	// for _url

		// for scroller
		int			m_mouse_x_prev, m_mouse_y_prev;
		Uint32	m_mouse_ticks, m_mouse_ticks_prev;
		hash< weak_ptr<as_object>, weak_ptr<as_object> > m_scrollbox;
		weak_ptr<as_object> m_sprite_to_scroll;
		float m_speed;

		root(movie_def_impl* def);
		~root();

		void add_scrollbox(as_object* mc, as_object* box);
		character* get_active_entity() const { return m_current_active_entity; }
		void set_active_entity(character* ch);
		bool	generate_mouse_button_events(mouse_button_state* ms);
		void	set_root_movie(character* root_movie);

		void	get_display_viewport(int* x0, int* y0, int* w, int* h);
		void	set_display_viewport(int x0, int y0, int w, int h);
		void	notify_mouse_state(int x, int y, int buttons);
		virtual void	get_mouse_state(int* x, int* y, int* buttons);
		character*	get_root_movie() const;

		void	do_mouse_drag();
		void	set_drag_state(character::drag_state& ds);
		void	stop_drag();
		movie_definition*	get_movie_definition();

		int	get_current_frame() const;
		int	get_frame_count() { return get_root_movie()->get_frame_count(); }
		float	get_frame_rate() const;
		void	set_frame_rate(float rate);

		virtual float	get_pixel_scale() const;

		character*	get_character(int character_id);
		void	set_background_color(const rgba& color);
		void	advance(float delta_time);

		bool	goto_frame(int target_frame_number);
		virtual bool	has_looped() const;

		void	display();

		virtual bool	goto_labeled_frame(const tu_string& label);
		virtual void	set_play_state(character::play_state s);
		virtual character::play_state	get_play_state() const;
		virtual void	set_variable(const char* path_to_var, const char* new_value);
		virtual const char*	get_variable(const char* path_to_var) const;

		virtual void	set_visible(bool visible);
		virtual bool	get_visible() const;

		virtual void* get_userdata();
		virtual void set_userdata(void * ud );

		virtual int	get_movie_version();
		virtual int	get_movie_width();
		virtual int	get_movie_height();

		// External interface for the host to report key events.
		void	notify_key_event(key::code k, Uint16 utf16char, bool down);

		void set_flash_vars(const tu_string& vars);

		void add_listener(as_object* obj)  { m_listener.add(obj); }
		bool remove_listener(as_object* obj) { return m_listener.remove(obj); }
		void add_mouse_listener(as_object* obj)  { m_mouse_listener.add(obj); }
		bool remove_mouse_listener(as_object* obj) { return m_mouse_listener.remove(obj); }
		void add_keypress_listener(as_object* obj)  { m_keypress_listener.add(obj); }
		bool remove_keypress_listener(as_object* obj) { return m_keypress_listener.remove(obj); }
		void set_advance_ctor(bool val) { m_advance_ctor = val; }

		const tu_string& get_infile() const { return m_infile; }
		void set_infile(const tu_string& infile);
		float get_frame_time() const { return m_frame_time; }
		bool is_as3() const;

		vm_stack* get_scope() const { return m_scope; }

	private:
		vm_stack* m_scope;	// AS3

	};
}

#endif
