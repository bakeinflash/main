#ifndef FUTUREDASH_APP_H
#define FUTUREDASH_APP_H

#include "bakeinflash/bakeinflash_root.h"
#include "validator.h"

namespace bakeinflash
{

	void	as_validator_ctor(const fn_call& fn);

	struct as_validator : public as_object
	{

		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_PLUGIN_VALIDATOR };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}


		as_validator(const tu_string& model, const tu_string& port);
		virtual ~as_validator();

		virtual void advance(float delta_time);
		bool open();

		bool m_enable;
		smart_ptr<validator> m_validator;
		int m_state;		// current state

	};
}

#endif
