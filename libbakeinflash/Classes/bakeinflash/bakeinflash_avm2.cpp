// bakeinflash_avm2.cpp	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// AVM2 implementation

#include "bakeinflash/bakeinflash_avm2.h"
#include "bakeinflash/bakeinflash_stream.h"
#include "bakeinflash/bakeinflash_log.h"
#include "bakeinflash/bakeinflash_abc.h"
#include "bakeinflash/bakeinflash_disasm.h"
#include "bakeinflash/bakeinflash_character.h"
#include "bakeinflash/bakeinflash_sprite.h"
//#include "bakeinflash_jit.h"
#include "bakeinflash/bakeinflash_as_classes/as_array.h"

namespace bakeinflash
{

	as3function_def::as3function_def(abc_def* abc, int method) :
		m_abc(abc),
		m_return_type( -1 ),
		m_name( -1 ),
		m_flags( 0 ),
		m_method(method),
		m_max_stack( 0 ),
		m_local_count( 0 ),
		m_init_scope_depth( 0 ),
		m_max_scope_depth( 0 )
	{
	}

	as3function_def::~as3function_def()
	{
	}

	as3function::as3function(as3function_def* def, as_object* target) :
		m_def(def),
		m_target(target)
	{
		assert(target != NULL);
//		assert(target->get_environment());

		m_this_ptr = this;

		// any function MUST have prototype
		builtin_member("prototype", new as_object());
	}

	as3function::~as3function()
	{
	}

	const char*	as3function::to_string()
	{
		// NOT THREAD SAFE!!!
		static char buffer[50];
		snprintf(buffer, 50, " <as3_function %p>", this);
		return buffer; 
	}

	void as3function::set_target(as_object* target)
	{
		//sprite_instance mc = cast_to<sprite_instance>(target);
		assert(target->get_environment());
		m_target = target;
	}

	void	as3function::operator()(const fn_call& fn)
	// dispatch
	{
		// set 'this'
		assert(fn.this_ptr);
		as_object* this_ptr = fn.this_ptr;
		if (this_ptr->m_this_ptr != NULL && this_ptr != this_ptr->m_this_ptr.get())
		{
			this_ptr = this_ptr->m_this_ptr.get();
		}

		if (m_target == NULL)
		{
			// hack, for class constructors saved in _global object
			m_target = this_ptr;
		}

		// Keep target alive during execution!
		smart_ptr<as_object> target(m_target.get());

		// Function has been declared in moviclip ==> we should use its environment
		// At the same time 'this_ptr' may refers to another object
		// see testcase in .h file
		as_environment* env = m_target->get_environment();
		if (env == NULL)
		{
			env = fn.env;
		}
		assert(env);


		// Create local registers.
		array<as_value>	local_register;
		local_register.resize(m_def->m_local_count + 1);

		// Register 0 holds the ?this? object. This value is never null.
		assert(this_ptr);
		local_register[0] = this_ptr;

		// Registers 1 through method_info.param_count holds parameter values.
		// If fewer than method_body_info.local_count values are supplied to the call then
		// the remaining values are either the values provided by default value declarations 
		// or the value undefined.
		for (int i = 0; i < m_def->m_param_type.size(); i++)
		{
			// A zero value denotes the any (?*?) type.
			if (i < fn.nargs)
			{
				local_register[i + 1] = fn.arg(i);
			}
			else
			{
				// TODO.. test
//				printf("TODO: test default values\n");
//				const char* name = m_def->m_abc->get_multiname(m_def->m_param_type[i]);
//				local_register[i + 1].set_undefined();
			}
		}

#if _ENABLE_JIT==1

//		if (m_compiled_code.is_valid() == false)
		{
//			compile();
//			m_compiled_code.initialize();
		}
		/*
		if (m_compiled_code.is_valid() )
		{
			try
			{
				m_compiled_code.call< array<as_value>&, vm_stack&, vm_stack&, as_value* >
					(local_register, *env, env->m_scope, fn.result );
			}
			catch( ... )
			{
				myprintf( "jitted code crashed" );
			}
		}
		else
		*/

#else

			// keep stack size on entry
			int stack_size = env->size();
			int scope_size = get_root()->get_scope()->size();

			IF_VERBOSE_ACTION(myprintf("\n***begin*** #%d, stack size=%d, scope size=%d\n", m_def->m_method, env->size(), scope_size));

			// push THIS in scope.. hack ?
			get_root()->get_scope()->push(this_ptr);

			// Execute the actions.
			m_def->execute(local_register, env, fn.result);

			get_root()->get_scope()->resize(scope_size);
			IF_VERBOSE_ACTION(myprintf("***end*** #%d, stack size=%d, scope size=%d\n\n", m_def->m_method, env->size(), scope_size));

			if (stack_size != env->size())
			{
				myprintf("****************** error: stack size on entry=%d, on exit=%d\n",	stack_size, env->size());

				// restore stack size
				env->resize(stack_size);

				// a bug
				assert(0);
			}

#endif

	}

	bool as3function_def::get_multiname(int index, as_environment* env, as_value* val)
	{
		//		tu_string ns;
		multiname::kind kind = (multiname::kind) m_abc->get_multiname_type(index);
		switch (kind)
		{
			case multiname::CONSTANT_MultinameL :
				// runtime multiname
				*val = env->pop();
				break;

			case multiname::CONSTANT_Multiname:
			case multiname::CONSTANT_QName:
				val->set_tu_string(m_abc->get_multiname(index));
				break;

			default:
				assert(!"todo");
				break;
		}
		return true;
	}


	// interperate action script bytecode
	void	as3function_def::execute(array<as_value>& lregister, as_environment* env, as_value* result)
	{
		// m_abc may be destroyed
		assert(m_abc != NULL);

		smart_ptr<as_object> this_ptr = lregister[0].to_object();		// local0 is always this
		assert(this_ptr);
		
		// some method has no body
		if (m_code.size() == 0)
		{
			return;
		}


		vm_stack* scope = get_root()->get_scope();
		assert(scope);

		int ip = 0;
		do
		{
			//printf("stack size %d\n", env->size() );
			Uint8 opcode = m_code[ip++];
			switch (opcode)
			{
				case 0x01: // bkpt
				{
					// If in debug mode, enter the debugger.
					IF_VERBOSE_ACTION(myprintf("EX: bkpt\n")); 
					break;
				}

				case 0x07: // dxnslate
				{
					// Sets the default XML namespace with a value determined at runtime.
					as_value val = env->pop();
					IF_VERBOSE_ACTION(myprintf("EX: dxnslate\t %s\n", val.to_string()));
					break;
				}

				case 0x08: // kill
				{
					// Kills a local register
					// The local register at index is killed. It is killed by setting its value to undefined

					int index;
					ip += read_vu30(index, &m_code[ip]);

					IF_VERBOSE_ACTION(myprintf("EX: kill\t index=%i\n", index));
					lregister[index].set_undefined();
					break;
				}

				case 0x09: // label
				{
					// Do nothing. Used to indicate that this location is the target of a branch.
					IF_VERBOSE_ACTION(myprintf("EX: label\n"));
					break;
				}

				case 0x0C: // ifnlt
				{
					// Branch if the first value is not less than the second value.. 
					// stack: ..,value1,value2=> ... 

					bool branch = env->top(1).to_number() >= env->top(0).to_number();
					if (branch)
					{
						int offset = read_s24(&m_code[ip]);
						ip += offset;
					}
					ip += 3;

					IF_VERBOSE_ACTION(myprintf("EX: ifnlt\t %d\n", branch));
					env->drop(2);
					break;
				}

				case 0x0D: // ifnle
				{
					// Branch if the first value is not less than or equal to the second value. 
					// stack: ..,value1,value2=> ... 

					bool branch = env->top(1).to_number() > env->top(0).to_number();
					if (branch)
					{
						int offset = read_s24(&m_code[ip]);
						ip += offset;
					}
					ip += 3;

					IF_VERBOSE_ACTION(myprintf("EX: ifnle\t %d\n", branch));
					env->drop(2);
					break;
				}

				case 0x0E: // ifngt
				{
					// Branch if the first value is not greater than the second value.
					// stack: ...value1,value2=> ... 

					const as_value& val1 = env->top(1);
					const as_value& val2 = env->top(0);

					// branch if undefined !!!
					bool notbranch = val1.is_number() && val2.is_number() && val2.to_number() < val1.to_number();
					if (notbranch == false)
					{
						int offset = read_s24(&m_code[ip]);
						ip += offset;
					}
					ip += 3;

					IF_VERBOSE_ACTION(myprintf("EX: ifngt\t %s, %s, %s\n",	val1.to_string(), val2.to_string(), notbranch ? "not branch" : "branch"));
					env->drop(2);
					break;
				}

				case 0x0F: // ifnge
				{
					// Compute value1 < value2 using the abstract relational comparison algorithm in ECMA-262
					// If the result of the comparison is not false, jump the number of bytes indicated by offset
					// stack: ...value1,value2=> ... 

					const as_value& val1 = env->top(1);
					const as_value& val2 = env->top(0);

					// If either of the compared values is NaN then the comparison value1 < value2 will return undefined. 
					// In that case ifnge will branch (undefined is not false)
					bool branch = val1.is_number() == false || val2.is_number() == false || (val1.is_number() && val2.is_number() && val1.to_number() < val2.to_number());
					if (branch)
					{
						int offset = read_s24(&m_code[ip]);
						ip += offset;
					}
					ip += 3;

					IF_VERBOSE_ACTION(myprintf("EX: ifnge\t %s, %s, %s\n",	val1.to_string(), val2.to_string(), branch ? "branch" : "not branch"));
					env->drop(2);
					break;
				}

				case 0x10: // jump
				{
					int offset = read_s24(&m_code[ip]);
					ip += offset;
					ip += 3;
					IF_VERBOSE_ACTION(myprintf("EX: jump\n"));
					break;
				}

				case 0x11: // iftrue
				{
					//Follows ECMA-262 11.9.3
					bool taken = env->top(0).to_bool();
					env->drop(1);
					if (taken == true)
					{
						int offset = read_s24(&m_code[ip]);
						ip += offset;
					}
					ip += 3;

					IF_VERBOSE_ACTION(myprintf("EX: iftrue\t %s\n", taken? "taken": "not taken"));
					break;
				}

				case 0x12: // iffalse
				{
					//Follows ECMA-262 11.9.3
					bool taken = env->top(0).to_bool();
					env->drop(1);
					if (taken == false)
					{
						int offset = read_s24(&m_code[ip]);
						ip += offset;
					}
					ip += 3;

					IF_VERBOSE_ACTION(myprintf("EX: iffalse\t %s\n", taken? "taken": "not taken"));
					break;
				}

				case 0x13: // ifeq
				{
					// branch if the first value is equal to the second value. 
					bool taken = env->top(1) == env->top(0);
					if (taken)
					{
						int offset = read_s24(&m_code[ip]);
						ip += offset;
					}
					ip += 3;

					IF_VERBOSE_ACTION(myprintf("EX: ifeq\t %s\n", taken? "taken": "not taken"));
					env->drop(2);
					break;
				}

				case 0x14: // ifne
				{
					// branch if the first value is not equal to the second value. 
					bool taken = env->top(1) == env->top(0);
					env->drop(2);
					if (taken == false)
					{
						int offset = read_s24(&m_code[ip]);
						ip += offset;
					}
					ip += 3;

					IF_VERBOSE_ACTION(myprintf("EX: ifne\t %s\n", taken? "taken": "not taken"));
					break;
				}

				case 0x15: // iflt
				{
					// Branch if the first value is less than the second value.. 
					// stack: ..,value1,value2=> ... 

					const as_value& val1 = env->top(1);
					const as_value& val2 = env->top(0);
					bool branch = val1 < val2.to_number();
					if (branch)
					{
						int offset = read_s24(&m_code[ip]);
						ip += offset;
					}
					ip += 3;

					IF_VERBOSE_ACTION(myprintf("EX: iflt\t %s, %s, %s\n",	val1.to_string(), val2.to_string(), branch ? "branch" : "not branch"));
					env->drop(2);
					break;
				}

				case 0x16: // ifle
				{
					// Branch if the first value is less or equal than the second value.. 
					// stack: ..,value1,value2=> ... 

					const as_value& val1 = env->top(1);
					const as_value& val2 = env->top(0);
					bool branch = !(val2.to_number() < val1.to_number());
					if (branch)
					{
						int offset = read_s24(&m_code[ip]);
						ip += offset;
					}
					ip += 3;

					IF_VERBOSE_ACTION(myprintf("EX: ifle\t %s, %s, %s\n",	val1.to_string(), val2.to_string(), branch ? "branch" : "not branch"));
					env->drop(2);
					break;
				}

				case 0x1A: // ifstrictne 
				{
					// branch if the first value is not equal to the second value. 
					bool taken = env->top(1) == env->top(0);
					env->drop(2);
					if (taken == false)
					{
						int offset = read_s24(&m_code[ip]);
						ip += offset;
					}
					ip += 3;

					IF_VERBOSE_ACTION(myprintf("EX: ifne\t %s\n", taken? "taken": "not taken"));
					break;
				}

				case 0x1B: // lookupswitch
				{
					// Jump to different locations based on an index
					int index = env->pop().to_int();

					int k = ip;

					int default_offset = read_s24(&m_code[k]);
					k += 3;

					int case_count;
					k += read_vu30(case_count, &m_code[k]);

					// There are case_count+1 case offsets
					case_count++;

					array<int> offsets;
					offsets.resize(case_count);
					for (int i = 0; i < case_count; i++)
					{
						offsets[i] = read_s24(&m_code[k]);
						k += 3;
					}

					// The base location is the address of the lookupswitch instruction itself. 
					ip += (index < 0 || index >= case_count) ? default_offset : offsets[index];
				//	ip += k;

					IF_VERBOSE_ACTION(myprintf("EX: lookupswitch, index=%d\n", index));
					break;
				}
 
	
				case 0x1D: // popscope
				{
					scope->pop();

					IF_VERBOSE_ACTION(myprintf("EX: popscope, size=%d\n", scope->size()));
					break;
				}

				case 0x1E:	// nextname
				{
					// Get the name of the next property when iterating over an object
					int index = env->top(0).to_int();
					as_object* obj = env->top(1).to_object();

					as_value val;
					if (obj && obj->get_member("_enumerator", &val))
					{
						as_array* a = cast_to<as_array>(val.to_object());
						val.set_undefined();
						if (a && index > 0 && index <= a->size())
						{
							val = a->operator[](index - 1);
						}
						else
						{
							obj->m_members.erase("_enumerator");
						}
					}

					IF_VERBOSE_ACTION(myprintf("EX: nextname\t %p, index=%d, item=%s\n", obj, index, val.to_string()));
					env->drop(1);
					env->top(0) = val;
					break;
				}

				case 0x20:  // pushnull
				{
					as_value value;
					value.set_null();
					env->push(value);

					IF_VERBOSE_ACTION(myprintf("EX: pushnull\n"));
					break;
				} 

				case 0x21:	// pushundefined
				{
					env->push(as_value());
					IF_VERBOSE_ACTION(myprintf("EX: pushundefined\n"));
					break;
				}

				case 0x24:	// pushbyte
				{
					Uint8 byte_value = m_code[ip];
					ip++;
					env->push(byte_value);

					IF_VERBOSE_ACTION(myprintf("EX: pushbyte\t %d\n", byte_value));
					break;
				}

				case 0x25:  // pushshort
				{
					int val;
					ip += read_vu30(val, &m_code[ip]);
					env->push(val);

					IF_VERBOSE_ACTION(myprintf("EX: pushshort\t %d\n", val));
					break;
				}

				case 0x26:  // pushtrue
				{
					env->push(true);
					IF_VERBOSE_ACTION(myprintf("EX: pushtrue\n"));
					break;
				}

				case 0x27:  // pushfalse
				{
					env->push(false);
					IF_VERBOSE_ACTION(myprintf("EX: pushfalse\n"));
					break;
				}

				case 0x28:  // pushnan
				{
					env->push(get_nan());
					IF_VERBOSE_ACTION(myprintf("EX: pushnan\n"));
					break;
				}

				case 0x29:  // pop the value from stack and discard it
				{
					env->pop();
					IF_VERBOSE_ACTION(myprintf("EX: pop\n"));
					break;
				}

				case 0x2A:  // dup
				{
					IF_VERBOSE_ACTION(myprintf("EX: dup %s\n", env->top(0).to_string()));
					env->push(env->top(0));
					break;
				} 

				case 0x2D:	// pushint
				{
					int index;
					ip += read_vu30(index, &m_code[ip]);
					int val = m_abc->get_integer(index);
					env->push(val);

					IF_VERBOSE_ACTION(myprintf("EX: pushint\t %d\n", val));
					break;
				}

				case 0x2C:	// pushstring
				{
					int index;
					ip += read_vu30(index, &m_code[ip]);
					env->push(m_abc->get_string(index).c_str());
					IF_VERBOSE_ACTION(myprintf("EX: pushstring\t '%s'\n", m_abc->get_string(index).c_str()));
					break;
				}

				case 0x2F:	// pushdouble
				{
					int index;
					ip += read_vu30(index, &m_code[ip]);
					double val = m_abc->get_double(index);
					env->push(val);

					IF_VERBOSE_ACTION(myprintf("EX: pushdouble\t %f\n", val));
					break;
				}

				case 0x30:	// pushscope
				{
					as_value val = env->pop();
					scope->push(val);

					IF_VERBOSE_ACTION(myprintf("EX: pushscope\t %s, size=%d\n", val.to_string(), scope->size()));
					break;
				}

				case 0x32:	// hasnext2
				{
					// Determine if the given object has any more properties

					int object_reg;
					ip += read_vu30(object_reg, &m_code[ip]);
					as_object* obj = lregister[object_reg].to_object();

					int index_reg;
					ip += read_vu30(index_reg, &m_code[ip]);
					int index = lregister[index_reg].to_int();
					
					bool retcode = false;
					if (obj)
					{
						as_array* a;
						if (index == 0)
						{
							// enumerate
							a = new as_array();
							obj->enumerate(a);
							obj->set_member("_enumerator", a);
						}
						else
						{
							as_value val;
							obj->get_member("_enumerator", &val);
							a = cast_to<as_array>(val.to_object());
						}

						if (a && index >= 0 && index < a->size())
						{
							retcode = true;
							lregister[index_reg] = index + 1;
						}
						else
						{
							lregister[index_reg] = 0;
							lregister[object_reg].set_null();

							obj->m_members.erase("_enumerator");

						}

					}
					IF_VERBOSE_ACTION(myprintf("EX: hasnext2\t %p, index=%d\n", obj, index_reg));
					env->push(retcode);
					break;
				}

				case 0x40:	// newfunction
				{
					// Create a new function object
					// stack: ... => ...,function_obj 
					// index is a u30 that must be an index of a method_info
					int index;
					ip += read_vu30(index, &m_code[ip]);
					as3function_def* fdef = m_abc->get_method_info(index);
					as_function* func = new as3function(fdef, env->get_target());

					env->push(func);
					IF_VERBOSE_ACTION(myprintf("EX: newfunction\n"));
					break;
				}

				case 0x42:	// Construct an instance. 
				{
					// Stack Е, object, arg1, arg2, ..., argn => Е, value 
					int nargs;
					ip += read_vu30(nargs, &m_code[ip]);

					as_object* obj = env->top(nargs).to_object();
					as_value new_obj;

					if (obj) //					&& obj->get_member(classname, &constructor))
					{
						as_value constructor;
						env->swap(nargs);
						myprintf("TODO: Construct an instance. 0x42 opcode\n");
						/*
						if (as_c_function* c_constructor = cast_to<as_c_function>(constructor.to_object()))
						{
							// C function is responsible for creating the new object and setting members.
							(*c_constructor)(fn_call(&new_obj, as_value(), env, nargs, env->get_top_index()));
						}
						else
						{
							// Create new instance of the classname
							smart_ptr<as_object>	new_obj_ptr = new as_object();

							// Call the actual constructor function; new_obj is its 'this'.
							// We don't need the function result.
							instance_info* ii = m_abc->get_instance_info(classname);
							as_function* s_constructor = m_abc->create_class_constructor(ii, new_obj_ptr.get());

							as_object* proto = new_obj_ptr->create_proto(s_constructor);

							// override m_this_ptr with just new created object
							proto->m_this_ptr = new_obj_ptr.get();

							if (s_constructor)
							{
								call_method(s_constructor, env, new_obj_ptr.get(), nargs, env->get_top_index());
							}
							else
							{
								IF_VERBOSE_ACTION(myprintf("EX: constructprop\t %p.%s(args:%d) FAILED\n", obj, classname.c_str(), nargs));
							}
							new_obj_ptr->set_ctor(s_constructor);
							new_obj.set_as_object(new_obj_ptr.get());
						}*/
					}

					IF_VERBOSE_ACTION(myprintf("EX: construct\t %p(args:%d)\n", new_obj.to_object(), nargs));
					env->drop(nargs);
					env->top(0) = new_obj;
					break;
				}

				case 0x46:  // callproperty
				{
					// Stack: ...,obj, [ns], [name],arg1,...,argn=> ...,value 
					int index;
					ip += read_vu30(index, &m_code[ip]);

					as_value name;
					get_multiname(index, env, &name);

					int nargs;
					ip += read_vu30(nargs, &m_code[ip]);

					as_value& obj = env->top(nargs);
					env->swap(nargs);

					as_value result;
					as_value func;
					if (obj.find_property(name.to_tu_string(), &func))
					{
						IF_VERBOSE_ACTION(myprintf("EX: callproperty\t %s.%s(nargs=%d)\n", obj.to_string(), name.to_string(), nargs));
						result = call_method(func, env, obj, nargs, env->get_top_index()); 
					}
					else
					{
						myprintf("EX: callproperty\t %s.%s(nargs=%d)\tFAILED\n", obj.to_string(), name.to_string(), nargs);
					}

					env->drop(nargs);
					env->top(0) = result;
					break;
				}

				case 0x47:	// returnvoid
				{
					IF_VERBOSE_ACTION(myprintf("EX: returnvoid\t\n"));

					if (result)
					{
						result->set_undefined();
					}
					else
					{
						// setter has no result
					}
					return;
				}

				case 0x48:	// returnvalue
				{
					IF_VERBOSE_ACTION(myprintf("EX: returnvalue \t%s\n", env->top(0).to_string()));
					*result = env->pop();
					return;
				}

				case 0x49:	// constructsuper
				{
					// stack: object, arg1, arg2, ..., argn
					// This will invoke the constructor on the base class of object with the given arguments

					int nargs;
					ip += read_vu30(nargs, &m_code[ip]);

					// Assume we are in a constructor
					const tu_string& class_name = m_abc->get_class_from_constructor(m_method);
					const tu_string& super_class_name = m_abc->get_super_class(class_name);

					smart_ptr<as_object> obj = env->top(nargs).to_object();

					IF_VERBOSE_ACTION(myprintf("EX: constructsuper\t %p(args:%d)\n", obj.get(), nargs));

					// get last prototype from the class chain ??
					as_object* prototype = obj.get();
					as_object* prevprototype = prototype;
					while (prototype->get_proto())
					{
						prevprototype = prototype;
						prototype = prototype->get_proto();
					}

					as_function* super_ctor = m_abc->create_class_constructor(super_class_name, obj.get());
					if (super_ctor == NULL)
					{
						as_value val;
						if (get_global()->get_member(super_class_name, &val))
						{
							super_ctor = cast_to<as_function>(val.to_object());
						}

						// copy static values, hack ?  слетвет
//						if (get_global()->get_member(class_name, &val))
//						{
//							val.to_object()->copy_to(obj);
//						}
					}
					assert(super_ctor);

					// create class methods
					instance_info* ii = m_abc->get_instance_info(class_name);
					ii->copy_to(obj.get(), prevprototype);

					env->swap(nargs);

					// call super ctor
					prototype->create_proto(super_ctor);	// create super's prototype
					call_method(super_ctor, env, obj.get(), nargs, env->get_top_index());
					env->drop(nargs + 1);
					break;
				}

				case 0x4A: //constructprop
				// Stack ..., obj, [ns], [name], arg1,...,argn => ..., value
				{
					int index;
					ip += read_vu30(index, &m_code[ip]);

					as_value classname;
					get_multiname(index, env, &classname);

					int nargs;
					ip += read_vu30(nargs, &m_code[ip]);

					as_object* obj = env->top(nargs).to_object();
					as_value constructor;
					as_value new_obj;

					env->swap(nargs);

					if (obj && obj->get_member(classname.to_tu_string(), &constructor))
					{
						if (as_c_function* c_constructor = cast_to<as_c_function>(constructor.to_object()))
						{
							// C function is responsible for creating the new object and setting members.
							(*c_constructor)(fn_call(&new_obj, as_value(), env, nargs, env->get_top_index()));
						}
						else
						{
							// Create new instance of the classname
							smart_ptr<as_object>	new_obj_ptr = new as_object();

							// Call the actual constructor function; new_obj is its 'this'.
							// We don't need the function result.
							instance_info* ii = m_abc->get_instance_info(classname.to_tu_string());
							as_function* s_constructor = m_abc->create_class_constructor(ii, new_obj_ptr.get());

							as_object* proto = new_obj_ptr->create_proto(s_constructor);

							// override m_this_ptr with just new created object
							proto->m_this_ptr = new_obj_ptr.get();

							if (s_constructor)
							{
								call_method(s_constructor, env, new_obj_ptr.get(), nargs, env->get_top_index());
							}
							else
							{
								IF_VERBOSE_ACTION(myprintf("EX: constructprop\t %p.%s(args:%d) FAILED\n", obj, classname.to_string(), nargs));
							}
							new_obj_ptr->set_ctor(s_constructor);
							new_obj.set_as_object(new_obj_ptr.get());
						}
					}
					else
					{
						myprintf("could't constructprop, object=%p, class=%s\n", obj, classname.to_string());
					}

					IF_VERBOSE_ACTION(myprintf("EX: constructprop\t %p.%s(args:%d)\n", new_obj.to_object(), classname.to_string(), nargs));
					env->drop(nargs);
					env->top(0) = new_obj;
					break;
				}

				case 0x4E:	// callsupervoid.. Call a method on a base class, discarding the return value.. super.myfunc(...) 
				{
					int index;
					ip += read_vu30(index, &m_code[ip]);
					
					as_value name;
					get_multiname(index, env, &name);

					int nargs;
					ip += read_vu30(nargs, &m_code[ip]);

					as_object* obj = env->top(nargs).to_object();

					as_value func;
					if (obj)
					{
						as_object* proto = obj->get_proto();
						if (proto && proto->get_member(name.to_tu_string(), &func))
						{
							sprite_instance* mc = cast_to<sprite_instance>(obj);
							const tu_string& mcname = mc->get_name();
							if (func.is_function())
							{
								IF_VERBOSE_ACTION(myprintf("EX: callsupervoid\t %p.%s %s(args:%d)\n", obj, mcname.c_str(), name.to_string(), nargs));
								env->swap(nargs);

								// keep proto
								smart_ptr<as_object> old_proto = proto;
								obj->set_proto(proto->get_proto());

								call_method(func, env, obj, nargs, env->get_top_index());

								// restore proto
								obj->set_proto(old_proto);
							}
							else
							{
								IF_VERBOSE_ACTION(myprintf("EX: callsupervoid\t %p.%s(args:%d)\tFAILED1\n", obj, name.to_string(), nargs));
							}
						}
						else
						{
							IF_VERBOSE_ACTION(myprintf("EX: callsupervoid\t %p.%s(args:%d)\tFAILED2\n", obj, name.to_string(), nargs));
						}
					}
					env->drop(nargs + 1);
					break;
				}

				case 0x4F:	// callpropvoid, Call a property, discarding the return value.
				// Stack: ..., obj, [ns], [name], arg1,...,argn => ...
				{
					int index;
					ip += read_vu30(index, &m_code[ip]);
					
					as_value name;
					get_multiname(index, env, &name);

					int nargs;
					ip += read_vu30(nargs, &m_code[ip]);

					as_object* obj = env->top(nargs).to_object();
					as_value func;
					if (obj && obj->get_member(name.to_tu_string(), &func))
					{
						if (func.is_function())
						{
							env->swap(nargs);
							call_method(func, env, obj, nargs, env->get_top_index());
							IF_VERBOSE_ACTION(myprintf("EX: callpropvoid\t %p.%s(args:%d)\n", obj, name.to_string(), nargs));
						}
						else
						{
							myprintf("EX: callpropvoid\t %p.%s(args:%d)\tFAILED\n", obj, name.to_string(), nargs);
						}
					}
					else
					{
						myprintf("EX: callpropvoid FAILED, object %p has no member '%s'\n", obj, name.to_string());
					}

					env->drop(nargs + 1);
					break;
				}

				case 0x55: // newobject
				{
					// nargs is a u30 that is the number of properties that will be created in newobj.
					int nargs;
					ip += read_vu30(nargs, &m_code[ip]);

					as_object* obj = new as_object();
					for (int arg_index = 0; arg_index < nargs; ++arg_index)
					{
						// for obj = {a;1,b:2,c:};
						as_value val = env->pop();
						as_value name = env->pop();
						obj->set_member(name.to_tu_string(), val);
					}
					env->push(obj);

					IF_VERBOSE_ACTION(myprintf("EX: newobject %p \t nargs:%i\n", obj, nargs));
					break;
				}

				case 0x56: //newarray
				{
					// A new value of type Array is created and assigned to newarray. The values on the stack will be assigned to the entries of the array,
					// so newarray[0] = value1, newarray[1] = value2, ...., newarray[N-1] = valueN. newarray is then pushed onto the stack. 

					int nargs;
					ip += read_vu30(nargs, &m_code[ip]);

					as_array* array = new as_array();
					int offset = env->size() - nargs;
					for (int i = 0; i < nargs; i++)
					{
						array->push(env->operator[](offset + i));
					}

					env->drop(nargs);
					env->push(array);

					IF_VERBOSE_ACTION(myprintf("EX: newarray\t %x  nargs:%i\n", array, nargs));
					break;
				}

				case 0x57: //newactivation
				{
					//stack: ... => ...,newactivation 
					// Creates a new activation object, newactivation, and pushes it onto the stack. 
					// Can only be used in methods that have the NEED_ACTIVATION flag set in their MethodInfo entry
					as_object* obj = new as_object();
					env->push(obj);

					IF_VERBOSE_ACTION(myprintf("EX: newactivation\n"));
					break;
				}

				case 0x58: // newclass
				{
					// When this instruction is executed, the scope stack must contain all the scopes of all base classes,
					// as the scope stack is saved by the created ClassClosure

					// stack:	..., basetype => ..., newclass
					int class_index;
					ip += read_vu30(class_index, &m_code[ip]);

					IF_VERBOSE_ACTION(myprintf("EX: newclass\t class index:%i, object=%p\n", class_index, this_ptr.get()));

					// add static members to class prototype
					class_info* ci = m_abc->get_class_info(class_index);
					ci->copy_to(this_ptr, this_ptr);

					smart_ptr<as_function> class_ctor = m_abc->create_initclass_function(class_index, this_ptr.get());
					if (class_ctor != NULL)
					{
						call_method(class_ctor, env, this_ptr.get(), 0, env->get_top_index());
					}
					env->top(0).set_as_object(this_ptr);

					break;
				}

				case 0x5D:	// findpropstrict
				{
					int index;
					ip += read_vu30(index, &m_code[ip]);

					as_value name;
					get_multiname(index, env, &name);

					// search property in scope
					as_object* obj = scope->get_property_owner(name.to_tu_string());

					IF_VERBOSE_ACTION(myprintf("EX: findpropstrict\t %s, owner=%p\n", name.to_string(), obj));

					//Search for a script entry to execute
					if (obj == NULL)
					{
						smart_ptr<as_object> new_class = new as_object();
						smart_ptr<as_function> func = m_abc->create_script_function(name.to_tu_string(), new_class.get());
						if (func != NULL)
						{
							// create class 
							as_object* global = get_global();

							global->set_member(name.to_tu_string(), new_class.get());

							IF_VERBOSE_ACTION(myprintf("\tfindpropstrict failed, creating new class %s\n", name.to_string()));

							call_method(func, env, new_class.get(), 0, env->get_top_index());

							obj = global;
						}
					}
//					IF_VERBOSE_ACTION(myprintf("EX: findpropstrict\t %s, owner=%p %s\n", name.c_str(), obj, obj==NULL ? "FAILED":""));
					env->push(obj);
					break;
				}

				case 0x5E:	// findproperty, Search the scope stack for a property
				{
					int index;
					ip += read_vu30(index, &m_code[ip]);

					as_value name;
					get_multiname(index, env, &name);

					as_object* obj = scope->get_property_owner(name.to_tu_string());
					if (obj)
					{
						IF_VERBOSE_ACTION(myprintf("EX: findproperty\t '%s', owner=%p\n", name.to_string(), obj));
						env->push(obj);
					}
					else
					{
						IF_VERBOSE_ACTION(myprintf("EX: findproperty\t '%s', owner=%p=global\n", name.to_string(), get_global()));
						env->push(get_global());
					}
					break;
				}

				case 0x60:	// getlex, Find and get a property.
				{
					int index;
					ip += read_vu30(index, &m_code[ip]);

					as_value name;
					get_multiname(index, env, &name);

					// search and get property in scope
					as_value val;
					scope->get_property(name.to_tu_string(), &val);

					// no in the env->m_scope.create new class
					if (val.is_undefined())
					{
						smart_ptr<as_object> new_class = new as_object();
						smart_ptr<as_function> func = m_abc->create_script_function(name.to_tu_string(), new_class.get());
						if (func != NULL)
						{
							// create class 
							as_object* global = get_global();

							global->set_member(name.to_tu_string(), new_class.get());

							call_method(func, env, new_class.get(), 0, env->get_top_index());

							val.set_as_object(new_class.get());
						}
					}
					IF_VERBOSE_ACTION(myprintf("EX: getlex\t %s, push stack=%s\n", name.to_string(), val.to_string()));
					env->push(val);
					break;
				}

				case 0x61: // setproperty
				{
				//	printf("ss=%d\n", env->size());
					int index;
					ip += read_vu30(index, &m_code[ip]);

					as_value val = env->pop();

					as_value name;
					get_multiname(index, env, &name);

					as_object * obj = env->top(0).to_object();
					IF_VERBOSE_ACTION(myprintf("EX: setproperty\t %x[%s]=%s\n", obj, name.to_string(), val.to_string()));

					if (obj)
					{
						obj->set_member(name.to_tu_string(), val);
					}
					env->drop(1);
					//printf("ss=%d\n", env->size());
					break;
				}

				case 0x62: // getlocal
				{
					int index;
					ip += read_vu30(index, &m_code[ip]);

//					IF_VERBOSE_ACTION(myprintf("EX: getlocal_%i, value=%s\n", index, lregister[index].to_string()));
					IF_VERBOSE_ACTION(myprintf("EX: getlocal_%i, ptr=%p\n", index, &lregister[index]));
					env->push(lregister[index]);
					break;
				}

				case 0x63: // setlocal
				{
					int index;
					ip += read_vu30(index, &m_code[ip]);

					IF_VERBOSE_ACTION(myprintf("EX: setlocal_%i, value=%s\n", index, env->top(0).to_string()));
					lregister[index] = env->pop();
					break;
				}

				case 0x65: // getscopeobject
				{
					int index = m_code[ip];
					++ip;
					env->push(scope->operator[](index));

					IF_VERBOSE_ACTION(myprintf("EX: getscopeobject\t index=%i, value=%s\n", index, env->top(0).to_string()));
					break;
				}

				case 0x66:	// getproperty
				{
					int index;
					ip += read_vu30(index, &m_code[ip]);

					as_value name;
					get_multiname(index, env, &name);

					smart_ptr<as_object> obj = env->top(0).to_object();		// keep alive
					if (obj)
					{
						env->top(0).set_undefined();
						obj->get_member(name.to_tu_string(), &env->top(0));
					}
					else
					{
						// try property/method of a primitive type, like String.length
						as_value val;
						env->top(0).find_property(name.to_tu_string(), &val);
						if (val.is_property())
						{
							val.get_property(env->top(0), &env->top(0));
						}
					}

					IF_VERBOSE_ACTION(myprintf("EX: getproperty\t %s, value=%s\n", name.to_string(), env->top(0).to_string()));
					break;
				}

				case 0x68:	// initproperty, Initialize a property.
				{
					int index;
					ip += read_vu30(index, &m_code[ip]);

					as_value name;
					get_multiname(index, env, &name);

					as_value& val = env->top(0);
					as_object* obj = env->top(1).to_object();
					IF_VERBOSE_ACTION(myprintf("EX: initproperty\t %p.%s=%s\n", obj, name.to_string(), val.to_string()));
					if (obj)
					{
						obj->set_member(name.to_tu_string(), val);
					}

					env->drop(2);
					break;
				}

				case 0x6C: //getslot
				{
					int slotindex ;
					ip += read_vu30(slotindex , &m_code[ip]);
					m_abc->get_slot(slotindex);

					as_object* obj = env->top(0).to_object();
					as_value val;

					// todo
					// Get the value of a slot
					// stack: ...obj=> ...,value 

					env->top(0) = val;
					break;
				}		
				
				case 0x6D: //setslot
				{
					int slotindex ;
					ip += read_vu30(slotindex , &m_code[ip]);
					m_abc->get_slot(slotindex);

					as_value& val = env->top(0);
					as_object* obj = env->top(1).to_object();

					// todo
					// Set the value of a slot

					env->drop(2);
					break;
				}

				case 0x73: //convert_i
				{
					env->top(0).set_int(env->top(0).to_int());

					IF_VERBOSE_ACTION(myprintf("EX: convert_i : %i \n", env->top(0).to_int())); 
					break;
				}

				case 0x75: //convert_d
				{
					env->top(0).set_double(env->top(0).to_number());

					IF_VERBOSE_ACTION(myprintf("EX: convert_d : %i \n", env->top(0).to_number())); 
					break;
				}

				case 0x76: //convert_b
				{
					env->top(0).set_bool(env->top(0).to_bool());

					IF_VERBOSE_ACTION(myprintf("EX: convert_b : %i \n", env->top(0).to_bool())); 
					break;
				}

				case 0x80: // Coerce a value to a specified type 
				{
					int index;
					ip += read_vu30( index, &m_code[ip]);

					as_value type_name;
					get_multiname(index, env, &type_name);

					IF_VERBOSE_ACTION(myprintf("EX: coerce : %s todo\n", type_name.to_string())); 
					break;
				}

				case 0x82:	// coerce_a
				{
					// Indicates to the verifier that the value on the stackis of the any type (*). Does nothing to value. 
					IF_VERBOSE_ACTION(myprintf("EX: coerce_a : %s\n", env->top(0).to_string())); 
					break;
				}

				case 0x84: // coerce_d
				{
					// Convert a value to a number.
					env->top(0).set_double( env->top(0).to_number());
					IF_VERBOSE_ACTION(myprintf("EX: coerce_d : %f\n",  env->top(0).to_number())); 
					break;
				}

				case 0x85: // coerce_s
				{
					env->top(0).set_string( env->top(0).to_string());

					IF_VERBOSE_ACTION(myprintf("EX: coerce_s : %s\n", env->top(0).to_string())); 
					break;
				}

				case 0x87: // astypelate
				{
					// Return the same value, or null if not of the specified type. 
					// stack: value,class=> ...,value 
					//Determines if object is an instance of constructor 
					// (doing the same comparison as ActionInstanceOf).

					as_function*constructor =  env->top(0).to_function();
					as_object* obj = env->top(1).to_object();
					if ((obj && obj->is_instance_of(constructor)) == false)
					{
						// todo
					//	obj = NULL;
					}

					env->drop(1);
					env->top(0).set_as_object(obj);
					IF_VERBOSE_ACTION(myprintf("EX: astypelate : %s\n", env->top(0).to_string())); 
					break;
				}

				case 0x91: // increment
				{
					env->top(0) += 1;
					IF_VERBOSE_ACTION(myprintf("EX: inc\n"));
					break;
				}

				case 0x93: // decrement
				{
					env->top(0) -= 1;
					IF_VERBOSE_ACTION(myprintf("EX: dec\n"));
					break;
				}

				case 0x94: // declocal
				{
					int index;
					ip += read_vu30(index, &m_code[ip]);
					lregister[index] = lregister[index].to_number() - 1;
					IF_VERBOSE_ACTION(myprintf("EX: declocal\t index=%i, value=%s\n", index, lregister[index].to_string()));
					break;
				}

				case 0x95: // typeof
				{
					// stack ...,value=> ...,typename 
					env->top(0).set_string(env->top(0).type_of());
					IF_VERBOSE_ACTION(myprintf("EX: typeof\n"));
					break;
				}

				case 0x96: // not
				{
					env->top(0).set_bool( !env->top(0).to_bool() );

					IF_VERBOSE_ACTION(myprintf("EX: not\n"));
					break;
				}

				case 0xA0:	// Add two values
				{
					if (env->top(0).is_string() || env->top(1).is_string())
					{
						tu_string str = env->top(1).to_string();
						str += env->top(0).to_string();
						env->top(1).set_tu_string(str);
					}
					else
					{
						env->top(1) += env->top(0).to_number();
					}
					env->drop(1);
					break;
				}

				case 0xA1:	// subtract
				{
					env->top(1) -= env->top(0).to_number();
					env->drop(1);
					break;
				}

				case 0xA2: // multiply
				{
					env->top(1) = env->top(1).to_number() * env->top(0).to_number();
					env->drop(1);

					IF_VERBOSE_ACTION(myprintf("EX: multiply\n"));
					break;
				}

				case 0xA3: // devide
				{
					env->top(1) = env->top(1).to_number() / env->top(0).to_number();
					env->drop(1);

					IF_VERBOSE_ACTION(myprintf("EX: devide\n"));
					break;
				}

				case 0xA4: // modulo
				{
					as_value	result;
					double	y = env->pop().to_number();
					double	x = env->pop().to_number();
					if (y != 0)
					{
						result.set_double(fmod(x, y));
					}
					env->push(result);
					IF_VERBOSE_ACTION(myprintf("EX: modulo\n"));
					break;
				}

				case 0xA5: // lshift
				{
					IF_VERBOSE_ACTION(myprintf("EX: lshift\n"));
					env->top(1).shl(env->top(0).to_int());
					env->drop(1);
					break;
				}

				case 0xA6: // rshift
				{
					IF_VERBOSE_ACTION(myprintf("EX: rshift\n"));
					env->top(1).asr(env->top(0).to_int());
					env->drop(1);
					break;
				}

				case 0xA8: // bitand
				{
					IF_VERBOSE_ACTION(myprintf("EX: bitand\n"));
					env->top(1) &= env->top(0).to_int();
					env->drop(1);
					break;
				}

				case 0xA9: // bitor
				{
					IF_VERBOSE_ACTION(myprintf("EX: bitor\n"));
					env->top(1) |= env->top(0).to_int();
					env->drop(1);
					break;
				}

				case 0xAB: // equals
				{
					IF_VERBOSE_ACTION(myprintf("EX: equals %s=%s ?\n", env->top(0).to_string(), env->top(1).to_string()));

					env->top(1).set_bool(env->top(1) == env->top(0));
					env->drop(1);
					break;
				}

				case 0xAD: //lessthan
				{
					IF_VERBOSE_ACTION(myprintf("EX: lessthan %s < %s ?\n", env->top(1).to_string(), env->top(0).to_string()));

					env->top(1).set_bool(env->top(1) < env->top(0).to_number());
					env->drop(1);
					break;
				}

				case 0xAE: //lessequals 
				{
					IF_VERBOSE_ACTION(myprintf("EX: lessequals  %s < %s ?\n", env->top(1).to_string(), env->top(0).to_string()));

					env->top(1).set_bool(env->top(1).to_number() <= env->top(0).to_number());
					env->drop(1);
					break;
				}

				case 0xAF: //greaterthan
				{
					IF_VERBOSE_ACTION(myprintf("EX: greaterthan  %s < %s ?\n", env->top(1).to_string(), env->top(0).to_string()));

					env->top(1).set_bool(env->top(0) < env->top(1).to_number());
					env->drop(1);
					break;
				}

				case 0xB0: //greaterequals 
				{
					IF_VERBOSE_ACTION(myprintf("EX: greaterequals  %s < %s ?\n", env->top(1).to_string(), env->top(0).to_string()));

					env->top(1).set_bool(!(env->top(1) < env->top(0).to_number()));
					env->drop(1);
					break;
				}

				case 0xB4:  // in
				{
					// Determine whether an object has a named property.
					// stack: ...name,obj=> ...,result 
					
					as_object* obj = env->top(1).to_object();
					const tu_string& name = env->top(0).to_tu_string();
					bool result = false;
					if (obj)
					{
						result = obj->get_member(name, NULL);
					}
					env->top(1).set_bool(result);
					env->drop(1);
					break;
				}

				case 0xC2:  // inclocal_i
				{
					int index;
					ip += read_vu30(index, &m_code[ip]);

					as_value & reg = lregister[ index ];
					reg.set_int( reg.to_int() + 1 );

					IF_VERBOSE_ACTION(myprintf("EX: inclocal_i %i\n", index ) );
					break;
				}

				case 0xD0:	// getlocal_0
				case 0xD1:	// getlocal_1
				case 0xD2:	// getlocal_2
				case 0xD3:	// getlocal_3
				{
					as_value& val = lregister[opcode & 0x03];
					env->push(val);
					IF_VERBOSE_ACTION(myprintf("EX: getlocal_%d\t %s\n", opcode & 0x03, val.to_string()));
					break;
				}

				case 0xD4:	// setlocal_0
				case 0xD5:	// setlocal_1
				case 0xD6:	// setlocal_2
				case 0xD7:	// setlocal_3
				{
					lregister[opcode & 0x03] = env->pop();

					IF_VERBOSE_ACTION(myprintf("EX: setlocal_%d\t %s\n", opcode & 0x03, lregister[opcode & 0x03].to_string()));
					break;
				}

				case 0xEF:	// debug
				{
					Uint8 debugtype = m_code[ip];
					ip++;

					int index;
					ip += read_vu30(index, &m_code[ip]);
					const tu_string& name = m_abc->get_string(index);

					Uint8 reg = m_code[ip];
					ip++;

					int extra;
					ip += read_vu30(extra, &m_code[ip]);
					IF_VERBOSE_ACTION(myprintf("EX: debug,  type=%d, name=%s, reg=%d\n", debugtype, name.c_str(), reg));
					break;
				}

				case 0xF0:	// debugline
				{
					int line;
					ip += read_vu30(line, &m_code[ip]);
					IF_VERBOSE_ACTION(myprintf("EX: debugline,  line=%d\n", line));
					break;
				}

				case 0xF1:	// debugfile 
				{
					int index;
					ip += read_vu30(index, &m_code[ip]);
					const tu_string& name = m_abc->get_string(index);
					IF_VERBOSE_ACTION(myprintf("EX: debugfile,  index=%d\n", index));
					break;
				}

				case 0xF2:	// bkptline 
				{
					// If in debug mode, set the debug line number and enter the debugger.
					break;
				}

				default:
					myprintf("TODO opcode 0x%02X\n", opcode);
					assert(0);
					return;
			}

		}
		while (ip < m_code.size());
	}

	void as3function_def::read(stream* in)
	// read method_info
	{
		int param_count = in->read_vu30();

		// The return_type field is an index into the multiname
		m_return_type = in->read_vu30();

		m_param_type.resize(param_count);
		for (int i = 0; i < param_count; i++)
		{
			m_param_type[i] = in->read_vu30();
		}

		m_name = in->read_vu30();
		m_flags = in->read_u8();

		if (m_flags & HAS_OPTIONAL)
		{
			int option_count = in->read_vu30();
			m_options.resize(option_count);

			for (int o = 0; o < option_count; ++o)
			{
				m_options[o].m_value = in->read_vu30();
				m_options[o].m_kind = in->read_u8();
			}
		}

		if (m_flags & HAS_PARAM_NAMES)
		{
			// The parameter name entry exists solely for external tool use and is not used by the AVM2. 
			m_param_name.resize(param_count);
			for (int o = 0; o < param_count; ++o)
			{
				m_param_name[o] = in->read_vu30();
			//	myprintf("method_info: arg=%d param_name='%s'\n",	o, m_abc->get_string(m_param_name[o]).c_str());
			}
		}

		IF_VERBOSE_PARSE(myprintf("method_info: name='%s', type='%s', params=%d\n",
			m_abc->get_string(m_name).c_str(), m_abc->get_multiname(m_return_type).c_str(), m_param_type.size()));
	}

	void as3function_def::read_body(stream* in)
	// read body_info
	{
		IF_VERBOSE_PARSE(myprintf("body_info[%d]\n", m_method));

		m_max_stack = in->read_vu30();
		m_local_count = in->read_vu30();
		m_init_scope_depth = in->read_vu30();
		m_max_scope_depth = in->read_vu30();

		int i, n;
		n = in->read_vu30();	// code_length
		m_code.resize(n);
		for (i = 0; i < n; i++)
		{
			m_code[i] = in->read_u8();
		}

		n = in->read_vu30();	// exception_count
		m_exception.resize(n);
		for (i = 0; i < n; i++)
		{
			except_info* e = new except_info();
			e->read(in, m_abc.get());
			m_exception[i] = e;
		}

		n = in->read_vu30();	// trait_count
		m_trait.resize(n);
		for (int i = 0; i < n; i++)
		{
			traits_info* trait = new traits_info();
			trait->read(in, m_abc.get());
			m_trait[i] = trait;
		}

		IF_VERBOSE_PARSE(myprintf("method	%i\n", m_method));
		IF_VERBOSE_PARSE(log_disasm_avm2(m_code, m_abc.get()));

	}

}

