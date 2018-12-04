#ifndef validator_pyramid_H
#define validator_pyramid_H

#include "base/tu_serial.h"
#include "validator.h"

namespace bakeinflash
{

struct validator_pyramid : public validator   
{
	validator_pyramid(const tu_string& port);
	virtual ~validator_pyramid();

	virtual bool init();
	virtual int advance();
	virtual void disable();
	virtual void get_state_name(int state, tu_string* msg) const;

private:

	int read();
	Uint8 get_crc(const Uint8* buf, int len);
	int parse(const Uint8* rbuf, int len);

	time_t m_time_advance;
	int m_rbuf_len;
	Uint8 m_rbuf[1024];
	bool m_enable;
	hash<int, tu_string> m_msglist;
};

}
#endif
