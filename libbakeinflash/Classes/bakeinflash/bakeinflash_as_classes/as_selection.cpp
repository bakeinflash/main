// as_color.cpp	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "bakeinflash/bakeinflash_as_classes/as_selection.h"
#include "bakeinflash/bakeinflash_character.h"
#include "bakeinflash/bakeinflash_root.h"

namespace bakeinflash
{

	void	as_selection_setfocus(const fn_call& fn)
	{
		as_selection* obj = cast_to<as_selection>(fn.this_ptr);
		if (obj)
		{
			if (fn.nargs > 0)
			{
				character* target = cast_to<character>(fn.env->find_target(fn.arg(0)));
				if (target)
				{
					target->on_event(event_id::SETFOCUS);
					fn.result->set_bool(true);
				}
				else
				{
					// kill focus
					character* ch = get_root()->get_active_entity();
					if (ch)
					{
						ch->on_event(event_id::KILLFOCUS);
					}
				}
			}
		}
		fn.result->set_bool(false);
	}

	as_object* selection_init()
	{
		as_object* sel = new as_selection();

		// methods
		sel->builtin_member("setFocus", as_selection_setfocus);

// TODO
//		sel->set_member("getFocus", as_selection_setfocus);
//		sel->set_member("addListener", as_selection_setfocus);
//		sel->set_member("removeListener", as_selection_setfocus);
		// ...
		return sel;
	}

	as_selection::as_selection()
	{
	}
};
