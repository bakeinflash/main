// mydb.h	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Very simple and convenient MYSQL plugin
// for the bakeinflash SWF player library.

#include "compatibility_include.h"

#if TU_CONFIG_LINK_TO_MYSQL == 1

#include "mydb.h"
//#include "bakeinflash/bakeinflash_mutex.h"
#include "bakeinflash/bakeinflash_log.h"

#define ulong Uint32

namespace bakeinflash
{
//	extern tu_mutex s_mysql_plugin_mutex;

	void	as_mysql_ctor(const fn_call& fn)
	{
		fn.result->set_as_object(new mydb());
	}

	void	mydb_connect(const fn_call& fn)
	//  Closes a previously opened connection & create new connection to db
	{
		mydb* db = cast_to<mydb>(fn.this_ptr);
		if (db)
		{
			if (fn.nargs < 5)
			{
				myprintf("Db.Connect() needs host, dbname, user, pwd, socket args\n");
				return;
			}

			if (db)
			{
				fn.result->set_bool(db->connect(fn.arg(0).to_string(),	// host
					fn.arg(1).to_string(),	// dbname
					fn.arg(2).to_string(),	// user
					fn.arg(3).to_string(),	// pwd
					fn.arg(4).to_string()));	// socket
			}
		}
	}

	void	mydb_disconnect(const fn_call& fn)
	{
		mydb* db = cast_to<mydb>(fn.this_ptr);
		if (db)
		{
			db->disconnect();
		}
	}

	void	mydb_open(const fn_call& fn)
	// Creates new table from sql statement & returns pointer to it
	{
		mydb* db = cast_to<mydb>(fn.this_ptr);
		if (db)
		{
			if (fn.nargs < 1)
			{
				myprintf("Db.open() needs 1 arg\n");
				return;
			}

			sql_table* tbl = db == NULL ? NULL : db->open(fn.arg(0).to_string());
			if (tbl)
			{
				fn.result->set_as_object(tbl);
				return;
			}
		}
	}

	void	mydb_run(const fn_call& fn)
	// Executes sql statement & returns affected rows
	{
		mydb* db = cast_to<mydb>(fn.this_ptr);
		if (db)
		{
			if (fn.nargs < 1)
			{
				myprintf("Db.run() needs 1 arg\n");
				return;
			}

			fn.result->set_int(db->run(fn.arg(0).to_string()));
		}
	}

	void	mydb_commit(const fn_call& fn)
	{
		mydb* db = cast_to<mydb>(fn.this_ptr);
		if (db)
		{
			db->commit();
		}
	}

	void mydb_autocommit_setter(const fn_call& fn)
	{
		mydb* db = cast_to<mydb>(fn.this_ptr);
		if (db && fn.nargs == 1)
		{
			db->set_autocommit(fn.arg(0).to_bool());
		}
	}

	void mydb_trace_setter(const fn_call& fn)
	{
//		tu_autolock locker(s_mysql_plugin_mutex);

		mydb* db = cast_to<mydb>(fn.this_ptr);
		if (db && fn.nargs == 1)
		{
			db->m_trace = fn.arg(0).to_bool();
		}
	}

	// DLL interface
//	extern "C"
//	{
//		as_object* bakeinflash_module_init(const array<as_value>& params)
//		{
//			return new mydb();
//		}
//	}

	mydb::mydb() :
		m_trace(false),
		m_db(NULL)
	{
		// methods
		builtin_member("connect", mydb_connect);
		builtin_member("disconnect", mydb_disconnect);
		builtin_member("open", mydb_open);
		builtin_member("run", mydb_run);
		builtin_member("commit", mydb_commit);
		builtin_member("auto_commit", as_value(as_value(), mydb_autocommit_setter));
		builtin_member("trace", as_value(as_value(), mydb_trace_setter));
	}

	mydb::~mydb()
	{
		disconnect();
	}

	void mydb::disconnect()
	{
//		tu_autolock locker(s_mysql_plugin_mutex);

		if (m_db != NULL)
		{
			mysql_close(m_db);    
			m_db = NULL;
		}
	}

	bool mydb::connect(const char* host, const char* dbname, const char* user, 
											const char* pwd, const char* socket)
	{
//		tu_autolock locker(s_mysql_plugin_mutex);

		// Closes a previously opened connection &
		// also deallocates the connection handle
		disconnect();

		m_db = mysql_init(NULL);
		if ( m_db == NULL )
		{
			return false;
		}

	//	mysql_options(m_db, MYSQL_OPT_CONNECT_TIMEOUT, "30");

		// This function always returns 0. 
		// If SSL setup is incorrect, mysql_real_connect() returns an error when you attempt to connect.
		mysql_ssl_set(m_db, "client-key.pem", "client-cert.pem", "ca-cert.pem", NULL, "DHE-RSA-AES256-SHA");

		// real connect
		if (mysql_real_connect(
			m_db,
			strlen(host) > 0 ? host : NULL,
			strlen(user) > 0 ? user : NULL,
			strlen(pwd) > 0 ? pwd : NULL,
			strlen(dbname) > 0 ? dbname : NULL,
			0,
			strlen(socket) > 0 ? socket : NULL,
			CLIENT_MULTI_STATEMENTS) == NULL)
		{
			myprintf("%s\n", mysql_error(m_db));
			return false;
		}

		const char* chiper = mysql_get_ssl_cipher(m_db);
		myprintf("mysql SSL cipher %s \n", chiper ? chiper : "NONE");

		set_autocommit(true);
		return true;
	}

	int mydb::run(const char *sql)
	{
//		tu_autolock locker(s_mysql_plugin_mutex);

		if (m_trace) myprintf("run: %s\n", sql);

		if (runsql(sql))
		{
			return (int) mysql_affected_rows(m_db);
		}
		return 0;
	}

	sql_table* mydb::open(const char* sql)
	{
//		tu_autolock locker(s_mysql_plugin_mutex);

		if (m_trace) myprintf("open: %s\n", sql);

		if (runsql(sql))
		{
			// query succeeded, process any data returned by it
			MYSQL_RES* result = mysql_store_result(m_db);
			if (result)
			{
				mytable* tbl = new mytable();
				tbl->retrieve_data(result);
				mysql_free_result(result);
				return tbl;
			}
			myprintf("select query does not return data\n");
			myprintf("%s\n", sql);
		}
		return NULL;
	}

	void mydb::set_autocommit(bool autocommit)
	{
//		tu_autolock locker(s_mysql_plugin_mutex);

		if (m_trace) myprintf("set autocommit=%s\n", autocommit ? "true" : "false");

		mysql_autocommit(m_db, autocommit ? 1 : 0);
	}

	void mydb::commit()
	{
//		tu_autolock locker(s_mysql_plugin_mutex);

		if (m_trace) myprintf("commit\n");

		mysql_commit(m_db);
	}

	bool mydb::runsql(const char* sql)
	{
		if (m_db == NULL)
		{
			myprintf("missing connection\n");
			myprintf("%s\n", sql);
			return false;
		}

		if (mysql_query(m_db, sql))
		{
			myprintf("%s\n", mysql_error(m_db));
			myprintf("%s\n", sql);
			return false;
		}
		return true;
	}
}

#endif
