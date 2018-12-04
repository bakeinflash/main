//
// A minimal test player app for the bakeinflash library.
//

#include "bakeinflash_logo.h"
#include "bakeinflash/bakeinflash_sprite.h"
#include "bakeinflash/bakeinflash_as_sprite.h"
#include "bakeinflash/bakeinflash_text.h"
#include "bakeinflash/bakeinflash_dlist.h"
#include "bakeinflash/bakeinflash_as_classes/as_textformat.h"

namespace bakeinflash
{
	void	draw_logo(root* rm)
	{
		/*
		var fontsize = 33;
		var str = '*** ' + msg + ' ***';
		var fld:TextField = _root.assert;
		if (fld == undefined)
		{
		_root.createTextField("assert", _root.getNextHighestDepth(), 10, 10, Math.min(Stage.width * 0.9, str.length * 20), fontsize * 1.3);
		var fld:TextField = _root.assert;
		fld.autoSize = true;
		fld.border = true;
		fld.multiline = true;
		fld.background = true;
		fld.backgroundColor = 0xFFFFFF;
		}

		xtrace(str);
		fld.text = str;

		var fmt:TextFormat = new TextFormat();
		fmt.font = "Myriad Pro";
		fmt.size = fontsize;
		fmt.align = "center";
		fld.setTextFormat(fmt);

		fld.textColor = 0xFF0000;
		fld._x = Stage.width / 2 - fld._width / 2;
		fld._y = Stage.height / 2 - fld._height / 2;
		*/

		as_value val;
		sprite_instance* m = cast_to<sprite_instance>(rm->get_root_movie());
		if (m && m->get_member("__logo__", &val) == false)
		{
			int highest_depth = m->get_highest_depth() - ADJUST_DEPTH_VALUE;
			int fontsize = 12;

			as_value res;
			as_environment env;
			env.push(fontsize * 1.6f);
			env.push(100);
			env.push(3);
			env.push(3);
			env.push(highest_depth);
			env.push("__logo__");		// name

			fn_call fn(&res, m, &env, 6, env.get_top_index());
			sprite_create_text_field(fn);

			edit_text_character* fld = cast_to<edit_text_character>(res.to_object());
			if (fld)
			{
				fld->set_member("autoSize", true);
				fld->set_member("border", true);
				fld->set_member("multiline", true);
				fld->set_member("background", true);
				fld->set_member("backgroundColor", 0xFFFFFF);
				fld->set_member("textColor", 0x00000);
				fld->set_member("text", "Demo Mode");

				smart_ptr<as_textformat> fmt = new as_textformat();
				fmt->set_member("font", "Arial");
				fmt->set_member("size", fontsize);
				fmt->set_member("align", "center");

				fld->reset_format(fmt);
			}
		}
	}

}	// namespace bakeinflash
