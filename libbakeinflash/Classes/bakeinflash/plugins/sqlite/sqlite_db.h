// sqlite_db.cpp	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2009

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// sqlite plugin implementation for the bakeinflash SWF player library.

#ifndef BAKEINFLASH_SQLITE_DB_H
#define BAKEINFLASH_SQLITE_DB_H

#include "../sql_db.h"
#include "sqlite_table.h"

struct sqlite3;

using namespace bakeinflash;

namespace sqlite_plugin
{
	void	as_sqlite_ctor(const fn_call& fn);

	struct sqlite_db : public sql_db
	{

		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_PLUGIN_SQLITE_DB };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return sql_db::is(class_id);
		}

		 sqlite_db();
		 ~sqlite_db();

		 bool connect(const char* dbfile, bool read_only, const char* vfs);
		 void disconnect();
		 sql_table* open(const char* sql);
		 int run(const char *sql);
		 void set_autocommit(bool autocommit);
		 void commit();
		 void create_function(const char* name, as_function* func);

		bool m_trace;
		smart_ptr<sqlite_table> m_result;

	private:

		bool runsql(const char* sql);
		sqlite3* m_db;
		bool m_autocommit;
		string_hash< smart_ptr<as_object_interface> > m_callback_context;
	};

	struct func_context : public as_object_interface
	{
		func_context(sqlite_db* this_ptr, as_function* func) :
			m_this_ptr(this_ptr),
			m_func(func)
		{
		}
		
		virtual bool is(int class_id) const
		{
			return false;
		}

		weak_ptr<sqlite_db> m_this_ptr;
		weak_ptr<as_function> m_func;
	};


}

#endif // bakeinflash_SQLITE_DB_H
