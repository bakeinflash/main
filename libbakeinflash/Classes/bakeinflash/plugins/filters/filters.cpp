// bakeinflash_3ds.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Lib3ds plugin implementation for bakeinflash library

#include "base/tu_config.h"
#include <stdlib.h>
#include "base/tu_timer.h"
#include "filters.h"

namespace bakeinflash
{

	void	as_bevelFilter_ctor(const fn_call& fn)
		// Constructor for ActionScript class
	{
	}

	void	as_embossFilter_ctor(const fn_call& fn)
		// Constructor for ActionScript class
	{
		if (fn.nargs >= 2)
		{
			sprite_instance* parent = cast_to<sprite_instance>(fn.arg(0).to_object());
			as_embossFilter* obj = new as_embossFilter(parent, fn.arg(1).to_tu_string());
			fn.result->set_as_object(obj);
		}

	}

	as_embossFilter::as_embossFilter(sprite_instance* parent, const tu_string& file) :
		character(parent, 0)
	{
		set_member("thresh", 10);
		set_member("threshold1", 10);
		set_member("angle", 0);
		set_member("brighness", 50);
		set_member("blur", 15);
		set_member("erode", 0);
		set_member("dilate", 6);

		parent->get_bound(&m_bound);

		tu_string wd = get_workdir();
		tu_string path = wd + file;
		cv::Mat img_input = cv::imread(path.c_str());

		// rgba ==> bgra
		cvtColor(img_input, img, CV_RGBA2BGRA);

		// converting the image to grayscale
		cv::cvtColor(img_input, mask, cv::COLOR_BGR2GRAY);
		//cv::imshow("gray", mask);

		parent->clear_display_objects();

		cxform cx;

		// needs to use world matrix because rendered bitmap is in global coords
		// _parent local coords ==> global coords
		matrix m; // = get_matrix();
		parent->get_parent()->get_world_matrix(&m);


		int depth = parent->get_highest_depth() + 1;
		m_def = new as_embossFilter_character_def();
		parent->m_display_list.add_display_object(this, depth, true, cx, m, 0.0f, 0, 0);

	}

	void	as_embossFilter::display()
	{
		cxform	cx;
		get_world_cxform(&cx);
		rgba	transformed_color; // = cx.transform(m_color);

		smart_ptr<bitmap_info> bi = apply();
		rect uv_bounds(0, bi->get_xratio(), 0, bi->get_yratio());
		render::draw_bitmap(m_matrix, bi, m_bound, uv_bounds, transformed_color);
	}

	bitmap_info* as_embossFilter::apply()
	{
		Uint32 t = tu_timer::get_ticks();

		as_value thresh;
		get_member("thresh", &thresh);
		as_value threshold1;
		get_member("threshold1", &threshold1);
		as_value angle;
		get_member("angle", &angle);
		as_value brighness;
		get_member("brighness", &brighness);
		as_value myblur;
		get_member("blur", &myblur);
		as_value myerode;
		get_member("erode", &myerode);
		as_value mydilate;
		get_member("dilate", &mydilate);

		//find edges and invert image for distance transform
		cv::Mat dist, bevel;
		Canny(mask, dist, threshold1.to_number(), threshold1.to_number() * 3);
		dist = 255 - dist;
		distanceTransform(dist, dist, CV_DIST_L2, CV_DIST_MASK_5);
		threshold(dist, dist, thresh.to_number(), thresh.to_number(), CV_THRESH_TRUNC);

		if (myerode.to_number() > 1 && myerode.to_number() < 50)
		{
			int type = cv::MORPH_ELLIPSE;
			//if (erosion_elem == 0) { erosion_type = MORPH_RECT; }
			//else if (erosion_elem == 1) { erosion_type = MORPH_CROSS; }
			//else if (erosion_elem == 2) { erosion_type = MORPH_ELLIPSE; }
			int erosion_size = myerode.to_number();
			cv::Mat element = getStructuringElement(type, cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1), cv::Point(erosion_size, erosion_size));
			erode(dist, dist, element);
		}
		if (mydilate.to_number() > 1 && mydilate.to_number() < 50)
		{
			int type = cv::MORPH_ELLIPSE;
			//if (erosion_elem == 0) { erosion_type = MORPH_RECT; }
			//else if (erosion_elem == 1) { erosion_type = MORPH_CROSS; }
			//else if (erosion_elem == 2) { erosion_type = MORPH_ELLIPSE; }
			int erosion_size = mydilate.to_number();
			cv::Mat element = getStructuringElement(type, cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1), cv::Point(erosion_size, erosion_size));
			dilate(dist, dist, element);
		}

		blur(dist, dist, cv::Size(1 + myblur.to_number(), 1 + myblur.to_number()));
		dist.convertTo(bevel, CV_8U);
		//imshow("2 dist", dist);


		equalizeHist(bevel, bevel);
		//	imshow("2",bevel);

		//convolve with secret sauce
		int k = int(angle.to_number() / 256*8);
		assert(k <= 8);
		float d[8][9] = { 
			{ -1, -1, -1, 0, 3, 0, 0, 0, 0 },
			{  0, -1, -1, 0, 3,-1, 0, 0, 0 },
			{  0,  0, -1, 0, 3,-1, 0, 0,-1 },
			{  0,  0,  0, 0, 3,-1, 0,-1,-1 },
			{  0,  0,  0, 0, 3, 0,-1,-1,-1 },
			{ 0,  0,  0, -1, 3,0,-1,-1, 0 },
			{ -1,  0,  0,-1, 3,0,-1, 0, 0 },
			{ -1, -1,  0,-1, 3,0, 0, 0, 0 }
		};
		cv::Mat kernel(3, 3, CV_32F, d[k]);
		kernel = kernel - cv::mean(kernel)[0];
		filter2D(dist, dist, CV_32F, kernel);

		//normalize filtering result to [-1, 1]
		double maxVal;
		minMaxLoc(dist, NULL, &maxVal);
		dist = 128 * dist / maxVal;

		//convert and display result
		dist.convertTo(bevel, CV_8U, 1, 128);
		bevel = bevel.mul(mask) / 255;
		//	imshow("3", bevel);

		int bufsize = img.cols * img.rows * 4;
		image::rgba*	im = new image::rgba(img.cols, img.rows);
		memcpy(im->m_data, img.data, bufsize);
		Uint8* pp = im->m_data;
		for (int i = 0, n = img.cols * img.rows; i < n; i++)
		{
			int a = bevel.data[i] + (brighness.to_number() - 128);
			pp[0] = iclamp(pp[0] + a, 0, 255);
			pp[1] = iclamp(pp[1] + a, 0, 255);
			pp[2] = iclamp(pp[2] + a, 0, 255);
			pp += 4;
		}

		//new bitmap_info(4, m_bitmap_info->get_width(), m_bitmap_info->get_height(), img);
		bitmap_info* bbi = render::create_bitmap_info(im);

	//	printf("timer: %d\n", tu_timer::get_ticks() - t);
		return bbi;
	}

} // end of namespace bakeinflash
