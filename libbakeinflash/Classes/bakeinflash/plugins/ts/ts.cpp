//
//
//
#include "ts.h"
#include "base/tu_timer.h"
#include "bakeinflash/bakeinflash_log.h"

#define ELO_MAX_LENGTH		10
#define ELO10_PACKET_LEN	8
#define ELO10_TOUCH		0x03
#define ELO10_PRESSURE		0x80
#define ELO10_LEAD_BYTE		'U'
#define ELO10_ID_CMD		'i'
#define ELO10_TOUCH_PACKET	'T'
#define ELO10_ACK_PACKET	'A'
#define ELI10_ID_PACKET		'I'

#define ELO_TX0 m_pt[0] 
#define ELO_TY0 m_pt[1] 
#define ELO_X0 m_pt[2]
#define ELO_Y0 m_pt[3]
#define ELO_TX1 m_pt[4] 
#define ELO_TY1 m_pt[5] 
#define ELO_X1 m_pt[6]
#define ELO_Y1 m_pt[7]

namespace bakeinflash
{
	void	as_systemGetVar(const fn_call& fn);
	void	as_systemSetVar(const fn_call& fn);

	elo::elo() :
		m_size(0),
		m_prev_b(0)
	{
		// read save data
		as_environment env;
		env.push("elo");
		as_value val;
		as_systemGetVar(fn_call(&val, as_value(), &env, 1, env.get_top_index()));
		if (val.is_undefined() == false)
		{
			val.to_tu_string().split(',', &m_pt);

			// sanity check
			if (m_pt.size() > 0 && m_pt.size() != 8)
			{
				m_pt.resize(0);
			}
		}
	}

	elo::~elo()
	{
		m_serial = NULL;
	}

	bool elo::open(const tu_string& model, const tu_string& port)
	{
		m_serial = new serial();
#ifdef WIN32
		bool rc = m_serial->open(port, NOPARITY, 9600, 8);
#else
		Uint32 c_cflag = 0;
		c_cflag = 0;
		c_cflag |= PARODD;
		c_cflag |= CS8;
		c_cflag |= B9600;
		c_cflag |= CLOCAL | CREAD;

		bool rc = m_serial->open(port, c_cflag, 9600, 8);
#endif
		return rc;
	}

	bool elo::read(int w, int h, int* ts_x, int* ts_y, int* state)
	{
		// re-calibrate ?
		as_value val;
		if (get_global()->get_member("elo", &val) && val.is_undefined() == false)
		{
			as_array* a = cast_to<as_array>(val.to_object());
			if (a && a->size() == 8)
			{
				// save settings
				tu_string str;
				m_pt.resize(a->size());
				for (int i = 0; i < a->size(); i++)
				{
					m_pt[i] = a->operator[](i).to_number();
					str += m_pt[i];
					if (i < a->size() - 1) str += ',';
				}

				// save
				as_environment env;
				env.push(str.c_str());
				env.push("elo");
				as_systemSetVar(fn_call(NULL, as_value(), &env, 2, env.get_top_index()));
			}

			// delete
			get_global()->set_member("elo", as_value());
		}

		if (m_serial != NULL && m_serial->is_open())
		{
			m_size += m_serial->read(m_buf + m_size, sizeof(m_buf) - m_size);

			// sync
			while (m_size > 0 && *m_buf != ELO10_LEAD_BYTE)
			{
				memmove(m_buf, m_buf + 1, sizeof(m_buf) - 1);
				m_size--;
			}
			if (m_size > 0) printf("read %d bytes from elo\n", m_size); //	dump((const char*)m_rbuf, n);

			while (m_size == sizeof(m_buf))
			{
				int b = ((m_buf[2] & ELO10_TOUCH) == 0) ? 0 : 1;
				if (m_prev_b == b)
				{
					// same state
					m_size = m_serial->read(m_buf, sizeof(m_buf));
					continue;
				}

				int x = ((m_buf[4] << 8) | m_buf[3]);
				int y = ((m_buf[6] << 8) | m_buf[5]);

				// no calibaration
				x = (int) (x * (w / 4096.0f));
				y = (int) (y * (h / 4096.0f));

				// devide by zero ?
				if (m_pt.size() == 8 && ELO_TX1- ELO_TX0 != 0 && ELO_TY1 - ELO_TY0 != 0)
				{
					// use calibaration
					double dx = (ELO_X1 - ELO_X0) / (ELO_TX1- ELO_TX0);
					double dy = (ELO_Y1 - ELO_Y0) / (ELO_TY1 - ELO_TY0);
					x = (int) (ELO_X0 + (dx * (x - ELO_TX0)));
					y = (int) (ELO_Y0 + (dy * (y - ELO_TY0)));
				}

				m_prev_b = b;
				*state = b;
				*ts_x = x;
				*ts_y = y;
				//						int pressure = 0;
				//						if (elo_data[2] & ELO10_PRESSURE)
				//						{
				//							pressure = ((elo_data[8] << 8) | elo_data[7]);
				//						}

				m_size = 0;
				return true;
			}
		}
		return false;
	}

}
