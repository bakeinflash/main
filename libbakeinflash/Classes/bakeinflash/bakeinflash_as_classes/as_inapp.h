// as_inapp.h	-- Vitaly Alexeev <alexeev.vitaly@yahoo.com> 2011

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// system utilities

#ifndef BAKEINFLASH_AS_inapp_H
#define BAKEINFLASH_AS_inapp_H

#include "bakeinflash/bakeinflash_action.h"	// for as_object

namespace bakeinflash
{

	struct as_inapp : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_INAPP };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		as_inapp();
		virtual ~as_inapp();
		void onload(void* products, void* invalid_products);
		void onpurchase(const char* productIdentifier, int rc);
		
		void* m_inapp;	// inapp manager
		
	};

	as_inapp* inapp_init();

}	// namespace bakeinflash

#endif
