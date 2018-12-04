// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#if TU_CONFIG_LINK_CEF == 1

#include "as_cef.h"
#include "bakeinflash/bakeinflash_root.h"
#include "bakeinflash/bakeinflash_sprite.h"
#include "bakeinflash/bakeinflash_as_classes/as_key.h"
#include "base/utf8.h"

static int s_x0 = 0;
static int s_y0 = 0;
static float s_retina = 1;
static float s_scale = 1;

namespace bakeinflash
{

	void sendMessage(const char* name, const bakeinflash::as_value& arg)
	{
		as_value function;
		as_object* obj = get_global();
		if (obj && obj->get_member("sendMessage", &function))
		{
			as_environment env;
			env.push(arg);
			env.push(name);
			as_value val = call_method(function, &env, obj, 2, env.get_top_index());
		}	
	}

	// Method that will be called once for each cookie. |count| is the 0-based
	// index for the current cookie. |total| is the total number of cookies. Set
	// |deleteCookie| to true (1) to delete the cookie currently being visited.
	// Return false (0) to stop visiting cookies. This function may never be
	// called if no cookies are found.
	int CEF_CALLBACK visitCookie(struct _cef_cookie_visitor_t* self, const struct _cef_cookie_t* cookie, int count, int total, int* deleteCookie)
	{
		*deleteCookie = 1; //true;
		return 1;		// true
	}


#ifdef WIN32
	void CEF_CALLBACK onResourceRedirect(struct _cef_request_handler_t* self, struct _cef_browser_t* browser, struct _cef_frame_t* frame, const cef_string_t* old_url, cef_string_t* new_url)
#else
	void CEF_CALLBACK onResourceRedirect(struct _cef_request_handler_t* self, struct _cef_browser_t* browser, struct _cef_frame_t* frame, struct _cef_request_t* request, cef_string_t* new_url)
#endif
	{
		cef_string_utf8_t dst = {};
		int n = cef_string_utf16_to_utf8(new_url->str, new_url->length, &dst);
		sendMessage("onLoadURL", dst.str);
		cef_string_utf8_clear(&dst);
	}

	_cef_render_handler_t* CEF_CALLBACK getRenderHandler(struct _cef_client_t* self)
	{
		const weak_ptr<as_cef>& thiz = *(const weak_ptr<as_cef>*)((Uint8*)self + sizeof(struct _cef_client_t));
		return &thiz->m_render;
	}

	_cef_request_handler_t* CEF_CALLBACK getRequestHandler(struct _cef_client_t* self)
	{
		const weak_ptr<as_cef>& thiz = *(const weak_ptr<as_cef>*)((Uint8*)self + sizeof(struct _cef_client_t));
		return &thiz->m_requesthandler;
	}

	int CEF_CALLBACK getViewRect(struct _cef_render_handler_t* self, struct _cef_browser_t* browser, cef_rect_t* rect)
	{
		const weak_ptr<as_cef>& thiz = *(const weak_ptr<as_cef>*)((Uint8*)self + sizeof(struct _cef_render_handler_t));

		rect->x = 0;
		rect->y = 0;
		rect->width = thiz->m_bi->get_width();
		rect->height = thiz->m_bi->get_height();
		return 1;
	}

	void CEF_CALLBACK onPaint(struct _cef_render_handler_t* self, struct _cef_browser_t* browser, cef_paint_element_type_t type, size_t dirtyRectsCount,
		cef_rect_t const* dirtyRects, const void* buffer, int width, int height)
	{
		// BGRA ==> RGBA
		Uint8* src = (Uint8*)buffer;
		Uint8* buf = (Uint8*)malloc(width * height * 4);
		Uint8* dst = buf;
		for (int i = 0; i < width * height; i++)
		{
			dst[0] = src[2];
			dst[1] = src[1];
			dst[2] = src[0];
			dst[3] = src[3];
			src += 4;
			dst += 4;
		}

		const weak_ptr<as_cef>& thiz = *(const weak_ptr<as_cef>*)((Uint8*)self + sizeof(struct _cef_render_handler_t));
		thiz->m_bi->update((const Uint8*)buf, width, height);

		free(buf);
	}


	void	as_global_cef_ctor(const fn_call& fn)
	{
		if (fn.nargs < 1)
		{
			myprintf("www ctor error, no args, needs target movie\n");
			return;
		}

		static int s_needs_to_init = true;
		if (s_needs_to_init)
		{
			s_needs_to_init = false;

			cef_main_args_t args = {};

			cef_settings_t settings = {};
			settings.multi_threaded_message_loop = false;
			settings.single_process = true;
			settings.command_line_args_disabled = false;
			settings.no_sandbox = 1;

			//		CefString(&settings.locales_dir_path) = "C:\\bakeinflash\\cef\\Resources\\locales";
			cef_initialize(&args, &settings, NULL,	NULL);
		}

		sprite_instance* mc = cast_to<sprite_instance>(fn.arg(0).to_object());
		if (mc)
		{
			fn.result->set_as_object(new as_cef(mc));
		}
	}

	void	as_cef_mouse_move(const fn_call& fn)
	{
		sprite_instance* mc = cast_to<sprite_instance>(fn.this_ptr);
		if (mc)
		{
			as_value val;
			mc->get_member("_CefPtr_", &val);
			as_cef* obj = cast_to<as_cef>(val.to_object());
			if (obj && obj->m_browser != NULL && obj->m_url != "")
			{
				// Local coord of mouse IN PIXELS.
				int	x, y, buttons;
				mc->get_mouse_state(&x, &y, &buttons);

				matrix	m;
				mc->get_world_matrix(&m);

				point	a(PIXELS_TO_TWIPS(x), PIXELS_TO_TWIPS(y));
				point	b;
				m.transform_by_inverse(&b, a);

				cef_mouse_event_t event;
				event.modifiers = 0;
				event.x = (int) ceil(TWIPS_TO_PIXELS(b.m_x));
				event.y = (int) ceil(TWIPS_TO_PIXELS(b.m_y));

				cef_browser_host_t* host = obj->m_browser->get_host(obj->m_browser);
				host->set_focus(host, 1);
				host->send_focus_event(host, 1);
				host->send_mouse_move_event(host, &event, false);
			}
		}
	}

	void	as_cef_mouse_press(const fn_call& fn)
	{
		sprite_instance* mc = cast_to<sprite_instance>(fn.this_ptr);
		if (mc)
		{
			as_value val;
			mc->get_member("_CefPtr_", &val);
			as_cef* obj = cast_to<as_cef>(val.to_object());
			if (obj && obj->m_browser && obj->m_url != "")
			{
				// Local coord of mouse IN PIXELS.
				int	x, y, buttons;
				mc->get_mouse_state(&x, &y, &buttons);

				matrix	m;
				mc->get_world_matrix(&m);

				point	a(PIXELS_TO_TWIPS(x), PIXELS_TO_TWIPS(y));
				point	b;
				m.transform_by_inverse(&b, a);

				cef_mouse_event_t event;
				event.modifiers = 0; // EVENTFLAG_LEFT_MOUSE_BUTTON;
				event.x = (int)ceil(TWIPS_TO_PIXELS(b.m_x));
				event.y = (int)ceil(TWIPS_TO_PIXELS(b.m_y));

				cef_browser_host_t* host = obj->m_browser->get_host(obj->m_browser);
				host->set_focus(host, 1);
				host->send_focus_event(host, 1);
				host->send_mouse_click_event(host, &event, MBT_LEFT, 0, 1);

			}
		}
	}

	void	as_cef_mouse_release(const fn_call& fn)
	{
		sprite_instance* mc = cast_to<sprite_instance>(fn.this_ptr);
		if (mc)
		{
			as_value val;
			mc->get_member("_CefPtr_", &val);
			as_cef* obj = cast_to<as_cef>(val.to_object());
			if (obj && obj->m_browser && obj->m_url != "")
			{
				// Local coord of mouse IN PIXELS.
				int	x, y, buttons;
				mc->get_mouse_state(&x, &y, &buttons);

				matrix	m;
				mc->get_world_matrix(&m);

				point	a(PIXELS_TO_TWIPS(x), PIXELS_TO_TWIPS(y));
				point	b;
				m.transform_by_inverse(&b, a);

				cef_mouse_event_t event;
				event.modifiers = 0; 
				event.x = (int)ceil(TWIPS_TO_PIXELS(b.m_x));
				event.y = (int)ceil(TWIPS_TO_PIXELS(b.m_y));

				cef_browser_host_t* host = obj->m_browser->get_host(obj->m_browser);
				host->send_mouse_click_event(host, &event, MBT_LEFT, 1, 1);
			}
		}
	}

	void	as_cef_keydown(const fn_call& fn)
	{
		sprite_instance* mc = cast_to<sprite_instance>(fn.this_ptr);
		if (mc)
		{
			as_value val;
			mc->get_member("_CefPtr_", &val);
			as_cef* obj = cast_to<as_cef>(val.to_object());
			if (obj && obj->m_browser != NULL && obj->m_url != "")
			{
				as_value key;
				get_global()->get_member("Key", &key);
				as_key* ko = cast_to<as_key>(key.to_object());
				int lastkey = ko->get_last_key_pressed();
				Uint16 utf16char = ko->get_last_utf16_key_pressed();

				cef_browser_host_t* host = obj->m_browser->get_host(obj->m_browser);

				cef_key_event_t event = {};
#ifdef WIN32
				event.windows_key_code = utf16char;
#else
				event.windows_key_code = utf16char;
				event.character = utf16char;
#endif

	//			printf("cef lastkey: %d, utf16key %d\n", lastkey, utf16char);

				event.type = KEYEVENT_RAWKEYDOWN;
				host->send_key_event(host, &event);

				event.type = KEYEVENT_KEYUP;
				host->send_key_event(host, &event);

				// hack, tab,left,right,home,end, del, enter
				if (lastkey != 9 && lastkey != 35 && lastkey != 37 && lastkey != 36 && lastkey != 39 && lastkey != 46 && lastkey != 13)
				{
					event.type = KEYEVENT_CHAR;
					host->send_key_event(host, &event);
				}
			}
		}
	}

	void	as_cef_url_getter(const fn_call& fn)
	{
		as_cef* obj = cast_to<as_cef>(fn.this_ptr);
		if (obj)
		{
			fn.result->set_string(obj->m_url.c_str());
		}
	}

	void	as_cef_url_setter(const fn_call& fn)
	{
		as_cef* obj = cast_to<as_cef>(fn.this_ptr);
		if (obj && fn.nargs > 0 && obj->m_browser != NULL)
		{
			obj->m_url = fn.arg(0).to_tu_string();

			sprite_instance* mc = obj->m_parent.get();
			assert(mc);
			if (obj->m_url == "")
			{
				// clear
				mc->builtin_member("onMouseMove", as_value());
				mc->builtin_member("onPress", as_value());
				mc->builtin_member("onRelease", as_value());
				mc->builtin_member("onKeyDown", as_value());
			}
			else
			{
				mc->builtin_member("onMouseMove", as_cef_mouse_move);
				mc->builtin_member("onPress", as_cef_mouse_press);
				mc->builtin_member("onRelease", as_cef_mouse_release);

				// Key.addListener(mc)
				as_value key;
				get_global()->get_member("Key", &key);

				as_value func;
				key.to_object()->get_member("addListener", &func);

				as_environment* env = mc->get_environment();
				env->push(mc);
				call_method(func, env, key, 1, env->get_top_index());
				env->drop(1);

				mc->builtin_member("onKeyDown", as_cef_keydown);
			}

			sendMessage("onLoadURL", fn.arg(0));

			cef_frame_t* frame = obj->m_browser->get_main_frame(obj->m_browser);
			if (obj->m_url.size() > 0)
			{
				static cef_cookie_visitor_t it = {};
				it.base.size = sizeof(it);
				it.visit = visitCookie;
#ifdef WIN32
				cef_cookie_manager_t* cm = cef_cookie_manager_get_global_manager();
#else
				cef_cookie_manager_t* cm = cef_cookie_manager_get_global_manager(NULL);
#endif
				cm->visit_all_cookies(cm, &it);

				cef_string_t url = {};
				cef_string_utf8_to_utf16(obj->m_url.c_str(), obj->m_url.size(), &url);
				frame->load_url(frame, &url);
				cef_string_clear(&url);
			}
			else
			{
				const char about[] = "about:blank";
				cef_string_utf16_t url = {};
				cef_string_ascii_to_utf16(about, sizeof(about), &url);
				frame->load_url(frame, &url);
				cef_string_utf16_clear(&url);
			}
		}
	}


	as_cef::as_cef(sprite_instance* mc) :
		m_browser(NULL),
		this_ptr_for_render(this),
		this_ptr_for_client(this),
		this_ptr_for_requesthandler(this),
		m_parent(mc)
	{
		assert(mc);

		memset(&m_window_info, 0, sizeof(m_window_info));
		memset(&m_bsettings, 0, sizeof(m_bsettings));
		memset(&m_render, 0, sizeof(m_render));
		memset(&m_client, 0, sizeof(m_client));
		memset(&m_requesthandler, 0, sizeof(m_requesthandler));


		// get actual size of characters in pixels
		matrix m;
		mc->get_world_matrix(&m);

		float xscale = m.get_x_scale() * s_scale / s_retina;
		float yscale = m.get_y_scale() * s_scale / s_retina;
		int w = (int) ceil(TWIPS_TO_PIXELS(mc->get_width() / xscale));
		int h = (int) ceil(TWIPS_TO_PIXELS(mc->get_height() / yscale));

		image::rgba* im = new image::rgba(w, h);
		memset(im->m_data, 0, im->m_height * im->m_pitch);
		m_bi = render::create_bitmap_info(im);
		// im has being deleted by bi->upload();

		movie_definition* rdef = get_root()->get_movie_definition();
		bitmap_character_def*	def = new bitmap_character_def(rdef, m_bi);
		mc->replace_me(def);

		m_render.base.size = sizeof(m_render);
		m_render.on_paint = onPaint;
		m_render.get_view_rect = getViewRect;

		m_requesthandler.base.size = sizeof(m_requesthandler);
		//m_requesthandler.on_before_browse = onBeforeBrowse;
		m_requesthandler.on_resource_redirect = onResourceRedirect;

		m_client.base.size = sizeof(m_client);
		m_client.get_render_handler = getRenderHandler;
		m_client.get_request_handler = getRequestHandler;

		m_window_info.windowless_rendering_enabled = true;
		m_window_info.transparent_painting_enabled = true;

		m_bsettings.size = sizeof(m_bsettings);
		m_bsettings.windowless_frame_rate = 30;

		cef_string_t url = {};
		cef_client_t* m_client_ptr = &m_client;

		m_browser = cef_browser_host_create_browser_sync(&m_window_info, &m_client, &url, &m_bsettings, NULL);

		mc->builtin_member("_CefPtr_", this);

		// this methods
		builtin_member("url", as_value(as_cef_url_getter, as_cef_url_setter));

		get_root()->add_listener(this);
	}

	as_cef::~as_cef()
	{
		if (m_browser)
		{
			cef_browser_host_t* host = m_browser->get_host(m_browser);
			host->close_browser(host, true);
		}
	}

	void	as_cef::advance(float delta_time)
	{
		cef_do_message_loop_work();
	}

}	// namespace bakeinflash

/*
def translate_to_cef_keycode(self, keycode):
        cef_keycode = keycode
        if self.is_alt2:
            # The key mappings here for right alt were tested
            # with the utf-8 charset on a webpage. If the charset
            # is different there is a chance they won't work correctly.
            alt2_map = {
                    # tilde
                    "96":172,
                    # 0-9 (48..57)
                    "48":125, "49":185, "50":178, "51":179, "52":188, 
                    "53":189, "54":190, "55":123, "56":91, "57":93,
                    # minus
                    "45":92,
                    # a-z (97..122)
                    "97":433, "98":2771, "99":486, "100":240, "101":490,
                    "102":496, "103":959, "104":689, "105":2301, "106":65121,
                    "107":930, "108":435, "109":181, "110":497, "111":243,
                    "112":254, "113":64, "114":182, "115":438, "116":956,
                    "117":2302, "118":2770, "119":435, "120":444, "121":2299,
                    "122":447,
                    }
            if str(keycode) in alt2_map:
                cef_keycode = alt2_map[str(keycode)]
            else:
                print("Kivy to CEF key mapping not found (right alt), " \
                        "key code = %s" % keycode)
            shift_alt2_map = {
                    # tilde
                    "96":172,
                    # 0-9 (48..57)
                    "48":176, "49":161, "50":2755, "51":163, "52":36, 
                    "53":2756, "54":2757, "55":2758, "56":2761, "57":177,
                    # minus
                    "45":191,
                    # A-Z (97..122)
                    "97":417, "98":2769, "99":454, "100":208, "101":458,
                    "102":170, "103":957, "104":673, "105":697, "106":65122,
                    "107":38, "108":419, "109":186, "110":465, "111":211,
                    "112":222, "113":2009, "114":174, "115":422, "116":940,
                    "117":2300, "118":2768, "119":419, "120":428, "121":165,
                    "122":431,
                    # special: <>?  :"  {}
                    "44":215, "46":247, "47":65110,
                    "59":65113, "39":65114,
                    "91":65112, "93":65108,
                    }
            if self.is_shift1 or self.is_shift2:
                if str(keycode) in shift_alt2_map:
                    cef_keycode = shift_alt2_map[str(keycode)]
                else:
                    print("Kivy to CEF key mapping not found " \
                            "(shift + right alt), key code = %s" % keycode)
        elif self.is_shift1 or self.is_shift2:
            shift_map = {
                    # tilde
                    "96":126,
                    # 0-9 (48..57)
                    "48":41, "49":33, "50":64, "51":35, "52":36, "53":37,
                    "54":94, "55":38, "56":42, "57":40,
                    # minus, plus
                    "45":95, "61":43,
                    # a-z (97..122)
                    "97":65, "98":66, "99":67, "100":68, "101":69, "102":70,
                    "103":71, "104":72, "105":73, "106":74, "107":75, "108":76,
                    "109":77, "110":78, "111":79, "112":80, "113":81, "114":82,
                    "115":83, "116":84, "117":85, "118":86, "119":87, "120":88,
                    "121":89, "122":90,
                    # special: <>?  :"  {}
                    "44":60, "46":62, "47":63,
                    "59":58, "39":34,
                    "91":123, "93":125,
            }
            if str(keycode) in shift_map:
                cef_keycode = shift_map[str(keycode)]
        # Other keys:
        other_keys_map = {
            # Escape
            "27":65307,
            # F1-F12
            "282":65470, "283":65471, "284":65472, "285":65473,
            "286":65474, "287":65475, "288":65476, "289":65477,
            "290":65478, "291":65479, "292":65480, "293":65481,
            # Tab
            "9":65289,
            # Left Shift, Right Shift
            "304":65505, "303":65506,
            # Left Ctrl, Right Ctrl
            "306":65507, "305": 65508,
            # Left Alt, Right Alt
            "308":65513, "313":65027,
            # Backspace
            "8":65288,
            # Enter
            "13":65293,
            # PrScr, ScrLck, Pause
            "316":65377, "302":65300, "19":65299,
            # Insert, Delete, 
            # Home, End, 
            # Pgup, Pgdn
            "277":65379, "127":65535, 
            "278":65360, "279":65367,
            "280":65365, "281":65366,
            # Arrows (left, up, right, down)
            "276":65361, "273":65362, "275":65363, "274":65364,
        }
        if str(keycode) in other_keys_map:
            cef_keycode = other_keys_map[str(keycode)]
        return cef_keycode
Maintainer of the CEF Python, PHP Desktop, CEF2go and CEF C API projects.
Czarek
 
Posts: 685
Joined: Sun Nov 06, 2011 2:12 am
Location: Poland

*/
#endif
