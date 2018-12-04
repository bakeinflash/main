#ifndef TOUCHSCREEN_H
#define TOUCHSCREEN_H

#include "bakeinflash/bakeinflash_root.h"
#include "base/tu_serial.h"

namespace bakeinflash
{
	struct elo : public ref_counted   
	{
		elo();
		~elo();

		//virtual void advance(float delta_time);
		bool open(const tu_string& model, const tu_string& port);
		bool read(int w, int h, int* mouse_x, int* mouse_y, int* state);

	private:

		smart_ptr<serial> m_serial;
		int m_size;
		Uint8 m_buf[10];
		int m_prev_b;

		// calibration data
		// [tx,ty,xx,yy],[tx,ty,xx,yy]   8 numbers
		array<double> m_pt;
	};

}

#endif
