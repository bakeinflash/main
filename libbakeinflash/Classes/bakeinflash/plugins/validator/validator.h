//
//  interface, base class
//
#ifndef validator_H
#define validator_H

#include "base/tu_serial.h"
//#include "games/var.h"

namespace bakeinflash
{

struct validator : public ref_counted   
{

	validator(const tu_string& port);
	virtual ~validator();

	virtual bool init() = 0;
	virtual int advance() = 0;
	virtual void disable() = 0;
	int get_state() const { return m_state; }
	virtual void get_state_name(int state, tu_string* msg) const = 0;

protected:

	Uint32 m_state;
	tu_string m_port;
	smart_ptr<serial> m_serial;

};


// Factory.
validator*	attach_validator(const tu_string& model, const tu_string& port);

}

#endif
