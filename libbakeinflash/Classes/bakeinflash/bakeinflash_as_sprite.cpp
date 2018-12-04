// bakeinflash_as_sprite.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation for SWF player.

// Useful links:
//
// http://sswf.sourceforge.net/SWFalexref.html
// http://www.openswf.org

#include "bakeinflash/bakeinflash_action.h"	// for as_object
#include "bakeinflash/bakeinflash_sprite.h"
#include "bakeinflash/bakeinflash_as_classes/as_number.h"
#include "bakeinflash/bakeinflash_as_classes/as_microphone.h"
#include "bakeinflash/bakeinflash_as_classes/as_bitmapdata.h"

namespace bakeinflash
{
	
	//
	// sprite built-in ActionScript methods
	//
	
	void	as_global_movieclip_ctor(const fn_call& fn)
	// Constructor for ActionScript class XMLSocket
	{
//		root* rm = get_root();
//		sprite_definition* empty_sprite_def = new sprite_definition(NULL);
//		character* ch = new sprite_instance(empty_sprite_def,	rm, rm->get_root_movie(), 0);
		fn.result->set_as_object(NULL);
	}
	
	sprite_instance* sprite_getptr(const fn_call& fn)
	{
		sprite_instance* sprite = cast_to<sprite_instance>(fn.this_ptr);
		if (sprite == NULL)
		{
			sprite = cast_to<sprite_instance>(fn.env->get_target());
		}
		assert(sprite);
		return sprite;
	}
	
	// usage1: public hitTest(x:Number, y:Number, [shapeFlag:Boolean]):Boolean
	// usage2: public hitTest(target:Object):Boolean
	void	sprite_hit_test(const fn_call& fn)
	{
		sprite_instance* sprite = sprite_getptr(fn);
		fn.result->set_bool(false);
		if (fn.nargs == 1) 
		{
			// Evaluates the bounding boxes of the target and specified instance,
			// and returns true if they overlap or intersect at any point.
			character* ch = cast_to<character>(fn.env->find_target(fn.arg(0)));
			if (ch)
			{
				fn.result->set_bool(sprite->hit_test(ch));
				return;
			}
			myprintf("hitTest: can't find target\n");
			return;
		}
		else
		if (fn.nargs >= 2) 
		{
			bool shape_flag = fn.nargs > 2 ? fn.arg(2).to_bool() : true;
			fn.result->set_bool(sprite->hit_test( fn.arg(0).to_number(), fn.arg(1).to_number(), shape_flag));
		}
	}
	
	void	sprite_start_drag(const fn_call& fn)
	{
		sprite_instance* sprite = sprite_getptr(fn);
		
		character::drag_state ds;
		ds.SetCharacter(sprite);
		
		if (fn.nargs > 0)
		{
			// arguments have been given to startDrag
			ds.SetLockCentered(fn.arg(0).to_bool());
			
			if (fn.nargs >= 5)
			{
				// bounds have been given
				float x0 = PIXELS_TO_TWIPS(fn.arg(1).to_float());
				float y0 = PIXELS_TO_TWIPS(fn.arg(2).to_float());
				float x1 = PIXELS_TO_TWIPS(fn.arg(3).to_float());
				float y1 = PIXELS_TO_TWIPS(fn.arg(4).to_float());
				
				float bx0, bx1, by0, by1;
				if (x0 < x1)
				{
					bx0 = x0; bx1 = x1; 
				}
				else
				{
					bx0 = x1; bx1 = x0; 
				}
				
				if (y0 < y1)
				{
					by0 = y0; by1 = y1; 
				}
				else
				{
					by0 = y1; by1 = y0; 
				}
				
				// we've got bounds
				ds.SetBounds(bx0, by0, bx1, by1);
			}
		}
		
		// inform the root
		get_root()->set_drag_state(ds);
	}
	
	void	sprite_stop_drag(const fn_call& fn)
	{
		//sprite_instance* sprite = sprite_getptr(fn);
		get_root()->stop_drag();
	}
	
	void	sprite_play(const fn_call& fn)
	{
		sprite_instance* sprite = sprite_getptr(fn);
		sprite->set_play_state(character::PLAY);
	}
	
	void	sprite_stop(const fn_call& fn)
	{
		sprite_instance* sprite = sprite_getptr(fn);
		sprite->set_play_state(character::STOP);
	}
	
	void	sprite_goto_and_play(const fn_call& fn)
	{
		sprite_instance* sprite = sprite_getptr(fn);
		if (fn.nargs < 1)
		{
			myprintf("error: sprite_goto_and_play needs one arg\n");
			return;
		}
		
		// gotoAndPlay(NaN) will be ignored
		if (fn.arg(0).is_string() || fn.arg(0).is_number() || fn.arg(0).is_property())
		{
			bool success = sprite->goto_frame(fn.arg(0).to_tu_string());
			if (success)
			{
				sprite->set_play_state(character::PLAY);
			}
		}
	}
	
	void	sprite_goto_and_stop(const fn_call& fn)
	{
		sprite_instance* sprite = sprite_getptr(fn);
		if (fn.nargs < 1)
		{
			myprintf("error: sprite_goto_and_stop needs one arg\n");
			return;
		}
		
		// gotoAndStop(NaN) will be ignored
		if (fn.arg(0).is_string() || fn.arg(0).is_number() || fn.arg(0).is_property())
		{
			bool success = sprite->goto_frame(fn.arg(0).to_tu_string());
			if (success)
			{
				sprite->set_play_state(character::STOP);
			}
		}
	}
	
	void	sprite_next_frame(const fn_call& fn)
	{
		sprite_instance* sprite = sprite_getptr(fn);
		int frame_count = sprite->get_frame_count();
		int current_frame = sprite->get_current_frame();
		if (current_frame < frame_count)
		{
			sprite->goto_frame(current_frame + 1);
		}
		sprite->set_play_state(character::STOP);
	}
	
	void	sprite_prev_frame(const fn_call& fn)
	{
		sprite_instance* sprite = sprite_getptr(fn);
		int current_frame = sprite->get_current_frame();
		if (current_frame > 0)
		{
			sprite->goto_frame(current_frame - 1);
		}
		sprite->set_play_state(character::STOP);
	}
	
	void	sprite_get_bytes_loaded(const fn_call& fn)
	{
		sprite_instance* sprite = sprite_getptr(fn);
		fn.result->set_int(sprite->get_loaded_bytes());
	}
	
	void	sprite_get_bytes_total(const fn_call& fn)
	{
		sprite_instance* sprite = sprite_getptr(fn);
		fn.result->set_int(sprite->get_file_bytes());
	}
	
	//swapDepths(target:Object) : Void
	//swapDepths(depth:Number) : Void
	void sprite_swap_depths(const fn_call& fn) 
	{ 
		sprite_instance* sprite = sprite_getptr(fn);
		if (fn.nargs != 1) 
		{ 
			myprintf("swapDepths needs one arg\n"); 
			return; 
		} 
		
		sprite_instance* target = NULL;
		
		if (fn.arg(0).is_number())
		{ 
			int target_depth = fn.arg(0).to_int();
			target_depth += ADJUST_DEPTH_VALUE;
			sprite_instance* parent = cast_to<sprite_instance>(sprite->get_parent());
			if (parent == NULL)
			{
				myprintf("can't do _root.swapDepths\n"); 
				return; 
			}
			
			character* ch = parent->m_display_list.get_character_at_depth(target_depth);
			if (ch)
			{
				target = cast_to<sprite_instance>(ch);
			}
			else
			{
				// no character at depth
				parent->m_display_list.change_character_depth(sprite, target_depth);
				return;
			}
		}
		else
			if (fn.arg(0).is_object()) 
			{ 
				target = cast_to<sprite_instance>(fn.arg(0).to_object());
			}
			else 
			{ 
				myprintf("swapDepths has received invalid arg\n"); 
				return; 
			} 
		
		if (sprite == NULL || target == NULL) 
		{ 
			myprintf("It is impossible to swap NULL character\n"); 
			return; 
		} 
		
		if (sprite->get_parent() == target->get_parent() && sprite->get_parent() != NULL) 
		{ 
			int target_depth = target->get_depth(); 
			target->set_depth(sprite->get_depth()); 
			sprite->set_depth(target_depth); 
			sprite_instance* parent = cast_to<sprite_instance>(sprite->get_parent());
			parent->m_display_list.swap_characters(sprite, target); 
		} 
		else 
		{ 
			myprintf("MovieClips should have the same parent\n"); 
		} 
	} 
	
	//duplicateMovieClip(name:String, depth:Number, [initObject:Object]) : MovieClip 
	void sprite_duplicate_movieclip(const fn_call& fn) 
	{ 
		sprite_instance* sprite = sprite_getptr(fn);
		if (fn.nargs >= 2) 
		{ 
			character* ch = sprite->clone_display_object(
																									 fn.arg(0).to_tu_string(), 
																									 fn.arg(1).to_int() + ADJUST_DEPTH_VALUE);
			
			if (fn.nargs == 3) 
			{ 
				as_object* init_object = fn.arg(2).to_object();
				if (init_object)
				{
					for (string_hash<as_value>::const_iterator it = init_object->m_members.begin(); 
						it != init_object->m_members.end(); ++it ) 
					{ 
						ch->set_member(it->first, it->second); 
					} 
				}
			} 
			fn.result->set_as_object(ch); 
			return;
		} 
		myprintf("duplicateMovieClip needs 2 or 3 args\n"); 
	} 
	
	void sprite_get_depth(const fn_call& fn)
	{
		sprite_instance* sprite = sprite_getptr(fn);
		fn.result->set_int(sprite->get_depth() - ADJUST_DEPTH_VALUE);
	}
	
	//createEmptyMovieClip(name:String, depth:Number) : MovieClip
	void sprite_create_empty_movieclip(const fn_call& fn)
	{
		sprite_instance* sprite = sprite_getptr(fn);
		if (fn.nargs != 2)
		{
			myprintf("createEmptyMovieClip needs 2 args\n");
			return;
		}
		
		character* ch = sprite->add_empty_movieclip(fn.arg(0).to_string(),
																								fn.arg(1).to_int() + ADJUST_DEPTH_VALUE);
		fn.result->set_as_object(ch);
	}
	
	// removeMovieClip() : Void 
	void sprite_remove_movieclip(const fn_call& fn) 
	{ 
		sprite_instance* sprite = sprite_getptr(fn);
		sprite_instance* parent = cast_to<sprite_instance>(sprite->get_parent());
		if (parent && sprite) 
		{ 
			parent->remove_display_object(sprite); 
		} 
	} 
	
	// loadMovie(url:String, [method:String]) : Void
	// TODO: implement [method:String]
	void sprite_loadmovie(const fn_call& fn) 
	{ 
		if (fn.nargs >= 1)
		{
			character* mc = fn.env->load_file(fn.arg(0).to_string(), fn.this_ptr);
			
			// extension
			fn.result->set_as_object(mc);
		}
		
	} 
	
	// public unloadMovie() : Void
	void sprite_unloadmovie(const fn_call& fn) 
	{ 
		//		sprite_instance* sprite = sprite_getptr(fn);
		//				UNUSED(sprite);
		fn.env->load_file("", fn.this_ptr);
	} 
	
	// getNextHighestDepth() : Number
	// Determines a depth value that you can pass to MovieClip.attachMovie(),
	// MovieClip.duplicateMovieClip(), or MovieClip.createEmptyMovieClip()
	// to ensure that Flash renders the movie clip in front of all other objects
	// on the same level and layer in the current movie clip.
	// The value returned is 0 or larger (that is, negative numbers are not returned).
	// Content created at design time (in the authoring tool) starts at depth -16383.
	void sprite_getnexthighestdepth(const fn_call& fn) 
	{ 
		int highest_depth;
		sprite_instance* sprite = sprite_getptr(fn);
		highest_depth = sprite->get_highest_depth() - ADJUST_DEPTH_VALUE;
		
		// highest_depth must be 0 or larger !!!
		//highest_depth = max( highest_depth, 0 );
		assert(highest_depth >= 0);
		fn.result->set_int(highest_depth);
	} 
	
	// public createTextField(instanceName:String, depth:Number,
	// x:Number, y:Number, width:Number, height:Number) : TextField
	void sprite_create_text_field(const fn_call& fn) 
	{ 
		sprite_instance* sprite = sprite_getptr(fn);
		fn.result->set_as_object(NULL);
		if (fn.nargs != 6)
		{
			myprintf("createTextField: the number of arguments must be 6\n");
			return;
		}
		
		fn.result->set_as_object(sprite->create_text_field(
																											 fn.arg(0).to_string(),	// field name
																											 fn.arg(1).to_int() + ADJUST_DEPTH_VALUE,	// depth
																											 fn.arg(2).to_int(),	// x
																											 fn.arg(3).to_int(),	// y
																											 fn.arg(4).to_int(),	// width
																											 fn.arg(5).to_int()	// height
																											 ));
	} 
	
	//public attachAudio(id:Object) : Void
	// id:Object - The object that contains the audio to play. 
	// Valid values are a Microphone object, a NetStream object that is playing an FLV file, 
	// and false (stops playing the audio).
	void sprite_attach_audio(const fn_call& fn) 
	{ 
		sprite_instance* sprite = sprite_getptr(fn);
		if (fn.nargs >= 1)
		{
      // fixme for iOS
		//	as_microphone* obj = cast_to<as_microphone>(fn.arg(0).to_object());
		//	if (obj)
		//	{
		//		obj->add_listener(sprite);
		//	}
		}
		//		myprintf("attachAudio needs arg\n"); 
	} 
	
	// public attachMovie(id:String, name:String, depth:Number, [initObject:Object]) : MovieClip
	// Takes a symbol from the library and attaches it to the movie clip.
	void sprite_attach_movie(const fn_call& fn) 
	{ 
		sprite_instance* sprite = sprite_getptr(fn);
		if (fn.nargs >= 3)
		{
			tu_string id = fn.arg(0).to_string();	// the exported name (sprite_definition)
			tu_string name = fn.arg(1).to_string();	// instance name
			int depth = fn.arg(2).to_int() + ADJUST_DEPTH_VALUE;
			sprite_instance* ch = sprite->attach_movie(id, name, depth);
			
			if (fn.nargs >= 4)
			{
				as_object* init_object = fn.arg(3).to_object();
				if (init_object)
				{
					//init_object->copy_to(ch);
					// use set_member() instaed of just copying

					for (string_hash<as_value>::const_iterator it = init_object->m_members.begin(); it != init_object->m_members.end(); ++it ) 
					{
							const tu_string& name =  it->first;
							const as_value& val =  it->second;
							ch->set_member(name, val);
					}
				}
			}
			fn.result->set_as_object(ch);
			return;
		}
	} 
	
	// public localToGlobal(pt:Object) : Void
	void sprite_local_global(const fn_call& fn) 
	{ 
		sprite_instance* sprite = sprite_getptr(fn);
		if (fn.nargs == 1)
		{
			as_object* pt = fn.arg(0).to_object();
			if (pt)
			{
				sprite->local_to_global(pt);
			}
		}
	}
	
	// public globalToLocal(pt:Object) : Void
	void sprite_global_local(const fn_call& fn) 
	{ 
		sprite_instance* sprite = sprite_getptr(fn);
		if (fn.nargs == 1)
		{
			as_object* pt = fn.arg(0).to_object();
			if (pt)
			{
				sprite->global_to_local(pt);
			}
		}
	}
	
	
	// bakeinflash extension
	void sprite_get_play_state(const fn_call& fn)
	{
		sprite_instance* sprite = sprite_getptr(fn);
		bool state = sprite->get_play_state() == character::STOP ? false : true;
		fn.result->set_bool(state);
	}

	void sprite_load_bitmaps(const fn_call& fn)
	{
		sprite_instance* sprite = sprite_getptr(fn);
		if (sprite)
		{
			delete sprite->m_bitmap_hash;
			sprite->m_bitmap_hash = new array < smart_ptr<bitmap_info> >();
			sprite->load_bitmaps(sprite->m_bitmap_hash);
		}
	}
	
	// drawing API
	
	//	public beginFill(rgb:Number, [alpha:Number]) : Void
	void sprite_begin_bitmapfill(const fn_call& fn)
	{
		if (fn.nargs < 1)
		{
			return;
		}
	}

	//	public beginFill(rgb:Number, [alpha:Number]) : Void
	void sprite_begin_fill(const fn_call& fn)
	{
		if (fn.nargs < 1)
		{
			return;
		}

		sprite_instance* sprite = sprite_getptr(fn);
		canvas* canva = sprite->get_canvas();
		assert(canva);
		
		rgba color(fn.arg(0).to_number());
		color.m_a = fn.nargs > 1 ? iclamp(int(fn.arg(1).to_number() / 100.0 * 255.0), 0, 255) : 255;
		canva->begin_fill(color);
	}
	
	//	public endFill() : Void
	void sprite_end_fill(const fn_call& fn)
	{
		sprite_instance* sprite = sprite_getptr(fn);
		canvas* canva = sprite->get_canvas();
		assert(canva);
		canva->end_fill();
	}
	
	//	public clear() : Void
	void sprite_clear(const fn_call& fn)
	{
		sprite_instance* sprite = sprite_getptr(fn);
		sprite->remove_display_object(get_canvas_name());
	}
	
	// public moveTo(x:Number, y:Number) : Void
	void sprite_move_to(const fn_call& fn)
	{
		sprite_instance* sprite = sprite_getptr(fn);
		canvas* canva = sprite->get_canvas();
		assert(canva);
		
		if (fn.nargs >= 2)
		{
			float x = PIXELS_TO_TWIPS(fn.arg(0).to_float());
			float y = PIXELS_TO_TWIPS(fn.arg(1).to_float());
			canva->move_to(x, y);
		}
	}
	
	//	public lineTo(x:Number, y:Number) : Void
	void sprite_line_to(const fn_call& fn)
	{
		sprite_instance* sprite = sprite_getptr(fn);
		canvas* canva = sprite->get_canvas();
		assert(canva);
		
		if (fn.nargs >= 2)
		{
			float x = PIXELS_TO_TWIPS(fn.arg(0).to_float());
			float y = PIXELS_TO_TWIPS(fn.arg(1).to_float());
			canva->line_to(x, y);
		}
	}
	
	//	public curveTo(controlX:Number, controlY:Number, anchorX:Number, anchorY:Number) : Void
	void sprite_curve_to(const fn_call& fn)
	{
		sprite_instance* sprite = sprite_getptr(fn);
		canvas* canva = sprite->get_canvas();
		assert(canva);
		
		if (fn.nargs >= 4)
		{
			float cx = PIXELS_TO_TWIPS(fn.arg(0).to_float());
			float cy = PIXELS_TO_TWIPS(fn.arg(1).to_float());
			float ax = PIXELS_TO_TWIPS(fn.arg(2).to_float());
			float ay = PIXELS_TO_TWIPS(fn.arg(3).to_float());
			canva->curve_to(cx, cy, ax, ay);
		}
	}
	
	// public lineStyle(thickness:Number, rgb:Number, alpha:Number, pixelHinting:Boolean,
	// noScale:String, capsStyle:String, jointStyle:String, miterLimit:Number) : Void
	void sprite_line_style(const fn_call& fn)
	{
		sprite_instance* sprite = sprite_getptr(fn);
		canvas* canva = sprite->get_canvas();
		assert(canva);
		
		// If a thickness is not specified, or if the parameter is undefined, a line is not drawn
		if (fn.nargs == 0)
		{
			canva->m_current_line = 0;
			canva->add_path(false);
			return;
		}
		
		Uint16 width = (Uint16) PIXELS_TO_TWIPS(fclamp(fn.arg(0).to_float(), 0, 255));
		rgba color(0, 0, 0, 255);
		tu_string caps_style = "";
		
		if (fn.nargs >= 2)
		{
			color.set(fn.arg(1).to_float());
			if (fn.nargs >= 3)
			{
				float alpha = fclamp(fn.arg(2).to_float(), 0, 100);
				color.m_a = Uint8(255 * (alpha/100));
				
				// capsStyle:String - Added in Flash Player 8. 
				// A string that specifies the type of caps at the end of lines.
				// Valid values are: "round", "square", and "none". 
				// If a value is not indicated, Flash uses round caps. 
				if (fn.nargs >= 6)
				{
					caps_style = fn.arg(5).to_tu_string();
				}
			}
		}
		
		canva->set_line_style(width, color, caps_style);
	}
	
	// AS3
	void sprite_add_script(const fn_call& fn)
	{
		sprite_instance* sprite = sprite_getptr(fn);
		for (int i = 0, n = fn.nargs; i < n; i += 2)
		{
			// arg #1 - frame number, 0 based
			// arg #2 - function
			sprite->add_script(fn.arg(i).to_int(), fn.arg(i + 1).to_function());
		}
	}

	// AS3
	void sprite_add_child(const fn_call& fn)
	{
		sprite_instance* mc = sprite_getptr(fn);
		if (mc && fn.nargs > 0) 
		{
			as_mcloader* mcl = cast_to<as_mcloader>(fn.arg(0).to_object());
			if (mcl)
			{
				mcl->set_target(0, mc);		// hack 0
			}
		}
	}

	// AS3
	// public function addEventListener(type:String, listener:Function, useCapture:Boolean = false, priority:int = 0, useWeakReference:Boolean = false):void
	void sprite_add_event_listener(const fn_call& fn)
	{
		sprite_instance* sprite = sprite_getptr(fn);
		if (fn.nargs >= 2) 
		{
			// arg #1 - event
			// arg #2 - function
			sprite->add_event_listener(fn.arg(0).to_tu_string(), fn.arg(1).to_function());
		}
	}

	// AS3
	
	// removeEventListener(type:String, listener:Function, useCapture:Boolean = false):void
	void sprite_remove_event_listener(const fn_call& fn)
	{
		sprite_instance* sprite = sprite_getptr(fn);
		if (fn.nargs >= 2) 
		{
			// arg #1 - event
			// arg #2 - function
			sprite->remove_event_listener(fn.arg(0).to_tu_string(), fn.arg(1).to_function());
		}
	}

	// AS3
	// public function dispatchEvent(event:Event):Boolean
	void sprite_dispatch_event(const fn_call& fn)
	{
		sprite_instance* sprite = sprite_getptr(fn);
		if (fn.nargs >= 1) 
		{
			// arg #1 - event
			sprite->sprite_dispatch_event(fn);
		}
	}

	// public getBounds(target:Object) : Object
	void	sprite_get_bounds(const fn_call& fn)
	{
		sprite_instance* sprite = sprite_getptr(fn);
		character* target = NULL;
		if (fn.nargs > 0) 
		{
			target = cast_to<character>(fn.arg(0).to_object());
		}
		
		if (target == NULL)
		{
			target = sprite;
		}
		
		// bounds in the _parent coords
		rect bound;
		sprite->get_bound(&bound);
		
		// _parent local coords ==> global coords
		character* parent = sprite->get_parent();
		matrix m;
		if (parent)
		{
			parent->get_world_matrix(&m);
			m.transform(&bound);
		}
		
		// global coords ==> target local coords
		matrix gm;
		target->get_world_matrix(&gm);
		m.set_inverse(gm);
		m.transform(&bound);
		
		bound.twips_to_pixels();
		as_object* obj = new as_object();
		obj->set_member("xMin", bound.m_x_min);
		obj->set_member("xMax", bound.m_x_max);
		obj->set_member("yMin", bound.m_y_min);
		obj->set_member("yMax", bound.m_y_max);
		
		fn.result->set_as_object(obj);
	}
	
	// public getRect(target:Object) : Object
	void	sprite_get_rect(const fn_call& fn)
	{
		// TODO: exclude any strokes on shapes
		sprite_get_bounds(fn);
	}

	void	sprite_to_string(const fn_call& fn)
	{
		sprite_instance* sprite = sprite_getptr(fn);
		if (fn.nargs > 0)
		{
			const as_value& val = fn.arg(0);
			fn.result->set_tu_string(val.to_string());
		}
	}

	// public setMask(mc:Object) : Void
	void	sprite_set_mask(const fn_call& fn)
	{
		sprite_instance* sprite = sprite_getptr(fn);
		if (fn.nargs > 0)
		{
			// end of submit prev mask, setMask(null)
			if (sprite->m_mask_clip != NULL)
			{
				sprite->m_mask_clip->set_visible(true);
			}
			
			sprite->m_mask_clip = cast_to<sprite_instance>(fn.arg(0).to_object());
			
			if (sprite->m_mask_clip != NULL)
			{
				// submit mask, setMask(mc)
				sprite->m_mask_clip->set_visible(false);
			}
		}
	}

	// public attachBitmap(bmp:BitmapData, depth:Number, [pixelSnapping:String], [smoothing:Boolean]) : Void
	void sprite_attach_bitmap(const fn_call& fn)
	{
		sprite_instance* sprite = sprite_getptr(fn);
		if (sprite && fn.nargs >= 2)
		{
			smart_ptr<as_bitmapdata> bd = cast_to<as_bitmapdata>(fn.arg(0).to_object());
			int depth = fn.arg(1).to_int();
			if (bd != NULL && depth >= 0)
			{
				sprite->attach_bitmapdata(bd, depth);
			}

		}
	}


}
