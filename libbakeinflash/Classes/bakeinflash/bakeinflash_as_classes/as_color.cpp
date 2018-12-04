// as_color.cpp	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// The Color class lets you set the RGB color value and color transform
// of movie clips and retrieve those values once they have been set. 

#include "bakeinflash/bakeinflash_as_classes/as_color.h"

namespace bakeinflash
{

	// Color(target:Object)
	void	as_global_color_ctor(const fn_call& fn)
	{
		if (fn.nargs == 1)
		{
			character* target = cast_to<character>(fn.arg(0).to_object());
			if (target)
			{
				fn.result->set_as_object(new as_color(target));
			}
		}
	}


	void	as_color_getRGB(const fn_call& fn)
	{
		as_color* obj = cast_to<as_color>(fn.this_ptr);
		if (obj != NULL && obj->m_target != NULL)
		{
			rgba* color = obj->m_target->get_rgba();
			if (color)
			{
				Uint8 r = (Uint8) ceil(color->m_r * 255.0f);
				Uint8 g = (Uint8) ceil(color->m_g * 255.0f);
				Uint8 b = (Uint8) ceil(color->m_b * 255.0f);
				fn.result->set_int(r << 16 | g << 8 | b);
			}
		}
	}

	void	as_color_setRGB(const fn_call& fn)
	{
		as_color* obj = cast_to<as_color>(fn.this_ptr);
		if (fn.nargs > 0 && obj != NULL && obj->m_target != NULL)
		{
			rgba color(fn.arg(0).to_number());
			obj->m_target->set_rgba(color);
		}
	}

	// TODO: Fix gettransform()
	void	as_color_gettransform(const fn_call& fn)
	{
		as_color* obj = cast_to<as_color>(fn.this_ptr);
		if (obj == NULL)
		{
			return;
		}
			
		if (obj->m_target == NULL)
		{
			return;
		}

		cxform	cx = obj->m_target->get_cxform();
		as_object* tobj = new as_object();
		tobj->set_member("ra", cx.m_[0][1] / 255.0f * 100.0f);	// percent (-100..100)
		tobj->set_member("rb", cx.m_[0][1]);	// value	(-255..255)
		tobj->set_member("ga", cx.m_[1][1] / 255.0f * 100.0f);	// percent (-100..100)
		tobj->set_member("gb", cx.m_[1][1]);	// value	(-255..255)
		tobj->set_member("ba", cx.m_[2][1] / 255.0f * 100.0f);	// percent (-100..100)
		tobj->set_member("bb", cx.m_[2][1]);	// value	(-255..255)
		tobj->set_member("aa", cx.m_[3][1] / 255.0f * 100.0f);	// percent (-100..100)
		tobj->set_member("ab", cx.m_[3][1]);	// value	(-255..255)

		fn.result->set_as_object(tobj);
	}

	void	as_color_settransform(const fn_call& fn)
	{
		if (fn.nargs < 1)
		{
			return;
		}

		as_color* obj = cast_to<as_color>(fn.this_ptr);
		if (obj == NULL || obj->m_target == NULL)
		{
			return;
		}
			
		as_object* tobj = fn.arg(0).to_object();
		if (tobj)
		{
			cxform	cx = obj->m_cxform;
			as_value v;

			if (tobj->get_member("ra", &v))
			{
				cx.m_[0][1] *= v.to_float() / 100.0f * 255.0f;
			}
			else
			if (tobj->get_member("rb", &v))
			{
				cx.m_[0][1] = v.to_float();
			}

			if (tobj->get_member("ga", &v))
			{
				cx.m_[1][1] *= v.to_float() / 100.0f * 255.0f;
			}
			else
			if (tobj->get_member("gb", &v))
			{
				cx.m_[1][1] = v.to_float();
			}

			if (tobj->get_member("ba", &v))
			{
				cx.m_[2][1] *= v.to_float() / 100.0f * 255.0f;
			}
			else
			if (tobj->get_member("bb", &v))
			{
				cx.m_[2][1] = v.to_float();
			}

			if (tobj->get_member("aa", &v))
			{
				cx.m_[3][1] *= v.to_float() / 100.0f * 255.0f;
			}
			else
			if (tobj->get_member("ab", &v))
			{
				cx.m_[3][1] = v.to_float();
			}

			obj->m_target->set_cxform(cx);
		}		
	}

	as_color::as_color(character* target) :
		m_target(target)
	{
		assert(target);
		m_cxform = target->get_cxform();

		builtin_member("getRGB", as_color_getRGB);
		builtin_member("setRGB", as_color_setRGB);
		builtin_member("getTransform", as_color_gettransform);
		builtin_member("setTransform", as_color_settransform);
		set_ctor(as_global_color_ctor);
	}

};
