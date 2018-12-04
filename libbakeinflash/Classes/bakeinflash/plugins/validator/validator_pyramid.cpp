//
//
//
#include "base/tu_timer.h"
#include "validator_pyramid.h"

namespace bakeinflash
{
	validator_pyramid::validator_pyramid(const tu_string& port) :
		validator(port),
		m_rbuf_len(0),
		m_enable(true)
	{
		m_time_advance = tu_timer::get_ticks();

		m_msglist.add(0, "IDLING");
		m_msglist.add(1, "JAMMED");
		m_msglist.add(2, "STACKER FULL");
		m_msglist.add(4, "NO CASSETTE");
		m_msglist.add(8, "FAILURE");
	}

	validator_pyramid::~validator_pyramid()
	{
		disable();
	}

	void validator_pyramid::disable()
	{
		Uint8 wbuf[16];
		wbuf[0] = 0x02;	// STX
		wbuf[1] = 8;
		wbuf[2] = 0x10;		// MSG|Ack
		wbuf[3] = 0x00;		// disable all
		wbuf[4] = 0x00;		// data1
		wbuf[5] = 0x00;		// data2
		wbuf[6] = 0x03;		// ETX
		wbuf[7] = get_crc(wbuf + 1, 8 - 3);
		m_serial->write(wbuf, 8);
	}

	bool validator_pyramid::init()
	{
#ifdef WIN32
		//	bool rc = m_serial->open(m_port, 9600, true, 7);
		bool rc = m_serial->open(m_port, EVENPARITY, 9600, 7);
#else
		bool rc = m_serial->open(m_port, PARENB | CS7 | B9600 | CLOCAL | CREAD, 9600, 7);
#endif

		return rc;
	}

	//	Checksum- (one byte checksum). The checksum is calculated on all bytes (except: STX, ETX and
	// the checksum byte itself). This is done by bitwise Exclusive OR-ing (XOR) the bytes.
	Uint8 validator_pyramid::get_crc(const Uint8* buf, int len)
	{
		Uint8 crc = buf[0];
		for (int i = 1; i < len; i++)
		{
			crc ^= buf[i];
		}
		return crc;
	}

	int validator_pyramid::advance()
	{
		if (m_serial != NULL && m_serial->is_open())
		{
			time_t now = tu_timer::get_ticks();
			if (now - m_time_advance >= 1000)
			{
				m_time_advance = now;

				// | STX | Length | MSG Type and Ack Number | Data Fields | ETX | Checksum |
				Uint8 wbuf[16];
				wbuf[0] = 0x02;	// STX
				wbuf[1] = 8;
				wbuf[2] = 0x10;		// MSG|Ack
				wbuf[3] = m_enable ? 0x7F : 0x00;		// data0, enable all/ disable all
				wbuf[4] = 0x00;		// data1
				wbuf[5] = 0x00;		// data2
				wbuf[6] = 0x03;		// ETX
				wbuf[7] = get_crc(wbuf + 1, 8 - 3);

				m_serial->write(wbuf, 8);
			}
			return read();
		}
		return 0;
	}

	int validator_pyramid::read()
	{
		int left = 1024 - m_rbuf_len;
		if (left <= 0)
		{
			//printf("something wrong.. clear all\n");
			m_rbuf_len = 0;
			left = 1024 - m_rbuf_len;
		}

		int money = 0;
		int n = m_serial->read(m_rbuf + m_rbuf_len, left);	// hack
		if (n > 0)
		{
			m_rbuf_len += n;
			//dump((const char*) m_rbuf, m_rbuf_len);

			if (m_rbuf[0] != 0x02)
			{
				//printf("remove bad data\n");
				m_rbuf_len = 0;
				return 0;
			}

			// check a packet
			if (m_rbuf_len >= 2)
			{
				int len = m_rbuf[1];
				if (m_rbuf_len >= len)
				{
					Uint8 crc = get_crc(m_rbuf + 1, len - 3);
					if (crc == m_rbuf[len -1])
					{
						money = parse(m_rbuf + 3, len - 5);
					}
					else
					{
						//printf("bar CRC\n");
					}

					// clear all
					m_rbuf_len = 0;
				}
			}
		}
		return money;
	}

	int validator_pyramid::parse(const Uint8* b, int len)
	{
		//	m_err = m_err.size() > 1 ? " " : "";

		//	bool Idling = (b[0] & 0x01) == 0 ? false : true;
		//	bool Accepting = (b[0] & 0x02) == 0 ? false : true;
		//	bool Escrowed = (b[0] & 0x04) == 0 ? false : true;
		//	bool Stacking = (b[0] & 0x08) == 0 ? false : true;
		bool Stacked = (b[0] & 0x10) == 0 ? false : true;
		//	bool Returning = (b[0] & 0x20) == 0 ? false : true;
		//	bool Returned = (b[0] & 0x40) == 0 ? false : true;

		//	bool Cheated = (b[1] & 0x01) == 0 ? false : true;
		//	bool Rejected = (b[1] & 0x02) == 0 ? false : true;
		bool Jammed = (b[1] & 0x04) == 0 ? false : true;
		if (Jammed)
		{
			m_state |= 1;
		}
		else
		{
			m_state &= ~(1);
		}

		bool Stacker_full = (b[1] & 0x08) == 0 ? false : true;
		if (Stacker_full)
		{
			//m_err += m_err.size() > 1 ? ", Stacker_full" : "Stacker_full";
			m_state |= 2;
		}
		else
		{
			m_state &= ~(2);
		}


		bool Cassette_present = (b[1] & 0x10) == 0 ? false : true;
		if (!Cassette_present)
		{
			//m_err += m_err.size() > 1 ? ", No Cassette" : "No Cassette";
			m_state |= 4;
		}
		else
		{
			m_state &= ~(4);
		}

		//	bool Initializing = (b[2] & 0x01) == 0 ? false : true;
		//	bool Invalid_command = (b[2] & 0x02) == 0 ? false : true;
		bool Failure = (b[2] & 0x04) == 0 ? false : true;
		if (Failure)
		{
			//m_err += m_err.size() > 1 ? ", Failure" : "Failure";
			m_state |= 8;
		}
		else
		{
			m_state &= ~(8);
		}

		static int money[8] = {0, 1, 2, 5, 10, 20, 50, 100};
		int index = (b[2] >> 3) & 0x07;
		return Stacked ? money[index] : 0;
	}

	void validator_pyramid::get_state_name(int state, tu_string* msg) const
	{
		m_msglist.get(state, msg);
	}

}
