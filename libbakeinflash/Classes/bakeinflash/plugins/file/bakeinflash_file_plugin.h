// file.h	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Very simple and convenient file I/O plugin for the bakeinflash SWF player library.

#ifndef bakeinflash_FILE_PLUGIN_H
#define bakeinflash_FILE_PLUGIN_H

#include "base/tu_file.h"
#include "bakeinflash/bakeinflash_action.h"	// for as_object
#include "bakeinflash/bakeinflash_tcp.h"

namespace bakeinflash
{

	struct file : public as_object
	{

		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_PLUGIN_FILE };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		file();
		virtual ~file();
		virtual void	advance(float delta_time);
		void handle_events();

		int open(const tu_string& path, const tu_string& mode);
		void download(const tu_string& url, const tu_string& path);
		tu_string month_to_int(const tu_string& month);
		tu_string get_datetime(const tu_string& file_name);

		tu_file* m_file;
		tu_string m_filename;

		tcp m_http;
		net_status m_status;
		Uint32 m_start;
		//tu_string m_url;
		tu_string m_last_modified;
		int m_size;
		tu_string m_target_filename;		// download target

	};

	void	as_file_plugin_ctor(const fn_call& fn);


}

#endif // bakeinflash_FILE_PLUGIN_H
