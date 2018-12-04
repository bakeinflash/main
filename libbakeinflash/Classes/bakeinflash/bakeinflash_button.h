// bakeinflash_button.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// SWF buttons.  Mouse-sensitive update/display, actions, etc.


#ifndef BAKEINFLASH_BUTTON_H
#define BAKEINFLASH_BUTTON_H


#include "bakeinflash/bakeinflash_impl.h"
#include "bakeinflash/bakeinflash_sound.h"


namespace bakeinflash
{
	//
	// button characters
	//
	enum mouse_state
	{
		MOUSE_UP,
		MOUSE_DOWN,
		MOUSE_OVER
	};

	struct button_record
	{
		bool	m_has_blend_mode;
		bool	m_has_filter_list;
		bool	m_hit_test;
		bool	m_down;
		bool	m_over;
		bool	m_up;
		int	m_character_id;
		character_def*	m_character_def;
		int	m_button_layer;
		matrix	m_button_matrix;
		cxform	m_button_cxform;
		int	m_blend_mode;

		bool	read(stream* in, int tag_type, movie_definition_sub* m);
	};
	

	struct button_action
	{
		enum condition
		{
			IDLE_TO_OVER_UP = 1 << 0,
			OVER_UP_TO_IDLE = 1 << 1,
			OVER_UP_TO_OVER_DOWN = 1 << 2,
			OVER_DOWN_TO_OVER_UP = 1 << 3,
			OVER_DOWN_TO_OUT_DOWN = 1 << 4,
			OUT_DOWN_TO_OVER_DOWN = 1 << 5,
			OUT_DOWN_TO_IDLE = 1 << 6,
			IDLE_TO_OVER_DOWN = 1 << 7,
			OVER_DOWN_TO_IDLE = 1 << 8,
		};
		int	m_conditions;
		array<action_buffer*>	m_actions;

		void	read(stream* in, int tag_type);
		void	clear();
	};


	struct button_character_definition : public character_def
	{
		struct sound_info
		{
			void read(stream* in);

			bool m_no_multiple;
			bool m_stop_playback;
			bool m_has_envelope;
			bool m_has_loops;
			bool m_has_out_point;
			bool m_has_in_point;
			Uint32 m_in_point;
			Uint32 m_out_point;
			Uint16 m_loop_count;
			array<sound_envelope> m_envelopes;
		};

		struct button_sound_info
		{
			Uint16 m_sound_id;
			sound_sample*	m_sam;
			sound_info m_sound_style;
		};

		struct button_sound_def
		{
			void	read(stream* in, movie_definition_sub* m);
			button_sound_info m_button_sounds[4];
		};


		bool m_menu;
		array<button_record>	m_button_records;
		array<button_action>	m_button_actions;
		button_sound_def*	m_sound;

		button_character_definition();
		virtual ~button_character_definition();
		character*	create_character_instance(character* parent, int id);
		void	read(stream* in, int tag_type, movie_definition_sub* m);
	};

	struct button_character_instance : public character
	{
		smart_ptr<button_character_definition>	m_def;
		array<smart_ptr<character> >	m_record_character;

		enum mouse_flags
		{
			IDLE = 0,
			FLAG_OVER = 1,
			FLAG_DOWN = 2,
			OVER_DOWN = FLAG_OVER|FLAG_DOWN,

			// aliases
			OVER_UP = FLAG_OVER,
			OUT_DOWN = FLAG_DOWN
		};

		int	m_last_mouse_flags, m_mouse_flags;		
		enum e_mouse_state
		{
			UP = 0,
			DOWN,
			OVER
		};
		e_mouse_state m_mouse_state;
		bool m_enabled;

		button_character_instance(button_character_definition* def, character* parent, int id);
		virtual ~button_character_instance();

		virtual	void	execute_frame_tags(int frame, bool state_only);
		virtual bool has_keypress_event();
		character*	get_root_movie() const;
		virtual void	advance_actions();
		void	display();
		inline int	transition(int a, int b) const;

		virtual bool get_topmost_mouse_entity( character * &te, float x, float y);
		virtual bool	on_event(const event_id& id);
		virtual void	get_mouse_state(int* x, int* y, int* buttons);

		//
		// ActionScript overrides
		//

		virtual bool	set_member(const tu_string& name, const as_value& val);
		virtual bool	get_member(const tu_string& name, as_value* val);
		virtual void	get_bound(rect* bound);
		virtual bool can_handle_mouse_event();
		virtual	bool is_enabled() const;
		virtual character_def* get_character_def();

		// AS3
		void add_event_listener(const tu_string& eventname, as_function* handler);
		string_hash< weak_ptr<as_function> >* m_as3listener;	// <event, handler>

	};

};	// end namespace bakeinflash


#endif // bakeinflash_BUTTON_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
