// as_urlvariables.h	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2010

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef BAKEINFLASH_AS_urlvariables_H
#define BAKEINFLASH_AS_urlvariables_H

#include "base/tu_config.h"
#include "bakeinflash/bakeinflash_action.h"	// for as_object
#include "bakeinflash/bakeinflash_character.h"
#include "bakeinflash/bakeinflash_tcp.h"

namespace bakeinflash
{

	void	as_urlvariables_ctor(const fn_call& fn);
	void	as_urlrequest_ctor(const fn_call& fn);

	struct as_urlvariables : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_URLVARIABLES };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		as_urlvariables(const tu_string& source);
		virtual ~as_urlvariables() {}

		void decode(const tu_string& str);
	};

	struct as_urlrequest : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_URLREQUEST };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		as_urlrequest(const tu_string& url);
		virtual ~as_urlrequest() {}

		const tu_string& get_url() const { return m_url; }

	private:

		tu_string m_url;

	};


}	// end namespace bakeinflash

#endif // bakeinflash_AS_urlvariables_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
