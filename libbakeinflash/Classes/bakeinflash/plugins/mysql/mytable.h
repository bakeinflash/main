// mytable.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// MYSQL table implementation for the bakeinflash SWF player library.

#ifndef bakeinflash_MYTABLE_H
#define bakeinflash_MYTABLE_H

#if TU_CONFIG_LINK_TO_MYSQL == 1

#include <mysql/mysql.h>
#include "bakeinflash/bakeinflash_action.h"	// for as_object
#include "../sql_table.h"

namespace bakeinflash
{

	struct mytable: public sql_table
	{

		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_PLUGIN_MYTABLE };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		mytable();
		~mytable();

		virtual bool	get_member(const tu_string& name, as_value* val);

		int size() const;
		bool prev();
		bool next();
		void first();
		int fld_count();
		bool goto_record(int index);
		const char* get_field_title(int n);
		void retrieve_data(MYSQL_RES* result);
		int get_recno() const;


	private:
		int m_index;
		array<smart_ptr<as_object> > m_data;	// [columns][rows]
		array<tu_string> m_title;
	};

}

#endif

#endif // bakeinflash_MYTABLE_H
