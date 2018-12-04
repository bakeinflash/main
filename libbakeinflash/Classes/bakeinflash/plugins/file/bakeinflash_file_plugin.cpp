// file.cpp	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Very simple and convenient file I/O plugin for the bakeinflash SWF player library.

#if defined _MSC_VER
	#include <direct.h>
	#include <sys/utime.h>
#else
//#elif defined __GNUC__
//#include <sys/types.h>
//#include <sys/stat.h>
	#include <utime.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "bakeinflash_file_plugin.h"
#include "base/tu_timer.h"
#include "bakeinflash/bakeinflash_root.h"
#include "bakeinflash/bakeinflash_log.h"

namespace bakeinflash
{

	// mkpath("/Users/griscom/one/two/three/", S_IRWXU);
	// must be terminated by '/' !!!
	static void mkpath(const char *path) 
	{
		char opath[4096];
		char *p;
		size_t len;

		strncpy(opath, path, sizeof(opath));
		opath[sizeof(opath) - 1] = '\0';
		len = strlen(opath);
		if (len == 0)
		{
			return;
		}
		else
		if (opath[len - 1] == '/' || opath[len - 1] == '\\')
		{
			opath[len - 1] = '\0';
		}

			for (p = opath; *p; p++)
			{
				if (*p == '/' || *p == '\\' )
				{
					*p = '\0';
					if (access(opath, F_OK))
					{
#if defined _MSC_VER
						mkdir(opath);
#else
						mkdir(opath, S_IRWXU);
#endif
					}
					*p = '/';
				}
			}

	}


	void	as_file_plugin_ctor(const fn_call& fn)
		// Constructor for ActionScript class File.
	{
		assert(fn.env);
		fn.result->set_as_object(new file());
	}

	void file_download(const fn_call& fn)
	{
		file* fi = cast_to<file>(fn.this_ptr);
		if (fi && fn.nargs > 1)
		{
			fi->download(fn.arg(0).to_tu_string(), fn.arg(1).to_tu_string());
		}
	}

	void file_open(const fn_call& fn)
	{
		int rc = -1;
		file* fi = cast_to<file>(fn.this_ptr);
		if (fi)
		{
			rc = fi->open(fn.arg(0).to_tu_string(), fn.arg(1).to_tu_string());
		}
		fn.result->set_int(rc);
	}

	// file.read(), text only
	void file_read(const fn_call& fn)
	{
		file* fi = cast_to<file>(fn.this_ptr);
		if (fi && fi->m_file)
		{
			if (fi->m_file->get_error() == TU_FILE_NO_ERROR)
			{
				char buf[256];
				fi->m_file->read_string(buf, 256);
				fn.result->set_string(buf);
			}
		}
	}

	// file.write()
	void file_write(const fn_call& fn)
	{
		file* fi = cast_to<file>(fn.this_ptr);
		if (fi && fn.nargs > 0 && fi->m_file)
		{
			if (fi->m_file->get_error() == TU_FILE_NO_ERROR)
			{
				fi->m_file->write_string(fn.arg(0).to_string());
			}
		}
	}

	// file.eof readonly property
	void file_get_eof(const fn_call& fn)
	{
		file* fi = cast_to<file>(fn.this_ptr);
		if (fi && fi->m_file)
		{
			fn.result->set_bool(fi->m_file->get_eof());
		}
	}

	// file.eof readonly property
	void file_get_size(const fn_call& fn)
	{
		file* fi = cast_to<file>(fn.this_ptr);
		if (fi && fi->m_file)
		{
			fn.result->set_int(fi->m_file->size());
		}
	}

	// file.eof readonly property
	void file_get_datetime(const fn_call& fn)
	{
		file* fi = cast_to<file>(fn.this_ptr);
		if (fi)
		{
			fn.result->set_tu_string(fi->get_datetime(fi->m_filename));
		}
	}

	// file.eof readonly property
	void file_get_datetime_local(const fn_call& fn)
	{
		file* fi = cast_to<file>(fn.this_ptr);
		if (fi)
		{
			struct stat attrib;         // create a file attribute structure
			stat(fi->m_filename.c_str(), &attrib);     // get the attributes of afile.txt
			time_t t = (time_t) attrib.st_mtime;

			struct tm* dt = localtime(&t);
			if (dt)
			{
				char buf[128];
				snprintf(buf, sizeof(buf), "%04d/%02d/%02d %02d:%02d:%02d", dt->tm_year + 1900, dt->tm_mon + 1, dt->tm_mday, dt->tm_hour, dt->tm_min, dt->tm_sec);
				fn.result->set_tu_string(buf);
			}
		}
	}

	// file.error readonly property
	void file_get_error(const fn_call& fn)
	{
		file* fi = cast_to<file>(fn.this_ptr);
		if (fi && fi->m_file)
		{
			fn.result->set_int(fi->m_file->get_error());
		}
	}

	// DLL interface
	//	extern "C"
	//	{
	//		 as_object* bakeinflash_module_init(const array<as_value>& params)
	//		{
	//			if (params.size() >= 2)
	//			{
	//				return new file(player, params[0].to_tu_string(), params[1].to_tu_string());
	//			}
	//			myprintf("new File(): not enough args\n");
	//			return NULL;
	//		}
	//	}

	int file::open(const tu_string& path, const tu_string& mode)
	{
		// is path relative ?
		m_filename = get_workdir();
		if (strstr(path.c_str(), ":") || *path.c_str() == '/')
		{
			m_filename = "";
		}
		m_filename += path;

		delete m_file;	// delete prev
		m_file = new tu_file(m_filename.c_str(), mode.c_str());
		return m_file->get_error();
	}

	file::file() :
		m_file(NULL),
		m_size(0)		// for download
	{
//		m_http.set_trace(true);

		// methods
		builtin_member("download", file_download);
		builtin_member("open", file_open);
		builtin_member("read", file_read);
		builtin_member("write", file_write);
		builtin_member("eof", as_value(file_get_eof, as_value()));	// readonly property
		builtin_member("error", as_value(file_get_error, as_value()));	// readonly property
		builtin_member("size", as_value(file_get_size, as_value()));	// readonly property
		builtin_member("datetime", as_value(file_get_datetime, as_value()));	// readonly property
		builtin_member("datetime_local", as_value(file_get_datetime_local, as_value()));	// readonly property
	}

	file::~file()
	{
		delete m_file;
	}

	void file::download(const tu_string& url, const tu_string& path)
	{
		m_target_filename = get_workdir();
		m_target_filename += path;
		get_root()->add_listener(this);

		m_http.close();
		m_http.connect(url);

		// handle events in next frame
		m_status = CONNECTING;	// connecting
		m_start = tu_timer::get_ticks();
	}

	void	file::handle_events()
	{
		// must be here because event hadler may cause sendandload again
		m_status = UNDEFINED;
		get_root()->remove_listener(this);

		as_value func;
		if (get_member("onHTTPStatus", &func))
		{
			as_environment env;
			env.push(m_http.m_status.c_str());
			call_method(func, &env, this, 1, env.get_top_index());
		}

		// if there is onData then onLoad is not called
		if (get_member("onCompleted", &func))
		{
			as_environment env;
			call_method(func, &env, this, 0, env.get_top_index());
		}
	}

	tu_string file::month_to_int(const tu_string& month)
	{
		const char* m[] = { "Jan", "Feb", "Mar", "Apr", "May", "June", "July", "Aug", "Sept", "Oct", "Nov", "Dec" };
		for (int i = 0; i < TU_ARRAYSIZE(m); i++)
		{
			if (strcmp(m[i], month.c_str()) == 0)
			{
				char buf[8];
				snprintf(buf, sizeof(buf), "%02d", i + 1);
				return buf;
			}
		}
		return "00";
	}

	void	file::advance(float delta_time)
	{
		switch (m_status)
		{
		case UNDEFINED:
		case LOAD_INIT:
		case READING_MEMBUF:
			break;

		case CONNECTING:	// connecting
			{
				bool rc = m_http.is_connected();
				if (rc)
				{
					// established connection
					m_status = CONNECTED;	// connected
				}
				else
				{
					// Timeout?
					Uint32 now = tu_timer::get_ticks();
					Uint32 timeout = HTTP_TIMEOUT * 1000;
					if (now  - m_start >= timeout || m_http.is_alive() == false)
					{
						// Timed out.
						m_status = HANDLE_EVENTS;
					}
				}
				break;
			}

		case CONNECTED:	// connected, write request
			{
				m_start = tu_timer::get_ticks();
				m_http.set_method("HEAD");		// read headers only
				m_http.write_http("");
				m_status = READING;	// read reply
				break;
			}

			case READING:	// read headers
			{
				// read datam_data
				const void* data = NULL;
				int size = 0;
				string_hash<as_value> headers;
				bool rc = m_http.read_http(&data, &size, &headers);
				if (rc)
				{
					m_last_modified = "";
					m_size = 0;

					if (m_http.m_status != "200")
					{
						// error
						m_status = HANDLE_EVENTS;
						break;
					}

					as_value val;
					if (headers.get("Last-Modified", &val))
					{
						array<tu_string> a;
						val.to_tu_string().split(',', &a);
						if (a.size() >= 2)
						{
							array<tu_string> b;
							a[1].split(' ', &b);

							// sanity check
							// Last-Modified: Sun, 12 Apr 2015 16:57:08 GMT
							if (b.size() == 6 && b[3].size() == 4 && b[2].size() >=3 && b[1].size() > 0 && b[4].size() == 8)
							{
								m_last_modified = b[3];		// year
								m_last_modified += '/';
								m_last_modified += month_to_int(b[2]);		// month
								m_last_modified += '/';
								m_last_modified += b[1].size() == 1 ? tu_string("0") + b[1] : b[1];
								m_last_modified += ' ';
								m_last_modified += b[4];
							}
							else
							{
								myprintf("file downloader: invalid last-modified date %s\n", a[1].c_str());
							}
						}
					}

					if (headers.get("Content-Length", &val))
					{
						m_size = val.to_int();
					}

					// same file ?
					tu_string local_datetime = get_datetime(m_target_filename);
					if (local_datetime != m_last_modified || m_last_modified == "")
					{
						m_start = tu_timer::get_ticks();
						m_http.set_method("GET");		// read file
						m_http.write_http("");
						m_status = DOWNLOADING;	// read file
					}
					else
					{
						m_status = HANDLE_EVENTS;
					}
				}
				else
				{
					// Timeout?
					Uint32 now = tu_timer::get_ticks();
					Uint32 timeout = HTTP_TIMEOUT * 1000;
					if (now  - m_start >= timeout)
					{
						// Timed out.
						m_status = HANDLE_EVENTS;
					}
				}
				break;
			}

			case DOWNLOADING:	// read file
			{
				// read datam_data
				const void* data = NULL;
				int size = 0;
				string_hash<as_value> headers;
				bool rc = m_http.read_http(&data, &size, &headers);
				if (rc && size > 0 && data && m_last_modified.size() >= 19)
				{
					// ensure dir
					mkpath(m_target_filename.c_str());

					// save file
					tu_file out(m_target_filename.c_str(), "wb");
					if (out.get_error() == TU_FILE_NO_ERROR)
					{
						out.write_bytes(data, size);
						out.close();

						array<tu_string> a;
						m_last_modified.split(' ', &a);
						if (a.size() == 2)
						{
							array<tu_string> b;
							a[0].split('/', &b);
							array<tu_string> c;
							a[1].split(':', &c);
							if (b.size() == 3 && c.size() == 3)
							{
								struct tm gmt;
								memset(&gmt, 0, sizeof(struct tm));
								gmt.tm_year = atoi(b[0].c_str()) - 1900; // (current year minus 1900).
								gmt.tm_mon = atoi(b[1].c_str()) - 1;
								gmt.tm_mday = atoi(b[2].c_str());
								gmt.tm_hour = atoi(c[0].c_str());
								gmt.tm_min = atoi(c[1].c_str());
								gmt.tm_sec = atoi(c[2].c_str());
								gmt.tm_isdst = -1;
								time_t mtime = mktime(&gmt);

								// hack, convert gmt to local time
								struct tm* gmt2 = gmtime(&mtime);
								time_t mtime2 = mktime(gmt2);
								time_t diff = (time_t) difftime(mtime, mtime2);
								mtime += diff;

								//mtime
								if (mtime != -1)
								{
									struct utimbuf new_times;
									new_times.actime = 0; //time(NULL); // foo.st_atime; // keep atime unchanged
									new_times.modtime = mtime;    // set mtime to current time 
									int rc = utime(m_target_filename.c_str(), &new_times);
									int k=1;
								}
								else
								{
									m_http.set_status("invalid last-modified time1");
								}
							}
							else
							{
								m_http.set_status("invalid last-modified time2");
							}
						}
						else
						{
							m_http.set_status("invalid last-modified time3");
						}
					}
					else
					{
						m_http.set_status("write file failure");
					}

					m_status = HANDLE_EVENTS;
				}
				else
				{
					// Timeout?
					Uint32 now = tu_timer::get_ticks();
					Uint32 timeout = HTTP_TIMEOUT * 1000;
					if (now  - m_start >= timeout)
					{
						// Timed out.
						m_status = HANDLE_EVENTS;
					}
				}
				break;
			}


		case HANDLE_EVENTS:	// read reply
			{
				m_http.close();
				handle_events();
				break;
			}

		}
	}

	tu_string file::get_datetime(const tu_string& file_name)
	{
		struct stat attrib;         // create a file attribute structure
		stat(file_name.c_str(), &attrib);     // get the attributes of afile.txt
		time_t t = (time_t) attrib.st_mtime;

		struct tm* dt = gmtime(&t); // Get the last modified time and put it into the time
		char buf[128];
		*buf = 0;
		if (dt)
		{
			snprintf(buf, sizeof(buf), "%04d/%02d/%02d %02d:%02d:%02d", dt->tm_year + 1900, dt->tm_mon + 1, dt->tm_mday, dt->tm_hour, dt->tm_min, dt->tm_sec);
			return buf;
		}
		return buf;
	}

}

