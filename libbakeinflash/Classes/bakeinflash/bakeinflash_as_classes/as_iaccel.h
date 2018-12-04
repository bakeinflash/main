// as_iaccel.h	-- Vitaly Alexeev <alexeev.vitaly@yahoo.com> 2011

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// system utilities

#ifndef BAKEINFLASH_AS_iACCEL_H
#define BAKEINFLASH_AS_iACCEL_H

#include "bakeinflash/bakeinflash_action.h"	// for as_object

#ifdef ANDROID
#include <jni.h>
#endif


namespace bakeinflash
{

	struct as_iaccel : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_ACCELEROMETER };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		as_iaccel(float freq);
		virtual ~as_iaccel();
		void didAccelerate(float x, float y, float z);
		void onShake();


//		 virtual bool	set_member(const tu_string& name, const as_value& val);
		 virtual bool	get_member(const tu_string& name, as_value* val);
		
		void* m_accel;	// accelerometer
		float m_x, m_y, m_z;		// data
		float m_angle;
		tu_string m_orientation;
	};

	void	as_global_iaccel_ctor(const fn_call& fn);

}	// namespace bakeinflash

#endif
