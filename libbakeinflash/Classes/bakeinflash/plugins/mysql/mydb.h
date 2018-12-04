// mydb.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// MYSQL plugin implementation for the bakeinflash SWF player library.

#ifndef bakeinflash_MYDB_H
#define bakeinflash_MYDB_H

#if TU_CONFIG_LINK_TO_MYSQL == 1

#include <mysql/mysql.h>
#include "../sql_db.h"
#include "../sql_table.h"
#include "mytable.h"

namespace bakeinflash
{
	void	as_mysql_ctor(const fn_call& fn);

	struct mydb : public sql_db
	{

		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_PLUGIN_MYDB };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return sql_db::is(class_id);
		}

		mydb();
		~mydb();

		bool connect(const char* host, const char* dbname, const char* user,
			const char* pwd, const char* socket);
		void disconnect();
		sql_table* open(const char* sql);
		int run(const char *sql);
		void set_autocommit(bool autocommit);
		void commit();

		bool m_trace;

	private:

		bool runsql(const char* sql);
		MYSQL* m_db;
	};

}

#endif

#endif // bakeinflash_MYDB_H
