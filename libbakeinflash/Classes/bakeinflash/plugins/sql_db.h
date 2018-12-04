// sqlite_db.cpp	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2009

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// interface module to mysql & sqlite plugins

#ifndef BAKEINFLASH_SQL_DB_H
#define BAKEINFLASH_SQL_DB_H

#include "sql_table.h"

using namespace bakeinflash;

struct sql_db : public as_object
{
	// Unique id of a bakeinflash resource
	enum { m_class_id = AS_PLUGIN_SQL_DB };
	virtual bool is(int class_id) const
	{
		if (m_class_id == class_id) return true;
		else return as_object::is(class_id);
	}

	sql_db() {}

	virtual sql_table* open(const char* sql) = 0;
	virtual int run(const char *sql) = 0;
};

#endif // bakeinflash_SQL_DB_H
