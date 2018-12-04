// as_urlvariables.h	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2010

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "bakeinflash/bakeinflash_root.h"
#include "as_urlvariables.h"
#include "as_global.h"
#include "base/tu_file.h"
#include "base/tu_timer.h"

namespace bakeinflash
{
  
	void	as_urlvariables_ctor(const fn_call& fn)
	{
		tu_string source;
		if (fn.nargs > 0)
		{
			source = fn.arg(0).to_tu_string();
		}
		as_urlvariables*	obj = new as_urlvariables(source);
		fn.result->set_as_object(obj);
	}

	void	as_urlrequest_ctor(const fn_call& fn)
	{
		tu_string url;
		if (fn.nargs > 0)
		{
			url = fn.arg(0).to_tu_string();
		}
		as_urlrequest*	obj = new as_urlrequest(url);
		fn.result->set_as_object(obj);
	}

	as_urlvariables::as_urlvariables(const tu_string& source) 
	{
		decode(source);
	}

	// parse 
	void as_urlvariables::decode(const tu_string& str)
	{
		if (str.size() > 0)
		{
			array<tu_string> pairs;
			str.split('&', &pairs);
			for (int k = 0; k < pairs.size(); k++)
			{
				array<tu_string> val;
				pairs[k].split('=', &val);

				if (val.size() == 2)
				{
					as_object::set_member(val[0], val[1].c_str());
				}
				else
				if (val.size() == 1)
				{
					as_object::set_member(val[0], "");	// no arg value
				}
			}
		}
	}

		as_urlrequest::as_urlrequest(const tu_string& url):
			m_url(url)
		{

		}

} // end of bakeinflash namespace
