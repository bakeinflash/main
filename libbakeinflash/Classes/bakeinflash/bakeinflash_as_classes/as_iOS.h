// as_iOS.h	-- Vitaly Alexeev <alexeev.vitaly@yahoo.com> 2011

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// system utilities

#ifndef BAKEINFLASH_AS_iOS_H
#define BAKEINFLASH_AS_iOS_H

#include "bakeinflash/bakeinflash_action.h"	// for as_object

#ifdef ANDROID
#include <jni.h>
#endif

namespace bakeinflash
{
/*
  struct as_facebook : public as_object
  {
    // Unique id of a bakeinflash resource
    enum { m_class_id = AS_PLUGIN_FACEBOOK };
    virtual bool is(int class_id) const
    {
      if (m_class_id == class_id) return true;
      else return as_object::is(class_id);
    }
    
    as_facebook(void* handler);

    void* m_handler;
  };
  */
  struct as_iOS : public as_object
  {
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_IOS };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		as_iOS();
		virtual ~as_iOS();

#ifdef ANDROID
		jobject m_jPreferences;
#endif
	};
	
	as_iOS* iOS_init();

}	// namespace bakeinflash

#endif
