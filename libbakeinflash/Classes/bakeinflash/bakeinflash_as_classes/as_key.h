// as_key.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script Key implementation code for the bakeinflash SWF player library.


#ifndef BAKEINFLASH_AS_KEY_H
#define BAKEINFLASH_AS_KEY_H

#include "bakeinflash/bakeinflash_action.h"	// for as_object
#include "bakeinflash/bakeinflash_root.h"	// for listener

namespace bakeinflash
{
	void	key_add_listener(const fn_call& fn);
	// Add a listener (first arg is object reference) to our list.
	// Listeners will have "onKeyDown" and "onKeyUp" methods
	// called on them when a key changes state.

	void	key_get_ascii(const fn_call& fn);
	// Return the ascii value of the last key pressed.

	void	key_get_code(const fn_call& fn);
	// Returns the keycode of the last key pressed.

	void	key_is_down(const fn_call& fn);
	// Return true if the specified (first arg keycode) key is pressed.

	void	key_is_toggled(const fn_call& fn);
	// Given the keycode of NUM_LOCK or CAPSLOCK, returns true if
	// the associated state is on.

	void	key_remove_listener(const fn_call& fn);
	// Remove a previously-added listener.

	struct as_key : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_KEY };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		Uint8	m_keymap[key::KEYCOUNT / 8 + 1];	// bit-array
		int	m_last_key_pressed;
		listener m_listeners;
		Uint16 m_last_utf16_key_pressed;

		as_key();

		bool	is_key_down(int code);
		void	set_key_down(int code, Uint16 utf16char);
		void	set_key_up(int code, Uint16 utf16char);
		int	get_last_key_pressed() const;
		int	get_last_utf16_key_pressed() const;
	};

	// creates 'Key' object
	as_key* key_init();

}	// namespace bakeinflash

#endif	// bakeinflash_AS_KEY_H
