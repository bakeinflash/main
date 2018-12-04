// bakeinflash_abc.h	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// do_abc tag reader

#ifndef bakeinflash_ABC_H
#define bakeinflash_ABC_H

#include "bakeinflash/bakeinflash_types.h"
#include "bakeinflash/bakeinflash_avm2.h"
#include "bakeinflash/bakeinflash_log.h"

namespace bakeinflash
{

	struct abc_def;
	struct as_function;
	struct movie_definition_sub;

	struct multiname
	{
		enum kind
		{
			CONSTANT_UNDEFINED = 0,
			CONSTANT_QName = 0x07,
			CONSTANT_QNameA = 0x0D,
			CONSTANT_RTQName = 0x0F,
			CONSTANT_RTQNameA = 0x10,
			CONSTANT_RTQNameL = 0x11,
			CONSTANT_RTQNameLA = 0x12,
			CONSTANT_Multiname = 0x09,
			CONSTANT_MultinameA = 0x0E,
			CONSTANT_MultinameL = 0x1B,
			CONSTANT_MultinameLA = 0x1C
		};

		kind m_kind;
		int m_flags;
		int m_ns;
		int m_ns_set;
		int m_name;

		multiname() :
			m_kind(CONSTANT_UNDEFINED),
			m_flags(0),
			m_ns(0),
			m_ns_set(0),
			m_name(0)
		{
		}

		inline bool is_qname() const
		{
			return m_kind == CONSTANT_QName;
		}
	};

	struct namespac
	{
		enum kind
		{
			CONSTANT_Undefined = 0,
			CONSTANT_Namespace = 0x08,
			CONSTANT_PackageNamespace = 0x16,
			CONSTANT_PackageInternalNs = 0x17,
			CONSTANT_ProtectedNamespace = 0x18,
			CONSTANT_ExplicitNamespace = 0x19,
			CONSTANT_StaticProtectedNs = 0x1A,
			CONSTANT_PrivateNs = 0x05
		};

		kind m_kind;
		int	m_name;

		namespac() :
			m_kind(CONSTANT_Undefined),
			m_name(0)
		{
		}

	};

	struct metadata_info : public ref_counted
	{
		void	read(stream* in, abc_def* abc);
	};


	//
	// instance_info
	//
	struct instance_info : public ref_counted
	{

		enum flags
		{
			CONSTANT_ClassSealed = 0x01,
			CONSTANT_ClassFinal = 0x02,
			CONSTANT_ClassInterface = 0x04,
			CONSTANT_ClassProtectedNs = 0x08
		};

		int m_name;
		int m_super_name;
		Uint8 m_flags;
		int m_protectedNs;
		array<int> m_interface;
		int m_iinit;
		array<smart_ptr<traits_info> > m_trait;
		weak_ptr<abc_def> m_abc;

		instance_info() :
			m_name(0),
			m_super_name(0),
			m_flags(0),
			m_protectedNs(0),
			m_iinit(0)
		{
		}

		void	read(stream* in, abc_def* abc);
		void copy_to(as_object* target, as_object* ch);
	};

	struct class_info : public ref_counted
	{
		weak_ptr<abc_def> m_abc;
		int m_cinit;
		array<smart_ptr<traits_info> > m_trait;

		void	read(stream* in, abc_def* abc);
		void	copy_to(as_object* target, as_object* ch);
	};

	struct script_info : public ref_counted
	{
		int m_init;
		array<smart_ptr<traits_info> > m_trait;

		void	read(stream* in, abc_def* abc);
	};

	struct abc_def : public ref_counted
	{
		// constant pool
		array<int> m_integer;
		array<Uint32> m_uinteger;
		array<double> m_double;
		array<tu_string> m_string;
		array<namespac> m_namespace;
		array< array<int> > m_ns_set;
		array<multiname> m_multiname;

//		array<smart_ptr<method_info> > m_method;
		array<smart_ptr<as3function_def> > m_method;
		array<smart_ptr<metadata_info> > m_metadata;
		array<smart_ptr<instance_info> > m_instance;
		array<smart_ptr<class_info> > m_class;
		array<smart_ptr<script_info> > m_script;

		void get_slot(int index) const;

		inline as3function_def* get_method_info(int index) const
		{
			return m_method[index].get(); 
		}

		inline const tu_string& get_string(int index) const
		{
			return m_string[index]; 
		}

		inline int get_integer(int index) const
		{
			return m_integer[index]; 
		}

		inline double get_double(int index) const
		{
			return m_double[index]; 
		}

		inline const tu_string& get_namespace(int index) const
		{
			return m_string[m_namespace[index].m_name];
		}

		const tu_string& get_multiname(int index) const;

		inline multiname::kind get_multiname_type(int index) const
		{
			return (multiname::kind)m_multiname[index].m_kind; 
		}

		inline const tu_string& get_multiname_namespace(int index) const
		{
			static tu_string empty;
			const multiname& mn = m_multiname[index];
			switch (mn.m_kind)
			{
				case multiname::CONSTANT_QName:
					return empty;

				case multiname::CONSTANT_Multiname:
				case multiname::CONSTANT_MultinameA:
					return get_namespace(mn.m_ns);

				default:
					myprintf( "implement get_multiname_namespace for this kind %i\n", mn.m_kind );
					return empty;
			} 
		}


		as_function* create_script_function(const tu_string& name, as_object* target) const;

		as_function* create_initclass_function(int class_index, as_object* target) const
		{
			class_info* ci = m_class[class_index];
			int i = ci->m_cinit;
			return new as3function(m_method[i].get(), target);
		}

		abc_def();
		virtual ~abc_def();

		void	read(stream* in, movie_definition_sub* m);
		void	read_cpool(stream* in);

		inline const tu_string& get_super_class(const tu_string& name) const
		{
			return get_multiname(get_instance_info(name)->m_super_name);
		}

		const tu_string& get_class_from_constructor(int method) const;

		// get class constructor
		as_function* create_class_constructor(const tu_string& name, as_object* target) const;
		as_function* create_class_constructor(instance_info* ii, as_object* target) const;

		instance_info* get_instance_info(int index) const;

		// find instance info by name
		instance_info* get_instance_info(const tu_string& class_name) const;
		class_info* get_class_info(const tu_string& full_class_name) const;
		class_info* get_class_info(int class_index) const;
	};
}


#endif // bakeinflash_ABC_H
