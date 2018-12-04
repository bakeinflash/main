//
//
//
#include "base/tu_timer.h"
#include "as_validator.h"
#include "validator_pyramid.h"
#include "validator_cashcode.h"

namespace bakeinflash
{

	void	as_validator_ctor(const fn_call& fn)
		// Constructor for ActionScript class
	{
		if (fn.nargs >= 2)
		{
			as_validator* obj = new as_validator(fn.arg(0).to_tu_string(), fn.arg(1).to_tu_string());
			fn.result->set_as_object(obj);
		}
	}

	void	as_validator_open(const fn_call& fn)
	{
		as_validator* obj = cast_to<as_validator>(fn.this_ptr);
		if (obj)
		{
			bool rc = obj->open();
			fn.result->set_bool(rc);
		}
	}

	void	as_validator_enabled_getter(const fn_call& fn)
	{
		as_validator* obj = cast_to<as_validator>(fn.this_ptr);
		if (obj)
		{
			fn.result->set_bool(obj->m_enable);
		}
	}

	void	as_validator_enabled_setter(const fn_call& fn)
	{
		as_validator* obj = cast_to<as_validator>(fn.this_ptr);
		if (obj)
		{
			obj->m_enable = fn.arg(0).to_bool();
		}
	}

	as_validator::as_validator(const tu_string& model, const tu_string& port) :
		m_enable(false),
		m_state(0)
	{
		builtin_member("open", as_validator_open);
		builtin_member("enabled", as_value(as_validator_enabled_getter, as_validator_enabled_setter));

		if (model == "cashcode")
		{
			m_validator = new validator_cashcode(port);
		}
		else
		if (model == "apex")
		{
			m_validator = new validator_pyramid(port);
		}
	}

	as_validator::~as_validator()
	{
	}

	bool as_validator::open()
	{
		if (m_validator != NULL && m_validator->init())
		{
			get_root()->add_listener(this);
			return true;
		}
		return false;
	}

	//	int as_validator::advance()
	void as_validator::advance(float delta_time)
	{
		if (m_validator != NULL)
		{
			int money = m_validator->advance();
			if (money > 0)
			{
				as_value func;
				if (get_member("onAccepted", &func))
				{
					as_environment env;
					env.push(money);
					call_method(func, &env, this, 1, env.get_top_index());
				}
			}

			int state = m_validator->get_state();
			if (state != m_state)
			{
				m_state = state;
				as_value func;
				if (get_member("onStatus", &func))
				{
					tu_string msg;
					m_validator->get_state_name(m_state, &msg);

					as_environment env;
					env.push(msg.c_str());
					call_method(func, &env, this, 1, env.get_top_index());
				}
			}

		}
	}

}
