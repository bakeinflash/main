// as_inapp.h	-- Vitaly Alexeev <alexeev.vitaly@yahoo.com> 2011

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// system utilities

#ifndef BAKEINFLASH_AS_VK_H
#define BAKEINFLASH_AS_VK_H

#include "bakeinflash/bakeinflash_action.h"	// for as_object

namespace bakeinflash
{

	struct as_vk : public as_object
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_VK };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_object::is(class_id);
		}

		as_vk(player* player);
		virtual ~as_vk();
		
		void getToken();
		bool wall(const tu_string& msg);
		void onPosted(const char* msg);
		void onToken(const char* msg);


		void* m_vk;
		tu_string m_userID;
    tu_string m_accessToken;
		tu_string m_appID;
	};

	void	as_vk_ctor(const fn_call& fn);

}	// namespace bakeinflash

#endif
