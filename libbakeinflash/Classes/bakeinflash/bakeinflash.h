// bakeinflash.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// SWF (Shockwave Flash) player library.  The info for this came from
// http://www.openswf.org, the flashsource project, and swfparse.cpp


#ifndef BAKEINFLASH_H
#define BAKEINFLASH_H

#include <ctype.h>	// for poxy wchar_t
#include <assert.h>
#include "base/image.h"	// for delete m_suspended_image
#include "base/container.h"	// for hash<...>
#include "base/smart_ptr.h"

class tu_file;
class render_handler;

// @@ forward decl to avoid including base/image.h; TODO change the
// render_handler interface to not depend on these structs at all.
namespace image { struct rgb; struct rgba; struct image_base; }

// forward decl
namespace jpeg { struct input; }
class tu_string;

// Forward decl for D3D render handlers, in case they are
// instantiated.  Harmless on non-D3D platforms.
struct IDirect3DDevice9;
struct IDirect3DDevice8;

namespace bakeinflash
{

	namespace video_pixel_format
	{
		enum code
		{
			INVALID = 0,
			RGB,
			RGBA,
			YUYV,
			BGR
		};
	}

	// Keyboard handling
	namespace key
	{
		enum code
		{
			INVALID = 0,
			BACKSPACE = 8,
			TAB,
			CLEAR = 12,
			ENTER,
			SHIFT = 16,
			CONTROL,
			ALT,
			CAPSLOCK = 20,
			ESCAPE = 27,
			SPACE = 32,
			PGDN,
			PGUP,
			END = 35,
			HOME,
			LEFT,
			UP,
			RIGHT,
			DOWN,
			INSERT = 45,
			DELETEKEY,
			HELP,
			_0 = 48,
			_1,
			_2,
			_3,
			_4,
			_5,
			_6,
			_7,
			_8,
			_9,
			A = 65,
			B,
			C,
			D,
			E,
			F,
			G,
			H,
			I,
			J,
			K,
			L,
			M,
			N,
			O,
			P,
			Q,
			R,
			S,
			T,
			U,
			V,
			W,
			X,
			Y,
			Z,
			KP_0 = 96,
			KP_1,
			KP_2,
			KP_3,
			KP_4,
			KP_5,
			KP_6,
			KP_7,
			KP_8,
			KP_9,
			KP_MULTIPLY,
			KP_ADD,
			KP_ENTER,
			KP_SUBTRACT,
			KP_DECIMAL,
			KP_DIVIDE,
			F1 = 112,
			F2,
			F3,
			F4,
			F5,
			F6,
			F7,
			F8,
			F9,
			F10,
			F11,
			F12,
			F13,
			F14,
			F15,

			NUM_LOCK = 144,

			RUS_a1 = 150,
			RUS_a2,
			RUS_a3,
			RUS_a4,
			RUS_a5,
			RUS_a6,
			RUS_a7,
			RUS_a8,
			RUS_a9,
			RUS_a10,
			RUS_a11,
			RUS_a12,
			RUS_a13,
			RUS_a14,
			RUS_a15,
			RUS_a16,
			RUS_a17,
			RUS_a18,
			RUS_a19,
			RUS_a20,
			RUS_a21,
			RUS_a22,
			RUS_a23,
			RUS_a24,
			RUS_a25,
			RUS_a26,
			RUS_a27,
			RUS_a28,
			RUS_a29,
			RUS_a30,
			RUS_a31,
			RUS_a32,

			SEMICOLON = 186,
			EQUALS = 187,
			COMMA = 188,
			MINUS = 189,
			PERIOD = 190,
			SLASH = 191,
			BACKTICK = 192,
			LEFT_BRACKET = 219,
			BACKSLASH = 220,
			RIGHT_BRACKET = 221,
			QUOTE = 222,

			RUS_1,
			RUS_2,
			RUS_3,
			RUS_4,
			RUS_5,
			RUS_6,
			RUS_7,
			RUS_8,
			RUS_9,
			RUS_10,
			RUS_11,
			RUS_12,
			RUS_13,
			RUS_14,
			RUS_15,
			RUS_16,
			RUS_17,
			RUS_18,
			RUS_19,
			RUS_20,
			RUS_21,
			RUS_22,
			RUS_23,
			RUS_24,
			RUS_25,
			RUS_26,
			RUS_27,
			RUS_28,
			RUS_29,
			RUS_30,
			RUS_31,
			RUS_32,

			KEYCOUNT
		};
	}	// end namespace key

	// Forward declarations.
	struct as_value;
	struct bitmap_info;
	struct character;
	struct execute_tag;
	struct font;
	struct render_handler;
	struct rgba;
	struct sound_handler;
	struct stream;
	struct video_handler;
	struct event_id;
	struct root;
	struct movie_def_impl;
	struct rect;
	struct as_environment;
	struct character_def;
	struct sound_sample;
	struct video_stream_definition;
	struct sprite_definition;
	struct as_s_function;
	struct as_object;
	struct movie_definition;
	struct as_function;
	struct tu_loadlib;
	struct glyph_entity;

	//
	// Log & error reporting control.
	//

	// Supply a function pointer to receive log & error messages.
	void	register_log_callback(void (*callback)(bool error, const char* message));

	// Control verbosity of specific categories.
	bool get_verbose_heap();
	bool get_verbose_parse();
	bool get_verbose_debug();
	bool get_verbose_action();
	void	set_verbose_action(bool verbose);
	void	set_verbose_parse(bool verbose);
	void	set_verbose_heap(bool verbose);

	// if true then unzip and load bitmaps in openGL during loading swf
	void	preload_bitmaps(bool alive);

	// if true then keep bitmaps alive in openGL
	bool	is_alive_bitmaps();
	void	keep_alive_bitmaps(bool alive);

	void	set_display_invisibles(bool show);
	bool	get_display_invisibles();

	// for embedded video
	bool	get_yuv2rgb_table();
	void	set_yuv2rgb_table(bool create);

	// Get and set the render handler.  This is one of the first
	// things you should do to initialise the player (assuming you
	// want to display anything).
	void	set_render_handler(render_handler* s);

	// Pass in a sound handler, so you can handle audio on behalf of
	// bakeinflash.  This is optional; if you don't set a handler, or set
	// NULL, then sounds won't be played.
	//
	// If you want sound support, you should set this at startup,
	// before loading or playing any movies!
	void	set_sound_handler(sound_handler* s);

	// You probably don't need this. (@@ make it private?)
	sound_handler*	get_sound_handler();

	// Register a callback to the host, for providing a file,
	// given a "URL" (i.e. a path name).  This is the only means
	// by which the bakeinflash library accesses file data, for
	// loading movies, cache files, and so on.
	//
	// bakeinflash will call this when it needs to open a file.
	//
	// NOTE: the returned tu_file* will be delete'd by bakeinflash
	// when it is done using it.  Your file_opener_function may
	// return NULL in case the requested file can't be opened.
	typedef tu_file* (*file_opener_callback)(const char* url_or_path);
	void	register_file_opener_callback(file_opener_callback opener);
	file_opener_callback get_file_opener_callback();

	// ActionScripts embedded in a movie can use the built-in
	// fscommand() function to send data back to the host
	// application.	 If you are interested in this data, register
	// a handler, which will be called when the embedded scripts
	// call fscommand().
	//
	// The handler gets the character* that the script is
	// embedded in, and the two string arguments passed by the
	// script to fscommand().
	typedef void (*fscommand_callback)(character* movie, const char* command, const char* arg);
	void	register_fscommand_callback(fscommand_callback handler);

	// Use this to control how finely curves are subdivided.  1.0
	// is the default; it's a pretty good value.  Larger values
	// result in coarser, more angular curves with fewer vertices.
	void	set_curve_max_pixel_error(float pixel_error);
	float	get_curve_max_pixel_error();

	// Some helpers that may or may not be compiled into your
	// version of the library, depending on platform etc.
	render_handler*	create_render_handler_xbox();
	render_handler*	create_render_handler_ogl();
	render_handler*	create_render_handler_ogles(bool hasStencilBuffer);
	render_handler*	create_render_handler_ogles2();
	render_handler* create_render_handler_d3d();

	bool	get_fileinfo(const char* url, int* width, int* height, int* fps);

	typedef  as_object* (*bakeinflash_module_init)(const array<as_value>& params);

	as_value	get_property(as_object* obj, int prop_number);
	void	set_property(as_object* obj, int prop_number, const as_value& val);

	fscommand_callback	get_fscommand_callback();
	void	register_fscommand_callback(fscommand_callback handler);

	string_hash<tu_loadlib*>* get_shared_libs();
	void clear_shared_libs();

	void register_type_handler( const tu_string& type_name, bakeinflash_module_init type_init_func );
	void clear_registered_type_handlers();
	bakeinflash_module_init find_type_handler( const tu_string& type_name );

	//
	// interface
	//
	const char* get_workdir();
	void set_workdir(const char* dir);

	void init_player();
	void clear_player();

	// external interface
	root* get_root();
	void set_root(root* m);
	character*	get_root_movie();
	void notify_key_event(key::code k, Uint16 utf16char, bool down);
	void verbose_action(bool val);
	void verbose_parse(bool val);

	const tu_string& get_flash_vars();
	void set_flash_vars(const tu_string& param);

	const tu_string& get_startdir();
	void set_startdir(const tu_string& dir);
	const tu_string& get_curdir();
	void set_curdir(const tu_string& dir);
	bool use_separate_thread();
	void set_separate_thread(bool flag);

	movie_definition*	create_movie(const char* filename);
	movie_definition*	create_movie(tu_file* in);
	smart_ptr<root>  load_file(const char* filename);


	// Create/hook built-ins.
	void action_init();

	// add user class
	void	add_class(const tu_string& name, const as_value& ctor);

	// library stuff, for sharing resources among different movies.
	string_hash<smart_ptr<character_def> >* get_chardef_library();
	const char* get_root_filename(const character_def* rdef);
	void clear_library();

	as_object* get_global();
	void notify_key_object(key::code k, Uint16 utf16char, bool down);

	const bool get_force_realtime_framerate();
	void set_force_realtime_framerate(bool force_realtime_framerate);
	bool get_log_bitmap_info();
	void set_log_bitmap_info(bool log_bitmap_info);

	// garbage manager
	void set_alive(as_object* obj, bool val = true);
	bool is_garbage(as_object* obj);
	void clear_heap();
	void set_as_garbage();
	void clear_garbage();

	const tu_string& get_canvas_name();

	void set_clear_garbage(bool val);
	bool get_clear_garbage();

	void create_YUV2RGBtable();
	const Uint8* get_ta_table();
	const Uint8* get_tb_table();
	const Uint8* get_tc_table();

	// Unique id of all bakeinflash resources
	enum as_classes
	{
		AS_OBJECT,
		AS_CHARACTER,
		AS_SPRITE,
		AS_FUNCTION,
		AS_C_FUNCTION,
		AS_S_FUNCTION,
		AS_3_FUNCTION,	// AS3
		AS_MOVIE_DEF,
		AS_MOVIE_DEF_SUB,
		AS_CHARACTER_DEF,
		AS_SPRITE_DEF,
		AS_VIDEO_DEF,
		AS_SOUND_SAMPLE,
		AS_VIDEO_INST,
		AS_KEY,
		AS_ARRAY,
		AS_COLOR,
		AS_SOUND,
		AS_FONT,
		AS_CANVAS,
		AS_NETSTREAM,
		AS_STRING,
		AS_PROPERTY,
		AS_SELECTION,
		AS_POINT,
		AS_MATRIX,
		AS_TRANSFORM,
		AS_COLOR_TRANSFORM,
		AS_NETCONNECTION,
		AS_LISTENER,
		AS_DATE,
		AS_EDIT_TEXT,
		AS_XML_SOCKET,
		AS_TEXTFORMAT,
		AS_MCLOADER,
		AS_LOADVARS,
		AS_TIMER,
		AS_MOUSE,
		AS_XML,
		AS_MICROPHONE,
		AS_IOS,
		AS_IMAP,
		AS_CAMERA,
		AS_CAMERA_OBJECT,
		AS_ACCELEROMETER,
		AS_INAPP,
		AS_ITABLE_VIEW,
		AS_ITEXT_VIEW,
		AS_IOS_SOUND,
		AS_IWEB_VIEW,
		AS_STAGE,
		AS_VK,
		AS_IGAMECENTER,
		AS_IPRINTER,
		AS_BITMAPDATA,
		AS_RECTANGLE,

		// AS3
		AS_EVENT,
		AS_MOUSE_EVENT,
		AS_EVENT_DISPATCHER,
		AS_URLVARIABLES,
		AS_URLREQUEST,
		AS_URLLOADER,

		// plugins
		AS_PLUGIN_MYDB,
		AS_PLUGIN_MYTABLE,
		AS_PLUGIN_3DS,
		AS_PLUGIN_FILE,
		AS_PLUGIN_SQLITE_DB,
		AS_PLUGIN_SQLITE_TABLE,
		AS_PLUGIN_SQL_DB,
		AS_PLUGIN_SQL_TABLE,
		AS_PLUGIN_SYSINFO,
    AS_PLUGIN_FACEBOOK,
    AS_PLUGIN_TWITTER,
		AS_PLUGIN_VALIDATOR,

		// user defined plugins
		// should be the last in this enum
		AS_USER_PLUGIN = 1000

	};

	// This is the base class for all ActionScript-able objects
	// ("as_" stands for ActionScript).
	struct as_object_interface : public ref_counted
	{
		virtual bool is(int class_id) const = 0;

		virtual const char*	to_string();
		virtual const tu_string&	to_tu_string();
		virtual double	to_number();
		virtual bool to_bool();

		virtual const char*	type_of() { return "object"; }

		virtual bool is_instance_of(const as_function* constructor) const { return false; }
		virtual bool is_string() const { return false; }
		virtual bool is_number() const { return false; }
		virtual bool is_object() const { return false; }
		virtual bool is_property() const { return false; }

		virtual bool	set_member(const tu_string& name, const as_value& val) { assert(0); return false; }
		virtual bool	get_member(const tu_string& name, as_value* val) { assert(0); return false; }
		virtual bool	find_property(const tu_string& name, as_value* val) { return false; }
		virtual bool	is_alive() const { return true; }
	};

	// cast_to<bakeinflash object>(obj) implementation (from Julien Hamaide)
	template <typename cast_class>
	cast_class* cast_to(as_object_interface* object)
	{
		if (object)
		{
			return object->is(cast_class::m_class_id) ? static_cast<cast_class*>(object) : 0;
		}
		return 0;
	}

	template <typename cast_class>
	const cast_class* cast_to(const as_object_interface* object)
	{
		if (object)
		{
			return object->is(cast_class::m_class_id) ? static_cast<const cast_class*>(object) : 0;
		}
		return 0;
	}

	// For caching precomputed stuff.  Generally of
	// interest to bakeinflash_processor and programs like it.
	struct cache_options
	{
		bool	m_include_font_bitmaps;

		cache_options()
			:
			m_include_font_bitmaps(true)
		{
		}
	};

	// Try to grab movie info from the header of the given .swf
	// file.
	//
	// Sets *version to 0 if info can't be extracted.
	//
	// You can pass NULL for any entries you're not interested in.
	void	get_movie_info(
		const char*	filename,
		int*		version,
		int*		width,
		int*		height,
		float*		frames_per_second,
		int*		frame_count,
		int*		tag_count);

	// Enable/disable attempts to read cache files (.gsc) when
	// loading movies.
	void	set_use_cache_files(bool use_cache);

	//
	// Use DO_NOT_LOAD_BITMAPS if you have pre-processed bitmaps
	// stored externally somewhere, and you plan to install them
	// via get_bitmap_info()->...
	enum create_bitmaps_flag
	{
		DO_LOAD_BITMAPS,
		DO_NOT_LOAD_BITMAPS
	};
	// Use DO_NOT_LOAD_FONT_SHAPES if you know you have
	// precomputed texture glyphs (in cached data) and you know
	// you always want to render text using texture glyphs.
	enum create_font_shapes_flag
	{
		DO_LOAD_FONT_SHAPES,
		DO_NOT_LOAD_FONT_SHAPES
	};

	//
	// Library management
	//

	// Release any library movies we've cached.  Do this when you want
	// maximum cleanup.
	void	clear_library();

	//
	// Sound callback handler.
	//

	// You may define a subclass of this, and pass an instance to
	// set_sound_handler().
	struct sound_handler
	{
		// audio for video
		typedef void (*aux_streamer_ptr)(as_object* netstream, unsigned char* stream, int len);

		enum format_type
		{
			FORMAT_RAW = 0,		// unspecified format.	Useful for 8-bit sounds???
			FORMAT_ADPCM = 1,	// bakeinflash doesn't pass this through; it uncompresses and sends FORMAT_NATIVE16
			FORMAT_MP3 = 2,
			FORMAT_UNCOMPRESSED = 3,	// 16 bits/sample, little-endian
			FORMAT_NELLYMOSER = 6,	// Mystery proprietary format; see nellymoser.com

			// bakeinflash tries to convert data to this format when possible:
			FORMAT_NATIVE16 = 7	// bakeinflash extension: 16 bits/sample, native-endian
		};

		// If stereo is true, samples are interleaved w/ left sample first.

		// bakeinflash calls at load-time with sound data, to be
		// played later.  You should create a sample with the
		// data, and return a handle that can be used to play
		// it later.  If the data is in a format you can't
		// deal with, then you can return 0 (for example), and
		// then ignore 0's in play_sound() and delete_sound().
		//
		// Assign handles however you like.
		virtual int	create_sound(
			void*		data,
			int		data_bytes,
			int		sample_count,
			format_type	format,
			int		sample_rate,	/* one of 5512, 11025, 22050, 44100 */
			bool		stereo) = 0;

		// loads external sound file
		virtual int	load_sound(const char* url) = 0;

		virtual void append_sound(int sound_handle, void* data, int data_bytes) = 0;

		// bakeinflash calls this when it wants you to play the defined sound.
		//
		// loop_count == 0 means play the sound once (1 means play it twice, etc)
		virtual void	play_sound(as_object* listener_obj, int sound_handle, int loop_count /* , volume, pan, etc? */) = 0;

		virtual int	get_volume(int sound_handle) = 0;
		virtual void	set_volume(int sound_handle, int volume) = 0;

		// set max volume in percent, [0..100]
		virtual void	set_max_volume(int vol) = 0;

		// Stop the specified sound if it's playing.
		// (Normally a full-featured sound API would take a
		// handle specifying the *instance* of a playing
		// sample, but SWF is not expressive that way.)
		virtual void	stop_sound(int sound_handle) = 0;
		virtual void	stop_all_sounds() = 0;

		// bakeinflash calls this when it's done with a particular sound.
		virtual void	delete_sound(int sound_handle) = 0;

		virtual ~sound_handler() {};
		virtual bool is_open() { return false; };
		virtual void pause(int sound_handle, bool paused) {};

		// The number of milliseconds a sound has been playing. 
		// If the sound is looped, the position is reset to 0 at the beginning of each loop.
		virtual int get_position(int sound_handle) { return 0; };

		// openAL sound handler uses this function to handle sound events
		virtual void advance(float delta_time) {};

		virtual int create_stream() { return 0; };
		virtual void push_stream(int stream_id, int format, Uint8* data, int size, int sample_rate, int channels) {};
		virtual void close_stream(int stream_id) {};

		virtual void push_action(bool stop_playback, int handler_id, int loop_count) {};
	};

	struct matrix; 
	struct rect; 

	// base class for video handler
	struct video_handler : public ref_counted
	{
		video_handler() :
			m_clear_background(false)
		{
		}

		virtual void display(video_pixel_format::code pixel_format, Uint8* data, int width, int height, const matrix* mat, const rect* bounds, const rgba& color) = 0;

		void clear_background(bool clear) { m_clear_background = clear; }

	protected:

		bool m_clear_background;
	};

	// tu_float is used in matrix & cxform because
	// Flash converts inf to zero when works with matrix & cxform
	/*
	struct tu_float
	{
	tu_float()	{ m_float = 0.0f; }
	tu_float(float x)	{ operator=(x); }
	tu_float(double x)	{ operator=((float) x); }

	operator float() const { return m_float; }
	inline void	operator=(float x)
	{
	m_float = x >= -3.402823466e+38F && x <= 3.402823466e+38F ? x : 0.0f;
	}
	void	operator+=(const float x) { operator=(m_float + x); }
	void	operator-=(const float x) { operator=(m_float - x); }
	void	operator*=(const float x) { operator=(m_float * x); }
	void	operator/=(const float x) { operator=(m_float / x); }

	private:
	float m_float;
	};
	*/


	//
	// matrix type, used by render handler
	//

	struct point;
	struct matrix
	{
		float	m_[2][3];

		static matrix	identity;

		matrix();
		void	set_identity();
		void	concatenate(const matrix& m);
		void	concatenate_translation(float tx, float ty);
		void	concatenate_scale(float s);
		void	set_lerp(const matrix& m1, const matrix& m2, float t);
		void	set_scale_rotation(float x_scale, float y_scale, float rotation);
		void	read(stream* in);
		void	print() const;
		void	transform(point* result, const point& p) const;
		void	transform(rect* bound) const;
		void	transform_vector(point* result, const point& p) const;
		void	transform_by_inverse(point* result, const point& p) const;
		void	transform_by_inverse(rect* bound) const;
		void	set_inverse(const matrix& m);
		bool	does_flip() const;	// return true if we flip handedness
		float	get_determinant() const;	// determinant of the 2x2 rotation/scale part only
		float	get_max_scale() const;	// return the maximum scale factor that this transform applies
		float	get_x_scale() const;	// return the magnitude scale of our x coord output
		float	get_y_scale() const;	// return the magnitude scale of our y coord output
		float	get_rotation() const;	// return our rotation component (in radians)
		void fliph();	// top<==>bottom
		void flipv();	//	left <==> right
		void	concatenate_xyscale(float xs, float ys);
		bool	is_valid() const;


		bool operator==(const matrix& m) const
		{
			return 
				m_[0][0] == m.m_[0][0] &&
				m_[0][1] == m.m_[0][1] &&
				m_[0][2] == m.m_[0][2] &&
				m_[1][0] == m.m_[1][0] &&
				m_[1][1] == m.m_[1][1] &&
				m_[1][2] == m.m_[1][2];
		}

		bool operator!=(const matrix& m) const
		{
			return 
				m_[0][0] != m.m_[0][0] ||
				m_[0][1] != m.m_[0][1] ||
				m_[0][2] != m.m_[0][2] ||
				m_[1][0] != m.m_[1][0] ||
				m_[1][1] != m.m_[1][1] ||
				m_[1][2] != m.m_[1][2];
		}

	};


	//
	// point: used by rect which is used by render_handler (otherwise would be in internal bakeinflash_types.h)
	//


	struct point
	{
		float	m_x, m_y;

		point() : m_x(0), m_y(0) {}
		point(float x, float y) : m_x(x), m_y(y) {}

		void	set_lerp(const point& a, const point& b, float t)
			// Set to a + (b - a) * t
		{
			m_x = a.m_x + (b.m_x - a.m_x) * t;
			m_y = a.m_y + (b.m_y - a.m_y) * t;
		}

		bool operator==(const point& p) const { return m_x == p.m_x && m_y == p.m_y; }

		bool	bitwise_equal(const point& p) const;

		float get_length() const;

		void twips_to_pixels();
		void pixels_to_twips();
	};


	//
	// rect: rectangle type, used by render handler
	//


	struct rect
	{
		float	m_x_min, m_x_max, m_y_min, m_y_max;

		rect() :
			m_x_min(0.0f),
			m_x_max(0.0f),
			m_y_min(0.0f),
			m_y_max(0.0f)
		{
		}

		rect(float xmin, float xmax, float ymin, float ymax) :
			m_x_min(xmin),
			m_x_max(xmax),
			m_y_min(ymin),
			m_y_max(ymax)
		{
		}

		void	read(stream* in);
		void	print() const;
		bool	point_test(float x, float y) const;
		bool	bound_test(const rect& bound) const;
		void	set_to_point(float x, float y);
		void	set_to_point(const point& p);
		void	expand_to_point(float x, float y);
		void	expand_to_point(const point& p);
		void	expand_to_rect(const rect& r);
		float	width() const { return m_x_max - m_x_min; }
		float	height() const { return m_y_max - m_y_min; }

		point	get_corner(int i) const;

		void	enclose_transformed_rect(const matrix& m, const rect& r);

		void	set_lerp(const rect& a, const rect& b, float t);

		void twips_to_pixels();
		void pixels_to_twips();
	};


	//
	// cxform: color transform type, used by render handler
	//
	struct cxform
	{
		float	m_[4][2];	// [RGBA][mult, add]

		cxform();
		void	concatenate(const cxform& c);
		rgba	transform(const rgba in) const;
		void	read_rgb(stream* in);
		void	read_rgba(stream* in);
		void	clamp();  // Force component values to be in range.
		void	print() const;

		static cxform	identity;
	};


	//
	// texture and render callback handler.
	//

	// Your render_handler creates bitmap_info's for bakeinflash.  You
	// need to subclass bitmap_info in order to add the
	// information and functionality your app needs to render using textures.
	struct bitmap_info : public ref_counted
	{
		virtual void upload() = 0;
		virtual int get_width() const { return 0; }
		virtual int get_height() const { return 0; }
		virtual float get_xratio() const = 0;
		virtual float get_yratio() const = 0;
		virtual void set_xratio(float val) = 0;
		virtual void set_yratio(float val) = 0;
		virtual unsigned char* get_data() const { return 0; }
		virtual int get_bpp() const { return 0; }	// byte per pixel

		virtual void save(const char* filename) {};

		// update texture
		virtual void update(const unsigned char* buf, int width, int height) {};
		virtual void keep_alive_source_image(bool yes) = 0;
	};

	// You must define a subclass of render_handler, and pass an
	// instance to set_render_handler().
	struct render_handler
	{
		virtual ~render_handler() {}

		// Your handler should return these with a ref-count of 0.  (@@ is that the right policy?)
		virtual bitmap_info*	create_bitmap_info(image::image_base* im) = 0;
		virtual video_handler*	create_video_handler() = 0;

		virtual void clear() = 0;
		virtual void read_buffer(int x, int y, int w, int h, Uint8* buf) = 0;

		// Bracket the displaying of a frame from a movie.
		// Fill the background color, and set up default
		// transforms, etc.
		virtual void	begin_display(
			rgba background_color,
			int viewport_x0, int viewport_y0,
			int viewport_width, int viewport_height,
			float x0, float x1, float y0, float y1) = 0;
		virtual void	end_display() = 0;

		// Geometric and color transforms for mesh and line_strip rendering.
		virtual void	set_matrix(const matrix& m) = 0;
		virtual void	set_cxform(const cxform& cx) = 0;
		virtual void	set_rgba(rgba* color) = 0;

		// Draw triangles using the current fill-style 0.
		// Clears the style list after rendering.
		//
		// coords is a list of (x,y) coordinate pairs, in
		// triangle-strip order.  The type of the array should
		// be Sint16[vertex_count*2]
		virtual void	draw_mesh_strip(const void* coords, int vertex_count) = 0;
		// As above, but coords is in triangle list order.
		virtual void	draw_triangle_list(const void* coords, int vertex_count) = 0;

		// Draw a line-strip using the current line style.
		// Clear the style list after rendering.
		//
		// Coords is a list of (x,y) coordinate pairs, in
		// sequence.  Each coord is a 16-bit signed integer.
		virtual void	draw_line_strip(const void* coords, int vertex_count) = 0;

		// Set line and fill styles for mesh & line_strip
		// rendering.
		enum bitmap_wrap_mode
		{
			WRAP_REPEAT,
			WRAP_CLAMP
		};

		enum bitmap_blend_mode
		{
			BLEND_NORMAL,
			BLEND_LAYER,
			BLEND_MULTIPLY,
			BLEND_SCREEN,
			BLEND_LIGHTEN,
			BLEND_DARKEN,
			BLEND_DIFFERENCE,
			BLEND_ADD,
			BLEND_SUBTRACT,
			BLEND_INVERT,
			BLEND_ALPHA,
			BLEND_ERASE,
			BLEND_OVERLAY,
			BLEND_HARDLIGHT
		};

		virtual void	fill_style_disable(int fill_side) = 0;
		virtual void	fill_style_color(int fill_side, const rgba& color) = 0;
		virtual void	fill_style_bitmap(int fill_side, bitmap_info* bi, const matrix& m,
			bitmap_wrap_mode wm, bitmap_blend_mode bm) = 0;

		virtual void	line_style_disable() = 0;
		virtual void	line_style_color(rgba color) = 0;
		virtual void	line_style_width(float width) = 0;
		virtual void	line_style_caps(int caps_style) = 0;

		// Special function to draw a rectangular bitmap;
		// intended for textured glyph rendering.  Ignores
		// current transforms.
		virtual void	draw_bitmap(
			const matrix&		m,
			bitmap_info*	bi,
			const rect&		coords,
			const rect&		uv_coords,
			rgba			color) = 0;
		virtual void	set_antialiased(bool enable) = 0;

		virtual bool test_stencil_buffer(const rect& bound, Uint8 pattern) = 0;
		virtual void begin_submit_mask() = 0;
		virtual void end_submit_mask() = 0;
		virtual void disable_mask() = 0;

		// Mouse cursor handling.
		enum cursor_type
		{
			SYSTEM_CURSOR,
			ACTIVE_CURSOR,
			VISIBLE_CURSOR,			// show cursor
			INVISIBLE_CURSOR		// hide cursor
		};
		virtual void set_cursor(cursor_type cursor) {}
		virtual bool is_visible(const rect& bound) = 0;
		virtual void open() = 0;
		virtual void show_cursor(int mouse_x, int mouse_y) {};
	};

	// Some optional helpers.
	namespace tools
	{
		struct process_options
		{
			bool	m_zip_whole_file;	// @@ not implemented yet (low priority?)
			bool	m_remove_image_data;	// removes existing image data; leaves minimal placeholder tags
			bool	m_remove_font_glyph_shapes;

			process_options()
				:
				m_zip_whole_file(false),
				m_remove_image_data(false),
				m_remove_font_glyph_shapes(false)
			{}
		};

		// Copy tags from *in to *out, applying the given
		// options.  *in should be a SWF-format stream.	 The
		// output will be a SWF-format stream.
		//
		// Returns 0 on success, or a non-zero error-code on
		// failure.
		int	process_swf(tu_file* swf_out, tu_file* swf_in, const process_options& options);
	}

	struct glyph_provider : public ref_counted
	{
		glyph_provider() {}
		virtual ~glyph_provider() {}

		//		virtual bitmap_info* get_char_image(character_def* shape_glyph, Uint16 code, 
		//																				const tu_string& fontname, bool is_bold, bool is_italic, int fontsize,
		//																				rect* bounds, float* advance) = 0;

		virtual	image::alpha* render_string(const tu_string& str, int alignment, const tu_string& fontname, 
			bool is_bold, bool is_italic, int fontsize, const array<int>& xleading, const array<int>& yleading, int vert_advance, 
			int w, int h, bool multiline, int cursor, float xscale, float yscale)
		{
			return NULL; 
		};


	};

	glyph_provider*	get_glyph_provider();
	void	set_glyph_provider(glyph_provider* gp);
	glyph_provider*	create_glyph_provider_freetype();
	glyph_provider*	create_glyph_provider_tu();

	void	use_cached_movie(bool yes);

}	// namespace bakeinflash


#endif // bakeinflash_H


// Local Variables:
// mode: C++
// c-basic-offset: 8
// tab-width: 8
// indent-tabs-mode: t
// End:
