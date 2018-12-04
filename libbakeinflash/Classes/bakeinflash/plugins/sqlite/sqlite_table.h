// sqlite_table.h	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2009

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// sqlite table implementation for the bakeinflash SWF player library.

#ifndef BAKEINFLASH_SQLITE_TABLE_H
#define BAKEINFLASH_SQLITE_TABLE_H

struct sqlite3_stmt;

#include "bakeinflash/bakeinflash_action.h"	// for as_object
#include "../sql_table.h"

using namespace bakeinflash;

namespace sqlite_plugin
{

	struct sqlite_table: public sql_table
	{

		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_PLUGIN_SQLITE_TABLE };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		 sqlite_table();
		 ~sqlite_table();

		 virtual bool	get_member(const tu_string& name, as_value* val);

		 int size() const;
		 bool prev();
		 bool next();
		 void first();
		 int fld_count();
		 bool goto_record(int index);
		 const char* get_field_title(int n);
		void retrieve_data(sqlite3_stmt* stmt);
		 int get_recno() const;


	private:
		int m_index;
		array<smart_ptr<as_object> > m_data;	// [columns][rows]
		array<tu_string> m_title;
	};

}

#endif // bakeinflash_SQLITE_TABLE_H
