//
// A minimal test player app for the bakeinflash library.
//
#if defined(TU_CONFIG_LINK_TO_VLD) && defined(_MSC_VER) && _MSC_VER >= 1400
#include <vld.h>
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_opengl.h>

#ifdef TU_USE_SDL_SOUND
#include "bakeinflash/bakeinflash_sound_handler_sdl.h" 
#else
#include "bakeinflash/bakeinflash_sound_handler_openal.h" 
#endif


#include "bakeinflash/bakeinflash.h"
#include <stdlib.h>
#include <stdio.h>
#include "base/utility.h"
#include "base/container.h"
#include "base/tu_file.h"
#include "base/tu_types.h"
#include "base/tu_timer.h"
#include "base/utf8.h"
#include "bakeinflash/bakeinflash_types.h" 
#include "bakeinflash/bakeinflash_impl.h"
#include "bakeinflash/bakeinflash_root.h"
#include "bakeinflash/bakeinflash_freetype.h"
#include "bakeinflash/bakeinflash_as_classes/as_broadcaster.h"
#include "bakeinflash/bakeinflash_logo.h"
#include "bakeinflash/bakeinflash_as_classes/as_global.h"

#include "bakeinflash/plugins/validator/as_validator.h"
#include "bakeinflash/plugins/ts/ts.h"
//#include "bakeinflash/plugins/filters/filters.h"
//#include "bakeinflash/plugins/tunnel/as_tunnel.h"
#include "bakeinflash/bakeinflash_cursor.h"

#include "net/http_server.h"

using namespace bakeinflash;

#ifdef _WIN32
#include "bakeinflash/bakeinflash_muxer.h"

#include <Winsock.h>
#include <direct.h>		// for getcwd
#define stricmp _stricmp
#define getcwd _getcwd
#define PATH_SLASH '\\'
#define PATH_CURDIR ".\\"

//#define USE_MUXER
#ifdef USE_MUXER
smart_ptr<muxer> s_muxer;
#endif

#else
#include <unistd.h>		// for getcwd
#define stricmp strcasecmp
#define PATH_SLASH '/'
#define PATH_CURDIR "./"
#endif                                  

static int s_delay = 1;
int s_real_fps = 0;
static bool s_go = true;
bool s_has_virtual_keyboard = false;
static SDL_Window* s_screen = NULL;
float s_scale = 1;

sound_handler*	sh = NULL;
bakeinflash::render_handler*	rh = NULL;
int screen_height = 0;
int screen_width = 0;
int fullscreen_mode = 0;

namespace bakeinflash
{
	void show_cursor(int mouse_x, int mouse_y);
}

static void	log_callback(bool error, const char* message)
	// Error callback for handling bakeinflash messages.
{
	fputs(message, stdout);
	fflush(stdout);
	/*
	if (error)
	{
	// Log, and also print to stderr.
	fputs(message, stderr);
	fflush(stderr);
	}
	else
	{
	fputs(message, stdout);
	fflush(stdout);
	}
	*/
}

static tu_file*	file_opener(const char* url)
	// Callback function.  This opens files for the bakeinflash library.
{
	tu_file* fi = NULL;
	tu_string filename;

#ifdef _WIN32
	// try current folder
	if (exist(url))
	{
		return new tu_file(url, "rb");
	}
#endif

	// absolute path ?
	if (strlen(url) > 1 && url[1] != ':')
	{
		filename = get_workdir();
	}
	filename += url;
	fi = new tu_file(filename.c_str(), "rb");
	if (fi && fi->get_error() != TU_FILE_NO_ERROR)
	{
		delete fi;
		fi = NULL;
	}

	// Find last slash or backslash.
	const char* ptr = filename.c_str() + filename.size();
	for (; ptr >= filename.c_str() && *ptr != PATH_SLASH; ptr--) {}
	int len = ptr - filename.c_str() + 1;

	tu_string curdir = tu_string(filename.c_str(), len);
	if (curdir == PATH_CURDIR)
	{
		char currentPath[FILENAME_MAX];
		getcwd(currentPath, sizeof(currentPath));
		curdir = currentPath;
		curdir += PATH_SLASH;
	}

	set_curdir(curdir);

	printf("loaded '%s'\n", filename.c_str());
	return fi;
}

static key::code	translate_key(Uint16 key)
	// For forwarding SDL key events to bakeinflash.
{
	key::code	c(key::INVALID);

	if (key >= SDLK_0 && key <= SDLK_9)
	{
		get_root()->m_shift_key_state = false;	// hack
		c = (key::code) ((key - SDLK_0) + key::_0);
		return c;
	}
	else if (key >= 65 && key <= (65 + 25))		// A..Z
	{
		get_root()->m_shift_key_state = true;	// hack
		c = (key::code) ((key - 65) + key::A);
		return c;
	}
	else if (key >= SDLK_a && key <= SDLK_z)		// a..z
	{
		get_root()->m_shift_key_state = false;	// hack
		c = (key::code) ((key - SDLK_a) + key::A);
		return c;
	}
	//	else if (key >= SDLK_F1 && key <= SDLK_F15)
	//	{
	//		c = (key::code) ((key - SDLK_F1) + key::F1);
	//		return c;
	//	}
	else if (key >= 1040 && key < (1040 + 32))
	{
		get_root()->m_shift_key_state = true;	// hack
		c = (key::code) ((key - 1040) + key::RUS_1);
		return c;
	}
	else if (key >= 1072 && key < (1072 + 32))
	{
		get_root()->m_shift_key_state = false;	// hack
		c = (key::code) ((key - 1072) + key::RUS_a1);
		return c;
	}

	// many keys don't correlate, so just use a look-up table.
	struct
	{
		int code;
		bool shift;
		key::code	gs;
	} table[] =
	{
		{ 8, true, key::BACKSPACE },
		{ 13, true, key::ENTER },
		{ 32, true, key::SPACE },
		{ 33, true, key::_1 },
		{ 34, true, key::QUOTE },
		{ 35, true, key::_3 },
		{ 36, true, key::_4 },
		{ 37, true, key::_5 },
		{ 38, true, key::_7 },
		{ 39, false, key::QUOTE },
		{ 40, true, key::_9 },
		{ 41, true, key::_0 },
		{ 42, true, key::_8 },
		{ 43, true, key::EQUALS },
		{ 44, false, key::COMMA },
		{ 45, false, key::MINUS },
		{ 46, false, key::PERIOD },
		{ 47, false, key::SLASH },
		{ 58, true, key::SEMICOLON },
		{ 59, false, key::SEMICOLON },
		{ 60, true, key::COMMA },
		{ 61, false, key::EQUALS },
		{ 62, true, key::PERIOD },
		{ 63, true, key::SLASH },
		{ 64, true, key::_2 },
		{ 91, false, key::LEFT_BRACKET },
		{ 92, false, key::BACKSLASH },
		{ 93, false, key::RIGHT_BRACKET },
		{ 94, true, key::_6 },
		{ 95, true, key::MINUS },
		{ 96, false, key::BACKTICK },
		{ 123, true, key::LEFT_BRACKET },
		{ 124, true, key::BACKSLASH },
		{ 125, true, key::RIGHT_BRACKET },
		{ 126, true, key::BACKTICK },

		// @@ TODO fill this out some more
		{ 0, false, key::INVALID }	};


	for (int i = 0; table[i].code != 0; i++)
	{
		if (key == table[i].code)
		{
			c = table[i].gs;
			get_root()->m_shift_key_state = table[i].shift;	// hack
			break;
		}
	}

	return c;
}

static key::code	translate_key_ctrl(SDL_Keycode key)
	// For forwarding SDL key events to bakeinflash.
{
	key::code	c(key::INVALID);

	if (key >= SDLK_0 && key <= SDLK_9)
	{
		c = (key::code) ((key - SDLK_0) + key::_0);
	}
	else if (key >= SDLK_a && key <= SDLK_z)
	{
		c = (key::code) ((key - SDLK_a) + key::A);
	}
	//	else if (key >= SDLK_F1 && key <= SDLK_F15)
	//	{
	//		c = (key::code) ((key - SDLK_F1) + key::F1);
	//	}
	/*	else if (key >= SDLK_WORLD_0 && key <= SDLK_WORLD_95)
	{
	// International keyboard syms
	if (key >= SDLK_WORLD_32)
	{
	c = (key::code) ((key - SDLK_WORLD_32) + key::RUS_a1);
	}
	else
	{
	c = (key::code) ((key - SDLK_WORLD_0) + key::RUS_1);
	}
	}*/
	else
	{
		// many keys don't correlate, so just use a look-up table.
		struct
		{
			SDL_Keycode	sdlk;
			key::code	gs;
		} table[] =
		{
			{ SDLK_RETURN, key::ENTER },
			{ SDLK_ESCAPE, key::ESCAPE },
			{ SDLK_LEFT, key::LEFT },
			{ SDLK_UP, key::UP },
			{ SDLK_RIGHT, key::RIGHT },
			{ SDLK_DOWN, key::DOWN },
			{ SDLK_SPACE, key::SPACE },
			{ SDLK_PAGEDOWN, key::PGDN },
			{ SDLK_PAGEUP, key::PGUP },
			{ SDLK_HOME, key::HOME },
			{ SDLK_END, key::END },
			{ SDLK_INSERT, key::INSERT },
			{ SDLK_DELETE, key::DELETEKEY },
			{ SDLK_BACKSPACE, key::BACKSPACE },
			{ SDLK_TAB, key::TAB },
			{ SDLK_RSHIFT, key::SHIFT },
			{ SDLK_LSHIFT, key::SHIFT },
			{ SDLK_PERIOD, key::PERIOD },
			{ SDLK_SLASH, key::SLASH },
			{ SDLK_BACKSLASH, key::BACKSLASH },
			{ SDLK_SEMICOLON, key::SEMICOLON },
			{ SDLK_QUOTE, key::QUOTE },
			{ SDLK_LEFTBRACKET, key::LEFT_BRACKET },
			{ SDLK_RIGHTBRACKET, key::RIGHT_BRACKET },
			{ SDLK_COMMA, key::COMMA },
			{ SDLK_LCTRL, key::CONTROL },
			{ SDLK_RCTRL, key::CONTROL },

			// @@ TODO fill this out some more
			{ SDLK_UNKNOWN, key::INVALID }
		};

		for (int i = 0; table[i].sdlk != SDLK_UNKNOWN; i++)
		{
			if (key == table[i].sdlk)
			{
				c = table[i].gs;
				break;
			}
		}
	}
	return c;
}


static void	fs_callback(character* movie, const char* command, const char* args)
	// For handling notification callbacks from ActionScript.
{
	assert(movie);

	if (stricmp(command, "fullscreen") == 0)
	{
		// TODO
	}
	else
		if (stricmp(command, "exit") == 0)
		{
			s_go = false;
		}
		else
			if (stricmp(command, "set_max_volume") == 0)
			{
				// set max sound volume in percent, [0..100]
				// usefull for embedded games
				sound_handler* s = get_sound_handler();
				if (s)
				{
					int vol = atoi(args);
					s->set_max_volume(vol);
				}
			}
			else
				if (stricmp(command, "notify_keydown") == 0)
				{
					Uint16 ch = utf8::decode_next_unicode_character(&args);
					key::code c = translate_key(ch);
					if (c != key::INVALID)
					{
						notify_key_event(c, c, true);
					}
				}
				else
					if (stricmp(command, "notify_keyup") == 0)
					{
						Uint16 ch = utf8::decode_next_unicode_character(&args);
						key::code c = translate_key(ch);
						if (c != key::INVALID)
						{
							notify_key_event(c, c, false);
						}
					}
					else
						if (stricmp(command, "set_delay") == 0)
						{
							// set the number of milli-seconds to delay in main loop
							int delay = atoi(args);
							// sanity check
							if (delay >= 0 && delay <= 1000)
							{
								s_delay = delay;
							}
						}
						else
							if (stricmp(command, "clear_events") == 0)
							{
								// clear queue of system events (mouse events, keyboard events, etc)
								SDL_Event	event;
								if (SDL_PollEvent(&event) == 0)
								{
									return;
								}
							}
							else
								if (stricmp(command, "set_workdir") == 0)
								{
									set_workdir(args);
								}
								else
									if (stricmp(command, "fps") == 0)
									{
										root* rm = get_root();
										int fps = atoi(args);
										rm->set_frame_rate((float)fps);
									}
									else
										if (stricmp(command, "resetSound") == 0)
										{
											delete sh;
#ifdef TU_USE_SDL_SOUND
											sh = create_sound_handler_sdl();
#else
											sh = create_sound_handler_openal();
#endif
											set_sound_handler(sh);	
											printf("reset sound handler\n");
										}
										else
											if (stricmp(command, "setWindowMinimumSize") == 0)
											{
												tu_string s = args;
												array<double> a;
												s.split(',', &a);
												if (a.size() >= 2 && a[0] >0 && a[1] > 0)
												{
													SDL_SetWindowMinimumSize(s_screen, (int) a[0], (int) a[1]);
												}
											}

}

void	print_usage()
	// Brief instructions.
{
	myprintf(
		"usage: bakeinflash [options] movie_file.swf\n"
		"\n"
		"Plays a SWF (Shockwave Flash) movie using OpenGL\n"
		"\n"
		"options:\n"
		"\n"
		"  -h          Print this info.\n"
		"  -r          Set Flash varibales, sample: myvar1=value1,myvar2=value2\n"
		"  -f          fullscreen mode\n"
		"  -k <mode>   cursor mode, 0=none, 1=system, 2=internal\n"
		"  -x          Disable/Enable Esc\n"
		"  -a          Set antialiasing , 0=none, enable FSAA: 2,4\n"
		"  -v          Be verbose; i.e. print log messages to stdout\n"
		"  -va         Be verbose about movie Actions\n"
		"  -vp         Be verbose about parsing the movie\n"
		"  -vh         Be verbose about heap size\n"
		"  -p          Preload bitmaps\n"
		"  -l          Keep bitmaps alive\n"
		"  -ts <port>  Use touch screen connected to <port>\n"
		"  -e          Export to .flv\n"
		"  -d          delay main loop\n"
		"  -w          title\n"
		"\n"
		);
}

// hide terminal
#if defined(NDEBUG) && defined(WIN32)
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

int	main(int argc, char *argv[])
{
	assert(tu_types_validate());

	const char* infile = NULL;
	int	sdl_cursor = 1;		// 1=system, 0=none, 2=internal
	bool	esc_enabled = true;
	tu_string flash_vars;
	bool preload_bitmaps = false;
	bool keep_alive_bitmaps = false;

	int flv_total_frames = 0;
	int flv_current_frame = 0;

	// by default it's used the simplest and the fastest edge antialiasing method
	// if you have modern video card you can use full screen antialiasing
	// full screen antialiasing level may be 2,4, ...
	// static int s_aa_level = 4;	// for windows
	//static int s_aa_level = 0;
	int antialiased = 1;

	tu_string elo_port;
	tu_string title = "bakeinflash";

#if defined(__APPLE__) && !defined(iOS)
	infile = "bevel.swf"; //"tunnel.swf";
#else
	for (int arg = 1; arg < argc; arg++)
	{
		if (argv[arg][0] == '-')
		{
			// Looks like an option.
			switch (argv[arg][1])
			{
			case 'h':
				// Help.
				print_usage();
				exit(1);
				break;

			case 'r':
				// Set flash_vars
				// myvar1=value1,myvar2=value2,myvar3=value3,...
				arg++;
				if (arg < argc)
				{
					flash_vars = argv[arg];
				}
				else
				{
					myprintf("-r arg must be followed by flash variables, e.g -r a=b,c=d\n");
					print_usage();
					exit(1);
				}
				break;

			case 'k':
				// disable/enable cursor
				arg++;
				if (arg < argc)
				{
					sdl_cursor = atoi(argv[arg]);
				}
				else
				{
					myprintf("-k arg must be followed by 0 or 1 or 2 to set cursor\n");
					print_usage();
					exit(1);
				}
				break;

			case 'd':
				// delay time
				arg++;
				if (arg < argc)
				{
					s_delay = atoi(argv[arg]);
				}
				else
				{
					myprintf("-d arg must be followed by time in ms\n");
					print_usage();
					exit(1);
				}
				break;

			case 'p':
				// preload_bitmaps
				arg++;
				if (arg < argc)
				{
					preload_bitmaps = atoi(argv[arg]) ? true : false;
				}
				else
				{
					myprintf("-p arg must be followed by 0 or 1 to disable/enable preload bitmaps\n");
					print_usage();
					exit(1);
				}
				break;

			case 'l':
				// keep_alive_bitmaps
				arg++;
				if (arg < argc)
				{
					keep_alive_bitmaps = atoi(argv[arg]) ? true : false;
				}
				else
				{
					myprintf("-l arg must be followed by 0 or 1 to disable/enable keep bitmaps alive\n");
					print_usage();
					exit(1);
				}
				break;

			case 'x':
				// disable/enable Esc
				arg++;
				if (arg < argc)
				{
					esc_enabled = atoi(argv[arg]) ? true : false;
				}
				else
				{
					myprintf("-k arg must be followed by 0 or 1 to disable/enable Esc\n");
					print_usage();
					exit(1);
				}
				break;

			case 'f':
				// fullscreen
				arg++;
				if (arg < argc)
				{
					fullscreen_mode = atoi(argv[arg]);
				}
				else
				{
					myprintf("-f arg must be followed by 0/1/2 to disable/enable fullscreen\n");
					print_usage();
					exit(1);
				}
				break;

			case 'a':
				// Set antialiasing on or off.
				arg++;
				if (arg < argc)
				{
					antialiased = atoi(argv[arg]);
				}
				else
				{
					myprintf("-a arg must be followed by 0 or 1 to disable/enable antialiasing\n");
					print_usage();
					exit(1);
				}
				break;

			case 'e':	// export flv
#ifdef USE_MUXER
				arg++;
				if (arg < argc)
				{
					flv_total_frames = atoi(argv[arg]);
				}
				else
				{
					myprintf("-e arg must be followed by frames amount\n");
					print_usage();
					exit(1);
				}

				s_muxer = new muxer();
#endif
				break;

			case 'v':
				// Be verbose; i.e. print log messages to stdout.
				if (argv[arg][2] == 'a')
				{
					// Enable spew re: action.
					bakeinflash::set_verbose_action(true);
				}
				else if (argv[arg][2] == 'p')
				{
					// Enable parse spew.
					bakeinflash::set_verbose_parse(true);
				}
				else if (argv[arg][2] == 'h')
				{
					// Enable parse heap size.
					bakeinflash::set_verbose_heap(true);
				}
				break;

			case 't':
				if (argv[arg][2] == 's')
				{
					arg++;
					elo_port = argv[arg];
				}
				break;

			case 'w':
				// Set antialiasing on or off.
				arg++;
				if (arg < argc)
				{
					title = argv[arg];
				}
				else
				{
					myprintf("-w arg must be followed by window name\n");
					print_usage();
					exit(1);
				}
				break;

			default:
				myprintf("invalid arg %c\n", argv[arg][1]);
				for (int i = 0; i < argc; i++)
				{
					printf("arg %d: %s\n", i, argv[i]);
				}
				exit(0);
				break;
			}
		}
		else
		{
			infile = argv[arg];
			break;
		}
	}
#endif

	if (infile == NULL)
	{
		myprintf("no input file\n");
		print_usage();
		exit(1);
	}

	bakeinflash::preload_bitmaps(preload_bitmaps);
	bakeinflash::keep_alive_bitmaps(keep_alive_bitmaps);
	bakeinflash::set_display_invisibles(false);
	bakeinflash::set_yuv2rgb_table(true);

	float	tex_lod_bias;

#ifdef _WIN32

	WSADATA wsaData;

	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
		myprintf("Error at WSAStartup()\n");
#endif


	// -1.0 tends to look good.
	tex_lod_bias = 0; //-1.2f;

	int	width = 0;
	int	height = 0;

	init_player();
	add_class("validator", as_validator_ctor);
//	add_class("embossFilter", as_embossFilter_ctor);
//	add_class("Tunnel3D", as_tunnel_ctor);

	const char* startup_file = argv[0];

	// Find last slash or backslash.
	const char* ptr = startup_file + strlen(startup_file);
	for (; ptr >= startup_file && *ptr != PATH_SLASH; ptr--) {}
	int len = ptr - startup_file + 1;

	tu_string startdir = tu_string(startup_file, len);
	if (startdir == PATH_CURDIR)
	{
		char currentPath[FILENAME_MAX];
		getcwd(currentPath, sizeof(currentPath));
		startdir = currentPath;
		startdir += PATH_SLASH;
	}

	set_startdir(startdir);

	// use this for multifile games
	// workdir is used when LoadMovie("myfile.swf", _root) is called
	tu_string swfile;
	tu_string workdir;
	if (infile)
	{
		// Find last slash
		const char* ptr = infile + strlen(infile);
		for (; ptr >= infile && *ptr != '/'; ptr--) {}
		// Use everything up to last slash as the "workdir".
		int len = ptr - infile + 1;
		if (len > 0)
		{
			swfile = ptr + 1;
			workdir = tu_string(infile, len);
			set_workdir(workdir.c_str());
		}
		else
		{
			// then  try backslash.
			const char* ptr = infile + strlen(infile);
			for (; ptr >= infile && *ptr != '\\'; ptr--) {}
			// Use everything up to last slash as the "workdir".
			int len = ptr - infile + 1;
			if (len > 0)
			{
				swfile = ptr + 1;
				workdir = tu_string(infile, len);
				set_workdir(workdir.c_str());
			}
		}


		if (workdir.size() == 0)
		{
			workdir = startdir;
#ifndef WIN32
			char lastbyte = workdir[workdir.size() - 1];
			if (lastbyte != PATH_SLASH)
			{
				workdir += PATH_SLASH;
			}
#endif
			set_workdir(workdir.c_str());
			swfile = infile;
		}
	}

	// OSX
#if defined(__APPLE__) && !defined(iOS)
	{
		workdir = startdir;
		const char* ptr = workdir.c_str() + workdir.size() - 2;
		for (; ptr >= workdir.c_str() && *ptr != '/'; ptr--) {}
		// Use everything up to last slash as the "workdir".
		int len = ptr - workdir.c_str() + 1;
		if (len > 0)
		{
			tu_string wd = tu_string(workdir.c_str(), len);
			wd += "Resources/";
			workdir = wd;
			set_workdir(workdir.c_str());

			startdir = wd;
			set_startdir(startdir);
		}
	}
#endif


	myprintf("program dir: '%s'\n", startdir.c_str());
	myprintf("work dir: '%s'\n", workdir.c_str());
	myprintf("swfile: '%s'\n", swfile.c_str());
	myprintf("antialiased: %d\n", antialiased);

	register_file_opener_callback(file_opener);
	register_fscommand_callback(fs_callback);
	register_log_callback(log_callback);

#ifdef TU_USE_SDL_SOUND
	sh = create_sound_handler_sdl();
#else
	sh = create_sound_handler_openal();
#endif
	set_sound_handler(sh);

	rh = create_render_handler_ogles(true);
	set_render_handler(rh);

#if TU_CONFIG_LINK_TO_FREETYPE == 1
	set_glyph_provider(create_glyph_provider_freetype());
#else
	set_glyph_provider(create_glyph_provider_tu());
#endif


	// decrease memory usage
	use_cached_movie(false);

	if (infile == NULL)
	{
		myprintf("usage: bakeinflash myfile.swf\n");
		return -1;
	}

	set_flash_vars(flash_vars);
	//float scale = 0;
	int x0 = 0;
	int y0 = 0;

	smart_ptr<root>	m = load_file(swfile.c_str());
	if (m == NULL)
	{
		myprintf("could not load %s\n", swfile.c_str());
		delete sh;
		delete rh;
		return -1;
	}

	tu_string finame = get_workdir();
	finame += "kbd.swf";
	s_has_virtual_keyboard = exist(finame.c_str());

	if (width == 0 || height == 0)
	{
		width = m->get_movie_width();
		height = m->get_movie_height();
	}

	// Initialize the SDL subsystems we're using. Linux
	// and Darwin use Pthreads for SDL threads, Win32
	// doesn't. Otherwise the SDL event loop just polls.
	//  Other flags are SDL_INIT_JOYSTICK | SDL_INIT_CDROM
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
	{
		myprintf("Unable to init SDL: %s\n", SDL_GetError());
		delete sh;
		delete rh;
		exit(1);
	}

	atexit(SDL_Quit);

	int bit_depth = 24;
	switch (bit_depth)
	{
	case 16:
		// 16-bit color, surface creation is likely to succeed.
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 15);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 5);
		break;

	case 24:
		// 24-bit color
		//	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
		//	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
		//	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
		//	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
		//	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);
		break;

	case 32:
		// 32-bit color etc, for getting dest alpha,
		// for MULTIPASS_ANTIALIASING (see bakeinflash_render_handler_ogl.cpp).
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
		break;

	default:
		assert(0);
	}

	// try to enable FSAA
	if (antialiased > 1)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, antialiased);
	}

	// Change the LOD BIAS values to tweak blurriness.
	if (tex_lod_bias != 0.0f)
	{
#ifdef FIX_I810_LOD_BIAS	
		// If 2D textures weren't previously enabled, enable
		// them now and force the driver to notice the update,
		// then disable them again.
		if (!glIsEnabled(GL_TEXTURE_2D)) {
			// Clearing a mask of zero *should* have no
			// side effects, but coupled with enbling
			// GL_TEXTURE_2D it works around a segmentation
			// fault in the driver for the Intel 810 chip.
			glEnable(GL_TEXTURE_2D);
			glClear(0);
			glDisable(GL_TEXTURE_2D);
		}
#endif // FIX_I810_LOD_BIAS
		glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT, GL_TEXTURE_LOD_BIAS_EXT, tex_lod_bias);
	}

	int movie_width = m->get_movie_width();
	int movie_height = m->get_movie_height();
	screen_height = movie_height;
	screen_width = movie_width;

	Uint32 flags = SDL_WINDOW_OPENGL;
	flags |= SDL_WINDOW_RESIZABLE;

	// Set the video mode.
	SDL_DisplayMode dm;
	int rc = SDL_GetCurrentDisplayMode(0, &dm);
	assert(rc == 0);

	int ndisplay = SDL_GetNumVideoDisplays();

	// fullscreen ?
	switch (fullscreen_mode)
	{
	case 1:
		screen_height = dm.h;
		screen_width = dm.w;
		flags |= SDL_WINDOW_BORDERLESS;
		break;

	case 2:
		screen_height = dm.h;
		screen_width = dm.w;
		flags |= SDL_WINDOW_FULLSCREEN;
		break;

	case 3:
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

		// hack
		if (ndisplay == 1)
		{
			screen_height = dm.h;
			screen_width = dm.w;
		}
		break;

	case 4:
		flags |= SDL_WINDOW_RESIZABLE;
		break;

	default:
		break;
	}

	float scale_x = (float)screen_width / movie_width;
	float scale_y = (float)screen_height / movie_height;
	//	scale = fmin(scale_x, scale_y);


	// move movie to center
	int view_width = width; //(int)(width * scale);
	int view_height = height; // (int)(height * scale);
	x0 = (screen_width - view_width) >> 1;
	y0 = (screen_height - view_height) >> 1;

	s_screen = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screen_width, screen_height, flags);
	if (s_screen == NULL)
	{
		myprintf("SDL_SetVideoMode() failed, %s\n", SDL_GetError());
		exit(1);
	}

	// descktop fullscreen
	// hack
	if (fullscreen_mode == 3)
	{
		SDL_SetWindowFullscreen(s_screen, 0);
		SDL_SetWindowPosition(s_screen, 0, 0);
	}

	SDL_GLContext glcontext = SDL_GL_CreateContext(s_screen); // создаем контекст OpenGL

	SDL_ShowCursor(sdl_cursor == 1 ? SDL_ENABLE : SDL_DISABLE);
	//	SDL_EnableUNICODE(true);
	//	SDL_EnableKeyRepeat(250, 33);

	rh->open();
	rh->set_antialiased(antialiased > 0);

	// Mouse state.
	int	mouse_x = 0;
	int	mouse_y = 0;
	int	mouse_buttons = 0;

	//			float	speed_scale = 1.0f;
	Uint32	start_ticks = 0;
	start_ticks = tu_timer::get_ticks();

	Uint32	last_ticks = start_ticks;
	//			int	frame_counter = 0;
	//			int	last_logged_fps = last_ticks;
	//			int fps = 0;

	// must be after swf loaded
	smart_ptr<elo> elots;
	if (elo_port.size() > 0)
	{
		elots = new elo();
		bool rc = elots->open("", elo_port);
		//		if (rc == false)
		//		{
		//			elots = NULL;
		//		}
	}

#ifdef USE_MUXER
	if (s_muxer != NULL)
	{
		int fps = (int)m->get_frame_rate();
		tu_string fi = tu_string(infile, strlen(infile) - 3);
		fi += "flv";
		if (s_muxer->begin_session(fi, 0, 0, 0, fps, width, height))
		{
			m->set_background_color(rgba(0, 0, 0, 0));
		}
		else
		{
			printf("couldn't create video\n");
			s_muxer = NULL;
		}
	}
#endif

	int frames = 0;
	bool window_has_mouse_focus = false;
	Uint32	prev_frames_ticks = tu_timer::get_ticks();

	int sdl_mstate = 0;
	int sdl_mx = 0;
	int sdl_my = 0;
	bool is_key_repeat = false;
	Uint16 utf16char = 0;

	while (s_go == true)
	{
		Uint32	ticks = tu_timer::get_ticks();
		int	delta_ticks = ticks - last_ticks;
		float	delta_t = delta_ticks / 1000.f;

		frames++;
		if (ticks - prev_frames_ticks >= 1000)
		{
			s_real_fps = (int)(frames * 1000.0f / (ticks - prev_frames_ticks));
			prev_frames_ticks = ticks;
			frames = 0;
			//					printf("fps=%d\n", real_fps);
		}

		bool ret = true;
		SDL_Event	event;

		// Handle input.
		while (ret)
		{
			if (SDL_PollEvent(&event) == 0)
			{
				break;
			}

			switch (event.type)
			{

			case SDL_MOUSEMOTION:
				window_has_mouse_focus = true;
				break;

			case SDL_WINDOWEVENT:
				{
					switch (event.window.event)
					{
					case SDL_WINDOWEVENT_ENTER:
						// Window has gained mouse focus
						window_has_mouse_focus = true;
						break;

					case SDL_WINDOWEVENT_LEAVE:
						// Window has lost mouse focus
						window_has_mouse_focus = false;
						break;

					case SDL_WINDOWEVENT_RESIZED:
						{
							screen_width = event.window.data1;
							screen_height = event.window.data2;
							view_width = screen_width;
							view_height = screen_height;

							scale_x = (float)screen_width / movie_width;
							scale_y = (float)screen_height / movie_height;

							as_value val;
							get_global()->get_member("Stage", &val);
							as_stage* stage = cast_to<as_stage>(val.to_object());
							if (stage)
							{
								stage->set_member("xScale", scale_x);
								stage->set_member("yScale", scale_y);

								// set width/height before call
								m = get_root();
								m->set_display_viewport(x0, y0, view_width, view_height);

								stage->on_resize();
							}
							break;
						}
					}
					break;
				}

			case SDL_QUIT:
				goto done;
				break;

			case SDL_TEXTINPUT:
				{
					if (is_key_repeat)
					{
						break;
					}

					const char* a = event.text.text;
					utf16char = utf8::decode_next_unicode_character(&a);
					key::code c = translate_key(utf16char);
					//		printf("SDL_TEXTINPUT %d\n", utf16char);
					if (c != key::INVALID)
					{
						notify_key_event(c, utf16char, true);
						//					notify_key_event(c, utf16char, false);
					}
					break;
				}

			case SDL_KEYDOWN:
				{
					is_key_repeat = event.key.repeat != 0;
					if (is_key_repeat)
					{
						break;
					}

					SDL_Keycode	key = event.key.keysym.sym;
					if (key == SDLK_ESCAPE && esc_enabled)
					{
						goto done;
					}

					if (key == SDLK_LCTRL || key == SDLK_RCTRL)
					{
						notify_key_event(key::CONTROL, key::CONTROL, true);
						break;
					}
					else
						if (key == SDLK_LALT || key == SDLK_RALT)
						{
							notify_key_event(key::ALT, key::ALT, true);
							break;
						}

						//  01x=SHIFT,      02x=rifgth SHIFT
						//  40x=CTRRL,      80x=rigth CTRLL
						// 100xx=left ALT, 200x=rigth ALT
						Uint16 mod = event.key.keysym.mod;
						if ((mod & 0xFF0)	// alt or ctrl
							|| (key == SDLK_BACKSPACE || key == SDLK_TAB
							|| key == SDLK_RIGHT || key == SDLK_LEFT
							|| key == SDLK_HOME || key == SDLK_END || key == SDLK_DELETE || key == SDLK_RETURN))
						{
							//	printf("down %X, mod=%X\n", key, mod);
							key::code c = translate_key_ctrl(key);
							if (c != key::INVALID)
							{
								//				printf("notify down %d\n", c);
								notify_key_event(c, c, true);
							}
						}
						break;
				}

			case SDL_KEYUP:
				{
					if (utf16char != 0)
					{
						key::code c = translate_key(utf16char);
						//	printf("SDL_TEXTINPUT up %d\n", utf16char);
						if (c != key::INVALID)
						{
							notify_key_event(c, utf16char, false);
						}
						utf16char = 0;
						break;
					}

					SDL_Keycode	key = event.key.keysym.sym;
					if (key == SDLK_LCTRL || key == SDLK_RCTRL)
					{
						notify_key_event(key::CONTROL, key::CONTROL, false);
						break;
					}
					else
						if (key == SDLK_LALT || key == SDLK_RALT)
						{
							notify_key_event(key::ALT, key::ALT, false);
							break;
						}

						Uint16 mod = event.key.keysym.mod;
						if ((mod & 0xFF0)	// alt or ctrl
							|| (key == SDLK_BACKSPACE || key == SDLK_TAB
							|| key == SDLK_RIGHT || key == SDLK_LEFT
							|| key == SDLK_HOME || key == SDLK_END || key == SDLK_DELETE || key == SDLK_RETURN))
						{
							//	printf("up %X, mod=%X\n", key, mod);
							key::code c = translate_key_ctrl(key);
							if (c != key::INVALID)
							{
								//			printf("notify up %d\n", c);
								notify_key_event(c, c, false);
							}
						}
						break;
				}

			default:
				break;
			}
		}

		m = get_root();
		m->set_display_viewport(x0, y0, view_width, view_height);

		// handle sdl mouse
		int mx, my, mstate;
		mstate = SDL_GetMouseState(&mx, &my);		// currect
		mx = (int)((mx - x0) / scale_x);
		my = (int)((my - y0) / scale_y);
		if (sdl_mx != mx || sdl_my != my || sdl_mstate != mstate)
		{
			// mousemove for touch screen
			// fixme for scroller
			if (elots != NULL && sdl_mstate != mstate)
			{
				m->notify_mouse_state(mx, my, mouse_buttons);
				m->advance(delta_t);
			}

			sdl_mx = mx;
			sdl_my = my;
			sdl_mstate = mstate;
			mouse_x = sdl_mx;
			mouse_y = sdl_my;
			mouse_buttons = sdl_mstate;
			m->notify_mouse_state(mouse_x, mouse_y, mouse_buttons);
		}

		// handle elo events
		if (elots != NULL && elots->read(width, height, &mx, &my, &mstate))
		{
			// mouse move
			mouse_x = mx;
			mouse_y = my;
			m->notify_mouse_state(mouse_x, mouse_y, mouse_buttons);
			m->advance(delta_t);

			//	printf("elo: x=%d y=%d b=%d\n", mx, my, mstate);
			mouse_buttons = mstate;
			m->notify_mouse_state(mouse_x, mouse_y, mouse_buttons);
		}

		//					myidash->advance(delta_t);
		Uint32 t_advance = tu_timer::get_ticks();
		m->advance(delta_t);
		t_advance = tu_timer::get_ticks() - t_advance;

		Uint32 t_display = tu_timer::get_ticks();
		m->display();
		t_display = tu_timer::get_ticks() - t_display;

		//draw_logo(m);

		if (sdl_cursor == 2 && window_has_mouse_focus)
		{
			rh->show_cursor(mouse_x, mouse_y);
		}

		Uint32 t_draw = tu_timer::get_ticks();
		SDL_GL_SwapWindow(s_screen);
		t_draw = tu_timer::get_ticks() - t_draw;
		last_ticks = ticks;

		// write video frame
#ifdef USE_MUXER
		if (s_muxer != NULL)
		{
			int vBufferSize = width * height * 4;
			Uint8* buf = (Uint8*) malloc(vBufferSize);
			glReadBuffer(GL_FRAMEBUFFER_EXT);
			glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buf);

			s_muxer->write_image(buf, vBufferSize, NULL, 0);
			free(buf);

			flv_current_frame++;
			if (flv_current_frame == flv_total_frames)
			{
				// flv is ready
				break;
			}
		}
#endif

		// Don't hog the CPU.
		if (s_delay > 0)
		{
			// sleep usually guarantees a minimal sleep time.
			tu_timer::sleep(s_delay);
		}

		// for perfomance testing
		//myprintf("loop time: %d, swapBuffer time: %d, advance time: %d, display time %d\n",	tu_timer::get_ticks() - ticks, t_draw, t_advance, t_display);

	}

done:

	SDL_Quit();

#ifdef USE_MUXER
	s_muxer = NULL;
#endif
	m = NULL;
	elots = NULL;

	clear_player();

	set_sound_handler(NULL);
	delete sh;

	set_render_handler(NULL);
	delete rh;

	return 0;
}

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
