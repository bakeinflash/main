// as_itextview.cpp
// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "bakeinflash/bakeinflash_as_classes/as_itextview.h"
#include "bakeinflash/bakeinflash_root.h"
//#include "as_itextview.h"  

#ifdef iOS
	#import <MapKit/MapKit.h>  
	#import "as_itextview_.h"
	#import "as_itextview.h"

	@interface EAGLView : UIView <UITextFieldDelegate> @end
	extern EAGLView* s_view;
	extern float s_scale;
	extern int s_x0;
	extern int s_y0;
	extern float s_retina;

#endif

namespace bakeinflash
{
	void	as_global_itextview_ctor(const fn_call& fn)
	{
		if (fn.nargs < 1)
		{
			return;
		}
		
		character* ch = cast_to<character>(fn.arg(0).to_object());
		if (ch)
		{
      fn.result->set_as_object(new as_itextview(ch));
    }
	}


	void	as_itextview_destroy(const fn_call& fn)
	{
		as_itextview* tv = cast_to<as_itextview>(fn.this_ptr);
		assert(tv);
		tv->destroy();
	}

}

#ifdef iOS


namespace bakeinflash
{
	as_itextview::as_itextview(character* ch) :
		m_edit(false),
		m_tableview(NULL),
    m_parent(ch)
	{
    assert(ch);
    
		builtin_member("destroy", as_itextview_destroy);
		
		m_text_color.set(0, 0, 0, 255);	// black
		m_bk_color.set(0, 0, 0, 0);	// transparent
    
    // get actual size of characters in pixels
    matrix m;
    m_parent->get_world_matrix(&m);
    
    float xscale = m.get_x_scale() * s_scale / s_retina;
    float yscale = m.get_y_scale() * s_scale / s_retina;
    
    float x = s_x0 / s_retina + TWIPS_TO_PIXELS(m.m_[0][2] * s_scale / s_retina);
    float y = s_y0 / s_retina + TWIPS_TO_PIXELS(m.m_[1][2] * s_scale / s_retina);
    float w = TWIPS_TO_PIXELS(m_parent->get_width() * xscale);
    float h = TWIPS_TO_PIXELS(m_parent->get_height() * yscale);
		CGRect rect = CGRectMake(x, y, w, h);
    
		myTextView* tv = [[myTextView alloc] initWithFrame:rect];
		tv.delegate = tv;
		tv.parent = this;
		tv.backgroundColor = [UIColor colorWithRed:m_bk_color.m_r/255.0 green:m_bk_color.m_g/255.0 blue:m_bk_color.m_b/255.0 alpha:m_bk_color.m_a/255.0];
		
		tv.textColor = [UIColor colorWithRed:m_text_color.m_r/255.0 green:m_text_color.m_g/255.0 blue:m_text_color.m_b/255.0 alpha:m_text_color.m_a/255.0];

		[s_view addSubview:tv];
		m_tableview = tv;
		
		[tv becomeFirstResponder];
    
    get_root()->add_listener(this);
	}
	
  void  as_itextview::advance(float delta_time)
  {
    myTextView* tv = (myTextView*) m_tableview;
    if (tv)
    {
      // get actual size of characters in pixels
      matrix m;
      m_parent->get_world_matrix(&m);
      
      float xscale = m.get_x_scale() * s_scale / s_retina;
      float yscale = m.get_y_scale() * s_scale / s_retina;
      
      float x = s_x0 / s_retina + TWIPS_TO_PIXELS(m.m_[0][2] * s_scale / s_retina);
      float y = s_y0 / s_retina + TWIPS_TO_PIXELS(m.m_[1][2] * s_scale / s_retina);
      float w = TWIPS_TO_PIXELS(m_parent->get_width() * xscale);
      float h = TWIPS_TO_PIXELS(m_parent->get_height() * yscale);
      CGRect rect = CGRectMake(x, y, w, h);
      
      tv.frame = rect;
    }
  }
  
	void as_itextview::destroy()
	{
    get_root()->remove_listener(this);
    
		myTextView* tv = (myTextView*) m_tableview;
		if (tv)
		{
			[tv removeFromSuperview];
			[tv release];
			m_tableview = NULL;
		}
	}
	
	as_itextview::~as_itextview()
	{
		destroy();
	}
	
	
	bool	as_itextview::get_member(const tu_string& name, as_value* val)
	{
		myTextView* tv = (myTextView*) m_tableview;
		if (name == "_visible")
		{
			val->set_bool(!tv.hidden);
			return true;
		}
		else
		if (name == "textColor")
		{
			//TODO
			return true;
		}
		else
		if (name == "text")
		{
			val->set_tu_string([tv.text UTF8String]);
      return true;
		}
		return as_object::get_member(name, val);
	}
	
	bool	as_itextview::set_member(const tu_string& name, const as_value& val)
	{
		myTextView* tv = (myTextView*) m_tableview;
		if (name == "_visible")
		{
			tv.hidden = !val.to_bool();
			return true;
		}
		else
		if (name == "fontSize")
		{
			[tv setFont:[UIFont systemFontOfSize: val.to_int()]];
      return true;
		}
		else
		if (name == "textColor")
		{
			as_object* obj = val.to_object();
			if (obj)
			{
				as_value r, g, b, a;
				obj->get_member("r", &r);
				obj->get_member("g", &g);
				obj->get_member("b", &b);
				obj->get_member("a", &a);
				m_text_color.set(r.to_int(), g.to_int(), b.to_int(), a.to_int());
				
				tv.textColor = [UIColor colorWithRed:m_text_color.m_r/255.0 green:m_text_color.m_g/255.0 blue:m_text_color.m_b/255.0 alpha:m_text_color.m_a/255.0];
				
			}
			return true;
		}
		else
		if (name == "text")
		{
			tv.text = [NSString stringWithUTF8String: val.to_string()];
      return true;
		}
		return as_object::set_member(name, val);
	}

}

#endif

//
//
//

@implementation myTextView
@synthesize parent;




@end

