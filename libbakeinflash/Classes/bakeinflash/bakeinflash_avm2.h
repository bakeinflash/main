// bakeinflash_avm2.h	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// AVM2 implementation


#ifndef bakeinflash_AVM2_H
#define bakeinflash_AVM2_H

#include "base/smart_ptr.h"
#include "bakeinflash/bakeinflash_function.h"
//#include "bakeinflash/bakeinflash_jit.h"

namespace bakeinflash
{
	struct abc_def;

	struct option_detail
	{
		int m_value;
		Uint8 m_kind;
	};

	struct traits_info : public ref_counted
	{
		enum kind
		{
			Trait_Slot = 0,
			Trait_Method = 1,
			Trait_Getter = 2,
			Trait_Setter = 3,
			Trait_Class = 4,
			Trait_Function = 5,
			Trait_Const = 6
		};

		enum attr
		{
			ATTR_Final = 0x1,
			ATTR_Override = 0x2,
			ATTR_Metadata = 0x4
		};

		int m_name;
		kind m_kind;
		Uint8 m_attr;

		// data
		union
		{
			struct
			{
				int m_slot_id;
				int m_type_name;
				int m_vindex;
				Uint8 m_vkind;
			} trait_slot;

			struct
			{
				int m_slot_id;
				int m_classi;
			} trait_class;

			struct
			{
				int m_slot_id;
				int m_function;
			} trait_function;
			
			struct
			{
				int m_disp_id;
				int m_method;
			} trait_method;

		};


		array<int> m_metadata;

		void	read(stream* in, abc_def* abc);
		void	copy_to(abc_def* m_abc, as_object* target, as_object* ch);

	};

	struct except_info : public ref_counted
	{
		int m_from;
		int m_to;
		int m_target;
		int m_exc_type;
		int m_var_name;

		void	read(stream* in, abc_def* abc);
	};

	struct as3function_def : public ref_counted
	{

		enum flags 
		{
			NEED_ARGUMENTS = 0x01,
			NEED_ACTIVATION = 0x02,
			NEED_REST = 0x04,
			HAS_OPTIONAL = 0x08,
			SET_DXNS = 0x40,
			HAS_PARAM_NAMES = 0x80
		};

		weak_ptr<abc_def> m_abc;

		// method_info
		int m_return_type;
		array<int> m_param_type;
		int m_name;
		Uint8 m_flags;
		array<option_detail> m_options;
		array<int> m_param_name;
		int m_method;	// index in method_info

		// body_info
		int m_max_stack;

		// this is the index of the highest-numbered local register plus one.	
		int m_local_count;

		int m_init_scope_depth;
		int m_max_scope_depth;
		membuf m_code;
		array<smart_ptr<except_info> > m_exception;
		array<smart_ptr<traits_info> > m_trait;

#if _ENABLE_JIT==1
		jit_function m_compiled_code;
#endif

		as3function_def(abc_def* abc, int method);
		virtual ~as3function_def();

		void	execute(array<as_value>& lregister, as_environment* env, as_value* result);
		void	compile();
		bool	get_multiname(int index, as_environment* env, as_value* val);

		void	compile_stack_resize(int count);
		void	read(stream* in);
		void	read_body(stream* in);

	};

	struct as3function : public as_function
	{
		// Unique id of a bakeinflash resource
		enum { m_class_id = AS_3_FUNCTION };
		virtual bool is(int class_id) const
		{
			if (m_class_id == class_id) return true;
			else return as_function::is(class_id);
		}

		as3function(as3function_def* def, as_object* target);
		virtual ~as3function();

		// Dispatch.
		virtual void	operator()(const fn_call& fn);

		virtual as_object* get_target() const { return m_target.get(); }
		virtual void set_target(as_object* target);
		virtual const char*	to_string();

		// if function has been declared in moviclip then we should use its environment
		// And for this purpose it is necessary to keep target that has created 'this'
		// testcase:
		// _root.myclip.onEnterFrame = _root.myfunc;
		// myfunc should use _root environment
		weak_ptr<as_object>	m_target;

		smart_ptr<as3function_def> m_def;

	};

}

#endif
