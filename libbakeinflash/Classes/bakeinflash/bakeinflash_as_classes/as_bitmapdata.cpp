// 	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2010

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "pugixml/pugixml.hpp"
#include "bakeinflash/bakeinflash_root.h"
#include "bakeinflash/bakeinflash_sprite.h"
#include "as_bitmapdata.h"
#include "base/png_helper.h"

namespace bakeinflash
{
  
	void	as_global_rectangle_ctor(const fn_call& fn)
	{
		if (fn.nargs >= 4)
		{
			rect r(fn.arg(0).to_float(), fn.arg(2).to_float(), fn.arg(1).to_float(), fn.arg(3).to_float());
			as_rectangle*	obj = new as_rectangle(r);
			fn.result->set_as_object(obj);
		}
	}

	// BitmapData(width:Number, height:Number,[transparent:Boolean], [fillColor:Number])
	void	as_global_bitmapdata_ctor(const fn_call& fn)
	{
		if (fn.nargs >= 2)
		{
			int w = fn.arg(0).to_int();
			int h = fn.arg(1).to_int();
			bool transparent = fn.nargs >= 3 ? fn.arg(2).to_bool() : true;
			Uint32 fillColor = (Uint32) (fn.nargs >= 4 ? fn.arg(3).to_number() : 0xFFFFFFFF);
			if (w > 0 && h > 0)
			{
				as_bitmapdata*	obj = new as_bitmapdata(fn.arg(0).to_int(), fn.arg(1).to_int(), transparent, fillColor);
				fn.result->set_as_object(obj);
			}
		}
	}

	//fillRect(rect:Rectangle, color:Number) : Void 
	// Fills a rectangular area of pixels with a specified ARGB color.
	void	as_bitmapdata_fillrect(const fn_call& fn)
	{
		as_bitmapdata* bd = cast_to<as_bitmapdata>(fn.this_ptr);
		as_rectangle*	rect = cast_to<as_rectangle>(fn.arg(0).to_object());

		if (bd && fn.nargs >= 2 && rect)
		{
			Uint32 fillColor = (Uint32) fn.arg(1).to_number();
			bd->fill_color(rect, fillColor);
		}
	}

	//draw(source:Object, [matrix:Matrix],[colorTransform:ColorTransform],[blendMode:Object],[cliprect:Rectangle],[smooth:Boolean]) : Void
	// Draws a source image or movie clip onto a destination image, using the Flash Lite player vector renderer.
	void	as_bitmapdata_draw(const fn_call& fn)
	{
		as_bitmapdata* bd = cast_to<as_bitmapdata>(fn.this_ptr);
		as_object * mc = fn.arg(0).to_object();
		if (bd && fn.nargs >= 1 && mc)
		{
			bd->draw(cast_to<sprite_instance>(mc));
		}
	}

	// extension
	void	as_bitmapdata_pixels_count(const fn_call& fn)
	{
		as_bitmapdata* bd = cast_to<as_bitmapdata>(fn.this_ptr);
		if (bd && fn.nargs >= 2)
		{
			rgba min_pixel;
			min_pixel.set_rgba(fn.arg(0).to_number());
			rgba max_pixel;
			max_pixel.set_rgba(fn.arg(1).to_number());
			fn.result->set_int(bd->count_pixels(min_pixel, max_pixel)); 
		}
	}

	void	as_bitmapdata_width(const fn_call& fn)
	{
		as_bitmapdata* bd = cast_to<as_bitmapdata>(fn.this_ptr);
		if (bd && bd->m_image)
		{
			fn.result->set_int(bd->m_image->m_width); 
		}
	}

	void	as_bitmapdata_height(const fn_call& fn)
	{
		as_bitmapdata* bd = cast_to<as_bitmapdata>(fn.this_ptr);
		if (bd && bd->m_image)
		{
			fn.result->set_int(bd->m_image->m_height); 
		}
	}

	as_bitmapdata::as_bitmapdata(int w, int h, bool transparent, Uint32 fillColor)
	{
		// fillColor = A 32-bit ARGB color
		m_image = new image::rgba(w, h);
		Uint8* p = m_image->m_data;
		Uint8 a = transparent ? (fillColor >> 24) & 0xFF : 0xFF;		// todo: test
		Uint8 r = (fillColor >> 16) & 0xFF;
		Uint8 g = (fillColor >> 8) & 0xFF;
		Uint8 b = fillColor & 0xFF;
		for (int y = 0; y < h; y++)
		{
			for (int x = 0; x < w; x++)
			{
				p[0] = r;
				p[1] = g;
				p[2] = b;
				p[3] = a;
				p += 4;
			}
		}

		builtin_member("fillRect", as_bitmapdata_fillrect);
		builtin_member("draw", as_bitmapdata_draw);
		builtin_member("countPixels", as_bitmapdata_pixels_count);
		builtin_member("width", as_value(as_bitmapdata_width, as_value()));
		builtin_member("height", as_value(as_bitmapdata_height, as_value()));
	}

	void as_bitmapdata::fill_color(as_rectangle* rect, Uint32 fillColor)
	{
		// fillColor = A 32-bit ARGB color
		if (m_image == NULL)
		{
			return;
		}

		int x0 = (int) rect->m_rect.m_x_min;
		if (x0 >= m_image->m_width)
		{
			return;
		}

		int y0 = (int) rect->m_rect.m_y_min;
		if (y0 >= m_image->m_height)
		{
			return;
		}

		int w = imin(x0 + (int) rect->m_rect.m_x_max, (int) m_image->m_width);
		int h = imin(y0 + (int) rect->m_rect.m_y_max, (int) m_image->m_height);

		Uint8 a = 0xFF; //(fillColor >> 24) & 0xFF;
		Uint8 r = (fillColor >> 16) & 0xFF;
		Uint8 g = (fillColor >> 8) & 0xFF;
		Uint8 b = fillColor & 0xFF;
		for (int y = y0; y < h; y++)
		{
			Uint8* p = m_image->m_data + m_image->m_pitch * y + x0 * 4;		// hack 4
			for (int x = x0; x < w; x++)
			{
				p[0] = r;
				p[1] = g;
				p[2] = b;
				p[3] = a;
				p += 4;
			}
		}
	}

	as_bitmapdata::~as_bitmapdata()
	{
		delete m_image;
	}

	void as_bitmapdata::draw(sprite_instance* mc)
	// render movieclip to bitmap
	{
		delete m_image;
		m_image = NULL;
		if (mc)
		{
			m_image = mc->render();
		}
	}

	Uint8* as_bitmapdata::get_data() const
	{
		if (m_image)
		{
			return m_image->m_data;
		}
		return NULL;
	}

	int as_bitmapdata::get_width() const
	{
		if (m_image)
		{
			return m_image->m_width;
		}
		return 0;
	}

	int as_bitmapdata::get_height() const
	{
		if (m_image)
		{
			return m_image->m_height;
		}
		return 0;
	}

	int as_bitmapdata::count_pixels(const rgba min, const rgba& max)
	{
		int n = 0;
		if (m_image)
		{
			Uint32 amin = (min.m_r << 24) | (min.m_g << 16) | (min.m_b << 8) | min.m_a;
			Uint32 amax = (max.m_r << 24) | (max.m_g << 16) | (max.m_b << 8) | max.m_a;
			for (int y = 0; y < m_image->m_height; y++)
			{
				for (int x = 0; x < m_image->m_width; x++)
				{
					// hack RGBA
					Uint8* p = m_image->m_data + (m_image->m_pitch * y) + x * 4;
					Uint32 pix = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
					if (pix >= amin && pix <= amax)
					{
						n++;
					}
				}

			}
		}
		return n;
	}

} // end of bakeinflash namespace
