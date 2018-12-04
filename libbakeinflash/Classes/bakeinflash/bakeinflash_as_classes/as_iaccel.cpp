// as_iaccel.cpp
// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "bakeinflash/bakeinflash_as_classes/as_iaccel.h"
#include "bakeinflash/bakeinflash_root.h"
//#include "as_iaccel.h"

#ifdef iOS
	#import "as_iaccel_.h"

	@interface EAGLView : UIView <UITextFieldDelegate> @end
	extern EAGLView* s_view;
#endif

namespace bakeinflash
{

	void	as_global_iaccel_ctor(const fn_call& fn)
	// Constructor for ActionScript class
	{
		if (fn.nargs < 1)
		{
			return;
		}
		
		float freq = fn.arg(0).to_number();
		fn.result->set_as_object(new as_iaccel(freq));
	}

	bool	as_iaccel::get_member(const tu_string& name, as_value* val)
	{
		if (name == "x")
		{
			val->set_double(m_x);
			return true;
		}
		else
		if (name == "y")
		{
			val->set_double(m_y);
			return true;
		}
		else
		if (name == "z")
		{
			val->set_double(m_z);
			return true;
		}
		else
		if (name == "orientation")
		{
			val->set_tu_string(m_orientation);
			return true;
		}
		return as_object::get_member( name, val );
	}

	void as_iaccel::onShake()
	{
		//myprintf("onShake");
		as_value function;
		if (get_member("onShake", &function))
		{
			as_environment env;
			call_method(function, &env, this, 0, env.get_top_index());
		}
	}
	
	void as_iaccel::didAccelerate(float x, float y, float z)
	{
		m_x = x;
		m_y = y;
		m_z = z;
		
		m_angle = atan2(y, -x);
		//myprintf("%f ", m_angle);
	
		as_value function;
		if (get_member("onAccelerate", &function))
		{
			as_environment env;
			call_method(function, &env, this, 0, env.get_top_index());
		}

		if (get_member("onOrientation", &function))
		{
			as_environment env;

			// Add 1.5 to the angle to keep the label constantly horizontal to the viewer.
		
			// Read my blog for more details on the angles. It should be obvious that you
			// could fire a custom shouldAutorotateToInterfaceOrientation-event here.
//			if (m_angle >= -2.25 && m_angle <= -0.75)
			if (m_angle >= -1.75 && m_angle <= -1.25)
			{
				if (m_orientation != "portrait")
				{
					m_orientation = "portrait";
					call_method(function, &env, this, 0, env.get_top_index());
				}
			}
			else
			if (m_angle >= -0.25 && m_angle <= 0.25)
			{
				if (m_orientation != "landscapeRight")
				{
					m_orientation = "landscapeRight";
					call_method(function, &env, this, 0, env.get_top_index());
				}
			}
			else
			if (m_angle >= 1.25 && m_angle <= 1.75)
			{
				if (m_orientation != "portraitUpsideDown")
				{
					m_orientation = "portraitUpsideDown";
					call_method(function, &env, this, 0, env.get_top_index());
				}
			}
			else
			if (m_angle <= -2.75 || m_angle >= 2.75)
			{
				if (m_orientation != "landscapeLeft")
				{
					m_orientation = "landscapeLeft";
					call_method(function, &env, this, 0, env.get_top_index());
				}
			}
		}
	}
}

#ifdef WIN32

namespace bakeinflash
{
	as_iaccel::as_iaccel(float freq) :
		m_x(0),
		m_y(0),
		m_z(0),
		m_angle(0) 
	{
	}

	as_iaccel::~as_iaccel()
	{
	}
}

#endif

#ifdef iOS

namespace bakeinflash
{
	as_iaccel::as_iaccel(float freq)		// freq = per second
	{
		myAccel* a = [myAccel alloc];
		a.parent = this;
		a.accel = [UIAccelerometer sharedAccelerometer];
		a.accel.updateInterval = 1.0 / freq;
		a.accel.delegate = a;
		m_accel = a;
	}
	
	as_iaccel::~as_iaccel()
	{
		myAccel* a = (myAccel*) m_accel;
		a.accel.delegate = nil;
		[a release];  
	}
	
}

@implementation myAccel
@synthesize parent;
@synthesize accel;
@synthesize lastAcceleration;

// Ensures the shake is strong enough on at least two axes before declaring it a shake.
// "Strong enough" means "greater than a client-supplied threshold" in G's.
static BOOL isShaking(UIAcceleration* last, UIAcceleration* current, double threshold)
{
	double
	deltaX = fabs(last.x - current.x),
	deltaY = fabs(last.y - current.y),
	deltaZ = fabs(last.z - current.z);
	
	return
	(deltaX > threshold && deltaY > threshold) ||
	(deltaX > threshold && deltaZ > threshold) ||
	(deltaY > threshold && deltaZ > threshold);
}

- (void)accelerometer:(UIAccelerometer*) accelerometer didAccelerate:(UIAcceleration *) acceleration
{
	//myprintf("%f %f %f\n", acceleration.x, acceleration.y, acceleration.z);
	bakeinflash::as_iaccel* a = bakeinflash::cast_to<bakeinflash::as_iaccel>((bakeinflash::as_iaccel*) parent);
	if (a) 
	{
		a->didAccelerate(acceleration.x, acceleration.y, acceleration.z);
	}
	
	if (self.lastAcceleration)
	{
		if (!histeresisExcited && isShaking(self.lastAcceleration, acceleration, 0.7)) 
		{
			histeresisExcited = YES;
			if (a)
			{
				a->onShake();
			}
		}
		else
		if (histeresisExcited && !isShaking(self.lastAcceleration, acceleration, 0.2))
		{
			histeresisExcited = NO;
		}
	}
	
	self.lastAcceleration = acceleration;
}

@end

#endif	// iOS

//

#ifdef ANDROID

namespace bakeinflash
{
	as_iaccel::as_iaccel(float freq) :
		m_x(0),
		m_y(0),
		m_z(0)
	{
	}

	as_iaccel::~as_iaccel()
	{
	}
}

#endif

