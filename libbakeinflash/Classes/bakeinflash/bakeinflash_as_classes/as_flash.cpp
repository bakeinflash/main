// as_geom.cpp	-- Julien Hamaide <julien.hamaide@gmail.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "bakeinflash/bakeinflash_as_classes/as_flash.h"
#include "bakeinflash/bakeinflash_as_classes/as_point.h"
#include "bakeinflash/bakeinflash_as_classes/as_matrix.h"
#include "bakeinflash/bakeinflash_as_classes/as_transform.h"
#include "bakeinflash/bakeinflash_as_classes/as_color_transform.h"
#include "bakeinflash/bakeinflash_as_classes/as_bitmapdata.h"

namespace bakeinflash
{
	//
	// geom object
	//

	as_object* geom_init()
	{
		// Create built-in geom object.
		as_object*	obj = new as_object();

		// constant
		obj->builtin_member("Point", as_global_point_ctor);
		obj->builtin_member("Matrix", as_global_matrix_ctor);
		obj->builtin_member("Transform", as_global_transform_ctor);
		obj->builtin_member("ColorTransform", as_global_color_transform_ctor);
		obj->builtin_member("Rectangle", as_global_rectangle_ctor);

		return obj;
	}

	as_object* display_init()
	{
		// Create built-in display object.
		as_object*	obj = new as_object();

		// constant
		obj->builtin_member("BitmapData", as_global_bitmapdata_ctor);
		return obj;
	}

	as_object* flash_init()
	{
		// Create built-in flash object.

		as_object* flash = new as_object();
		flash->set_member("geom", geom_init());
		flash->set_member("display", display_init());
		return flash;
	}

}
