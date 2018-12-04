// as_netconnection.h	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com> 2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef BAKEINFLASH_NETCONNECTION_H
#define BAKEINFLASH_NETCONNECTION_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "bakeinflash/bakeinflash_action.h"	// for as_object

namespace bakeinflash
{

	void	as_global_netconnection_ctor(const fn_call& fn);

	struct as_netconnection : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_NETCONNECTION };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		as_netconnection();
	};

} // end of bakeinflash namespace

#endif

