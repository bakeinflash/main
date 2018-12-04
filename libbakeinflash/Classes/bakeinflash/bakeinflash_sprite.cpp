// bakeinflash_sprite.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation for SWF player.

// Useful links:
//
// http://sswf.sourceforge.net/SWFalexref.html
// http://www.openswf.org

#include "bakeinflash/bakeinflash_action.h"
#include "bakeinflash/bakeinflash_impl.h"
#include "bakeinflash/bakeinflash_stream.h"
#include "bakeinflash/bakeinflash_sprite_def.h"
#include "bakeinflash/bakeinflash_sprite.h"
#include "bakeinflash/bakeinflash_as_sprite.h"
#include "bakeinflash/bakeinflash_text.h"
#include "bakeinflash/bakeinflash_as_classes/as_string.h"
#include "bakeinflash/bakeinflash_as_classes/as_broadcaster.h"
#include "bakeinflash/bakeinflash_as_classes/as_event.h"
#include "bakeinflash/bakeinflash_as_classes/as_print.h"
#include "bakeinflash/bakeinflash_as_classes/as_bitmapdata.h"
#include "base/png_helper.h"

#if TU_CONFIG_LINK_TO_LIBHPDF == 1
// hack
#include <SDL2/SDL_opengl.h>
#endif

extern float s_scale;

namespace bakeinflash
{

	struct as_mcloader;

	const char*	next_slash_or_dot(const char* word);
	void	execute_actions(as_environment* env, const array<action_buffer*>& action_list);

	// this stuff should be high optimized
	// thus I can't use here set_member(...);
	sprite_instance::sprite_instance(movie_definition_sub* def,	root* r, character* parent, int id) :
		character(parent, id),
		m_def(def),
		m_root(r),
		m_play_state(PLAY),
		m_current_frame(0),
		m_target_frame(0),
		m_mouse_state(UP),
		m_enabled(true),
		m_on_event_load_called(false),
		m_script(NULL),
		m_as3listener(NULL),
		m_do_advance(true),
		m_do_ctor(true),
		m_bitmap_hash(NULL),
		m_last_visited_frame(-1),		// AS3, 0 based
		m_scroll_mode(0)
	{
		assert(m_def != NULL);
		assert(m_root != NULL);

		//m_root->add_ref();	// @@ circular!
		m_as_environment.set_target(this);
		
		// Initialize the flags for init action executed.
		m_init_actions_executed.resize(m_def->get_frame_count());
//		memset(&m_init_actions_executed[0], 0, sizeof(m_init_actions_executed[0]) * m_init_actions_executed.size());
    for (int i = 0; i < m_init_actions_executed.size(); i++)
    {
      m_init_actions_executed[i] = false;
    }

		set_alive(this);
		set_ctor(as_global_movieclip_ctor);
		get_root()->set_advance_ctor(true);
//		use_proto("MovieClip");
	}

	sprite_instance::~sprite_instance()
	{
		delete m_script;
		delete m_bitmap_hash;
		delete m_as3listener;
	}

	bool sprite_instance::has_keypress_event()
	{
		as_value unused;
		return get_member("onKeyPress", &unused);
	}

	void sprite_instance::get_bound(rect* bound)
	{
		int i, n = m_display_list.size();
		if (n == 0)
		{
			return;
		}

		bound->m_x_min = FLT_MAX;
		bound->m_x_max = - FLT_MAX;
		bound->m_y_min = FLT_MAX;
		bound->m_y_max = - FLT_MAX;

		const matrix& m = get_matrix();
		for (i = 0; i < n; i++)
		{
			character* ch = m_display_list.get_character(i);
			if (ch != NULL)
			{
				rect ch_bound;
				ch->get_bound(&ch_bound);

				m.transform(&ch_bound);

				bound->expand_to_rect(ch_bound);
			}
		}
	}

	character* sprite_instance::add_empty_movieclip(const char* name, int depth)
	{
		cxform color_transform;
		matrix matrix;

		// empty_sprite_def will be deleted during deliting sprite
		sprite_definition* empty_sprite_def = new sprite_definition(NULL);

		sprite_instance* sprite =	new sprite_instance(empty_sprite_def, m_root, this, 0);
		sprite->set_name(name);

		m_display_list.add_display_object( sprite, depth, true, color_transform, matrix, 0.0f, 0, 0); 

		return sprite;
	}

	void	sprite_instance::set_play_state(play_state s)
	// Stop or play the sprite.
	{
		// pauses stream sound
		sound_handler* sound = get_sound_handler();
		if (sound)
		{
			if (m_def->m_ss_id >= 0)
			{
				sound->pause(m_def->m_ss_id, m_play_state == PLAY ? true : false);
			}
		}
		m_play_state = s;
	}

	sprite_instance::play_state	sprite_instance::get_play_state() const
	{
		return m_play_state; 
	}

	// Functions that qualify as mouse event handlers.

	bool sprite_instance::can_handle_mouse_event()
	// Return true if we have any mouse event handlers.
	{
		return get_mouse_flag();
	}

	bool sprite_instance::get_topmost_mouse_entity( character * &top_ent, float x, float y)
	// Return the topmost entity that the given point
	// covers that can receive mouse events.  NULL if
	// none.  Coords are in parent's frame.
	{
		if (get_visible() == false || is_enabled() == false)
		{
			return false;
		}

		const matrix&	m = get_matrix();
		point	p;
		m.transform_by_inverse(&p, point(x, y));

		character*	top_te = NULL;
		bool this_has_focus = false;
		int i, n = m_display_list.size();

		top_ent = NULL;

		// Go backwards, to check higher objects first.
		for (i = n - 1; i >= 0; i--)
		{
			character* ch = m_display_list.get_character(i);
			character* te = NULL;

			if (ch != NULL && ch->get_visible())
			{
				if (ch->get_topmost_mouse_entity(te, p.m_x, p.m_y))
				{
					this_has_focus = true;
					// The containing entity that 1) is closest to root and 2) can
					// handle mouse events takes precedence.
					if (te && te->can_handle_mouse_event())
					{
						top_te = te;
						break;
					}
				}
			}
		}

		//  THIS is closest to root
		if (this_has_focus && can_handle_mouse_event())
		{
			top_ent = this;
		}
		else
		// else character which has event is closest to root
		if (top_te)
		{
			top_ent = top_te;
		}

		// else if we have focus then return not NULL
		return this_has_focus;
	}


	// This code is very tricky and hard to get right.  It should
	// only be changed when verified by an automated test.  Here
	// is the procedure:
	//
	// 1. Identify a bug or desired feature.
	//
	// 2. Create a .swf file that isolates the bug or new feature.
	// The .swf should use trace() statements to log what it is
	// doing and define the correct behavior.
	//
	// 3. Collect the contents of flashlog.txt from the standalone
	// Macromedia flash player.  Create a new test file under
	// tests/ where the first line is the name of the new test
	// .swf, and the rest of the file is the correct trace()
	// output.
	//
	// 4. Verify that bakeinflash fails the new test (by running
	// ./bakeinflash_batch_test.py tests/name_of_new_test.txt)
	//
	// 5. Fix bakeinflash behavior.  Add the new test to
	// passing_tests[] in bakeinflash_batch_test.py.
	//
	// 6. Verify that all the other passing tests still pass!
	// (Run ./bakeinflash_batch_test.py and make sure all tests are
	// OK!  If not, then the new behavior is actually a
	// regression.)
	void sprite_instance::advance_dlist()
	{
		// Update current and next frames.
		if (m_play_state == PLAY &&	m_do_advance == true)
		{
			int prev_frame = m_current_frame;
			if (m_on_event_load_called)
			{
				m_current_frame++;
				if (m_current_frame >= m_def->get_frame_count())
				{
					m_current_frame = 0;
				}
				m_target_frame = m_current_frame;
			}

			// Execute the current frame's tags.
			// execute_frame_tags(0) already executed in dlist.cpp 
			if (m_current_frame != prev_frame) 
			{ 
				// Macromedia Flash does not call remove display object tag
				// for 1-st frame therefore we should do it for it :-)
				if (m_current_frame == 0 && m_def->get_frame_count() > 1)
				{
					// affected depths
					const array<execute_tag*>& playlist = m_def->get_playlist(0);
					array<int> affected_depths;
					for (int i = 0; i < playlist.size(); i++)
					{
						int depth = (playlist[i]->get_depth_id_of_replace_or_add_tag()) >> 16;
						if (depth != -1)
						{
							affected_depths.push_back(depth);
						}
					}

					if (affected_depths.size() > 0)
					{
						m_display_list.clear_unaffected(affected_depths);
					}
					else
					{
						m_display_list.clear();
					}
				}
				execute_frame_tags(m_current_frame);
			}
		}

		m_do_advance = true;

		// Advance everything in the display list.
		m_display_list.advance_dlist();
	}

	void sprite_instance::advance_ctor()
	{
		// child movieclip frame rate is the same the root movieclip frame rate
		// that's why it is not needed to analyze 'm_time_remainder'
		if (get_root()->is_as3())
		{
			m_display_list.advance_ctor();

			if (m_do_ctor)
			{
				m_do_ctor = false;
				// run constructor
				m_def->instanciate_registered_class(this);
			}
		}
		else
		{
			if (m_do_ctor)
			{
				m_do_ctor = false;
				// run constructor
				m_def->instanciate_registered_class(this);
			}

			m_display_list.advance_ctor();
		}
	}

	void sprite_instance::advance_actions()
	{
		// child movieclip frame rate is the same the root movieclip frame rate
		// that's why it is not needed to analyze 'm_time_remainder'

		if (m_scroll_way.size() > 0)
		{
			scroll();
		}

		if (m_do_ctor)
		{
			m_do_ctor = false;

			// run constructor
			m_def->instanciate_registered_class(this);
		}

		if (m_on_event_load_called)
		{
			as_value func;
			static tu_string funcname("onEnterFrame");
			if (as_object::get_member(funcname, &func))
			{
				bakeinflash::call_method(func, &m_as_environment, this, 0, m_as_environment.get_top_index());
			}
		}

		// to this clip action
		do_actions();

		// do everything in the display list.
		m_display_list.advance_actions();

		// onLoad must be called after all scripts in this and child clips
		if (m_on_event_load_called == false)
		{
			// clip sprite onload 
			// _root.onLoad() will not be executed since do_actions()
			// for frame(0) has not executed yet.
			// _root.onLoad() will be executed later in root::advance()
			on_event(event_id::LOAD);

			// it maybe changed in on_event(...)
			// so must be after that
			m_on_event_load_called = true;
		}

		// 'this' and its variables is not garbage
		this_alive();
		set_alive(this);
	}

	// load textures
	void sprite_instance::load_bitmaps(array < smart_ptr<bitmap_info> >* bitmap_hash)
	{
//				array < smart_ptr<bitmap_info> >	m_bitmap_hash;
		int k = m_current_frame;
		for (int i = 0; i < m_def->get_frame_count(); i++)
		{
			execute_frame_tags(i, true);
			m_display_list.load_bitmaps(bitmap_hash);
		}
		execute_frame_tags(k, true);
	}

	// this and all children (bush) is not garbage
	// called from button instance only
	void sprite_instance::this_alive()
	{

		// Whether there were we here already ?
		if (get_clear_garbage() == false)
		{
			return;
		}

		if (is_garbage(this))
		{
			as_object::this_alive();

			// mark display list as alive
			for (int i = 0, n = m_display_list.size(); i < n; i++)
			{
				character*	ch = m_display_list.get_character(i);
				if (ch)
				{
					ch->this_alive();
				}
			}
		}
	}

	// Execute the tags associated with the specified frame.
	// frame is 0-based
	void sprite_instance::execute_frame_tags(int frame, bool state_only)
	{
		// Keep this (particularly m_as_environment) alive during execution!
		smart_ptr<as_object>	this_ptr(this);

		assert(frame >= 0);
		assert(frame < m_def->get_frame_count());

		m_def->wait_frame(frame);

		// Execute this frame's init actions, if necessary.
		if (m_init_actions_executed[frame] == false)
		{
			const array<execute_tag*>*	init_actions = m_def->get_init_actions(frame);
			if (init_actions && init_actions->size() > 0)
			{
				// Need to execute these actions.
				for (int i= 0; i < init_actions->size(); i++)
				{
					execute_tag*	e = (*init_actions)[i];
					e->execute_now(this);
				}

				// Mark this frame done, so we never execute these init actions
				// again.
				m_init_actions_executed[frame] = true;
			}
		}

		const array<execute_tag*>&	playlist = m_def->get_playlist(frame);
		for (int i = 0; i < playlist.size(); i++)
		{
			execute_tag*	e = playlist[i];

			if (state_only)
			{
				e->execute_state(this);
			}
			else
			{
				e->execute(this);
			}
		}

		// start stream sound
		if (state_only == false)
		{
			sound_handler* sound = get_sound_handler();
			if (sound)
			{
				if (m_def->m_ss_start == frame)
				{
					if (m_def->m_ss_id >= 0)
					{
						sound->stop_sound(m_def->m_ss_id);
						sound->play_sound(NULL, m_def->m_ss_id, 0);
					}
				}
			}
		}
	}

	// Execute the tags associated with the specified frame, IN REVERSE.
	// I.e. if it's an "add" tag, then we do a "remove" instead.
	// Only relevant to the display-list manipulation tags: add, move, remove, replace.
	void	sprite_instance::execute_frame_tags_reverse(int frame)
	{
		// frame is 0-based
		// Keep this (particularly m_as_environment) alive during execution!
		smart_ptr<as_object>	this_ptr(this);

		assert(frame >= 0);
		assert(frame < m_def->get_frame_count());

		const array<execute_tag*>&	playlist = m_def->get_playlist(frame);
		for (int i = 0; i < playlist.size(); i++)
		{
			execute_tag*	e = playlist[i];
			e->execute_state_reverse(this, frame);
		}
	}


	execute_tag*	sprite_instance::find_previous_replace_or_add_tag(int frame, int depth, int id)
	{
		uint32	depth_id = ((depth & 0x0FFFF) << 16) | (id & 0x0FFFF);

		for (int f = frame - 1; f >= 0; f--)
		{
			const array<execute_tag*>&	playlist = m_def->get_playlist(f);
			for (int i = playlist.size() - 1; i >= 0; i--)
			{
				execute_tag*	e = playlist[i];
				if (e->get_depth_id_of_replace_or_add_tag() == depth_id)
				{
					return e;
				}
			}
		}

		return NULL;
	}

	// Execute any remove-object tags associated with the specified frame.
	// frame is 0-based
	void	sprite_instance::execute_remove_tags(int frame)
	{
		assert(frame >= 0);
		assert(frame < m_def->get_frame_count());

		const array<execute_tag*>&	playlist = m_def->get_playlist(frame);
		for (int i = 0; i < playlist.size(); i++)
		{
			execute_tag*	e = playlist[i];
			if (e->is_remove_tag())
			{
				e->execute_state(this);
			}
		}
	}

	void	sprite_instance::do_actions()
	// Take care of this frame's actions.
	{
		// Keep m_as_environment alive during any method calls!
		smart_ptr<as_object>	this_ptr(this);

		execute_actions(&m_as_environment, m_action_list);
		m_action_list.resize(0);

		// AS3
		int current_frame = get_current_frame();
		if (m_script && get_root()->is_as3() && m_last_visited_frame != current_frame)
		{
			m_last_visited_frame = current_frame;

			// run frame script once per frame
			smart_ptr<as_function> func;
			if (m_script->get(current_frame, &func) && func != NULL)
			{
				bakeinflash::call_method(func, &m_as_environment, this, 0, 0);
			}
		}
	}

	void sprite_instance::do_actions(const array<action_buffer*>& action_list)
	{
		for (int i = 0; i < action_list.size(); i++)
		{
			action_list[i]->execute(&m_as_environment);
		}
	}

	bool sprite_instance::goto_frame(const tu_string& target_frame)
	{
		// Flash tries to convert STRING to NUMBER,
		// if the conversion is OK then Flash uses this NUMBER as target_frame.
		// else uses arg as label of target_frame
		// Thanks Francois Guibert
		double number_value;

		// try string as number
		if (string_to_number(&number_value, target_frame.c_str()))
		{
			return goto_frame((int) number_value - 1);	// Convert to 0-based
		}
		return goto_labeled_frame(target_frame);
	}

	// Set the sprite state at the specified frame number.
	// 0-based frame numbers!!  (in contrast to ActionScript and Flash MX)
	bool sprite_instance::goto_frame(int target_frame_number)
	{
		//default property is to stop on goto frame
		m_play_state = STOP;
		m_do_advance = false;

		// Macromedia Flash ignores goto_frame(bad_frame)
		if (target_frame_number > m_def->get_frame_count() - 1 || target_frame_number < 0)
		{
			return false;
		}
		if (target_frame_number == m_current_frame)	// to prevent infinitive recursion
		{
			return true;
		}

		m_target_frame = target_frame_number;

		if (target_frame_number < m_current_frame)
		{
			for (int f = m_current_frame; f > target_frame_number; --f)
			{
				execute_frame_tags_reverse(f);	
			}
		}
		else
		{
			for (int f = m_current_frame + 1; f < target_frame_number; ++f)
			{
				execute_frame_tags(f, true);
			}
		}

		//m_action_list.clear();
		execute_frame_tags(target_frame_number, false);

		m_current_frame = target_frame_number;
		m_target_frame = m_current_frame;
		return true;
	}


	// Look up the labeled frame, and jump to it.
	bool sprite_instance::goto_labeled_frame(const tu_string& label)
	{
		int	target_frame = -1;
		if (m_def->get_labeled_frame(label, &target_frame))
		{
			return goto_frame(target_frame);
		}
		IF_VERBOSE_ACTION(myprintf("error: movie_impl::goto_labeled_frame('%s') unknown label\n", label.c_str()));
		return false;
	}

	void sprite_instance::display()
	{
		if (get_visible() == false)
		{
			// We're invisible, so don't display!
			return;
		}

		// is the movieclip masked ?
		if (m_mask_clip != NULL)
		{
			render::begin_submit_mask();

			m_mask_clip->set_visible(true);
			m_mask_clip->display();
			m_mask_clip->set_visible(false);

			render::end_submit_mask();

			m_display_list.display();

			render::disable_mask();
		}
		else
		{
			m_display_list.display();
		}
	}

	character* sprite_instance::add_display_object( Uint16 character_id, const tu_string& name,
		const array<swf_event*>& event_handlers, int depth, bool replace_if_depth_is_occupied,
		const cxform& color_transform, const matrix& matrix, float ratio, Uint16 clip_depth, Uint8 blend_mode)
		// Add an object to the display list.
	{
		assert(m_def != NULL);

		character_def*	cdef = m_def->get_character_def(character_id);
		if (cdef == NULL)
		{
			myprintf("sprite::add_display_object(): unknown cid = %d\n", character_id);
			return NULL;
		}

		// If we already have this object on this
		// plane, then move it instead of replacing it.
		character*	existing_char = m_display_list.get_character_at_depth(depth);
		if (existing_char && character_id == existing_char->get_id() && name == existing_char->get_name())
		{
			move_display_object(depth, true, color_transform, true, matrix, ratio, clip_depth, blend_mode);

			// Attach event handlers (if any).
			for (int i = 0, n = event_handlers.size(); i < n; i++)
			{
				const tu_string& name = event_handlers[i]->m_event.get_function_name();

				// Create a function to execute the actions.
				array<with_stack_entry>	empty_with_stack;
				as_s_function*	func = new as_s_function(&event_handlers[i]->m_method, 0, empty_with_stack);
				func->set_length(event_handlers[i]->m_method.get_length());
				func->set_target(existing_char);

				existing_char->set_member(name, func);
			}

			return NULL;
		}

		assert(cdef);
		smart_ptr<character>	ch = cdef->create_character_instance(this, character_id);
		assert(ch != NULL);
		ch->set_name(name);

		// Attach event handlers (if any).
		for (int i = 0, n = event_handlers.size(); i < n; i++)
		{
			const tu_string& name = event_handlers[i]->m_event.get_function_name();

			// Create a function to execute the actions.
			array<with_stack_entry>	empty_with_stack;
			as_s_function*	func = new as_s_function(&event_handlers[i]->m_method, 0, empty_with_stack);
			func->set_length(event_handlers[i]->m_method.get_length());
			func->set_target(ch);

			ch->set_member(name, func);
		}

		m_display_list.add_display_object( ch.get(), depth, replace_if_depth_is_occupied, color_transform,
			matrix, ratio, clip_depth, blend_mode);

		// run constructor
		//cdef->instanciate_registered_class(ch);

		assert(ch == NULL || ch->get_ref_count() > 1);
		return ch.get();
	}


	void sprite_instance::move_display_object( int depth, bool use_cxform, const cxform& color_xform,
			bool use_matrix, const matrix& mat, float ratio, Uint16 clip_depth, Uint8 blend_mode)
		// Updates the transform properties of the object at
		// the specified depth.
	{
		m_display_list.move_display_object(depth, use_cxform, color_xform, use_matrix, mat, ratio, clip_depth, blend_mode);
	}


	/*sprite_instance*/
	void sprite_instance::replace_display_object( Uint16 character_id, const tu_string& name, int depth,
		bool use_cxform, const cxform& color_transform, bool use_matrix, const matrix& mat, float ratio,
		Uint16 clip_depth, Uint8 blend_mode)
	{
		assert(m_def != NULL);

		character_def*	cdef = m_def->get_character_def(character_id);
		if (cdef == NULL)
		{
			myprintf("sprite::replace_display_object(): unknown cid = %d\n", character_id);
			return;
		}
		assert(cdef);

		smart_ptr<character>	ch = cdef->create_character_instance(this, character_id);
		assert(ch != NULL);

		if (name.size() > 0)
		{
			ch->set_name(name);
		}

		m_display_list.replace_display_object( ch.get(), depth, use_cxform, color_transform, use_matrix,
			mat, ratio, clip_depth, blend_mode);

//		ch->on_event(event_id::CONSTRUCT);	// isn't tested
	}

	void sprite_instance::replace_me(character_def*	def)
	{
		assert(def);
		character* parent = get_parent();

		// is 'this' root ?
		if (parent)
		{
			m_play_state = PLAY;
			m_current_frame = 0;
			m_target_frame = m_current_frame;
			m_mouse_state = UP;
			m_enabled = true;
			m_on_event_load_called = false;
			m_do_advance = true;
			m_do_ctor = false;

			delete m_bitmap_hash;
			m_bitmap_hash = NULL;

			clear_display_objects();
			m_action_list.clear();

			cxform cx;
			matrix m;
			int depth = 0;

			// todo: check for sprite
			// tested for jpeg
			character* owner = cast_to<bitmap_character_def>(def) ? this : parent;
			character* ch = def->create_character_instance(owner, 0);

			m_display_list.add_display_object(ch, depth, true, cx, m, 0.0f, 0, 0); 
		}
		else
		{
			myprintf("can't replace _root\n");
		}
	}

	void sprite_instance::replace_me(movie_definition* md)
	{
		assert(md);

		// is 'this' root ?
		character* parent = get_parent();
		if (parent == NULL)
		{
			root* new_inst = md->create_instance();
			character* ch = new_inst->get_root_movie();
			set_root(new_inst);
		}
		else
		{
			m_def = cast_to<movie_def_impl>(md);
			assert(m_def);

			m_play_state = PLAY;
			m_current_frame = 0;
			m_target_frame = m_current_frame;
			m_mouse_state = UP;
			m_enabled = true;
			m_on_event_load_called = false;
			m_do_advance = true;
			m_do_ctor = true;

			delete m_bitmap_hash;
			m_bitmap_hash = NULL;

			get_root()->set_advance_ctor(true);

			// Initialize the flags for init action executed.
			m_init_actions_executed.resize(m_def->get_frame_count());

//			memset(&m_init_actions_executed[0], 0, sizeof(m_init_actions_executed[0]) * m_init_actions_executed.size());
			for (int i = 0; i < m_init_actions_executed.size(); i++)
			{
				m_init_actions_executed[i] = false;
			}

			clear_display_objects();
			m_action_list.clear();

			execute_frame_tags(0);
			advance_ctor();

			// иначе вызывает рекурсивный onLoad
			m_on_event_load_called = true;

			advance_actions();
		}
	}

	void sprite_instance::replace_display_object( character* ch, const tu_string& name, int depth, bool use_cxform,
			const cxform& color_transform, bool use_matrix, const matrix& mat, float ratio, Uint16 clip_depth,
			Uint8 blend_mode)
	{
		assert(ch != NULL);

		if (name != "")
		{
			ch->set_name(name);
		}

		m_display_list.replace_display_object( ch, depth, use_cxform, color_transform, use_matrix, mat,
			ratio, clip_depth, blend_mode);
	}

	// Remove the object at the specified depth.
	// If id != -1, then only remove the object at depth with matching id.
	void sprite_instance::remove_display_object(int depth, int id)
	{
		m_display_list.remove_display_object(depth, id);
	}

	void sprite_instance::clear_display_objects()
	// Remove all display objects
	{
		m_display_list.clear();
	}

	// Add the given action buffer to the list of action
	// buffers to be processed at the end of the next
	// frame advance.
	void sprite_instance::add_action_buffer(action_buffer* a)
	{
		m_action_list.push_back(a);
	}

	// For debugging -- return the id of the character at the specified depth.
	// Return -1 if nobody's home.
	int	sprite_instance::get_id_at_depth(int depth)
	{
		int	index = m_display_list.get_display_index(depth);
		if (index == -1)
		{
			return -1;
		}

		character*	ch = m_display_list.get_display_object(index).m_character.get();

		return ch->get_id();
	}

	int	sprite_instance::get_highest_depth()
	{
		return m_display_list.get_highest_depth();
	}

	//
	// ActionScript support
	//

	void sprite_instance::set_variable(const char* path_to_var, const char* new_value)
	{
		assert(m_parent == NULL);	// should only be called on the root movie.

		if (path_to_var == NULL)
		{
			myprintf("error: NULL path_to_var passed to set_variable()\n");
			return;
		}
		if (new_value == NULL)
		{
			myprintf("error: NULL passed to set_variable('%s', NULL)\n", path_to_var);
			return;
		}

		array<with_stack_entry>	empty_with_stack;
		tu_string	path(path_to_var);
		as_value	val(new_value);

		m_as_environment.set_variable(path, val, empty_with_stack);
	}

	// useful for catching of the calls
	bool sprite_instance::set_member(const tu_string& name, const as_value& val)
	{
		// first try built-ins sprite properties
		as_standard_member	std_member = get_standard_member(name);
		switch (std_member)
		{
			default:
				break;

			case M_SCROLLBOX:
			{
				get_root()->add_scrollbox(this, val.to_object());
				break;
			}

			case M_MOUSE_MOVE:
			{
				if (val.is_function())
				{
					m_root->add_mouse_listener(this);
				}
				else
				{
					m_root->remove_mouse_listener(this);
				}
				break;
			}
		
			case M_ENABLED:
			{
				as_value new_val(val);
				call_watcher(name, as_value(m_enabled), &new_val);
				m_enabled = new_val.to_bool();
				return true;
			}
		}

		return character::set_member(name, val);
	}

	// Set *val to the value of the named member and
	// return true, if we have the named member.
	// Otherwise leave *val alone and return false.
	bool sprite_instance::get_member(const tu_string& name, as_value* val)
	{

		// first try built-ins sprite methods
		if (get_builtin(get_root()->is_as3() ? BUILTIN_SPRITE_METHOD_AS3: BUILTIN_SPRITE_METHOD, name, val))
		{
			return true;
		}

		// then try built-ins sprite properties
		as_standard_member	std_member = get_standard_member(name);
		switch (std_member)
		{
			case M_ENABLED:
			{
				val->set_bool(m_enabled);
				return true;
			}

			case M_CURRENTFRAME:
			{
				int n = get_current_frame();
				if (n >= 0)
				{
					val->set_int(n + 1);
				}
				else
				{
					val->set_undefined();
				}
				return true;
			}
			case M_TOTALFRAMES:
			{
				// number of frames.  Read only.
				int n = get_frame_count();
				if (n >= 0)
				{
					val->set_int(n);
				}
				else
				{
					val->set_undefined();
				}
				return true;
			}
			case M_FRAMESLOADED:
			{
				int n = get_loading_frame();
				if (n >= 0)
				{
					val->set_int(n);
				}
				else
				{
					val->set_undefined();
				}
				return true;
			}
			default:
				break;
		}

		// Not a built-in property.  Check items on our display list.
		character*	ch = m_display_list.get_character_by_name(name);
		if (ch)
		{
			// Found object.
			val->set_as_object(ch);
			return true;
		}

		// finally try standart character properties & movieclip variables
		return character::get_member(name, val);
	}

	void sprite_instance::call_frame_actions(const as_value& frame_spec)
	// Execute the actions for the specified frame.	 The
	// frame_spec could be an integer or a string.
	{
		int	frame_number = -1;

		// Figure out what frame to call.
		if (frame_spec.is_string())
		{
			if (m_def->get_labeled_frame(frame_spec.to_tu_string(), &frame_number) == false)
			{
				// Try converting to integer.
				frame_number = frame_spec.to_int();
			}
		}
		else
		{
			// convert from 1-based to 0-based
			frame_number = frame_spec.to_int() - 1;
		}

		if (frame_number < 0 || frame_number >= m_def->get_frame_count())
		{
			// No dice.
			myprintf("error: call_frame('%s') -- unknown frame\n", frame_spec.to_string());
			return;
		}

		int	top_action = m_action_list.size();

		// Execute the actions.
		const array<execute_tag*>&	playlist = m_def->get_playlist(frame_number);
		for (int i = 0; i < playlist.size(); i++)
		{
			execute_tag*	e = playlist[i];
			if (e->is_action_tag())
			{
				e->execute(this);
			}
		}

		// Execute any new actions triggered by the tag,
		// leaving existing actions to be executed.
		while (m_action_list.size() > top_action)
		{
			m_action_list[top_action]->execute(&m_as_environment);
			m_action_list.remove(top_action);
		}
		assert(m_action_list.size() == top_action);
	}


	/* sprite_instance */

	void sprite_instance::stop_drag()
	{
		assert(m_parent == NULL);	// we must be the root movie!!!

		m_root->stop_drag();
	}

	character* sprite_instance::clone_display_object(const tu_string& newname, int depth)
	// Duplicate the object with the specified name and add it with a new name 
	// at a new depth.
	{
		sprite_instance* parent = cast_to<sprite_instance>(get_parent());
		sprite_instance* ch = NULL; 
		if (parent) 
		{ 
			// clone a previous external loaded movie ?
			if (get_id() == -1)	
			{
					ch = new sprite_instance(cast_to<movie_definition_sub>(m_def.get()), get_root(),	parent,	-1);

				ch->set_parent(parent);
				ch->set_root(get_root());
				ch->set_name(newname);

				parent->m_display_list.add_display_object( ch,  depth, true, get_cxform(),  get_matrix(),
						get_ratio(),  get_clip_depth(), get_blend_mode()); 
			}
			else
			{
				ch = new sprite_instance(m_def.get(), get_root(),	parent,	0);
				ch->set_parent(parent);
				ch->set_root(get_root());
				ch->set_name(newname);

				parent->m_display_list.add_display_object( ch,  depth, true, get_cxform(),  get_matrix(), 
							get_ratio(),  get_clip_depth(), get_blend_mode()); 
			}

			// copy this's members to new created character
			copy_to(ch);

		}
		else
		{
			myprintf("can't clone _root\n");
		}
		return ch;
	}

	void sprite_instance::remove_display_object(const tu_string& name)
	// Remove the object with the specified name.
	{
		character* ch = m_display_list.get_character_by_name(name);
		if (ch)
		{
			// @@ TODO: should only remove movies that were created via clone_display_object --
			// apparently original movies, placed by anim events, are immune to this.
			remove_display_object(ch->get_depth(), ch->get_id());
		}
	}

	void sprite_instance::remove_display_object(character* ch)
	// Remove the object with the specified pointer.
	{
		m_display_list.remove_display_object(ch);
	}

	bool sprite_instance::on_event(const event_id& id)
	// Dispatch event handler(s), if any.
	{
		// Keep m_as_environment alive during any method calls!
		smart_ptr<as_object>	this_ptr(this);

		// In ActionScript 2.0, event method names are CASE SENSITIVE.
		// In ActionScript 1.0, event method names are CASE INSENSITIVE.
		const tu_string&	method_name = id.get_function_name();
		as_value	method;
		//if (m_members.get(method_name, &method) && method.is_undefined() == false)	// так нельзя, прототип тоже надо проверить
		if (as_object::get_member(method_name, &method) && method.is_undefined() == false)
		{
			int nargs = 0;
			if (id.m_args)
			{
				nargs = id.m_args->size();
				for (int i = nargs - 1; i >= 0; i--)
				{
					m_as_environment.push((*id.m_args)[i]);
				}
			}

			
			// need 1 arg for AS3 event handler
			smart_ptr<as_event> ev;
			if (nargs == 0 && get_root()->is_as3())
			{
				nargs = 1;
				ev = new as_event(method_name, this);
				m_as_environment.push(ev.get());
			}

			bakeinflash::call_method(method, &m_as_environment, this, nargs, m_as_environment.get_top_index());

			m_as_environment.drop(nargs);
			return true;
		}
		return false;
	}

	tu_string sprite_instance::call_method(const char* method_name, as_value * arguments, int argument_count )
	{
		return bakeinflash::call_method( &m_as_environment, this, method_name, arguments, argument_count );
	}

	bool sprite_instance::hit_test(character* ch)
	{
		rect this_bound;
		get_bound(&this_bound);
		matrix m;
		if (m_parent != NULL)
		{
			m_parent->get_world_matrix(&m);
			m.transform(&this_bound);
		}

		rect ch_bound;
		ch->get_bound(&ch_bound);
		if (ch->m_parent != NULL)
		{
			ch->m_parent->get_world_matrix(&m);
			m.transform(&ch_bound);
		}

		rect r;		// intersection
		r.m_x_min = (float) fmax(this_bound.m_x_min, ch_bound.m_x_min);
		r.m_y_min = (float) fmax(this_bound.m_y_min, ch_bound.m_y_min);
		r.m_x_max = (float) fmin(this_bound.m_x_max, ch_bound.m_x_max);
		r.m_y_max = (float) fmin(this_bound.m_y_max, ch_bound.m_y_max);
		r.twips_to_pixels();

		if (r.m_x_min < r.m_x_max && r.m_y_min < r.m_y_max)
		{
#if TU_USE_FLASH_COMPATIBLE_HITTEST == 1
			return true;
#else

			// this hitTest is not compatible with Flash but
			// it works with absolutely accuracy 
			// if you want hitTest two bitmaps you should trace they into shapes

			rgba background_color(0, 0, 0, 0);
			movie_def_impl* def = cast_to<movie_def_impl>(get_root()->get_movie_definition());

			render::begin_display(background_color,
				get_root()->m_viewport_x0, get_root()->m_viewport_y0,
				get_root()->m_viewport_width, get_root()->m_viewport_height,
				def->m_frame_size.m_x_min, def->m_frame_size.m_x_max,
				def->m_frame_size.m_y_min, def->m_frame_size.m_y_max);

			render::begin_submit_mask();
			display();

			render::begin_submit_mask();
			ch->display();

			bool hittest = render::test_stencil_buffer(r, 2);

			render::disable_mask();
			render::disable_mask();

			render::end_display();

			return hittest;

#endif

		}
		return false;
	}

	bool	sprite_instance::hit_test(double x, double y, bool test_shape)
	{
		rect this_bound;
		
		get_bound(&this_bound);
		
		if (m_parent != NULL)
		{
			matrix m;
			m_parent->get_world_matrix(&m);
			m.transform(&this_bound);
		}
		
		this_bound.twips_to_pixels();

		if (this_bound.point_test((float) x, (float) y))
		{
			if (!test_shape )
			{
				return true;
			}
			else
			{
				// this hitTest is not compatible with Flash but
				// it works with absolutely accuracy 
				// if you want hitTest two bitmaps you should trace they into shapes

				rgba background_color(0, 0, 0, 0);
				movie_def_impl* def = cast_to<movie_def_impl>(get_root()->get_movie_definition());

				render::begin_display(background_color,
					get_root()->m_viewport_x0, get_root()->m_viewport_y0,
					get_root()->m_viewport_width, get_root()->m_viewport_height,
					def->m_frame_size.m_x_min, def->m_frame_size.m_x_max,
					def->m_frame_size.m_y_min, def->m_frame_size.m_y_max);

				render::begin_submit_mask();
				display();

				rect r;

				r.m_x_min = (float) x;
				r.m_y_min = (float) y;

				r.m_x_max = r.m_x_min + 1;
				r.m_y_max = r.m_y_min + 1;


				bool hittest = render::test_stencil_buffer(r, 1);

				render::end_submit_mask();
				render::disable_mask();

				render::end_display();

				return hittest;
			}
		}
		return false;
	}

	uint32	sprite_instance::get_file_bytes() const
	{
		movie_def_impl* root_def = cast_to<movie_def_impl>(m_def.get());
		if (root_def)
		{
			return root_def->get_file_bytes();
		}
		return 0;
	}

	uint32 sprite_instance::get_loaded_bytes() const
	{
		movie_def_impl* root_def = cast_to<movie_def_impl>(m_def.get());
		if (root_def)
		{
			return root_def->get_loaded_bytes();
		}
		return 0;
	}

	character* sprite_instance::create_text_field(const char* name, int depth, int x, int y, int width, int height)
	// Creates a new, empty text field as a child of the movie clip
	{
		edit_text_character_def* textdef = new edit_text_character_def(width, height);

		character* textfield = textdef->create_character_instance(this, 0);
		textfield->set_name(name);

		matrix m;
		m.concatenate_translation(PIXELS_TO_TWIPS(x), PIXELS_TO_TWIPS(y));

		cxform color_transform;
		m_display_list.add_display_object(textfield, depth, true, color_transform, m, 0.0f, 0, 0); 

//		textfield->on_event(event_id::CONSTRUCT);	// isn't tested
		return textfield;
	}

	void sprite_instance::clear_refs(hash<as_object*, bool>* visited_objects, as_object* this_ptr)
	{
		// Is it a reentrance ?
		if (visited_objects->get(this, NULL))
		{
			return;
		}

		// will be set in as_object::clear_refs
//		visited_objects->set(this, true);

		as_object::clear_refs(visited_objects, this_ptr);

		// clear display list
		m_display_list.clear_refs(visited_objects, this_ptr);

		// clear self-refs from environment
		m_as_environment.clear_refs(visited_objects, this_ptr);
	}

	sprite_instance* sprite_instance::attach_movie(const tu_string& id, const tu_string name, int depth)
	{

		// check the import.
		character_def* res = find_exported_resource(id);
		if (res == NULL)
		{
			IF_VERBOSE_ACTION(myprintf("import error: resource '%s' is not exported\n", id.c_str()));
			return NULL;
		}

		sprite_definition* sdef = cast_to<sprite_definition>(res);
		if (sdef == NULL)
		{
			return NULL;
		}

		sprite_instance* sprite = new sprite_instance(sdef, get_root(), this, -1);
		sprite->set_name(name);

		m_display_list.add_display_object( sprite, depth, true, m_color_transform, matrix(), 0.0f, 0, 0); 

//		sprite->advance(1);	// force advance
		return sprite;
	}

	void sprite_instance::dump(int tabs)
	{
		tu_string tab;
		for (int i = 0; i < tabs; i++)
		{
			tab += ' ';
		}
		myprintf("%s*** movieclip %p ***\n", tab.c_str(), this);
		as_object::dump(tabs + 2);
		m_display_list.dump(tabs + 2);
	}

	character_def* sprite_instance::find_exported_resource(const tu_string& symbol)
	{
		movie_definition_sub*	def = cast_to<movie_def_impl>(get_movie_definition());
		if (def)
		{
			character_def* res = def->get_exported_resource(symbol);
			if (res)
			{
				return res;
			}
		}

		// try parent 
		character* parent = get_parent();
		if (parent)
		{
			return parent->find_exported_resource(symbol);
		}

		IF_VERBOSE_ACTION(myprintf("can't find exported resource '%s'\n", symbol.c_str()));
		return NULL;
	}

	void sprite_instance::local_to_global(as_object* obj)
	{
		as_value x;
		obj->get_member("x", &x);
		as_value y;
		obj->get_member("y", &y);
		if (x.is_number() == false || y.is_number() == false)
		{
			return;
		}

		point pt(x.to_float(), y.to_float());
		pt.pixels_to_twips();
		point result;
		matrix m;
		get_world_matrix(&m);
		m.transform(&result, pt);
		result.twips_to_pixels();

		obj->set_member("x", result.m_x);
		obj->set_member("y", result.m_y);
	}

	void sprite_instance::global_to_local(as_object* obj)
	{
		as_value x;
		obj->get_member("x", &x);
		as_value y;
		obj->get_member("y", &y);
		if (x.is_number() == false || y.is_number() == false)
		{
			return;
		}

		point pt(x.to_float(), y.to_float());
		pt.pixels_to_twips();
		point result;
		matrix m, gm;
		get_world_matrix(&gm);
		m.set_inverse(gm);
		m.transform(&result, pt);
		result.twips_to_pixels();

		obj->set_member("x", result.m_x);
		obj->set_member("y", result.m_y);
	}

	canvas* sprite_instance::get_canvas()
	{
		character* ch = m_display_list.get_character_by_name(get_canvas_name());
		if (ch == NULL)
		{
			canvas* canvas_def = new canvas();
			ch = canvas_def->create_character_instance(this, -1);
			ch->set_name(get_canvas_name());
			matrix identity;
			m_display_list.add_display_object(ch, get_highest_depth(), true, m_color_transform, identity, 0.0f, 0, 0); 
		}
		return cast_to<canvas>(ch->get_character_def());
	}

	void sprite_instance::enumerate(as_environment* env)
	{
		assert(env);

		// enumerate variables
		as_object::enumerate(env);

		// enumerate characters
		for (int i = 0, n = m_display_list.size(); i<n; i++)
		{
			character* ch = m_display_list.get_character(i);
			if (ch)
			{
				m_display_list.set_instance_name(ch);
				env->push(as_value(ch->get_name()));
			}
		}
	}

	void sprite_instance::enumerate(as_array* a)
	{
		assert(a);

		// enumerate variables
		as_object::enumerate(a);

		// enumerate characters
		for (int i = 0, n = m_display_list.size(); i<n; i++)
		{
			character* ch = m_display_list.get_character(i);
			if (ch)
			{
				m_display_list.set_instance_name(ch);
				a->push(as_value(ch->get_name()));
			}
		}
	}

	bool sprite_instance::is_enabled() const
	{
		if (m_enabled)
		{
			// check parent
			character* parent = get_parent();
			if (parent)
			{
				return parent->is_enabled();
			}
			return true;
		}
		return false;
	}

	rgba* sprite_instance::get_rgba()
	{
		// TODO
		return NULL;
	}

	void sprite_instance::set_rgba(const rgba& color)
	{
		for (int i = 0, n = m_display_list.size(); i < n; i++)
		{
			character* ch = m_display_list.get_character(i);
			if (ch != NULL)
			{
				ch->set_rgba(color);
			}
		}	
	}

	// not SAFE!, for trace(clip)
	const tu_string&	sprite_instance::to_tu_string()
	{
		static tu_string buf;
		character* ch = this;
		buf.clear();
		while (ch && ch->get_parent())
		{
			buf = tu_string('.') + ch->get_name() + buf;
			ch = ch->get_parent();
		}
		buf = tu_string("_level0") + buf;
		return buf;
	}

	// AS3
	void sprite_instance::add_script(int frame, as_function* func)
	// frame is 0-based
	{
		if (frame >= 0 && frame < m_def->get_frame_count())
		{
			if (m_script == NULL)
			{
				m_script = new hash<int, smart_ptr<as_function> >;
			}
			m_script->set(frame, func);
		}
	}

	// AS3
	// fixme: optimize
	void sprite_instance::add_event_listener(const tu_string& eventname, as_function* handler)
	{
		if (handler && eventname.size() > 0)
		{
			// todo: optimize
			if (eventname == "onEnterFrame")
			{
				as_object::set_member("onEnterFrame", handler);
			}
			else
			if (eventname == "onPress")
			{
				// hack..must be click ==> onClick
				as_object::set_member("onPress", handler);
			}
			else
			if (eventname == "onRelease")
			{
				// hack..must be click ==> onClick
				as_object::set_member("onRelease", handler);
			}
			else
			if (eventname == "onMouseMove")
			{
				// hack..must be click ==> onClick
				as_object::set_member("onMouseMove", handler);
			}
			else
			{
				// user defined events
				if (m_as3listener == NULL)
				{
					m_as3listener = new string_hash< weak_ptr<as_function> >();
				}
				m_as3listener->set(eventname, handler);
			}
		}
	}

	// AS3
	void sprite_instance::remove_event_listener(const tu_string& eventname, as_function* handler)
	{
		if (handler && eventname.size() > 0)
		{
			// todo: optimize
			if (eventname == "onEnterFrame")
			{
				as_object::set_member("onEnterFrame", as_value());
			}
			else
			if (eventname == "onPress")
			{
				// hack..must be click ==> onClick
				as_object::set_member("onPress", as_value());
			}
			else
			if (eventname == "onRelease")
			{
				// hack..must be click ==> onClick
				as_object::set_member("onRelease", as_value());
			}
			else
			if (eventname == "onMouseMove")
			{
				// hack..must be click ==> onClick
				as_object::set_member("onMouseMove", as_value());
			}
			else
			{
				// user defined events
				if (m_as3listener)
				{
					m_as3listener->set(eventname, NULL);
				}
			}
		}

	}

	// AS3
	void sprite_instance::sprite_dispatch_event(const fn_call& fn)
	{
		if (m_as3listener && fn.nargs > 0)
		{
			as_event* event = cast_to<as_event>(fn.arg(0).to_object());
			if (event)
			{
				as_value eventname;
				event->get_member("type", &eventname);

				weak_ptr<as_function> handler;
				if (m_as3listener->get(eventname.to_tu_string(), &handler) && handler != NULL)
				{
					bakeinflash::call_method(handler.get(), fn.env, fn.this_ptr,	fn.nargs, fn.env->get_top_index());
				}
			}
		}

	}

	as_function* sprite_instance::get_class_constructor(character* ch)
	{
		movie_def_impl* def = cast_to<movie_def_impl>(m_def);
		if (def)
		{
			return def->get_class_constructor(ch);
		}

		// check parent
		character* parent = get_parent();
		if (parent)
		{
			return parent->get_class_constructor(ch);
		}
		return NULL;
	}

	// render movieclip to bitmap and form pdef
	void sprite_instance::print()
	{
#if TU_CONFIG_LINK_TO_LIBHPDF == 1

		// save
		bool visible = get_visible();
		set_visible(true);

		display();

		// get viewport size
		GLint vp[4]; 
		glGetIntegerv(GL_VIEWPORT, vp); 
		const int& vieww = vp[2];
		const int& viewh = vp[3];

		glReadBuffer(GL_BACK);

		int vBufferSize = vieww * viewh * 4;
		Uint8* buf = (Uint8*) malloc(vBufferSize);
		glReadPixels(0, 0, vieww, viewh, GL_RGBA, GL_UNSIGNED_BYTE, buf);

		// flip
		Uint8* buf2 = (Uint8*) malloc(vBufferSize);
		int view_pitch = vieww * 4;	// rgba
		uint8_t* src = buf + view_pitch * viewh;
		uint8_t* dst = buf2;
		for (int i = 0; i < viewh; i++)
		{
			src -= view_pitch;
			memcpy(dst, src, view_pitch);
			dst += view_pitch;
		}

		rect bound;
		get_bound(&bound);
		bound.twips_to_pixels();
		int x = (int) bound.m_x_min;
		int y = (int) bound.m_y_min;
		int w = (int) bound.m_x_max - x;
		int h = (int) bound.m_y_max - y;

		// cropp 
		int deltaw = vieww - (x + w);
		if (deltaw < 0)
		{
			w += deltaw;
		}
		int deltah = viewh - (y + h);
		if (deltah < 0)
		{
			h += deltah;
		}

		if (x >= vieww || y >= viewh || w <= 0 || h <= 0)
		{
			// outside of screen
			return;
		}

	//	png_helper::write_rgba("c:\\bakeinflash\\xxx.png", buf2, vieww, viewh, 4);

		// rgba ==> rgb
		Uint8* buf3 = (Uint8*) malloc(w * h * 3);
		dst = buf3;
		for (int i = 0; i < h; i++)
		{
			src = buf2 + (y + i) * view_pitch + x * 4 ;
			for (int j = 0; j < w; j++)
			{
				float a = src[3] / 255.0f;
				*dst++ =	(Uint8) (*src++ * a);	// R
				*dst++ =	(Uint8) (*src++ * a);	// G
				*dst++ =	(Uint8) (*src++ * a);	// B
				src++;
			}
		}

//		png_helper::write_rgba("c:\\bakeinflash\\xxx.png", buf3, w, h, 3);
		char finame[1024];
#ifdef WIN32
		char tmp_folder[1024];
		GetTempPath(1024, tmp_folder);
		snprintf(finame, sizeof(finame), "%smc.pdf", tmp_folder);
#else
#endif

		create_pdf(buf3, w, h, finame);
		print_image(buf3, w, h, finame);

		free(buf);
		free(buf2);
		free(buf3);

		// restore
		set_visible(visible);

#endif
	}

	void sprite_instance::scroll_start(float speed)
	// prepare m_scroll_way
	{
		float box_width = m_scrollbox_bounds.width();
		float box_height = m_scrollbox_bounds.height();

		// bounds in the _parent coords
		rect this_bounds;
		get_bound(&this_bounds);

		// _parent local coords ==> global coords
		matrix m;
		get_parent()->get_world_matrix(&m);
		m.transform(&this_bounds);
		this_bounds.twips_to_pixels();

		float x0 = this_bounds.m_x_min - m_scrollbox_bounds.m_x_min;
		float y0 = this_bounds.m_y_min - m_scrollbox_bounds.m_y_min;
		int xpage = (int) fround(x0 / box_width);
		int ypage = (int) fround(y0 / box_height);
		int total_xpages = (int) ceil(this_bounds.width() / box_width);
		int total_ypages = (int) ceil(this_bounds.height() / box_height);
	//	printf("speed=%f, x0=%f, page=%d, pages=%d,box_left=%f,box_width=%f,boundwidth=%f\n", speed, x0, page,total_pages,box_left,box_width,bound.width());

		float way = 0;
		if (m_scroll_mode & 1)	// vert ?
		{
			if (speed < -0.5)		// next page.. move to right
			{
				ypage = int(y0 / box_height) - 1;
			}
			else
			if (speed > 0.5)		// prev page.. move to left
			{
				ypage = (int) floor(y0 / box_height) + 1;
			}

			// sanity
			if (ypage > 0)
			{
				ypage = 0;
			}
			if (abs(ypage) >= total_ypages)
			{
				ypage = - (total_ypages - 1);
			}

			// goto this page
			way = this_bounds.m_y_min - m_scrollbox_bounds.m_y_min - ypage * box_height;
		}
		else
		{
			if (speed < -0.5)		// next page.. move to right
			{
				xpage = int(x0 / box_width) - 1;
			}
			else
			if (speed > 0.5)		// prev page.. move to left
			{
				xpage = (int) floor(x0 / box_width) + 1;
			}

			// sanity
			if (xpage > 0)
			{
				xpage = 0;
			}
			if (abs(xpage) >= total_xpages)
			{
				xpage = - (total_xpages - 1);
			}

			// goto the page
			if (this_bounds.width() >= m_scrollbox_bounds.width())
			{
				if (m_scroll_mode & 2)	// nopages ?
				{
					// no pages
					if (this_bounds.m_x_max < m_scrollbox_bounds.m_x_max)
					{
						way = this_bounds.m_x_max - m_scrollbox_bounds.m_x_max;
					}
					else
					if (this_bounds.m_x_min > m_scrollbox_bounds.m_x_min)
					{
						way = this_bounds.m_x_min - m_scrollbox_bounds.m_x_min;
					}
					else
					{
						way = -speed * 12;
						if (speed > 0 && (this_bounds.m_x_min - way > m_scrollbox_bounds.m_x_min))
						{
							way = -(m_scrollbox_bounds.m_x_min - this_bounds.m_x_min);
						}

						if (speed < 0 && (this_bounds.m_x_max - way < m_scrollbox_bounds.m_x_max))
						{
							way = -(m_scrollbox_bounds.m_x_max - this_bounds.m_x_max);
						}
					}

				}
				else
				{
					// with pages
					way = this_bounds.m_x_min - m_scrollbox_bounds.m_x_min - xpage * box_width;
				}
			}
			else
			{
				assert(xpage == 0);
				if (this_bounds.m_x_min < m_scrollbox_bounds.m_x_min)
				{
					way = this_bounds.m_x_min - m_scrollbox_bounds.m_x_min;
				}
				else
				if (this_bounds.m_x_max > m_scrollbox_bounds.m_x_max)
				{
					way = this_bounds.m_x_max - m_scrollbox_bounds.m_x_max;
				}
				else
				{
					way = -speed * 12;
					if (speed > 0 && (this_bounds.m_x_max - way > m_scrollbox_bounds.m_x_max))
					{
						way = -(m_scrollbox_bounds.m_x_max - this_bounds.m_x_max);
					}

					if (speed < 0 && (this_bounds.m_x_min - way < m_scrollbox_bounds.m_x_min))
					{
						way = -(m_scrollbox_bounds.m_x_min - this_bounds.m_x_min);
					}
				}
			}
		}

		// calc paths
		float w = way / 8.0f;
		float v = 0.95f;
		m_scroll_way.resize(35);
		for (int i = 0, n = m_scroll_way.size(); i < n; i++)
		{
			m_scroll_way[n - i - 1] = w;
			w *= v;
			v /= 1.02f;
		}

		float s = 0;
		for (int i = 0, n = m_scroll_way.size(); i < n; i++)
		{
			s += m_scroll_way[i];
		}
		m_scroll_way[m_scroll_way.size() - 1] += way - s;	// adjust
	//	for (int i = 0; i < m_scroll_way.size(); i++) printf("%6.6f ", m_scroll_way[i]); printf("\n");
	}

	void sprite_instance::scroll()
	{
		// bounds in the _parent coords
		rect bound;
		get_bound(&bound);

		// _parent local coords ==> global coords
		matrix m;
		get_parent()->get_world_matrix(&m);
		m.transform(&bound);

		bound.twips_to_pixels();

		const matrix& parent = get_parent()->get_matrix();
		float	scale = m_scroll_mode & 1 ? parent.get_y_scale() : parent.get_x_scale();
		int index = m_scroll_mode & 1;

		m_matrix.m_[index][2] -= PIXELS_TO_TWIPS(m_scroll_way.back() / scale);			// pixels
		m_scroll_way.pop_back();
	}

	
	image::rgba* sprite_instance::render()
	// render movieclip to bitmap
	{
		int vx, vy, vw, vh;
		get_root()->get_display_viewport(&vx, &vy, &vw, &vh);

		// bounds in the _parent coords
		rect bound;
		get_bound(&bound);

		// _parent local coords ==> global coords
		character* parent = get_parent();
		matrix m;
		if (parent)
		{
			parent->get_world_matrix(&m);
			m.transform(&bound);
		}
		bound.twips_to_pixels();

		int x = (int) ceil(bound.m_x_min * s_scale);
		int y = (int) ceil(bound.m_y_min * s_scale);
		int w = (int) floor(bound.width() * s_scale);
		int h = (int) floor(bound.height() * s_scale);

		if (1) //x >= 0 && (x + w <= vw) && (vh - h - y) >= 0 && (vh - y <= vh) && w > 0 && h > 0)
		{
			// save state
			bool visible = get_visible();
			set_visible(true);

			render::clear();
			display();


			int size = w * h * 4;
			Uint8* buf = (Uint8*) malloc(size);

			render::read_buffer(vx + x, vy + vh - h - y, w, h, buf);	// window coords
		//	png_helper::write_rgba("c:\\bakeinflash\\xxx.png", buf, w, h, 4);

			// flip and save
			image::rgba* im = new image::rgba(w, h);
			int pitch  = w * 4;	// rgba
			uint8_t* src = buf + pitch * h;
			uint8_t* dst = im->m_data;
			for (int i = 0; i < h; i++)
			{
				src -= pitch;
				memcpy(dst, src, pitch);
				dst += pitch;
			}
			// png_helper::write_rgba("c:\\bakeinflash\\xxx.png", im->m_data, w, h, 4);
			free(buf);

			// restore state
			set_visible(visible);
			return im;
		}
		return NULL;
	}

	void sprite_instance::attach_bitmapdata(as_bitmapdata* bd, int depth)
	{
		if (bd && bd->m_image)
		{
			int w = bd->m_image->m_width;
			int h = bd->m_image->m_height;
			bitmap_info* bi = render::create_bitmap_info(bd->m_image);
			bi->keep_alive_source_image(true);

			movie_definition* rdef = get_root()->get_movie_definition();
			assert(rdef);
			bitmap_character_def*	def = new bitmap_character_def(rdef, bi);

			character* ch = def->create_character_instance(this, 0);
			m_display_list.add_display_object(ch, depth, true, m_color_transform, matrix(), 0.0f, 0, 0); 
		}
	}

}
