// as_class.cpp	-- Julien Hamaide <julien.hamaide@gmail.com> 2008

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Action Script 3 Class object

#include "as_class.h"

namespace bakeinflash
{

	void as_class::set_class(as_object* target, class_info* info)
	{
		assert(info);
		m_class = info;

		for (int i = 0; i < m_class->m_trait.size(); i++)
		{
			traits_info* ti = m_class->m_trait[i].get();
			ti->copy_to(m_class->m_abc.get(), target, this);
		}
	}

}
