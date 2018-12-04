// as_mcloader.cpp	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script MovieClipLoader implementation code for the bakeinflash SWF player library.

#include "bakeinflash/bakeinflash_as_classes/as_mcloader.h"
#include "bakeinflash/bakeinflash_root.h"
#include "bakeinflash/bakeinflash_character.h"
#include "bakeinflash/bakeinflash_sprite.h"
#include "bakeinflash/bakeinflash_action.h"
#include "bakeinflash/bakeinflash_function.h"
#include "as_urlvariables.h"
#include "base/tu_timer.h"
#include "base/tu_file.h"

namespace bakeinflash
{
	void	as_mcloader_addlistener(const fn_call& fn)
	{
		as_mcloader* mcl = cast_to<as_mcloader>(fn.this_ptr);
		fn.result->set_bool(false);
		as_object* obj = fn.arg(0).to_object();
		if (mcl && fn.nargs > 0 && obj)
		{
			mcl->m_listeners.push_back(obj);
			fn.result->set_bool(true);
		}
	}

	void	as_mcloader_removelistener(const fn_call& fn)
	{
		as_mcloader* mcl = cast_to<as_mcloader>(fn.this_ptr);
		fn.result->set_bool(false);
		as_object* obj = fn.arg(0).to_object();
		if (mcl && fn.nargs > 0 && obj)
		{
			for (int i = 0; i < mcl->m_listeners.size(); )
			{
				if (mcl->m_listeners[i] == obj || mcl->m_listeners[i] == NULL)
				{
					mcl->m_listeners.remove(i);
					continue;
				}
				i++;
			}
			fn.result->set_bool(true);
		}
	}

	void	as_mcloader_loadclip(const fn_call& fn)
	{
		as_mcloader* mcl = cast_to<as_mcloader>(fn.this_ptr);
		fn.result->set_bool(false);

		if (get_root()->is_as3())
		{
			if (mcl && fn.nargs > 0)
			{
				as_urlrequest* urlRequest =	cast_to<as_urlrequest>(fn.arg(0).to_object());
				if (urlRequest)
				{
					const tu_string& url = urlRequest->get_url();
					mcl->load(url, NULL);		// target will be assigned by addChild
				}
			}
		}
		else
		{
			if (mcl && fn.nargs > 1)
			{
				mcl->load(fn.arg(0).to_string(), cast_to<sprite_instance>(fn.env->find_target(fn.arg(1))));
				fn.result->set_bool(true);
			}
		}
	}

	void	as_mcloader_unloadclip(const fn_call& fn)
	{
		fn.result->set_bool(false);
		if (fn.nargs > 0)
		{
			fn.env->load_file("", fn.arg(0));	// unload movie
			fn.result->set_bool(true);
		}
	}

	void	as_mcloader_getprogress(const fn_call& fn)
	{
		if (fn.nargs == 1)
		{
			sprite_instance* m = cast_to<sprite_instance>(fn.arg(0).to_object());
			if (m)
			{
				as_object* info = new as_object();
				info->set_member("bytesLoaded", (int) m->get_loaded_bytes());
				info->set_member("bytesTotal", (int) m->get_file_bytes());
				fn.result->set_as_object(info);
				return;
			}
		}
		fn.result->set_as_object(NULL);
	}

	void	as_global_mcloader_ctor(const fn_call& fn)
		// Constructor for ActionScript class MovieClipLoader
	{
		fn.result->set_as_object(new as_mcloader());
	}

	as_mcloader::as_mcloader()
	{
		builtin_member("addListener", as_mcloader_addlistener);
		builtin_member("removeListener", as_mcloader_removelistener);
		builtin_member("loadClip", as_mcloader_loadclip);
		builtin_member("unloadClip", as_mcloader_unloadclip);
		builtin_member("getProgress", as_mcloader_getprogress);

		// AS3
		builtin_member("load", as_mcloader_loadclip);

	}

	as_mcloader::~as_mcloader()
	{
	}

	bool as_mcloader::load(const tu_string& url, sprite_instance* target)
	{
		get_root()->add_listener(this);

		smart_ptr<loaderdef> ld = new loaderdef();
		m_ld.push_back(ld);

		ld->m_target = target;
		ld->m_url = get_full_url(get_workdir(), url.c_str());
		ld->m_http.close();


		// Find last slash or backslash.
		const char* ptr = ld->m_url.c_str() + ld->m_url.size();
		for (; ptr >= ld->m_url.c_str() && *ptr != '/'; ptr--) {}
		int len = ptr - ld->m_url.c_str() + 1;

		tu_string curdir = tu_string(ld->m_url.c_str(), len);
		set_curdir(curdir);

		if (exist(ld->m_url.c_str()))
		{
			// handle events in next frame
			ld->m_status = READING_MEMBUF;

			handle(m_ld.size() - 1, onLoadStart);
			return true;
		}

		ld->m_http.connect(ld->m_url);

		// handle events in next frame
		ld->m_status = CONNECTING;	// connecting
		ld->m_start = tu_timer::get_ticks();
		return true;
	}

	void	as_mcloader::advance(float delta_time)
	{
		int advanced_items = 0;
		for (int i = 0; i < m_ld.size(); i++)
		{
			switch (m_ld[i]->m_status)
			{
				case UNDEFINED:
				case HANDLE_EVENTS:
				case DOWNLOADING:
					break;
                
			case CONNECTING:	// connecting
			{
				bool rc = m_ld[i]->m_http.is_connected();
				if (rc)
				{
					// established connection
					m_ld[i]->m_status = CONNECTED;	// connected
					handle(i, onLoadStart);
				}
				else
				{
					// Timeout?
					Uint32 now = tu_timer::get_ticks();
					if (now - m_ld[i]->m_start >= HTTP_TIMEOUT * 1000 || m_ld[i]->m_http.is_alive() == false)
					{
						// Timed out.
						myprintf("timeout-error\n");
						handle(i, onLoadError);
					}
				}
				advanced_items++;
				break;
			}

			case CONNECTED:	// connected, write request
			{
				m_ld[i]->m_start = tu_timer::get_ticks();
				tu_string data;
				m_ld[i]->m_http.write_http(data); //to_string());
				m_ld[i]->m_status = READING;	// read reply
				advanced_items++;
				break;
			}

			case READING_MEMBUF:
			{
				m_ld[i]->m_url = get_full_url(get_workdir(), m_ld[i]->m_url.c_str());
				if (exist(m_ld[i]->m_url.c_str()))
				{
					set_target(i, new tu_file(m_ld[i]->m_url.c_str(), "rb"));
				}
				advanced_items++;
				break;
			}

			case READING:	// read reply
			{
				// read data
				const void* data = NULL;
				int size = 0;
				string_hash<as_value> headers;
				bool rc = m_ld[i]->m_http.read_http(&data, &size, &headers);
				if (rc)
				{
					tu_file*	in = new tu_file(tu_file::memory_buffer, size, (void*) data);
					set_target(i, in);
				}
				else
				{
					// Timeout?
					Uint32 now = tu_timer::get_ticks();
					if (now - m_ld[i]->m_start >= HTTP_TIMEOUT * 1000)
					{
						// Timed out.
						myprintf("timeout-error\n");
						handle(i, onLoadError);
					}
				}
				advanced_items++;
				break;
			}

		//	case LOAD_INIT:
		//		handle(i, onLoadInit);
		//		advanced_items++;
		//		break;
			}
		}

		if (advanced_items == 0)
		{
			m_ld.clear();
			get_root()->remove_listener(this);
		}

	}

	void	as_mcloader::handle(int index, mcl_event ev)
	{
		as_value func;
		as_environment env;
		switch (ev)
		{
			case onLoadInit:
			{
				for (int i = 0; i < m_listeners.size(); i++)
				{
					if (m_listeners[i] != NULL && m_listeners[i]->get_member("onLoadInit", &func))
					{
						env.push(m_ld[index]->m_target.get());
						call_method(func, &env, m_listeners[i].get(), 1, env.get_top_index());
					}
				}

				// finished
				m_ld[index]->m_status = UNDEFINED;
				break;
			}
			case onLoadComplete:
			{
				for (int i = 0; i < m_listeners.size(); i++)
				{
					if (m_listeners[i] != NULL && m_listeners[i]->get_member("onLoadComplete", &func))
					{
						env.push(m_ld[index]->m_target.get());
						call_method(func, &env, m_listeners[i].get(), 1, env.get_top_index());
					}
				}
				m_ld[index]->m_status = UNDEFINED; // LOAD_INIT;
				m_ld[index]->m_http.close();
				break;
			}

			case onLoadError:
			{
				for (int i = 0; i < m_listeners.size(); i++)
				{
					if (m_listeners[i] != NULL && m_listeners[i]->get_member("onLoadError", &func))
					{
						myprintf("%s\n", m_ld[index]->m_url.c_str());
						env.push("URL Not Found");
						env.push(m_ld[index]->m_target.get());
						call_method(func, &env, m_listeners[i].get(), 2, env.get_top_index());
					}
				}
				m_ld[index]->m_status = UNDEFINED;
				m_ld[index]->m_http.close();
				break;
			}

			case onLoadStart:
			{
				for (int i = 0; i < m_listeners.size(); i++)
				{
					if (m_listeners[i] != NULL && m_listeners[i]->get_member("onLoadStart", &func))
					{
						env.push(m_ld[index]->m_target.get());
						call_method(func, &env, m_listeners[i].get(), 1, env.get_top_index());
					}
				}
				break;
			}

			default:
				assert(0);
			break;
		}
	}

	// for AS3
	void as_mcloader::set_target(int i, sprite_instance* mc)
	{
		if (m_ld.size() > i)
		{
			m_ld[i]->m_target = mc;
		}
	}

	void as_mcloader::set_target(int index, tu_file* in)
	{
		handle(index, onLoadComplete);

		if (in == NULL)
		{
			myprintf("can't open file\n");
			handle(index, onLoadError);
			return;
		}
	
		Uint8 signature[3];
		in->read_bytes(signature, 3);
		in->set_position(0);

		smart_ptr<sprite_instance> target = m_ld[index]->m_target;

		// test file signature
		if (memcmp(signature, "CWS", 3) == 0 || memcmp(signature, "C++", 3) == 0 || memcmp(signature, "FWS", 3) == 0)
		{
			// 'in' will be deleted late in 'create_movie'
			movie_definition*	md = create_movie(in);
			if (md && md->get_frame_count() > 0 && target != NULL)
			{
				target->replace_me(md);
				handle(index, onLoadInit);
			}
			else
			{
				if (target == NULL)
				{
					myprintf("**error: can not load file, no target\n");
				}
				handle(index, onLoadError);
				return;
			}
		}
		else
		if (memcmp(signature, "\xFF\xD8\xFF", 3) == 0)
		{
			image::rgb*	im = image::read_jpeg(in);
			if (target != NULL && im)
			{
				bitmap_info* bi = render::create_bitmap_info(im);

				movie_definition* rdef = get_root()->get_movie_definition();
				bitmap_character_def*	jpeg = new bitmap_character_def(rdef, bi);

				target->replace_me(jpeg);
					
				handle(index, onLoadInit);
			}
			delete in;
		}
		else
		{
			myprintf("loadClip '%s', invalid signature\n", m_ld[index]->m_url.c_str());
			handle(index, onLoadError);
			delete in;
			return;
		}
	}

};
