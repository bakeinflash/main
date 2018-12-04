// -v cashcode=com1 >C:\bakeinflash\log.txt
//
//
#include "base/tu_timer.h"
#include "validator_cashcode.h"

#define POLYNOMIAL 0x08408  

namespace bakeinflash
{

	validator_cashcode::validator_cashcode(const tu_string& port) :
		validator(port),
		m_rbuf_len(0),
		m_enable(true),
		m_response(0)
	{
		m_time_advance = tu_timer::get_ticks();

		m_msglist.add(0x00, "IDLING");
		m_msglist.add(0x10, "POWER UP");
		m_msglist.add(0x13, "INITIALIZE");
		m_msglist.add(0x14, "IDLING");
		m_msglist.add(0x15, "ACCEPTING");
		m_msglist.add(0x17, "STACKING");
		m_msglist.add(0x18, "RETURNING");
		m_msglist.add(0x19, "DISABLED");
		m_msglist.add(0x1A, "HOLDING");
		m_msglist.add(0x1B, "BUSY");
		m_msglist.add(0x1C, "REJECTING");
		m_msglist.add(0x30, "ILLEGAL COMMAND");
		m_msglist.add(0x42, "CASSETTE REMOVED");
		m_msglist.add(0x80, "ESCROW BILL");
		m_msglist.add(0x81, "STACKED");
		m_msglist.add(0x82, "RETURNED");
		m_msglist.add(0xFF, "NAK");
	}

	validator_cashcode::~validator_cashcode()
	{
		disable();
	}

	void validator_cashcode::disable()
	{
		Uint8 wbuf[16];
		wbuf[0] = 0x02;	// SYNC, Message transmission start code [02H], fixed
		wbuf[1] = 0x03;		// Peripheral address, 03H Bill Validator
		wbuf[2] = 12;		// Data length, Total number of bytes including SYNC and CRC
		wbuf[3] = 0x34;	// ENABLE
		wbuf[4] = 0;
		wbuf[5] = 0;
		wbuf[6] = 0;
		wbuf[7] = 0;
		wbuf[8] = 0;
		wbuf[9] = 0;
		*(Uint16*) (wbuf + (wbuf[2] - 2)) = get_crc(wbuf, wbuf[2] - 2);
		m_serial->write(wbuf, wbuf[2]);
	}

	bool validator_cashcode::init()
	{
		// stty -F /dev/ttyS0 9600 cs8 -cstopb -parenb clocal -crtscts -ixon -ixoff ignpar -icrnl -opost -isig -icanon -iexten -echo
		//	bool rc = m_serial->open(m_port, 9600, false, 8);

#ifdef WIN32
		bool rc = m_serial->open(m_port, NOPARITY, 9600, 8);
#else
		bool rc = m_serial->open(m_port, PARODD | CS8 | B9600 | CLOCAL | CREAD, 9600, 8);
#endif

		return rc;
	}

	Uint16 validator_cashcode::get_crc(const Uint8* bufData, unsigned int sizeData)
	{
		unsigned int CRC, i;
		unsigned char j;
		CRC = 0;
		for(i=0; i < sizeData; i++)
		{
			CRC ^= bufData[i];
			for(j=0; j < 8; j++)
			{
				if(CRC & 0x0001) {CRC >>= 1; CRC ^= POLYNOMIAL;}
				else CRC >>= 1;
			}
		}
		return (Uint16) CRC;
	} 

	int validator_cashcode::parse(const Uint8* b, int len)
	{
		if (m_response == 0)	// was it POLL ?
		{
			//	printf("send ACK\n");
			Uint8 wbuf[16];
			memcpy(wbuf, "\x02\x03\x06\x00", 4);		// send ACK
			*(Uint16*) (wbuf + (wbuf[2] - 2)) = get_crc(wbuf, wbuf[2] - 2);
			m_serial->write(wbuf, wbuf[2]);
		}

		int money = 0;
		m_response = 0;
		m_state = b[0];
		switch (b[0])
		{
			case 0x00:
				//			m_err = "ACK";
				break;
			case 0x10:
				//			m_err = "POWER UP";
				m_response = b[0];
				break;
			case 0x13:
				//			m_err = "INITIALIZE";
				break;
			case 0x14:
				//			m_err = "IDLING";
				break;
			case 0x15:
				//			m_err = "ACCEPTING";
				break;
			case 0x17:
				//			m_err = "STACKING";
				break;
			case 0x18:
				//			m_err = "RETURNING";
				break;
			case 0x19:
				//			m_err = "DISABLED";
				m_response = b[0];
				break;
			case 0x1A:
				//			m_err = "HOLDING";
				break;
			case 0x1B:
				//			m_err = "BUSY";
				break;
			case 0x1C:
				//			m_err = "REJECTING";
				break;
			case 0x30:
				//			m_err = "ILLEGAL COMMAND";
				break;
			case 0x42:
				//m_state = "CASSETTE REMOVED";
				break;
			case 0x80:
				//			m_err = "ESCROW BILL";
				m_response = b[0];
				break;
			case 0x81:
				{
					static int s_cashtable[] = {0, 0, 10, 50, 100, 500, 1000, 5000};
					if (b[1] >= 0 && b[1] < TU_ARRAYSIZE(s_cashtable))
					{
						money = s_cashtable[b[1]];
					}
					//printf("STACKED type=%d, money=%d\n", b[1], money);
					break;
				}
			case 0x82:
				//			m_err = "RETURNED";
				m_response = b[0];
				break;
			case 0xFF:
				//			m_err = "NAK";
				break;
			default:
				//			m_err = "unhandled response ";
				//			m_err += b[0];
				break;
		}
		return money;
	}

	int validator_cashcode::advance()
	{
		int money = 0;
		if (m_serial != NULL && m_serial->is_open())
		{
			//printf("advance\n");
			money = read();

			time_t now = tu_timer::get_ticks();
			if (now - m_time_advance >= 200)
			{
				m_time_advance = now;

				Uint8 wbuf[16];
				wbuf[0] = 0x02;	// SYNC, Message transmission start code [02H], fixed
				wbuf[1] = 0x03;		// Peripheral address, 03H Bill Validator
				wbuf[2] = 0;
				switch (m_response)
				{
				case 0:		// idling
					{
						// POLL
						wbuf[2] = 6;		// Data length, Total number of bytes including SYNC and CRC
						wbuf[3] = 0x33;		// POLL
						break;
					}

				case 0x10:	// powerup
					{
						// power up.. run reset
						wbuf[2] = 6;		// Data length, Total number of bytes including SYNC and CRC
						wbuf[3] = 0x30;		// RESET
						break;
					}

				case 0x19:	// disabled
					{
						// enable bill types
						wbuf[2] = 12;		// Data length, Total number of bytes including SYNC and CRC
						wbuf[3] = 0x34;	// ENABLE
						wbuf[4] = 0xFF;
						wbuf[5] = 0xFF;
						wbuf[6] = 0xFF;
						wbuf[7] = 0xFF;
						wbuf[8] = 0xFF;
						wbuf[9] = 0xFF;
						break;
					}

				case 0x80:	// escrow
					{
						// stack
						wbuf[2] = 6;		// Data length, Total number of bytes including SYNC and CRC
						wbuf[3] = 0x35;		// STACK
						break;
					}
				}

				if (wbuf[2] > 0)
				{
					*(Uint16*) (wbuf + (wbuf[2] - 2)) = get_crc(wbuf, wbuf[2] - 2);
					m_serial->write(wbuf, wbuf[2]);
				}
			}
		}
		return money;
	}

	int validator_cashcode::read()
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

			if (m_rbuf[0] != 0x02)
			{
				//printf("removed wrong data\n");
				m_rbuf_len = 0;
				return 0;
			}

			// check a packet
			if (m_rbuf_len >= 5)
			{
				int len = m_rbuf[2];
				if (m_rbuf_len >= len)
				{
					//dump("read", (const char*) m_rbuf, m_rbuf_len);

					Uint16 crc = get_crc(m_rbuf, len - 2);
					if (crc == *(Uint16*) (m_rbuf + len - 2))
					{
						money = parse(m_rbuf + 3, len - 5);
					}
					else
					{
						//printf("wrong CRC\n");
					}

					// clear all
					m_rbuf_len = 0;
				}
			}
		}
		return money;
	}

	void validator_cashcode::get_state_name(int state, tu_string* msg) const
	{
		m_msglist.get(state, msg);
	}


}
