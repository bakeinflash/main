#include "bakeinflash/bakeinflash.h"
#include <stdlib.h>
#include <stdio.h>
#include "base/utility.h"
#include "base/container.h"
#include "base/tu_file.h"
#include "base/tu_types.h"
#include "base/tu_timer.h"
#include "bakeinflash/bakeinflash_types.h"
#include "bakeinflash/bakeinflash_impl.h"
#include "bakeinflash/bakeinflash_root.h"
#include "bakeinflash/bakeinflash_freetype.h"
#include "bakeinflash/bakeinflash_object.h"
#include "bakeinflash/bakeinflash_text.h"
#include "bakeinflash/bakeinflash_sound_handler_openal.h"


#import <UIKit/UIKit.h>

static int	s_width = 0;
static int	s_height = 0;
float s_scale = 1;	// extern
int s_x0 = 0;
int s_y0 = 0;
float s_retina;
static int s_view_width = 0;
static int s_view_height = 0;
static Uint32	start_ticks = 0;
static Uint32	last_ticks = 0;
static tu_string flash_vars;
int s_real_fps = 0;
bool s_has_virtual_keyboard = false;

//static int iAppSize = 0;
//static char iApp[1024*1024*8] = "sssssssssssssssss";

using namespace bakeinflash;

static smart_ptr<as_object> thisPtr;
static smart_ptr<root>	m;

sound_handler*	sh = NULL;

bool exist(const char* file);


@interface bakeinFlash : NSObject <UIApplicationDelegate>
+ (const char*) getWorkdir;
+ (const char*) getInfile;
+ (float) getActivityPos;
+ (int) getActivityStyle;
+ (bool) getBitmapAlive;
+ (void) setAutoOrientation :(bool) val;
+ (bool) getAutoOrientation;
+ (bool) isPortrait;
+ (const char*) getMemorySWF :(const unsigned char**) ptr :(int*) len;
+ (bool) getFont :(int) i :(const char**) fontname :(const char**) filename :(const unsigned char**) ptr :(int*) len;
+ (bool) getClass :(int) i :(const char**) classname : (bakeinflash::as_c_function_ptr*) ctor;
+ (bool) getMSAA;
+ (void) setMSAA :(bool) val;
+ (bool) getShowAdvance;
+ (const char*) getFlashVars;
+ (void) setFlashVars :(NSString*) str;
+ (int) getFitMode;
+ (void) setFitMode :(int) mode;

@end


typedef void (*bakeinflash_fscommand_callback)(const char* command, const char* arg);
static bakeinflash_fscommand_callback s_bakeinflash_fscommand_callback = NULL;
void bakeinflash_register_fscommand_callback(bakeinflash_fscommand_callback handler)
{
	s_bakeinflash_fscommand_callback = handler;
}

static void	message_log(const char* message)
// Process a log message.
{
	if (get_verbose_parse())
	{
		fputs(message, stdout);
		fflush(stdout);
	}
}

static void	log_callback(bool error, const char* message)
// Error callback for handling bakeinflash messages.
{
	if (error)
	{
		// Log, and also print to stderr.
		message_log(message);
		fputs(message, stderr);
		fflush(stderr);
	}
	else
	{
		message_log(message);
	}
}

static string_hash< const unsigned char* > s_file;
static string_hash< int > s_file_size;
static string_hash<tu_string> s_file_map;

void bakeinflash_setfile(const tu_string& name, const unsigned char* buf, int bufsize)
{
	if (bufsize > 0)
	{
		s_file[name] = buf;
		s_file_size[name] = bufsize;
    printf("add memory file: %s\n", name.c_str());
	}
	else
	{
		s_file_map[name] = (char*) buf;		// file name
	}
}


bool get_memoryfile(const char* url, Uint8** ptr, int* length)
// Callback function.  This opens files for the bakeinflash library.
{
	if (s_file.get(url, NULL))
	{
		*ptr = (Uint8*) s_file[url];
		*length = s_file_size[url];
	//	printf("get_memoryfile: %s\n", url);
		return true;
	}
//	printf("no memory file: %s\n", url);
	return false;
}

static tu_file*	file_opener(const char* url)
// Callback function.  This opens files for the bakeinflash library.
{
	if (s_file.get(url, NULL))
	{
		return new tu_file(tu_file::memory_buffer, s_file_size[url], (void*) s_file[url]);
	}
	
	tu_string filename = get_workdir();
	tu_string val;
	if (s_file_map.get(url, &val))
	{
		filename += val;
	}
	else
	{
		filename += url;
	}
	
	//printf("external file: %s\n", filename.c_str());
	if (access(filename.c_str(), F_OK) == 0)
	{
		return new tu_file(filename.c_str(), "rb");
	}
	
	// try .dat
	if (filename.size() > 4)
	{
		filename = filename.utf8_substring(0, filename.size() - 4) + tu_string(".dat");
		return new tu_file(filename.c_str(), "rb");
	}
	return NULL;
}

static void	fs_callback(character* movie, const char* command, const char* args)
// For handling notification callbacks from ActionScript.
{
	assert(movie);
    
    tu_string cmd = command;
    if (cmd == "close")
    {
        exit(0);
    }
    
	if (s_bakeinflash_fscommand_callback)
	{
		(*s_bakeinflash_fscommand_callback)(command, args);
	}
}

void	bakeinflash_add_class(const tu_string& name, const as_value& ctor)
{
	add_class(name, ctor);
}

void bakeinflash_clear()
{
	for (string_hash<const unsigned char*>::iterator it = s_file.begin(); it != s_file.end(); ++it)
	{
		const unsigned char* buf = it->second;
		free((void*) buf);
	}
	s_file.clear();
	s_file_size.clear();
}

void bakeinflash_keep_bitmaps_alive(bool alive)
{
	bakeinflash::keep_alive_bitmaps(alive);
}

void bakeinflash_init(const char* workdir, const char* infile, int w, int h, float retina, bool hasES2)
{
	register_file_opener_callback(file_opener);
	register_fscommand_callback(fs_callback);
	if (get_verbose_parse())
	{
		register_log_callback(log_callback);
	}
	
	sh = create_sound_handler_openal();
	set_sound_handler(sh);
	
//	bakeinflash::render_handler*	myrender = es2 ? create_render_handler_ogles2() : create_render_handler_ogles();
	bakeinflash::render_handler*	myrender = create_render_handler_ogles(hasES2);
  myrender->open();
	set_render_handler(myrender);
	
    init_player();

	set_workdir(workdir);
    set_startdir(workdir);

  
  tu_string finame = get_workdir();
  finame += "kbd.swf";
  tu_file fi(finame.c_str(), "r");
  if (fi.get_error() == TU_FILE_NO_ERROR)
  {
    s_has_virtual_keyboard = true;
  }
  
	//	set_glyph_provider(create_glyph_provider_tu());
	set_glyph_provider(create_glyph_provider_freetype());
	
	m = load_file(infile);
	if (m == NULL)
	{
		printf("can't open input file %s", infile);
		return;
	}

	s_width = w;
	s_height = h;
	
	s_retina = retina;
	float scalex = (float) s_width / m->get_movie_width();
	float scaley = (float) s_height / m->get_movie_height();
  
  int fit_mode = [bakeinFlash getFitMode];
  switch (fit_mode)
  {
    case 1:
      s_scale = fmax(scalex, scaley);
      break;
      
    default:
      s_scale = fmin(scalex, scaley);
      break;
  }
	
	// move movie to center
	s_view_width =  m->get_movie_width() * s_scale;
	s_view_height =  m->get_movie_height() * s_scale;
	s_x0 =  (s_width - s_view_width) / 2;
	s_y0 =  (s_height - s_view_height) / 2;
	
	m->set_display_viewport(s_x0, s_y0, s_view_width, s_view_height);
	printf("screen: %dx%d, movie: %dx%d, x0y0: %dx%d, scale: %f\n", s_width, s_height, s_view_width, s_view_height, s_x0, s_y0, s_scale);
	
  bakeinflash::set_display_invisibles(false);
  
	start_ticks = tu_timer::get_ticks();
	last_ticks = start_ticks;
}

void onMouseDown(float x, float y)
{
	m->notify_mouse_state((x - s_x0) / s_scale, (y - s_y0) / s_scale, 1);
}

void onMouseUp(float x, float y)
{
	m->notify_mouse_state((x - s_x0) / s_scale, (y - s_y0) / s_scale, 0);
}

void onMouseMove(float x, float y, int flg)
{
	m->notify_mouse_state((x - s_x0) / s_scale, (y - s_y0) / s_scale, flg);
}

int frames = 0;
Uint32	prev_frames_ticks = tu_timer::get_ticks();
void bakeinflash_advance(bool profile)
{
	Uint32	ticks = tu_timer::get_ticks();
	int	delta_ticks = ticks - last_ticks;
	float	delta_time = delta_ticks / 1000.f;
  
  frames++;
	if (ticks - prev_frames_ticks >= 1000)
   {
     s_real_fps = (int) (frames * 1000.0f / (ticks - prev_frames_ticks));
     prev_frames_ticks = ticks;
     frames = 0;
     //					printf("fps=%d\n", real_fps);
   }
	
	last_ticks = ticks;
	
	assert(m);
	Uint32 t_advance = tu_timer::get_ticks();
	m->advance(delta_time);
	t_advance = tu_timer::get_ticks() - t_advance;
	
	Uint32 t_display = tu_timer::get_ticks();
	m->display();
	t_display = tu_timer::get_ticks() - t_display;
	
	// for perfomance testing
	if (profile) printf("advance time: %d, display time %d\n",	t_advance, t_display); 
}
