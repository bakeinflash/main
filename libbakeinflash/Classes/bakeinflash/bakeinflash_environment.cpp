// bakeinflash_value.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "base/tu_file.h"
#include "bakeinflash/bakeinflash.h"
#include "bakeinflash/bakeinflash_value.h"
#include "bakeinflash/bakeinflash_character.h"
#include "bakeinflash/bakeinflash_sprite.h"
#include "bakeinflash/bakeinflash_log.h"
#include "bakeinflash/bakeinflash_function.h"
#include "bakeinflash/bakeinflash_render.h"

#if TU_CONFIG_LINK_TO_LIB3DS == 1
#include "bakeinflash/plugins/lib3ds/bakeinflash_3ds_inst.h"
#endif

namespace bakeinflash
{

	// misc
	enum file_type
	{
		UNKNOWN,
		SWF,
		JPG,
		X3DS,
		TXT,
		URL
	};

	// static
	file_type get_file_type(const char* url)
	{
		tu_string fn = url;
		if (fn.size() < 5)	// At least 5 symbols
		{
			return UNKNOWN;
		}

		if (strncasecmp(url, "http://", 7) == 0)
		{
			return URL;
		}

		tu_string fn_ext = fn.utf8_substring(fn.size() - 4, fn.size());
		if (fn_ext == ".swf")
		{
			return SWF;
		}
		else
			if (fn_ext == ".jpg")
			{
				return JPG;
			}
			else
				if (fn_ext == ".3ds")
				{
					return X3DS;
				}
				else
					if (fn_ext == ".txt")
					{
						return TXT;
					}
					return UNKNOWN;
	}

	// static
	tu_string get_full_url(const tu_string& workdir, const char* url)
	{
		if (strlen(url) >= 2 && (url[1] == ':' || url[0] == '/'))
		{
			// path like c:\my.swf or /home/my.swf
			return url;
		}

		if (strncasecmp(url, "http://", 7) == 0 || strncasecmp(url, "https://", 8) == 0)
		{
			// path like c:\my.swf or /home/my.swf or URL is absolute
			return url;
		}

		if (strncasecmp(url, "file:///", 8) == 0)
		{
			// path like file:///c:\my.swf or /home/my.swf or URL is absolute
			return url + 8;
		}

		// path is relative
		return workdir + url;
	}

	// static
	const char*	next_slash_or_dot(const char* word)
		// Search for next '.' or '/' character in this word.  Return
		// a pointer to it, or to NULL if it wasn't found.
	{
		for (const char* p = word; *p; p++)
		{
			if (*p == '.' && p[1] == '.')
			{
				p++;
			}
			else if (*p == '.' || *p == '/')
			{
				return p;
			}
		}

		return NULL;
	}

	void vm_stack::dump()
	{
		myprintf("===begin top stack\n");
		for (int i = size() - 1; i >= 0; i--)
		{
			const as_value& val = (*this)[i];
			val.print("", 0);
		}
		myprintf("===end stack\n");
	}

	void vm_stack::resize(int new_size)
	{
		assert(new_size <= array<as_value>::size());

		if (new_size < m_stack_size)
		{
			drop(m_stack_size - new_size);
		}
		m_stack_size = new_size; 
	}

	as_value&	vm_stack::pop()
	{
		if (m_stack_size > 0)
		{
			m_stack_size--;
			return (*this)[m_stack_size];
		}

		// empty stack
		static as_value undefined;
		return undefined; 
	}

	void	vm_stack::drop(int count)
	{
		if (count > 0)
		{
			m_stack_size -= count;
			if (m_stack_size < 0)
			{
				m_stack_size = 0;
			}

			// clear refs to avoid memory leaks
			for (int i = m_stack_size, n = array<as_value>::size(); i < n; i++)
			{
				(*this)[i].set_undefined();
				if (--count == 0)
				{
					break;
				}
			}
		}
	}

	void vm_stack::clear_refs(hash<as_object*, bool>* visited_objects, as_object* this_ptr)
	{
		for (int i = 0; i < array<as_value>::size(); i++)
		{
			as_object* obj = (*this)[i].to_object();
			if (obj)
			{
				if (obj == this_ptr)
				{
					(*this)[i].set_undefined();
				}
				else
				{
					obj->clear_refs(visited_objects, this_ptr);
				}
			}
		}
	}

	as_object* vm_stack::get_property_owner(const tu_string& name)
	{
		for (int i = size() - 1; i >= 0; i--)
		{
			as_value val;
			as_value& ival = operator[](i);
			if (ival.get_property_owner(name, &val))
			{
				return ival.to_object();
			}
		}
		return NULL;
	}

	bool vm_stack::get_property(const tu_string& name, as_value* val)
	{
		for (int i = size() - 1; i >= 0; i--)
		{
			if ((*this)[i].find_property(name, val))
			{
				return true;
			}
		}
		return false;
	}

	as_environment::as_environment()
	{
	}

	as_environment::~as_environment()
	{
		// Clean up stack frame.
		for (int i = 0; i < m_local_frames.size(); i++)
		{
			delete m_local_frames[i];
		}
	}


	void	vm_stack::push(const as_value& val) 
	{
		if (m_stack_size < array<as_value>::size())
		{
			// this reduces NEW operators
			//				(*this)[m_stack_size] = as_value(val);
			reset(val);
		}
		else
		{
			push_back(as_value(val)); 
		}
		m_stack_size++;
	}

	void as_environment::swap(int n)
		// swap stack items
	{
		for (int i = 0, k = n >> 1; i < k; i++)
		{
			as_value val = top(i);
			top(i) = top(n - i - 1);
			top(n - i - 1) = val;
		}
	}

	// Implementation of:
	//
	// loadVariables(url:String, target:Object, [method:String]) : Void
	// loadMovie(url:String, target:Object, [method:String]) : Void
	// loadMovie(url:String, target:String, [method:String]) : Void
	// unloadMovie(target:MovieClip) : Void
	// unloadMovie(target:String) : Void
	//
	// url=="" means that the load_file() works as unloadMovie(target)
	// TODO: implement [method]
	character* as_environment::load_file(const char* url, const as_value& target_value, int method)
	{
		if (strlen(url) > 8 && strncmp(url, "file:///", 8) == 0)
		{
			url += 8;
		}

		sprite_instance* target = cast_to<sprite_instance>(find_target(target_value));
		if (target == NULL)
		{
			IF_VERBOSE_ACTION(myprintf("load_file: target %s is't found\n", target_value.to_string()));
			return NULL;
		}

		// is unloadMovie() ?
		if (strlen(url) == 0)
		{
			character* parent = target->get_parent();
			if (parent)
			{
				parent->remove_display_object(target);
			}
			else	// target is _root, unloadMovie(_root)
			{
				target->clear_display_objects();
			}
			return NULL;
		}

		// is path relative ?
		tu_string file_name = url; //get_full_url(get_workdir(), url);
		switch (get_file_type(file_name.c_str()))
		{
		default:
			break;

		case TXT:
			{
				// Reads data from an external file, such as a text file and sets the values for
				// variables in a target movie clip. This action can also be used
				// to update variables in the active SWF file with new values. 
				tu_file fi(file_name.c_str(), "r");
				if (fi.get_error() == TU_FILE_NO_ERROR)
				{
					fi.go_to_end();
					int len = fi.get_position();
					fi.set_position(0);

					char* buf = (char*) malloc(len);
					if (fi.read_string(buf, len) > 0)
					{
						// decode data in the standard MIME format and copy theirs into target
						//#if TU_ENABLE_NETWORK == 1
						//						as_loadvars lv();
						//						lv.decode(buf);
						//						lv.copy_to(target);
						//#else
						myprintf("Compile bakeinflash with TU_ENABLE_NETWORK=1 to use loadVariable() function");
						//#endif
					}
					free(buf);
				}
				break;
			}

		case URL:
		case SWF:
			{
				movie_definition*	md = create_movie(file_name.c_str());
				if (md && md->get_frame_count() > 0)
				{
					target->replace_me(md);
					return target;
				}
				break;
			}

		case X3DS:
			{
#if TU_CONFIG_LINK_TO_LIB3DS == 0
				myprintf("bakeinflash is not linked to lib3ds -- can't load 3DS file\n");
#else
				static string_hash<weak_ptr<x3ds_definition> > s_3ds_defs;

				tu_string file_name = get_full_url(get_workdir(), url);

				weak_ptr<x3ds_definition> x3ds;
				s_3ds_defs.get(file_name, &x3ds);
				if (x3ds == NULL)
				{
					x3ds = new x3ds_definition(file_name);
					s_3ds_defs.set(file_name, x3ds);
				}

				if (x3ds->is_loaded())
				{
					rect bound;
					target->get_bound(&bound);
					x3ds->set_bound(bound);

					character* ch = x3ds->create_character_instance(target->get_parent(), 0);
					const tu_string name = target->get_name();
					int depth = target->get_depth();
					cxform cx;
					matrix m;
					target->get_parent()->replace_display_object(ch, name, depth, false, cx, false, m, 1, 0, 0);
					return ch;
				}
#endif
				break;
			}

		case JPG:
			{
#if TU_CONFIG_LINK_TO_JPEGLIB == 0
				myprintf("bakeinflash is not linked to jpeglib -- can't load jpeg image data!\n");
#else
				tu_string file_name = get_full_url(get_workdir(), url);
				image::rgb* im = image::read_jpeg(file_name.c_str());
				if (im)
				{
					bitmap_info* bi = render::create_bitmap_info(im);
					movie_definition* rdef = get_root()->get_movie_definition();
					bitmap_character_def*	jpeg = new bitmap_character_def(rdef, bi);
					target->replace_me(jpeg);
					return target;
				}
#endif
				break;
			}
		}
		return NULL;
	}


	bool	as_environment::get_variable(const tu_string& varname, as_value* val, const array<with_stack_entry>& with_stack) const
		// Return the value of the given var, if it's defined.
	{
		// first try raw var, их больше всего так как
		if (get_variable_raw(varname, val, with_stack))
		{
			return true;
		}

		// Path lookup rigamarole.
		as_object*	target = get_target();
		tu_string	path;
		tu_string	var;
		if (parse_path(varname, &path, &var))
		{
			// @@ Use with_stack here too???  Need to test.
			target = find_target(path.c_str());
			if (target)
			{
				return target->get_member(var, val);
			}
			else
				if ((target = get_global()->find_target(path.c_str())))
				{
					return target->get_member(var, val);
				}
				else
				{
					IF_VERBOSE_ACTION(myprintf("find_target(\"%s\") failed\n", path.c_str()));
					return false;
				}
		}
		return false;
	}


	bool	as_environment::get_variable_raw(
		const tu_string& varname,
		as_value* val,
		const array<with_stack_entry>& with_stack) const
		// varname must be a plain variable name; no path parsing.
	{
		assert(val);

		// First check the with-stack.
		for (int i = with_stack.size() - 1; i >= 0; i--)
		{
			as_object*	obj = with_stack[i].m_object.get();
			if (obj && obj->get_member(varname, val))
			{
				// Found the var in this context.
				return true;
			}
		}

		// Then check locals.
		if (get_local(varname, val))
		{
			return true;
		}

		// Check movie members.
		if (m_target != NULL && m_target->get_member(varname, val))
		{
			return true;
		}

		// Check this, _global, _root
		as_standard_member	varname_id = get_standard_member(varname);
		switch (varname_id)
		{
		default:
			break;

		case M_GLOBAL:
			val->set_as_object(get_global());
			return true;

		case MTHIS:
			val->set_as_object(get_target());
			return true;

		case M_ROOT:
		case M_LEVEL0:
			val->set_as_object(get_root()->get_root_movie());
			return true;

		case M_NAN:
			val->set_double(get_nan());
			return true;
		}

		// check _global.member
		if (get_global()->get_member(varname, val))
		{
			return true;
		}

		// Fallback.
		IF_VERBOSE_ACTION(myprintf("get_variable_raw(\"%s\") failed, returning UNDEFINED.\n", varname.c_str()));
		return false;
	}


	character*	as_environment::get_target() const
	{
		return cast_to<character>(m_target.get());
	}

	void as_environment::set_target(as_object* target)
	{
		m_target = target;
	}

	void as_environment::set_target(as_value& target, character* original_target)
	{
		if (target.is_string())
		{
			tu_string path = target.to_tu_string();
			IF_VERBOSE_ACTION(myprintf("-------------- ActionSetTarget2: %s", path.c_str()));
			if (path.size() > 0)
			{
				character* tar = cast_to<character>(find_target(path.c_str()));
				if (tar)
				{
					set_target(tar);
					return;
				}
			}
			else
			{
				set_target(original_target);
				return;
			}
		}
		else
			if (target.is_object())
			{
				IF_VERBOSE_ACTION(myprintf("-------------- ActionSetTarget2: %s", target.to_string()));
				character* tar = cast_to<character>(find_target(target));
				if (tar)
				{
					set_target(tar);
					return;
				}
			}
			IF_VERBOSE_ACTION(myprintf("can't set target %s\n", target.to_string()));
	}

	void	as_environment::set_variable(
		const tu_string& varname,
		const as_value& val,
		const array<with_stack_entry>& with_stack)
		// Given a path to variable, set its value.
	{
		IF_VERBOSE_ACTION(myprintf("-------------- %s = %s\n", varname.c_str(), val.to_string()));//xxxxxxxxxx

		// Path lookup rigamarole.
		character*	target = get_target();
		tu_string	path;
		tu_string	var;
		if (parse_path(varname, &path, &var))
		{
			target = cast_to<character>(find_target(path.c_str()));
			if (target)
			{
				target->set_member(var, val);
			}
		}
		else
		{
			set_variable_raw(varname, val, with_stack);
		}
	}


	void	as_environment::set_variable_raw(
		const tu_string& varname,
		const as_value& val,
		const array<with_stack_entry>& with_stack)
		// No path rigamarole.
	{
		// Check the with-stack.
		for (int i = with_stack.size() - 1; i >= 0; i--)
		{
			as_object*	obj = with_stack[i].m_object.get();
			as_value unused;
			if (obj && obj->get_member(varname, &unused))
			{
				// This object has the member; so set it here.
				obj->set_member(varname, val);
				return;
			}
		}

		// Check locals.
		if (update_local(varname, val))
		{
			// Set local var.
			return;
		}

		if (m_target != NULL)
		{
			m_target->set_member(varname, val);
		}
		else
		{
			// assume local var
			// This case happens for example so
			// class myclass
			// {
			//		function myfunc()
			//		{
			//			for (i=0;...)		should be for (var i=0; ...)
			//			{
			//			}
			//		}
			//	}
			set_local(varname, val);
			IF_VERBOSE_ACTION(myprintf("can't set_variable_raw %s=%s, target is NULL, it's assumed as local\n",
				varname.c_str(), val.to_string()));
			IF_VERBOSE_ACTION(myprintf("probably you forgot to declare variable '%s'\n", varname.c_str()));

		}
	}


	bool	as_environment::set_local(const tu_string& name, const as_value& val)
		// Set/initialize the value of the local variable.
	{
		if (m_local_frames.size() == 0)
		{
			m_local_frames.push_back(NULL);
		}

		if (m_local_frames.back() == NULL)
		{
			m_local_frames.back() = new string_hash<as_value>;
		}
		m_local_frames.back()->set(name, val);
		return true;
	}

	bool	as_environment::get_local(const tu_string& name, as_value* val) const
	{
		for (int i = m_local_frames.size() - 1; i >= 0; i--)
		{
			if (m_local_frames[i] && m_local_frames[i]->get(name, val))
			{
				return true;
			}
		}
		return false;
	}

	bool	as_environment::update_local(const tu_string& name, const as_value& val)
	{
		if (m_local_frames.size() > 0 && m_local_frames.back() && m_local_frames.back()->get(name, NULL))
		{
			m_local_frames.back()->set(name, val);
			return true;
		}
		return false;
	}

	void	as_environment::declare_local(const tu_string& name)
		// Create the specified local var if it doesn't exist already.
	{
		// NOP ??
		// Is it in the current frame already?
		/*		if (m_local_frames.size() > 0 && m_local_frames.back()->get(name, NULL) == false)
		{
		// Not in frame; create a new local var.
		m_local_frames.back()->set(name, as_value());
		}
		else
		{
		// In frame already; don't mess with it.
		}*/
	}

	as_value* as_environment::get_register(int reg)
	{
		as_value* val = local_register_ptr(reg);
		IF_VERBOSE_ACTION(myprintf("-------------- get_register(%d): %s at %p\n", 
			reg, val->to_string(), val->to_object()));
		return val;
	}

	void as_environment::set_register(int reg, const as_value& val)
	{
		IF_VERBOSE_ACTION(myprintf("-------------- set_register(%d): %s at %p\n",
			reg, val.to_string(), val.to_object()));
		*local_register_ptr(reg) = val;
	}

	as_value*	as_environment::local_register_ptr(int reg)
		// Return a pointer to the specified local register.
		// Local registers are numbered starting with 1.
		//
		// Return value will never be NULL.  If reg is out of bounds,
		// we log an error, but still return a valid pointer (to
		// global reg[0]).  So the behavior is a bit undefined, but
		// not dangerous.
	{
		// We index the registers from the end of the register
		// array, so we don't have to keep base/frame
		// pointers.

		assert(reg >=0 && reg <= m_local_register.size());

		// Flash 8 can have zero register (-1 for zero)
		return &m_local_register[m_local_register.size() - reg - 1];
	}

	// Should be highly optimized !!!
	bool	as_environment::parse_path(const tu_string& var_path, tu_string* path, tu_string* var)
		// See if the given variable name is actually a sprite path
		// followed by a variable name.  These come in the format:
		//
		// 	/path/to/some/sprite/:varname
		//
		// (or same thing, without the last '/')
		//
		// or
		//	path.to.some.var
		//
		// If that's the format, puts the path part (no colon or
		// trailing slash) in *path, and the varname part (no color)
		// in *var and returns true.
		//
		// If no colon, returns false and leaves *path & *var alone.
	{
		// Search for colon.
		const char* colon = strrchr(var_path.c_str(), ':');
		if (colon)
		{
			// Make the subparts.
			*var = colon + 1;

			// delete prev '/' if it is not first character
			if (colon > var_path.c_str() + 1 && *(colon - 1) == '/')
			{
				colon--;
			}
			*path = var_path;
			path->resize(int(colon - var_path.c_str()), true);
			return true;
		}
		else
		{
			// Is there a dot?  Find the last one, if any.
			colon = strrchr(var_path.c_str(), '.');
			if (colon)
			{
				// Make the subparts.
				*var = colon + 1;
				*path = var_path;
				path->resize(int(colon - var_path.c_str()), true);
				return true;
			}
		}
		return false;
	}

	as_object*	as_environment::find_target(const as_value& target) const
	{
		if (m_target != NULL)
		{
			return m_target->find_target(target);
		}
		return target.to_object();
	}

	bool	as_environment::set_member(const tu_string& name, const as_value& val)
	{
		if (m_target != NULL)
		{
			return m_target->set_member(name, val);
		}
		return false;
	}

	bool	as_environment::get_member(const tu_string& name, as_value* val)
	{
		if (m_target != NULL)
		{
			return m_target->get_member(name, val);
		}
		return false;
	}

	void as_environment::clear_refs(hash<as_object*, bool>* visited_objects, as_object* this_ptr)
	{
		// target
		if (m_target.get() == this_ptr)
		{
			m_target = NULL;
		}

		// local vars
		for (int i = 0, n = m_local_frames.size(); i < n; i++)
		{
			if (m_local_frames[i])
			{
				for (string_hash<as_value>::iterator it = m_local_frames[i]->begin(); it != m_local_frames[i]->end(); ++it)
				{
					as_object* obj = it->second.to_object();
					if (obj)
					{
						if (obj == this_ptr)
						{
							it->second.set_undefined();
						}
						else
						{
							obj->clear_refs(visited_objects, this_ptr);
						}
					}
				}
			}
		}

		// clear refs to 'this_ptr' from stack
		vm_stack::clear_refs(visited_objects, this_ptr);

		// global register
		for (int i = 0, n = GLOBAL_REGISTER_COUNT; i < n; i++)
		{
			as_object* obj = m_global_register[i].to_object();
			if (obj)
			{
				if (obj == this_ptr)
				{
					m_global_register[i].set_undefined();
				}
				else
				{
					obj->clear_refs(visited_objects, this_ptr);
				}
			}
		}

		// local register
		for (int i = 0, n = m_local_register.size(); i < n; i++)
		{
			as_object* obj = m_local_register[i].to_object();
			if (obj)
			{
				if (obj == this_ptr)
				{
					m_local_register[i].set_undefined();
				}
				else
				{
					obj->clear_refs(visited_objects, this_ptr);
				}
			}
		}
	}


}
