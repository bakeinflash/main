// bakeinflash_render.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Interface to renderer back-end.


#ifndef BAKEINFLASH_RENDER_H
#define BAKEINFLASH_RENDER_H


#include "bakeinflash/bakeinflash_types.h"
#include "bakeinflash/bakeinflash.h"
#include "base/image.h"


namespace bakeinflash
{
	render_handler*	get_render_handler();

	namespace render
	{
		bitmap_info*	create_bitmap_info(image::image_base* im);
		video_handler*	create_video_handler();


		void clear();
		void read_buffer(int x, int y, int w, int h, Uint8* buf);

		// Bracket the displaying of a frame from a movie.
		// Fill the background color, and set up default
		// transforms, etc.
		void	begin_display(
			rgba background_color,
			int viewport_x0, int viewport_y0,
			int viewport_width, int viewport_height,
			float x0, float x1, float y0, float y1);
		void	end_display();

		// Geometric and color transforms for mesh and line_strip rendering.
		void	set_matrix(const matrix& m);
		void	set_cxform(const cxform& cx);
		void	set_rgba(rgba* color);

		// Draw triangles using the current fill-style 0.
		// Clears the style list after rendering.
		//
		// coords is a list of (x,y) coordinate pairs, in
		// triangle-strip order.  The type of the array should
		// be float[vertex_count*2]
		void	draw_mesh_strip(const coord_component coords[], int vertex_count);

		// Draw triangles using the current fill-style 0.
		// Clears the style list after rendering.
		//
		// coords is a list of (x,y) coordinate pairs, in
		// triangle-list order.  The type of the array should
		// be float[vertex_count*2]
		void draw_triangle_list(const coord_component coords[], int vertex_count);

		// Draw a line-strip using the current line style.
		// Clear the style list after rendering.
		//
		// Coords is a list of (x,y) coordinate pairs, in
		// sequence.
		void	draw_line_strip(const coord_component coords[], int vertex_count);

		void	fill_style_disable(int fill_side);
		void	fill_style_color(int fill_side, const rgba& color);
		void	fill_style_bitmap(int fill_side, bitmap_info* bi, const matrix& m,
			 render_handler::bitmap_wrap_mode wm, render_handler::bitmap_blend_mode bm);

		void	line_style_disable();
		void	line_style_color(rgba color);
		void	line_style_width(float width);
		void	line_style_caps(int caps_style);

		bool	test_stencil_buffer(const rect& bound, Uint8 pattern);
		void	begin_submit_mask();
		void	end_submit_mask();
		void	disable_mask();
		void	set_mask_bound(const rect& bound);
		const rect&	get_mask_bound();

		// Special function to draw a rectangular bitmap;
		// intended for textured glyph rendering.  Ignores
		// current transforms.
		void	draw_bitmap(const matrix& m, bitmap_info* bi, const rect& coords, const rect& uv_coords, rgba color);

		void set_cursor(render_handler::cursor_type cursor);
		bool is_visible(const rect& bound);
	};	// end namespace render
};	// end namespace bakeinflash


#endif // bakeinflash_RENDER_H

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
