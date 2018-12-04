// bakeinflash.cpp	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// here are collected all statics of bakeinflash library and also
// init_library and clear_library implementation

#if TU_CONFIG_LINK_TO_SSL == 1
	#include <openssl/ssl.h>
#endif

#include "base/tu_timer.h"
#include "base/tu_file.h"
#include "base/tu_random.h"
#include "bakeinflash/bakeinflash_object.h"
#include "bakeinflash/bakeinflash_action.h"

// action script classes
#include "bakeinflash/bakeinflash_as_sprite.h"
#include "bakeinflash/bakeinflash_text.h"
#include "bakeinflash/bakeinflash_json.h"	// extension
#include "bakeinflash/bakeinflash_md5.h"	// extension
#include "bakeinflash/bakeinflash_as_classes/as_array.h"
#include "bakeinflash/bakeinflash_as_classes/as_sound.h"
#include "bakeinflash/bakeinflash_as_classes/as_key.h"
#include "bakeinflash/bakeinflash_as_classes/as_math.h"
#include "bakeinflash/bakeinflash_as_classes/as_mcloader.h"
#include "bakeinflash/bakeinflash_as_classes/as_camera.h"
#include "bakeinflash/bakeinflash_as_classes/as_microphone.h"
#include "bakeinflash/bakeinflash_as_classes/as_netstream.h"
#include "bakeinflash/bakeinflash_as_classes/as_netconnection.h"
#include "bakeinflash/bakeinflash_as_classes/as_textformat.h"
#include "bakeinflash/bakeinflash_as_classes/as_string.h"
#include "bakeinflash/bakeinflash_as_classes/as_color.h"
#include "bakeinflash/bakeinflash_as_classes/as_date.h"
#include "bakeinflash/bakeinflash_as_classes/as_xmlsocket.h"
#include "bakeinflash/bakeinflash_as_classes/as_broadcaster.h"
#include "bakeinflash/bakeinflash_as_classes/as_selection.h"
#include "bakeinflash/bakeinflash_as_classes/as_number.h"
#include "bakeinflash/bakeinflash_as_classes/as_boolean.h"
#include "bakeinflash/bakeinflash_as_classes/as_global.h"
#include "bakeinflash/bakeinflash_as_classes/as_sharedobject.h"
#include "bakeinflash/bakeinflash_as_classes/as_mouse.h"
#include "bakeinflash/bakeinflash_as_classes/as_xml.h"
#include "bakeinflash/bakeinflash_as_classes/as_event.h"
#include "bakeinflash/bakeinflash_as_classes/as_event_dispatcher.h"
#include "bakeinflash/bakeinflash_as_classes/as_urlvariables.h"
#include "bakeinflash/bakeinflash_as_classes/as_iOS.h"
#include "bakeinflash/bakeinflash_as_classes/as_iwww.h"
#include "bakeinflash/bakeinflash_as_classes/as_flash.h"
#include "bakeinflash/bakeinflash_as_classes/as_bitmapdata.h"

// iOS
#if iOS
#include "bakeinflash/bakeinflash_as_classes/as_imap.h"
#include "bakeinflash/bakeinflash_as_classes/as_iaccel.h"
#include "bakeinflash/bakeinflash_as_classes/as_inapp.h"
#include "bakeinflash/bakeinflash_as_classes/as_iprinter.h"
#include "bakeinflash/bakeinflash_as_classes/as_itableview.h"
#include "bakeinflash/bakeinflash_as_classes/as_igamecenter.h"
#include "bakeinflash/bakeinflash_as_classes/as_itextview.h"
#endif

// plugins
#include "bakeinflash/plugins/sqlite/sqlite_db.h"
#include "bakeinflash/plugins/mysql/mydb.h"
#include "bakeinflash/plugins/file/bakeinflash_file_plugin.h"
#include "bakeinflash/plugins/cef/as_cef.h"
#include "bakeinflash/plugins/sysinfo/sysinfo.h"

namespace bakeinflash
{

	// statics

	static bool s_force_realtime_framerate(false);
	static bool s_clear_garbage(false);

	static tu_string s_workdir;
	static tu_string s_startdir;
	static tu_string s_curdir;
	static tu_string s_flash_vars;

	static smart_ptr<as_object>	s_global;
	static smart_ptr<root> s_current_root;
	static string_hash<smart_ptr<character_def> >* s_chardef_library(NULL);
	static hash<as_object*, bool> s_heap;

	// for video
	static Uint8* s_ta(NULL);	//[256][256] = {255};
	static Uint8* s_tb(NULL);	//[256][256][256] = {255};
	static Uint8* s_tc(NULL);	//[256][256] = {255};

	// interface

	void set_clear_garbage(bool val) { s_clear_garbage = val; }
	bool get_clear_garbage() { return s_clear_garbage; }

	// it's used to watch texture memory
	// bool s_log_bitmap_info;
	//	bool get_log_bitmap_info() { return s_log_bitmap_info; }
	//	void set_log_bitmap_info(bool log_bitmap_info) { s_log_bitmap_info = log_bitmap_info; }


	const Uint8* get_ta_table() { return s_ta; }
	const Uint8* get_tb_table() { return s_tb; }
	const Uint8* get_tc_table() { return s_tc; }

	void	as_global_itableview_ctor(const fn_call& fn);// hack
	void clears_tag_loaders();
	//	void clear_disasm();

	//
	//	bakeinflash's statics
	//


	// SSL context
#if TU_CONFIG_LINK_TO_MYSQL== 1
	static SSL_CTX* ssl_ctx = NULL;
	SSL_CTX* get_ssl_context()
	{
		if (ssl_ctx == NULL)
		{
			SSL_load_error_strings();
			SSL_library_init();
			ssl_ctx = SSL_CTX_new(SSLv23_client_method());
		}
		return ssl_ctx;
	}
#endif

	static glyph_provider* s_glyph_provider;
	void set_glyph_provider(glyph_provider* gp)
	{
		s_glyph_provider = gp;
	}
	glyph_provider* get_glyph_provider()
	{
		return s_glyph_provider;
	}

	static bool	s_use_cached_movie_def = false;
	void	use_cached_movie(bool yes)
	{
		s_use_cached_movie_def = yes;
	}

	// load movies from separate thread
	static bool	s_use_separate_loader = false;

	//
	// file_opener callback stuff
	//

	static file_opener_callback	s_opener_function = NULL;

	void	register_file_opener_callback(file_opener_callback opener)
		// Host calls this to register a function for opening files,
		// for loading movies.
	{
		s_opener_function = opener;
	}

	file_opener_callback get_file_opener_callback()
	{
		return s_opener_function;
	}


	// standard method map, this stuff should be high optimized

	static string_hash<as_value>*	s_standard_method_map[BUILTIN_COUNT];
	void clear_standard_method_map()
	{
		for (int i = 0; i < BUILTIN_COUNT; i++)
		{
			if (s_standard_method_map[i])
			{
				delete s_standard_method_map[i];
			}
		}
	}

	bool get_builtin(builtin_object id, const tu_string& name, as_value* val)
	{
		if (s_standard_method_map[id])
		{
			return s_standard_method_map[id]->get(name, val);
		}
		return false;
	}

	string_hash<as_value>* new_standard_method_map(builtin_object id)
	{
		if (s_standard_method_map[id] == NULL)
		{
			s_standard_method_map[id] = new string_hash<as_value>;
		}
		return s_standard_method_map[id];
	}

	void standard_method_map_init()
	{
		// setup builtin methods
		string_hash<as_value>* map;

		// as_object builtins
		map = new_standard_method_map(BUILTIN_OBJECT_METHOD);
		map->add("addProperty", as_object_addproperty);
		map->add("registerClass", as_object_registerclass);
		map->add("hasOwnProperty", as_object_hasownproperty);
		map->add("watch", as_object_watch);
		map->add("unwatch", as_object_unwatch);

		// for debugging
#ifdef _DEBUG
		map->add("dump", as_object_dump);
#endif

		// as_number builtins
		map = new_standard_method_map(BUILTIN_NUMBER_METHOD);
		map->add("toString", as_number_to_string);
		map->add("valueOf", as_number_valueof);

		// as_boolean builtins
		map = new_standard_method_map(BUILTIN_BOOLEAN_METHOD);
		map->add("toString", as_boolean_to_string);
		map->add("valueOf", as_boolean_valueof);

		// as_string builtins
		map = new_standard_method_map(BUILTIN_STRING_METHOD);
		map->add("toString", string_to_string);
		map->add("fromCharCode", string_from_char_code);
		map->add("charCodeAt", string_char_code_at);
		map->add("concat", string_concat);
		map->add("indexOf", string_index_of);
		map->add("lastIndexOf", string_last_index_of);
		map->add("slice", string_slice);
		map->add("split", string_split);
		map->add("substring", string_substring);
		map->add("substr", string_substr);
		map->add("toLowerCase", string_to_lowercase);
		map->add("toUpperCase", string_to_uppercase);
		map->add("charAt", string_char_at);
		map->add("length", as_value(string_length, as_value()));

		// sprite_instance builtins
		map = new_standard_method_map(BUILTIN_SPRITE_METHOD);
		map->add("play", sprite_play);
		map->add("stop", sprite_stop);
		map->add("gotoAndStop", sprite_goto_and_stop);
		map->add("gotoAndPlay", sprite_goto_and_play);
		map->add("nextFrame", sprite_next_frame);
		map->add("prevFrame", sprite_prev_frame);
		map->add("getBytesLoaded", sprite_get_bytes_loaded);
		map->add("getBytesTotal", sprite_get_bytes_total);
		map->add("swapDepths", sprite_swap_depths);
		map->add("duplicateMovieClip", sprite_duplicate_movieclip);
		map->add("getDepth", sprite_get_depth);
		map->add("createEmptyMovieClip", sprite_create_empty_movieclip);
		map->add("removeMovieClip", sprite_remove_movieclip);
		map->add("hitTest", sprite_hit_test);
		map->add("startDrag", sprite_start_drag);
		map->add("stopDrag", sprite_stop_drag);
		map->add("loadMovie", sprite_loadmovie);
		map->add("unloadMovie", sprite_unloadmovie);
		map->add("getNextHighestDepth", sprite_getnexthighestdepth);
		map->add("createTextField", sprite_create_text_field);
		map->add("attachMovie", sprite_attach_movie);
		map->add("attachAudio", sprite_attach_audio);
		map->add("localToGlobal", sprite_local_global);
		map->add("globalToLocal", sprite_global_local);
		map->add("getRect", sprite_get_rect);
		map->add("getBounds", sprite_get_bounds);
		map->add("setMask", sprite_set_mask);
		map->add("attachBitmap", sprite_attach_bitmap);

		// drawing API
		map->add("beginBitmapFill", sprite_begin_bitmapfill);
		map->add("beginFill", sprite_begin_fill);
		map->add("endFill", sprite_end_fill);
		map->add("lineTo", sprite_line_to);
		map->add("moveTo", sprite_move_to);
		map->add("curveTo", sprite_curve_to);
		map->add("clear", sprite_clear);
		map->add("lineStyle", sprite_line_style);

		// bakeinflash extension
		// return true if movieclip is in PLAY state
		map->add("getPlayState", sprite_get_play_state);
		map->add("loadBitmaps", sprite_load_bitmaps);

		//
		// AS3
		//

		// as_object builtins
		map = new_standard_method_map(BUILTIN_OBJECT_METHOD_AS3);
		map->add("addProperty", as_object_addproperty);
		map->add("registerClass", as_object_registerclass);
		map->add("hasOwnProperty", as_object_hasownproperty);

		// for debugging
#ifdef _DEBUG
		map->add("dump", as_object_dump);
#endif

		// as_number builtins
		map = new_standard_method_map(BUILTIN_NUMBER_METHOD_AS3);
		map->add("toString", as_number_to_string);
		map->add("valueOf", as_number_valueof);

		// as_boolean builtins
		map = new_standard_method_map(BUILTIN_BOOLEAN_METHOD_AS3);
		map->add("toString", as_boolean_to_string);
		map->add("valueOf", as_boolean_valueof);

		// as_string builtins
		map = new_standard_method_map(BUILTIN_STRING_METHOD_AS3);
		map->add("String", string_to_string);	// AS3
		map->add("toString", string_to_string);
		map->add("fromCharCode", string_from_char_code);
		map->add("charCodeAt", string_char_code_at);
		map->add("concat", string_concat);
		map->add("search", string_index_of);	
		map->add("lastIndexOf", string_last_index_of);
		map->add("slice", string_slice);
		map->add("split", string_split);
		map->add("substring", string_substring);
		map->add("substr", string_substr);
		map->add("toLowerCase", string_to_lowercase);
		map->add("toUpperCase", string_to_uppercase);
		map->add("charAt", string_char_at);
		map->add("length", as_value(string_length, as_value()));

		// sprite_instance builtins
		map = new_standard_method_map(BUILTIN_SPRITE_METHOD_AS3);
		map->add("play", sprite_play);
		map->add("stop", sprite_stop);
		map->add("gotoAndStop", sprite_goto_and_stop);
		map->add("gotoAndPlay", sprite_goto_and_play);
		map->add("nextFrame", sprite_next_frame);
		map->add("prevFrame", sprite_prev_frame);
		map->add("getBytesLoaded", sprite_get_bytes_loaded);
		map->add("getBytesTotal", sprite_get_bytes_total);
		map->add("swapDepths", sprite_swap_depths);
		map->add("duplicateMovieClip", sprite_duplicate_movieclip);
		map->add("getDepth", sprite_get_depth);
		map->add("createEmptyMovieClip", sprite_create_empty_movieclip);
		map->add("removeMovieClip", sprite_remove_movieclip);
		map->add("hitTest", sprite_hit_test);
		map->add("startDrag", sprite_start_drag);
		map->add("stopDrag", sprite_stop_drag);
		map->add("loadMovie", sprite_loadmovie);
		map->add("unloadMovie", sprite_unloadmovie);
		map->add("getNextHighestDepth", sprite_getnexthighestdepth);
		map->add("createTextField", sprite_create_text_field);
		map->add("attachMovie", sprite_attach_movie);
		map->add("attachAudio", sprite_attach_audio);
		map->add("localToGlobal", sprite_local_global);
		map->add("globalToLocal", sprite_global_local);
		map->add("getRect", sprite_get_rect);
		map->add("getBounds", sprite_get_bounds);
		map->add("setMask", sprite_set_mask);
		map->add("String", sprite_to_string);		// AS3

		// drawing API
		map->add("beginFill", sprite_begin_fill);
		map->add("endFill", sprite_end_fill);
		map->add("lineTo", sprite_line_to);
		map->add("moveTo", sprite_move_to);
		map->add("curveTo", sprite_curve_to);
		map->add("clear", sprite_clear);
		map->add("lineStyle", sprite_line_style);

		// bakeinflash extension
		// return true if movieclip is in PLAY state
		map->add("getPlayState", sprite_get_play_state);
		map->add("loadBitmaps", sprite_load_bitmaps);

		// AS3
		map->add("addFrameScript", sprite_add_script);
		map->add("addEventListener", sprite_add_event_listener);	// hack, mustbe in EventDispatcher 
		map->add("removeEventListener", sprite_remove_event_listener);	// hack, mustbe in EventDispatcher 
		map->add("dispatchEvent", sprite_dispatch_event);	// hack, mustbe in EventDispatcher 
		map->add("addChild", sprite_add_child);
		
	}

	// Standard property lookup.

	static string_hash<as_standard_member>	s_standard_property_map;
	static string_hash<as_standard_member>	s_standard_property_map_as3;
	void clear_standard_property_map()
	{
		s_standard_property_map.clear();
		s_standard_property_map_as3.clear();
	}

	const char* get_bakeinflash_version()
	{
#if defined(WIN32)
		static tu_string s_bakeinflash_version("WIN bakeinflash " __DATE__ " " __TIME__ );
#elif defined(iOS)
		static tu_string s_bakeinflash_version("iOS bakeinflash " __DATE__ " " __TIME__ );
#elif defined(ANDROID)
		static tu_string s_bakeinflash_version("ANDROID bakeinflash " __DATE__ " " __TIME__ );
#else
		static tu_string s_bakeinflash_version("LINUX bakeinflash " __DATE__ " " __TIME__);
#endif

		return s_bakeinflash_version.c_str();
	}

	// dynamic library stuff, for sharing DLL/shared library among different movies.

	static string_hash<tu_loadlib*> s_shared_libs;
	string_hash<tu_loadlib*>* get_shared_libs()
	{
		return &s_shared_libs;
	}

	void clear_shared_libs()
	{
		for (string_hash<tu_loadlib*>::iterator it = s_shared_libs.begin();
			it != s_shared_libs.end(); ++it)
		{
			delete it->second;
		}
		s_shared_libs.clear();
	}

	struct registered_type_node
	{
		registered_type_node(const tu_string& classname, bakeinflash_module_init type_init_func) :
			m_next(NULL),
			m_classname(classname),
			m_type_init(type_init_func)
		{
		}

		registered_type_node *m_next;
		tu_string             m_classname;
		bakeinflash_module_init   m_type_init;
	};

	static registered_type_node* s_registered_types = NULL;

	void register_type_handler(const tu_string& type_name, bakeinflash_module_init type_init_func )
	{
		registered_type_node** node = &s_registered_types;
		while(*node)
		{
			node = &((*node)->m_next);
		}
		*node = new registered_type_node(type_name, type_init_func);
	}

	void clear_registered_type_handlers()
	{
		registered_type_node *curr = s_registered_types;
		s_registered_types = NULL;
		while(curr)
		{
			registered_type_node *next = curr->m_next;
			delete curr;
			curr = next;
		}
	}

	bakeinflash_module_init find_type_handler(const tu_string& type_name)
	{
		registered_type_node *node = s_registered_types;
		while(node)
		{
			if (node->m_classname == type_name)
			{
				return node->m_type_init;
			}
			node = node->m_next;
		}
		return NULL;
	}

	// External interface.

	static fscommand_callback	s_fscommand_handler = NULL;

	fscommand_callback	get_fscommand_callback()
	{
		return s_fscommand_handler;
	}

	void	register_fscommand_callback(fscommand_callback handler)
	{
		s_fscommand_handler = handler;
	}

	as_standard_member	get_standard_member(const tu_string& name)
	{
		if (s_standard_property_map.size() == 0)
		{
			// AS2
			s_standard_property_map.set_capacity(int(AS_STANDARD_MEMBER_COUNT));
			s_standard_property_map.add("_x", M_X);
			s_standard_property_map.add("_y", M_Y);
			s_standard_property_map.add("_xscale", M_XSCALE);
			s_standard_property_map.add("_yscale", M_YSCALE);
			s_standard_property_map.add("_currentframe", M_CURRENTFRAME);
			s_standard_property_map.add("_totalframes", M_TOTALFRAMES);
			s_standard_property_map.add("_alpha", M_ALPHA);
			s_standard_property_map.add("_visible", M_VISIBLE);
			s_standard_property_map.add("_width", M_WIDTH);
			s_standard_property_map.add("_height", M_HEIGHT);
			s_standard_property_map.add("_rotation", M_ROTATION);
			s_standard_property_map.add("_target", M_TARGET);
			s_standard_property_map.add("_framesloaded", M_FRAMESLOADED);
			s_standard_property_map.add("_name", M_NAME);
			s_standard_property_map.add("_droptarget", M_DROPTARGET);
			s_standard_property_map.add("_url", M_URL);
			s_standard_property_map.add("_highquality", M_HIGHQUALITY);
			s_standard_property_map.add("_focusrect", M_FOCUSRECT);
			s_standard_property_map.add("_soundbuftime", M_SOUNDBUFTIME);
			s_standard_property_map.add("_xmouse", M_XMOUSE);
			s_standard_property_map.add("_ymouse", M_YMOUSE);
			s_standard_property_map.add("_parent", M_PARENT);
			s_standard_property_map.add("text", M_TEXT);
			s_standard_property_map.add("textWidth", M_TEXTWIDTH);
			s_standard_property_map.add("textColor", M_TEXTCOLOR);
			s_standard_property_map.add("border", M_BORDER);
			s_standard_property_map.add("multiline", M_MULTILINE);
			s_standard_property_map.add("wordWrap", M_WORDWRAP);
			s_standard_property_map.add("type", M_TYPE);
			s_standard_property_map.add("backgroundColor", M_BACKGROUNDCOLOR);
			s_standard_property_map.add("_this", M_THIS);
			s_standard_property_map.add("this", MTHIS);
			s_standard_property_map.add("_root", M_ROOT);
			s_standard_property_map.add(".", MDOT);
			s_standard_property_map.add("..", MDOT2);
			s_standard_property_map.add("_level0", M_LEVEL0);
			s_standard_property_map.add("_global", M_GLOBAL);
			s_standard_property_map.add("enabled", M_ENABLED);
			s_standard_property_map.add("password", M_PASSWORD);
			s_standard_property_map.add("onMouseMove", M_MOUSE_MOVE);
			s_standard_property_map.add("NaN", M_NAN);
			s_standard_property_map.add("scrollBox", M_SCROLLBOX);		// extension

			// AS3
			s_standard_property_map_as3.set_capacity(int(AS_STANDARD_MEMBER_COUNT));
			s_standard_property_map_as3.add("x", M_X);
			s_standard_property_map_as3.add("y", M_Y);
			s_standard_property_map_as3.add("scaleX", M_XSCALE);
			s_standard_property_map_as3.add("scaleY", M_YSCALE);
			s_standard_property_map_as3.add("currentFrame", M_CURRENTFRAME);
			s_standard_property_map_as3.add("totalFrames", M_TOTALFRAMES);
			s_standard_property_map_as3.add("alpha", M_ALPHA);
			s_standard_property_map_as3.add("visible", M_VISIBLE);
			s_standard_property_map_as3.add("width", M_WIDTH);	
			s_standard_property_map_as3.add("height", M_HEIGHT);	
			s_standard_property_map_as3.add("rotation", M_ROTATION);
			s_standard_property_map_as3.add("target", M_TARGET);
			s_standard_property_map_as3.add("framesLoaded", M_FRAMESLOADED);
			s_standard_property_map_as3.add("name", M_NAME);	
			//s_standard_property_map_as3.add("_droptarget", M_DROPTARGET);
			//s_standard_property_map_as3.add("_url", M_URL);
			//s_standard_property_map_as3.add("_highquality", M_HIGHQUALITY);
			//s_standard_property_map_as3.add("_focusrect", M_FOCUSRECT);
			//s_standard_property_map_as3.add("_soundbuftime", M_SOUNDBUFTIME);
			s_standard_property_map_as3.add("mouseX", M_XMOUSE);	
			s_standard_property_map_as3.add("mouseY", M_YMOUSE);	
			s_standard_property_map_as3.add("parent", M_PARENT);	
			s_standard_property_map_as3.add("text", M_TEXT);
			s_standard_property_map_as3.add("textWidth", M_TEXTWIDTH);
			s_standard_property_map_as3.add("textColor", M_TEXTCOLOR);
			s_standard_property_map_as3.add("border", M_BORDER);
			s_standard_property_map_as3.add("multiline", M_MULTILINE);
			s_standard_property_map_as3.add("wordWrap", M_WORDWRAP);
			s_standard_property_map_as3.add("type", M_TYPE);
			s_standard_property_map_as3.add("backgroundColor", M_BACKGROUNDCOLOR);
			//s_standard_property_map_as3.add("_this", M_THIS);
			s_standard_property_map_as3.add("this", MTHIS);
			//s_standard_property_map_as3.add("_root", M_ROOT);
			s_standard_property_map_as3.add(".", MDOT);
			s_standard_property_map_as3.add("..", MDOT2);
			//s_standard_property_map_as3.add("_level0", M_LEVEL0);
			//s_standard_property_map_as3.add("_global", M_GLOBAL);
			s_standard_property_map_as3.add("enabled", M_ENABLED);
			s_standard_property_map_as3.add("password", M_PASSWORD);
			s_standard_property_map_as3.add("onMouseMove", M_MOUSE_MOVE);
			s_standard_property_map_as3.add("NaN", M_NAN);
		}

		as_standard_member	result = M_INVALID_MEMBER;

		if (get_root()->is_as3())
		{
			s_standard_property_map_as3.get(name, &result);
		}
		else
		{
			s_standard_property_map.get(name, &result);
		}
		return result;
	}

	//
	// properties by number
	//

	static const tu_string	s_property_names[] =
	{
		tu_string("_x"),
		tu_string("_y"),
		tu_string("_xscale"),
		tu_string("_yscale"),
		tu_string("_currentframe"),
		tu_string("_totalframes"),
		tu_string("_alpha"),
		tu_string("_visible"),
		tu_string("_width"),
		tu_string("_height"),
		tu_string("_rotation"),
		tu_string("_target"),
		tu_string("_framesloaded"),
		tu_string("_name"),
		tu_string("_droptarget"),
		tu_string("_url"),
		tu_string("_highquality"),
		tu_string("_focusrect"),
		tu_string("_soundbuftime"),
		tu_string("mysteryquality"), //tu_string("@@ mystery quality member"),  //this seems like a stupid bug to me . . . but I don't want it accessing the heap yet.
		tu_string("_xmouse"),
		tu_string("_ymouse"),
	};

	as_value	get_property(as_object* obj, int prop_number)
	{
		as_value	val;
		if (prop_number >= 0 && prop_number < int(sizeof(s_property_names)/sizeof(s_property_names[0])))
		{
			obj->get_member(s_property_names[prop_number], &val);
		}
		else
		{
			myprintf("error: invalid property query, property number %d\n", prop_number);
		}
		return val;
	}

	void	set_property(as_object* obj, int prop_number, const as_value& val)
	{
		if (prop_number >= 0 && prop_number < int(sizeof(s_property_names)/sizeof(s_property_names[0])))
		{
			obj->set_member(s_property_names[prop_number], val);
		}
		else
		{
			myprintf("error: invalid set_property, property number %d\n", prop_number);
		}
	}

	//
	//	player
	//

	void init_player()
	{
		s_chardef_library = new string_hash<smart_ptr<character_def> >();


		// important!!  чтобы не слетал
		s_heap.set_capacity(1000);
		s_global = new as_object();

		action_init();

		// timer should be inited only once
		tu_timer::init_timer();

		standard_method_map_init();

		// set startup random position
		Uint64 t = tu_timer::get_systime();
		t &= 0xFF;	// truncate
		for (Uint32 i = 0; i < t; i++)
		{
			tu_random::next_random();
		}

	}

	void clear_player()
	{
		// Clean up bakeinflash as much as possible, so valgrind will help find actual leaks.
		// Maximum release of resources.  Calls clear_library() and
		// fontlib::clear(), and also clears some extra internal stuff
		// that may have been allocated (e.g. global ActionScript
		// objects).  This should get all bakeinflash structures off the
		// heap, with the exception of any objects that are still
		// referenced by the host program and haven't had drop_ref()
		// called on them.

		// for video
		free(s_ta);
		free(s_tb);
		free(s_tc);

		s_current_root = NULL;
		s_global = NULL;

		clear_heap();

		bakeinflash_engine_mutex().lock();

		clear_library();

		// Clear shared stuff only when all players are deleted
		clears_tag_loaders();
		clear_shared_libs();
		clear_registered_type_handlers();
		clear_standard_method_map();
		//			clear_disasm();
		delete s_glyph_provider;
		s_glyph_provider = NULL;

		bakeinflash_engine_mutex().unlock();

		clear_standard_property_map();

		delete s_chardef_library;

#if TU_CONFIG_LINK_CEF == 1
		cef_shutdown(); 
#endif

#if TU_CONFIG_LINK_TO_SSL == 1
		if (ssl_ctx)
		{
			SSL_CTX_free(ssl_ctx);
		}
#endif

#ifdef ACTION_BUFFER_PROFILING
		log_and_reset_profiling();
#endif

	}

	const tu_string& get_flash_vars()
		// Allow pass user variables to Flash
	{
		return s_flash_vars;
	}

	void set_flash_vars(const tu_string& param)
		// Allow pass user variables to Flash
	{
		s_flash_vars = param;
	}

	void verbose_action(bool val)
	{
		set_verbose_action(val);
	}

	void verbose_parse(bool val)
	{
		set_verbose_parse(val);
	}

	void	as_global_trace(const fn_call& fn);
	void	action_init()
		// Create/hook built-ins.
	{
		//
		// global init
		//

		set_alive(s_global);

		s_global->builtin_member("trace", as_global_trace);
		s_global->builtin_member("Object", as_global_object_ctor);
		s_global->builtin_member("Sound", as_global_sound_ctor);
		s_global->builtin_member("getURL", as_global_geturl);
		s_global->builtin_member("Array", new as_global_array());
		s_global->builtin_member("MovieClip", as_global_movieclip_ctor);
		s_global->builtin_member("TextField", as_global_textfield_ctor);
		s_global->builtin_member("TextFormat", as_global_textformat_ctor);
		s_global->builtin_member("SharedObject", new as_sharedobject());
		s_global->builtin_member("Mouse", new as_mouse());
		s_global->builtin_member("MovieClipLoader", as_global_mcloader_ctor);
		s_global->builtin_member("String", get_global_string_ctor());
		s_global->builtin_member("Number", as_global_number_ctor);
		s_global->builtin_member("Boolean", as_global_boolean_ctor);
		s_global->builtin_member("Color", as_global_color_ctor);
		s_global->builtin_member("Date", as_global_date_ctor);
		s_global->builtin_member("Selection", selection_init());
		s_global->builtin_member("XMLSocket", as_global_xmlsock_ctor);
		s_global->builtin_member("LoadVars", as_global_loadvars_ctor);
		s_global->builtin_member("XML", as_xml_ctor);

    
#if !defined(__APPLE__) && !defined(WINPHONE)
		s_global->builtin_member("sysinfo", as_sysinfo_ctor);
#endif
    
		as_object * capabilities = new as_object();
		capabilities->set_member( "version", "WIN 9,0,45,0" );
		s_global->builtin_member("Capabilities", capabilities);


		// ASSetPropFlags
		s_global->builtin_member("ASSetPropFlags", as_global_assetpropflags);

		// for video
		s_global->builtin_member("NetStream", as_global_netstream_ctor);
		s_global->builtin_member("NetConnection", as_global_netconnection_ctor);

#if defined(__APPLE__) && !defined(iOS) && !defined(Android) && !defined(WINPHONE) || defined(_WIN32)
		s_global->builtin_member("Microphone", microphone_init());
		s_global->builtin_member("Camera", camera_init());
#endif
    
		s_global->builtin_member("Math", math_init());
		s_global->builtin_member("Key", key_init());
		s_global->builtin_member("AsBroadcaster", broadcaster_init());
		s_global->builtin_member("flash", flash_init());

		// global builtins functions
		s_global->builtin_member("Stage",  stage_init());
		s_global->builtin_member("setInterval",  as_global_setinterval);
		s_global->builtin_member("clearInterval",  as_global_clearinterval);
		s_global->builtin_member("setTimeout",  as_global_settimeout);
		s_global->builtin_member("clearTimeout",  as_global_clearinterval);
		s_global->builtin_member("getVersion",  as_global_get_version);
		s_global->builtin_member("parseFloat",  as_global_parse_float);
		s_global->builtin_member("parseInt",  as_global_parse_int);
		s_global->builtin_member("isNaN",  as_global_isnan);
		s_global->builtin_member("$version", as_value(as_global_get_version, as_value()));
		s_global->builtin_member("updateAfterEvent", as_global_update_after_event);
		s_global->builtin_member("JSON", json_init());
		s_global->builtin_member("System", iOS_init());
		s_global->builtin_member("print", as_global_print);

		s_global->builtin_member("MD5", md5_init());

#if iOS
	//	s_global->builtin_member("Map", as_global_imap_ctor);
		s_global->builtin_member("inApp", inapp_init());
		s_global->builtin_member("Accelerometer", as_global_iaccel_ctor);
		s_global->builtin_member("tableView", as_global_itableview_ctor);
		s_global->builtin_member("textView", as_global_itextview_ctor);
		s_global->builtin_member("gameCenter", as_global_igamecenter_ctor);
		s_global->builtin_member("Printer", as_global_iprinter_ctor);
#endif

#if TU_CONFIG_LINK_TO_SQLITE == 1
		s_global->builtin_member("sqlite", sqlite_plugin::as_sqlite_ctor);
#endif

#if TU_CONFIG_LINK_TO_MYSQL == 1
		s_global->builtin_member("myDB", as_mysql_ctor);
#endif

#ifndef WINPHONE
		s_global->builtin_member("File", as_file_plugin_ctor);
		s_global->builtin_member("www", as_global_iwebview_ctor);
#endif

#if TU_CONFIG_LINK_CEF == 1
		s_global->builtin_member("www", as_global_cef_ctor);
#endif

		// AS3
		s_global->builtin_member("Event",  new as_global_event());
		s_global->builtin_member("MouseEvent",  new as_global_mouseevent());
		s_global->builtin_member("DataEvent",  new as_global_dataevent());
		s_global->builtin_member("IOErrorEvent",  new as_global_ioerrorevent());
		s_global->builtin_member("SecurityErrorEvent",  new as_global_securityerrorevent());
		s_global->builtin_member("EventDispatcher", as_global_event_dispatcher_ctor);
		s_global->builtin_member("URLVariables", as_urlvariables_ctor);
		s_global->builtin_member("Loader", as_global_mcloader_ctor);
		s_global->builtin_member("URLRequest", as_urlrequest_ctor);
		s_global->builtin_member("URLLoader", as_urlloader_ctor);
	}

	void	add_class(const tu_string& name, const as_value& ctor)
	{
		s_global->builtin_member(name, ctor);
	}

	as_object* get_global()
	{
		return s_global ? s_global.get() : NULL;
	}

	void notify_key_object(key::code k, Uint16 utf16char, bool down)
	{
		as_value	kval;
		as_object* global = get_global();
		global->get_member("Key", &kval);
		as_key*	ko = cast_to<as_key>(kval.to_object());
		if (ko)
		{
			if (down) ko->set_key_down(k, utf16char);
			else ko->set_key_up(k, utf16char);
		}
		else
		{
			myprintf("bakeinflash::notify_key_event(): no Key built-in\n");
		}
	}

	root* get_root()
	{
		// on exit m_current_root may be NULL
		//		assert(m_current_root.get() != NULL);
		return s_current_root.get();
	}

	character*	get_root_movie()
	{
		if (s_current_root != NULL)
		{
			return s_current_root->get_root_movie();
		}
		return NULL;
	}

	void notify_key_event(key::code k, Uint16 utf16char, bool down)
	{
		s_current_root->notify_key_event(k, utf16char, down);
	}

	void set_root(root* m)
	{
		assert(m != NULL);
		s_current_root = m;
	}

	const char* get_workdir()
	{
		return s_workdir.c_str();
	}

	void set_workdir(const char* dir)
	{
		assert(dir != NULL);
		s_workdir = dir;
	}

	const tu_string& get_startdir()
	{
		return s_startdir;
	}

	void set_startdir(const tu_string& dir)
	{
		s_startdir = dir;
	}

	const tu_string& get_curdir()
	{
		return s_curdir;
	}

	void set_curdir(const tu_string& dir)
	{
		s_curdir = dir;
	}

	// library stuff, for sharing resources among different movies.
	string_hash<smart_ptr<character_def> >* get_chardef_library()
	{
		return s_chardef_library;
	}

	const char* get_root_filename(const character_def* rdef)
	// get filename by root movie definition
	{
		for (string_hash<smart_ptr<character_def> >::iterator it = s_chardef_library->begin();
			it != s_chardef_library->end(); ++it)
		{
			if (it->second == rdef)
			{
				return it->first.c_str();
			}
		}
		return NULL;
	}

	void clear_library()
		// Drop all library references to movie_definitions, so they
		// can be cleaned up.
	{
		for (string_hash<smart_ptr<character_def> >::iterator it = s_chardef_library->begin(); it != s_chardef_library->end(); ++it)
		{
			if (it->second->get_ref_count() > 1)
			{
				myprintf("memory leaks is found out: on exit movie_definition_sub ref_count > 1\n");
				myprintf("this = %p, ref_count = %d\n", it->second.get(),it->second->get_ref_count());

				// to detect memory leaks
				while (it->second->get_ref_count() > 1) 
				{
					it->second->drop_ref();
				}
			}
		}
		s_chardef_library->clear();
	}

	void	ensure_loaders_registered();

	movie_definition*	create_movie(tu_file* in)
	{
		assert(in);

		ensure_loaders_registered();

		movie_def_impl*	m = new movie_def_impl(DO_LOAD_BITMAPS, DO_LOAD_FONT_SHAPES);

		//		if (s_use_cached_movie_def)
		//		{
		//			get_chardef_library()->add(filename, m);
		//		}

		m->read(in);

		// "in" will be deleted after termination of the loader thread
		//	delete in;

		return m;
	}

	// filename=NULL means 
	movie_definition*	create_movie(const char* filename)
	{
		assert(filename);

		// Is the movie already in the library?
		if (s_use_cached_movie_def)
		{
			smart_ptr<character_def>	m;
			get_chardef_library()->get(filename, &m);
			if (m != NULL)
			{
				// Return cached movie.
				return cast_to<movie_definition>(m.get());
			}
		}

		if (s_opener_function == NULL)
		{
			// Don't even have a way to open the file.
			myprintf("error: no file opener function; can't create movie.	"
				"See bakeinflash::register_file_opener_callback\n");
			return NULL;
		}

		tu_file* in = s_opener_function(filename);
		if (in == NULL)
		{
			myprintf("failed to open '%s'; can't create movie.\n", filename);
			return NULL;
		}
		else if (in->get_error())
		{
			myprintf("error: file opener can't open '%s'\n", filename);
			delete in;
			return NULL;
		}

		ensure_loaders_registered();

		movie_def_impl*	m = new movie_def_impl(DO_LOAD_BITMAPS, DO_LOAD_FONT_SHAPES);

		if (s_use_cached_movie_def)
		{
			get_chardef_library()->add(filename, m);
		}

		m->read(in);

		// "in" will be deleted after termination of the loader thread
		//	delete in;

		return m;
	}

	smart_ptr<root> load_file(const char* infile)
		// Load the actual movie.
	{
		smart_ptr<bakeinflash::movie_definition>	md = create_movie(infile);
		if (md == NULL)
		{
			myprintf("error: can't create a movie from '%s'\n", infile);
			return NULL;
		}

		smart_ptr<bakeinflash::root>	m = md->create_instance();
		if (m == NULL)
		{
			myprintf("error: can't create movie instance\n");
			return NULL;
		}

		int	movie_version = m->get_movie_version();

#ifdef _DEBUG
		myprintf("Playing %s, swf version %d\n", infile, movie_version);
#else
		IF_VERBOSE_PARSE(myprintf("Playing %s, swf version %d\n", infile, movie_version));
#endif

		m->set_infile(tu_string(get_workdir()) + tu_string(infile));
		return m;
	}

	const bool get_force_realtime_framerate()
	{
		return s_force_realtime_framerate;
	}

	void set_force_realtime_framerate(const bool force_realtime_framerate)
	{
		s_force_realtime_framerate = force_realtime_framerate;
	}

	// garbage collector

	void set_alive(as_object* obj, bool alive)
	{
		if (obj)
		{
			hash<as_object*, bool>::iterator it = s_heap.find(obj);
			if (alive)
			{
				if (it == s_heap.end())
				{
					obj->add_ref();
					s_heap.add(obj, false);
				}
				else
				{
					it->second = false;
				}
			}
			else
			{
				if (it != s_heap.end())
				{
					it->second = true;
					s_clear_garbage = true;
				}
			}
		}
	}

	bool is_garbage(as_object* obj)
	{
		bool is_garbage = false;
		if (obj)
		{
			s_heap.get(obj, &is_garbage);
		}
		return is_garbage;
	}

	// on enter frame
	void set_as_garbage()
	{
		if (s_clear_garbage)
		{
			for (hash<as_object*, bool>::iterator it = s_heap.begin(); it != s_heap.end(); ++it)
			{
				it->second = true;
			}
		}
	}

	// on exit frame
	void clear_garbage()
	{
		if (s_clear_garbage)
		{
			if (get_verbose_heap())
			{
				myprintf("-------------- heap size %d\n", s_heap.size());
			}

			s_global->this_alive();
			for (hash<as_object*, bool>::iterator it = s_heap.begin(); it != s_heap.end(); )
			{
				if (it->second)	// is garbage ?
				{
					as_object* obj = it->first;
					if (obj->get_ref_count() > 1)	// is in heap only ?
					{
						hash<as_object*, bool> visited_objects;
						obj->clear_refs(&visited_objects, obj);
					}

					// remove from heap
					obj->drop_ref();

					hash<as_object*, bool>::iterator current = it;
					++it;
					s_heap.erase(current);
				}
				else
				{
					++it;
				}
			}
			//			myprintf("cleared garbage\n");
			s_clear_garbage = false;
		}
	}

	// on exit from app
	void clear_heap()
	{
		for (hash<as_object*, bool>::iterator it = s_heap.begin(); it != s_heap.end(); ++it)
		{
			as_object* obj = it->first;
			if (obj->get_ref_count() > 1)
			{
				hash<as_object*, bool> visited_objects;
				obj->clear_refs(&visited_objects, obj);
			}

			// remove from heap
			obj->drop_ref();
		}
		s_heap.clear();
	}


	//
	//
	//
	bool use_separate_thread()
	{
		return s_use_separate_loader;
	}

	void set_separate_thread(bool flag)
	{
		s_use_separate_loader = flag;
	}

	const tu_string& get_canvas_name()
	{
		static tu_string s_canvas_name("__canvas__");
		return s_canvas_name;
	}	


	//
	//  for video
	//

	void create_YUV2RGBtable()
	{
		// sanity check
		if (s_ta && s_tb && s_tc)
		{
			return;
		}

		Uint8* p;
		//	*p++ = imin(Y2, (int) fsaturate(1.164f * (Y1 - 16) + 1.596f * (V1 - 128)));		// R
		s_ta = (Uint8*) malloc(256 * 256);
		p = s_ta;
		for (int y1 = 0; y1 < 256; y1++)
		{
			for (int v1 = 0; v1 < 256; v1++)
			{
				*p++ = isaturate(int(1.164f * (y1 - 16) + 1.596f * (v1 - 128)));
			}
		}


		//	*p++ = imin(Y2, (int) fsaturate(1.164f * (Y1 - 16) - 0.813f * (V1 - 128) - 0.391f * (U1 - 128)));		// G
		s_tb = (Uint8*) malloc(256 * 256 * 256);
		p = s_tb;
		for (int y1 = 0; y1 < 256; y1++)
		{
			for (int v1 = 0; v1 < 256; v1++)
			{
				for (int u1 = 0; u1 < 256; u1++)
				{
					*p++ = isaturate(int(1.164f * (y1 - 16) - 0.813f * (v1 - 128) - 0.391f * (u1 - 128)));
				}
			}
		}

		//					*p++ = imin(Y2, (int) fsaturate(1.164f * (Y1 - 16) + 2.018f * (U1 - 128)));		// B
		s_tc = (Uint8*) malloc(256 * 256);
		p = s_tc;
		for (int y1 = 0; y1 < 256; y1++)
		{
			for (int u1 = 0; u1 < 256; u1++)
			{
				*p++ = isaturate(int(1.164f * (y1 - 16) + 2.018f * (u1 - 128)));
			}
		}
	}

}



// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
