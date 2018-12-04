// sysinfo.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// bakeinflash plugin, gets dir entity

#ifndef BAKEINFLASH_SYSINFO_PLUGIN_H
#define BAKEINFLASH_SYSINFO_PLUGIN_H

#include "bakeinflash/bakeinflash_object.h"
#include "bakeinflash/bakeinflash_action.h"	// for as_object

namespace bakeinflash
{

	void	as_sysinfo_ctor(const fn_call& fn);

	struct sysinfo : public as_object
	{                                      
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_PLUGIN_SYSINFO };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		sysinfo();

		void get_dir(as_object* info, const tu_string& path, bool recursive);
		bool get_hdd_serno(tu_string* sn, const char* dev);
		int get_freemem();

	};
}

#endif	// BAKEINFLASH_SYSINFO_PLUGIN_H

