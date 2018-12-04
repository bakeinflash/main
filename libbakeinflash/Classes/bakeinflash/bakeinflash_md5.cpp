#include "bakeinflash/bakeinflash_as_classes/as_array.h"
#include "base/md5.h"

namespace bakeinflash
{
	void	md5_encrypt(const fn_call& fn)
	{
		if (fn.nargs > 0)
		{
			MD5 md5(fn.arg(0).to_string());
			tu_string hash = md5.hexdigest();
//			myprintf("source=%s\nhash=%s\n", fn.arg(0).to_string(), hash.c_str());
			fn.result->set_tu_string(md5.hexdigest());
		}
	}

	as_object* md5_init()
	{
		// Create built-in math object.
		as_object*	obj = new as_object();
		obj->builtin_member("encrypt", md5_encrypt);
		return obj;
	}

}
