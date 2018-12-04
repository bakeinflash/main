//
//
//
#include "base/tu_timer.h"
#include "validator.h"
#include "validator_pyramid.h"
#include "validator_cashcode.h"

namespace bakeinflash
{

	validator::validator(const tu_string& port) :
		m_state(0),
		m_port(port)
	{
		m_serial = new serial();
	}

	validator::~validator()
	{
	}

	validator*	attach_validator(const tu_string& model, const tu_string& port)
	{
		if (model == "apex")
		{
			return new validator_pyramid(port);
		}
		else
		if (model == "cashcode")
		{
			return new validator_cashcode(port);
		}
		return NULL;
	}
}
