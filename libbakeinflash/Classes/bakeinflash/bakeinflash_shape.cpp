// bakeinflash_shape.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Quadratic bezier outline shapes, the basis for most SWF rendering.


#include "bakeinflash/bakeinflash_shape.h"

#include "bakeinflash/bakeinflash_impl.h"
#include "bakeinflash/bakeinflash_log.h"
#include "bakeinflash/bakeinflash_render.h"
#include "bakeinflash/bakeinflash_stream.h"
#include "bakeinflash/bakeinflash_tesselate.h"

#include "base/tu_file.h"

#include <float.h>

//#define PRE_TESSELATE_SHAPES

// TODO: fix a some bugs
//#define USE_NEW_TESSELATOR

//#define DEBUG_DISPLAY_SHAPE_PATHS
#ifdef DEBUG_DISPLAY_SHAPE_PATHS
	// For debugging only!
	bool	bakeinflash_debug_show_paths = false;
#endif // DEBUG_DISPLAY_SHAPE_PATHS

extern bool bakeinflash_tesselate_dump_shape;

namespace bakeinflash
{
	static float	s_curve_max_pixel_error = 1.0f;

	void	set_curve_max_pixel_error(float pixel_error)
	{
		s_curve_max_pixel_error = fclamp(pixel_error, 1e-6f, 1e6f);
	}

	float	get_curve_max_pixel_error()
	{
		return s_curve_max_pixel_error;
	}

	//
	// edge
	//

	edge::edge() :
		m_cx(0), m_cy(0),
		m_ax(0), m_ay(0)
	{
	}

	edge::edge(float cx, float cy, float ax, float ay)
	{
		// edges can't be infinite
		m_cx = isfinitef(cx) ? cx : 0.0f;
		m_cy = isfinitef(cy) ? cy : 0.0f;
		m_ax = isfinitef(ax) ? ax : 0.0f;
		m_ay = isfinitef(ay) ? ay : 0.0f;
	}

	void	edge::tesselate_curve() const
	// Send this segment to the tesselator.
	{
		tesselate::add_curve_segment(m_cx, m_cy, m_ax, m_ay);
	}


	void	edge::tesselate_curve_new() const
	// Send this segment to the tesselator.
	{
		tesselate_new::add_curve_segment(m_cx, m_cy, m_ax, m_ay);
	}


	bool	edge::is_straight() const
	{
		return m_cx == m_ax && m_cy == m_ay;
	}


	//
	// path
	//


	path::path()
		:
		m_new_shape(false)
	{
		reset(0, 0, 0, 0, 0);
	}

	path::path(float ax, float ay, int fill0, int fill1, int line)
	{
		reset(ax, ay, fill0, fill1, line);
	}


	void	path::reset(float ax, float ay, int fill0, int fill1, int line)
	// Reset all our members to the given values, and clear our edge list.
	{
		m_ax = ax;
		m_ay = ay;
		m_fill0 = fill0;
		m_fill1 = fill1;
		m_line = line;

		m_edges.resize(0);

		assert(is_empty());
	}


	bool	path::is_empty() const
	// Return true if we have no edges.
	{
		return m_edges.size() == 0;
	}

	void	path::tesselate() const
	// Push this path into the tesselator.
	{
		tesselate::begin_path(
			m_fill0 - 1,
			m_fill1 - 1,
			m_line - 1,
			m_ax, m_ay);
		for (int i = 0; i < m_edges.size(); i++)
		{
			m_edges[i].tesselate_curve();
		}
		tesselate::end_path();
	}


	void	path::tesselate_new() const
	// Push this path into the tesselator.
	{
		tesselate_new::begin_path(
			m_fill0 - 1,
			m_fill1 - 1,
			m_line - 1,
			m_ax, m_ay);
		for (int i = 0; i < m_edges.size(); i++)
		{
			m_edges[i].tesselate_curve_new();
		}
		tesselate_new::end_path();
	}


	// Utility.

	template<class T>
	void	write_le(tu_file* out, T value);

	template<>
	void	write_le<float>(tu_file* out, float value)
	{
		out->write_float32(value);
	}

	template<>
	void	write_le<Sint16>(tu_file* out, Sint16 value)
	{
		out->write_le16(value);
	}

	template<class T>
	T	read_le(tu_file* in);

	template<>
	float	read_le<float>(tu_file* in)
	{
		return in->read_float32();
	}

	template<>
	Sint16    read_le<Sint16>(tu_file* in)
	{
		return in->read_le16();
	}

	void	write_coord_array(tu_file* out, const array<coord_component>& pt_array)
	// Dump the given coordinate array into the given stream.
	{
		int	n = pt_array.size();

		out->write_le32(n);
		for (int i = 0; i < n; i++)
		{
			write_le<coord_component>(out, pt_array[i]);
		}
	}


	void	read_coord_array(tu_file* in, array<coord_component>* pt_array)
	// Read the coordinate array data from the stream into *pt_array.
	{
		int	n = in->read_le32();

		pt_array->resize(n);
		for (int i = 0; i < n; i ++)
		{
			(*pt_array)[i] = read_le<coord_component>(in);
		}
	}


	//
	// mesh
	//

	
	mesh::mesh()
	{
	}

	void	mesh::set_tri_strip(const point pts[], int count)
	{
		m_triangle_strip.resize(count * 2);	// 2 coords per point
		
		// convert to ints.
		for (int i = 0; i < count; i++)
		{
			m_triangle_strip[i * 2] = coord_component(pts[i].m_x);
			m_triangle_strip[i * 2 + 1] = coord_component(pts[i].m_y);
		}

//		m_triangle_strip.resize(count);
//		memcpy(&m_triangle_strip[0], &pts[0], count * sizeof(point));
	}

	void mesh::reserve_triangles(int expected_triangle_count)
	{
		m_triangle_list.reserve(expected_triangle_count * 6);  // 6 coords per triangle
	}


	void mesh::add_triangle(const coord_component pts[6])
	{
		m_triangle_list.append(pts, 6);
	}


	void	mesh::display(const base_fill_style& style, float ratio, render_handler::bitmap_blend_mode bm) const
	{
		// pass mesh to renderer.
		if (m_triangle_strip.size() > 0)
		{
			style.apply(0, ratio, bm);
			render::draw_mesh_strip(&m_triangle_strip[0], m_triangle_strip.size() >> 1);
		}
		if (m_triangle_list.size() > 0) {
			style.apply(0, ratio, bm);
			render::draw_triangle_list(&m_triangle_list[0], m_triangle_list.size() >> 1);
		}
	}

	//
	// line_strip
	//


	line_strip::line_strip()
	// Default constructor, for array<>.
		:
		m_style(-1)
	{}


	line_strip::line_strip(int style, const point coords[], int coord_count)
	// Construct the line strip (polyline) made up of the given sequence of points.
		:
		m_style(style)
	{
		assert(style >= 0);
		assert(coords != NULL);
		assert(coord_count > 1);

//		m_coords.resize(coord_count);
//		memcpy(&m_coords[0], coords, coord_count * sizeof(coords[0]));
		m_coords.resize(coord_count * 2);	// 2 coords per vert
		
		// convert to ints.
		for (int i = 0; i < coord_count; i++)
		{
			m_coords[i * 2] = coord_component(coords[i].m_x);
			m_coords[i * 2 + 1] = coord_component(coords[i].m_y);
		}
	}

	void line_strip::tesselate_line(float line_width) const
	{
		float r = line_width / 2.0f;
		int circle_segments = (int) fclamp(TWIPS_TO_PIXELS(r) / 2, 2, 24);
		array<coord_component>&	strip = const_cast< array<coord_component>& >(m_triangle_strip);

		// math:
		// a = (y2-y1)/(x2-x1)
		// b = y1-a*x1
		// a = -1/a  perpendicular to the given
		// alpha = arctan(a)

		int points = m_coords.size() >> 1;
		int vertex_count = (points - 1) * ((circle_segments + 1) * 4 * 2 + 6);
		strip.resize(vertex_count);

		float x1, y1, x2, y2, dx, dy;
		int m = 0;
		float ppk = (float) M_PI / circle_segments;
		for (int i = 0; i < points - 1; i++)
		{
			int i2 = i * 2;
			x1 = m_coords[i2];
			y1 = m_coords[i2 + 1];
			x2 = m_coords[i2 + 2];
			y2 = m_coords[i2 + 3];

			float k = (y2 - y1) / (x2 - x1);
			float b = y1 - k * x1;

			float a = -1 / k;
			float aa = (float) atan2(a, 1);

			dx = r * cosf(aa);
			dy = r * sinf(aa);

			float x1a = x1 - dx;
			float y1a = y1 - dy;
			float x1b = x1 + dx;
			float y1b = y1 + dy;
			float x2a = x2 - dx;
			float y2a = y2 - dy;
			float x2b = x2 + dx;
			float y2b = y2 + dy;
			float xv = x1 - x2;
			float yv = y1 - y2;

			float sign = 1;
			if ((xv < 0 && yv >= 0 && fabs(k) != 0) || (xv >= 0 && yv >= 0 && fabs(k) != 0))
			{
				sign = -1;
			}

			// center of circle
			float cx = x1;
			float cy = y1;
			for (int i = 0; i <= circle_segments; i++)
			{
				float ra = aa - sign * i *  ppk;
				float rx = cx + (r * (float) cos(ra));
				float ry = cy + (r * (float) sin(ra));
				strip[m++] = cx;
				strip[m++] = cy;
				strip[m++] = rx;
				strip[m++] = ry;
			}

			// middle part
			strip[m++] = x1b;
			strip[m++] = y1b;
			strip[m++] = x2a;
			strip[m++] = y2a;
			strip[m++] = x2b;
			strip[m++] = y2b;

			// center of circle
			cx = x2;
			cy = y2;
			for (int i = 0; i <= circle_segments; i++)
			{
				float ra = aa + sign *  i *  ppk;
				float rx = cx + (r * (float) cos(ra));
				float ry = cy + (r * (float) sin(ra));
				strip[m++] = cx;
				strip[m++] = cy;
				strip[m++] = rx;
				strip[m++] = ry;
			}
		}
		assert(m==strip.size());
	}

	void	line_strip::display(const line_style& style, float ratio) const
	// Render this line strip in the given style.
	{
		assert(m_coords.size() > 1);
		assert((m_coords.size() & 1) == 0);

		float line_width = style.get_width();
		if (line_width >= PIXELS_TO_TWIPS(2))
		{
			if (m_triangle_strip.size() == 0)
			{
				tesselate_line(line_width);
			}
			render::fill_style_color(0,	style.get_color());
			render::draw_mesh_strip(&m_triangle_strip[0], m_triangle_strip.size() >> 1);
		}
		else
		{
			style.apply(ratio);
			render::draw_line_strip(&m_coords[0], m_coords.size() >> 1);
		}
	}

	// Utility: very simple greedy tri-stripper.  Useful for
	// stripping the stacks of trapezoids that come out of our
	// tesselator.
	struct tri_stripper
	{
		// A set of strips; we'll join them together into one
		// strip during the flush.
		array< array<point> >	m_strips;
		int	m_last_strip_used;

		tri_stripper()
			: m_last_strip_used(-1)
		{
		}

		void	add_trapezoid(const point& l0, const point& r0, const point& l1, const point& r1)
		// Add two triangles to our strip.
		{
			// See if we can attach this mini-strip to an existing strip.

			if (l0.bitwise_equal(r0) == false)
			{
				// Check the next strip first; trapezoids will
				// tend to arrive in rotating order through
				// the active strips.
				assert(m_last_strip_used >= -1 && m_last_strip_used < m_strips.size());
				int i = m_last_strip_used + 1, n = m_strips.size();
				for ( ; i < n; i++)
				{
					array<point>&	str = m_strips[i];
					assert(str.size() >= 3);	// should have at least one tri already.
				
					int	last = str.size() - 1;
					if (str[last - 1].bitwise_equal(l0) && str[last].bitwise_equal(r0))
					{
						// Can join these tris to this strip.
						str.push_back(l1);
						str.push_back(r1);
						m_last_strip_used = i;
						return;
					}
				}
				for (i = 0; i <= m_last_strip_used; i++)
				{
					array<point>&	str = m_strips[i];
					assert(str.size() >= 3);	// should have at least one tri already.
				
					int	last = str.size() - 1;
					if (str[last - 1].bitwise_equal(l0) && str[last].bitwise_equal(r0))
					{
						// Can join these tris to this strip.
						str.push_back(l1);
						str.push_back(r1);
						m_last_strip_used = i;
						return;
					}
				}
			}
			// else this trapezoid is pointy on top, so
			// it's almost certainly the start of a new
			// strip.  Don't bother searching current
			// strips.

			// Can't join with existing strip, so start a new strip.
			m_strips.resize(m_strips.size() + 1);
			m_strips.back().resize(4);
			m_strips.back()[0] = l0;
			m_strips.back()[1] = r0;
			m_strips.back()[2] = l1;
			m_strips.back()[3] = r1;
		}


		void	add_triangle(const point& v0, const point& v1, const point& v2)
		// Add a triangle to our strip.
		{
			// TODO: should probabably just rip out the
			// tristripper and directly use the trilists
			// produced by the new triangulator.
			//
			// Dumb temp code!
			if (m_strips.size() == 0) {
				// Start a new strip.
				m_strips.resize(m_strips.size() + 1);
			}

			m_strips[0].push_back(v0);
			m_strips[0].push_back(v0);
			m_strips[0].push_back(v1);
			m_strips[0].push_back(v2);
			m_strips[0].push_back(v2);
		}

		void	flush(mesh_set* m, int style) const
		// Join sub-strips together, and push the whole thing into the given mesh_set,
		// under the given style.
		{
			if (m_strips.size())
			{
				array<point>	big_strip;

				big_strip = m_strips[0];
				assert(big_strip.size() >= 3);

				for (int i = 1, n = m_strips.size(); i < n; i++)
				{
					// Append to the big strip.
					const array<point>&	str = m_strips[i];
					assert(str.size() >= 3);	// should have at least one tri already.
				
					int	last = big_strip.size() - 1;
					if (big_strip[last] == str[1]
					    && big_strip[last - 1] == str[0])
					{
 						// Strips fit right together.  Append.
						big_strip.append(&str[2], str.size() - 2);
					}
					else if (big_strip[last] == str[0]
						 && big_strip[last - 1] == str[1])
					{
						// Strips fit together with a half-twist.
						point	to_dup = big_strip[last - 1];
						big_strip.push_back(to_dup);
						big_strip.append(&str[2], str.size() - 2);
					}
					else
					{
						// Strips need a degenerate to link them together.
						point	to_dup = big_strip[last];
						big_strip.push_back(to_dup);
						big_strip.push_back(str[0]);
						big_strip.append(str);
					}
				}

				m->set_tri_strip(style, &big_strip[0], big_strip.size());
			}
		}
	};


	//
	// mesh_set
	//


	mesh_set::mesh_set()
		:
		m_error_tolerance(0)	// invalid -- don't use this constructor; it's only here for array (@@ fix array)
	{
	}

	mesh_set::mesh_set(const tesselate::tesselating_shape* sh, float error_tolerance)
	// Tesselate the shape's paths into a different mesh for each fill style.
		:
		m_error_tolerance(error_tolerance)
	{
		// For collecting trapezoids emitted by the old tesselator.
		struct collect_traps : public tesselate::trapezoid_accepter
		{
			mesh_set*	m;	// the mesh_set that receives trapezoids.
			bool m_new_layer;

			// strips-in-progress.
			hash<int, tri_stripper*>	m_strips;

			collect_traps(mesh_set* set) : m(set), m_new_layer(true) {}
			virtual ~collect_traps() {}

			// Overrides from trapezoid_accepter
			virtual void	accept_trapezoid(int style, const tesselate::trapezoid& tr)
			{
				// Add trapezoid to appropriate stripper.

				tri_stripper*	s = NULL;
				m_strips.get(style, &s);
				if (s == NULL)
				{
					s = new tri_stripper;
					m_strips.add(style, s);
				}

				s->add_trapezoid(
					point(tr.m_lx0, tr.m_y0),
					point(tr.m_rx0, tr.m_y0),
					point(tr.m_lx1, tr.m_y1),
					point(tr.m_rx1, tr.m_y1));
			}

			virtual void	accept_line_strip(int style, const point coords[], int coord_count)
			// Remember this line strip in our mesh set.
			{
				if (m_new_layer) {
					m->new_layer();
					m_new_layer = false;
				}
				m->add_line_strip(style, coords, coord_count);
			}

			void	flush()
			// Push our strips into the mesh set.
			{
				if (m_new_layer) {
					m->new_layer();
					m_new_layer = false;
				}
				for (hash<int, tri_stripper*>::const_iterator it = m_strips.begin();
				     it != m_strips.end();
				     ++it)
				{
					// Push strip into m.
					tri_stripper*	s = it->second;
					s->flush(m, it->first);
					
					delete s;
				}
			}

			void end_shape() {
				m_new_layer = true;
			}
		};

		// For collecting triangles emitted by the new tesselator.
		struct collect_tris : public tesselate_new::mesh_accepter
		{
			mesh_set*	ms;	// the mesh_set that receives triangles.
			mesh* m;
			bool m_new_layer;

			collect_tris(mesh_set* set) : ms(set), m(NULL), m_new_layer(true) {
			}
			virtual ~collect_tris() {}

			// Overrides from mesh_accepter
			
			virtual void	accept_line_strip(int style, const point coords[], int coord_count)
			// Remember this line strip in our mesh set.
			{
				if (m_new_layer) {
					ms->new_layer();
					m_new_layer = false;
				}
				ms->add_line_strip(style, coords, coord_count);
			}

			virtual void begin_trilist(int style, int expected_triangle_count)
			{
				assert(m == NULL);
				if (m_new_layer) {
					ms->new_layer();
					m_new_layer = false;
				}
				m = ms->get_mutable_mesh(style);
				m->reserve_triangles(expected_triangle_count);
			}

			virtual void accept_trilist_batch(const point trilist[], int point_count)
			// Accept one or more triangles to add to the
			// mesh for the specified style.
			{
				assert(m != NULL);
				
				// Convert input from float coords to
				// coord_component and add them to the mesh.
				coord_component tri[6];
				for (int i = 0; i < point_count; i += 3) {
					tri[0] = static_cast<coord_component>(trilist[i].m_x);
					tri[1] = static_cast<coord_component>(trilist[i].m_y);
					tri[2] = static_cast<coord_component>(trilist[i + 1].m_x);
					tri[3] = static_cast<coord_component>(trilist[i + 1].m_y);
					tri[4] = static_cast<coord_component>(trilist[i + 2].m_x);
					tri[5] = static_cast<coord_component>(trilist[i + 2].m_y);
					m->add_triangle(tri);
				}
			}

			virtual void end_trilist()
			{
				m = NULL;
			}

			virtual void end_shape()
			{
				m_new_layer = true;
			}
		};

#ifndef USE_NEW_TESSELATOR
		// Old tesselator.
		collect_traps	accepter(this);
		sh->tesselate(error_tolerance, &accepter);
		accepter.flush();
#else  // USE_NEW_TESSELATOR
		// New tesselator.
		collect_tris	accepter(this);
		sh->tesselate_new(error_tolerance, &accepter);
#endif // USE_NEW_TESSELATOR

		// triangles should be collected now into the meshes for each fill style.
	}


	mesh_set::~mesh_set()
	{
	}

	mesh_set::layer::~layer() {
		for (int i = 0; i < m_line_strips.size(); i++) {
			delete m_line_strips[i];
		}
		for (int i = 0; i < m_meshes.size(); i++) {
			delete m_meshes[i];
		}
	}


	void mesh_set::new_layer()
	// Make room for a new layer.
	{
		m_layers.resize(m_layers.size() + 1);
	}

	void	mesh_set::display(
		const matrix& mat,
		const cxform& cx,
		const array<fill_style>& fills,
		const array<line_style>& line_styles, render_handler::bitmap_blend_mode bm, rgba* color) const
	// Throw our meshes at the renderer.
	{
		assert(m_error_tolerance > 0);

		// Setup transforms.
		render::set_matrix(mat);
		render::set_cxform(cx);
		render::set_rgba(color);

		// Dump layers into renderer.
		for (int j = 0; j < m_layers.size(); j++) {
			const layer& l = m_layers[j];
			
			// Dump meshes into renderer, one mesh per style.
			for (int i = 0; i < l.m_meshes.size(); i++) {
				if (l.m_meshes[i]) {
					l.m_meshes[i]->display(fills[i], 1.0f, bm);
				}
			}

			// Dump line-strips into renderer.
			{for (int i = 0; i < l.m_line_strips.size(); i++)
			{
				int	style = l.m_line_strips[i]->get_style();
				l.m_line_strips[i]->display(line_styles[style], 1.0f);
			}}
		}
	}


	void mesh_set::expand_styles_to_include(int style)
	// 
	{
		assert(style >= 0);
		assert(style < 10000);	// sanity check

		layer* l = &m_layers.back();

		// Expand our mesh list if necessary.
		if (style >= l->m_meshes.size()) {
			l->m_meshes.resize(style + 1);
		}

		if (l->m_meshes[style] == NULL) {
			l->m_meshes[style] = new mesh;
		}
	}

	void	mesh_set::set_tri_strip(int style, const point pts[], int count)
	// Set mesh associated with the given fill style to the
	// specified triangle strip.
	{
		expand_styles_to_include(style);
		m_layers.back().m_meshes[style]->set_tri_strip(pts, count);
	}

	mesh* mesh_set::get_mutable_mesh(int style)
	{
		expand_styles_to_include(style);
		return m_layers.back().m_meshes[style];
	}

	void	mesh_set::add_line_strip(int style, const point coords[], int coord_count)
	// Add the specified line strip to our list of things to render.
	{
		assert(style >= 0);
		assert(style < 1000);	// sanity check
		assert(coords != NULL);
		assert(coord_count > 1);

		m_layers.back().m_line_strips.push_back(new line_strip(style, coords, coord_count));
	}

	//
	// helper functions.
	//


	static void	read_fill_styles(array<fill_style>* styles, stream* in, int tag_type, movie_definition_sub* m)
	// Read fill styles, and push them onto the given style array.
	{
		assert(styles);

		// Get the count.
		int	fill_style_count = in->read_u8();
		if (tag_type > 2)
		{
			if (fill_style_count == 0xFF)
			{
				fill_style_count = in->read_u16();
			}
		}

		IF_VERBOSE_PARSE(myprintf("  read_fill_styles: count = %d\n", fill_style_count));

		// Read the styles.
		for (int i = 0; i < fill_style_count; i++)
		{
			(*styles).resize((*styles).size() + 1);
			(*styles)[(*styles).size() - 1].read(in, tag_type, m);
		}
	}


	static void	read_line_styles(array<line_style>* styles, stream* in, int tag_type, movie_definition_sub* m)
	// Read line styles and push them onto the back of the given array.
	{
		// Get the count.
		int	line_style_count = in->read_u8();

		IF_VERBOSE_PARSE(myprintf("  read_line_styles: count = %d\n", line_style_count));

		// @@ does the 0xFF flag apply to all tag types?
		// if (tag_type > 2)
		// {
			if (line_style_count == 0xFF)
			{
				line_style_count = in->read_u16();
				IF_VERBOSE_PARSE(myprintf("  read_line_styles: count2 = %d\n", line_style_count));
			}
		// }

		// Read the styles.
		for (int i = 0; i < line_style_count; i++)
		{
			(*styles).resize((*styles).size() + 1);
			(*styles)[(*styles).size() - 1].read(in, tag_type, m);
		}
	}


	//
	// shape_character_def
	//


	shape_character_def::shape_character_def() :
		m_uses_nonscaling_strokes(false),
		m_uses_scaling_strokes(false)
	{
	}


	shape_character_def::~shape_character_def()
	{
		// Free our mesh_sets.
		for (int i = 0; i < m_cached_meshes.size(); i++)
		{
			delete m_cached_meshes[i];
		}
	}

	const shape_character_def &     shape_character_def::operator =( const shape_character_def & def )
	{
		m_fill_styles = def.m_fill_styles;
		m_line_styles = def.m_line_styles;
		m_paths = def.m_paths;
		m_bound = def.m_bound;
		m_edge_bounds = def.m_edge_bounds;
		m_uses_nonscaling_strokes = def.m_uses_nonscaling_strokes;
		m_uses_scaling_strokes = def.m_uses_scaling_strokes;
		return *this;
	}

	void	shape_character_def::read(stream* in, int tag_type, bool with_style, movie_definition_sub* m)
	{
		if (with_style)
		{
			m_bound.read(in);
	
			// DefineShape4, Flash 8
			if (tag_type == 83)
			{
				m_edge_bounds.read(in);
				Uint8 b = in->read_u8();
                assert( (b & 0xFC) == 0 );
				m_uses_nonscaling_strokes = b & 0x02 ? true : false;
				m_uses_scaling_strokes = b & 0x01 ? true : false;
			}

			read_fill_styles(&m_fill_styles, in, tag_type, m);
			read_line_styles(&m_line_styles, in, tag_type, m);
		}

		//
		// SHAPE
		//
		int	num_fill_bits = in->read_uint(4);
		int	num_line_bits = in->read_uint(4);

		IF_VERBOSE_PARSE(myprintf("  shape_character read: nfillbits = %d, nlinebits = %d\n", num_fill_bits, num_line_bits));

		// These are state variables that keep the
		// current position & style of the shape
		// outline, and vary as we read the edge data.
		//
		// At the moment we just store each edge with
		// the full necessary info to render it, which
		// is simple but not optimally efficient.
		int	fill_base = 0;
		int	line_base = 0;
		float	x = 0, y = 0;
		path	current_path;

#define SHAPE_LOG 0
		// SHAPERECORDS
		for (;;) {
			int	type_flag = in->read_uint(1);
			if (type_flag == 0)
			{
				// Parse the record.
				int	flags = in->read_uint(5);
				if (flags == 0) {
					// End of shape records.

					// Store the current path if any.
					if (! current_path.is_empty())
					{
						m_paths.push_back(current_path);
						current_path.m_edges.resize(0);
					}

					break;
				}
				if (flags & 0x01)
				{
					// move_to = 1;

					// Store the current path if any, and prepare a fresh one.
					if (! current_path.is_empty())
					{
						m_paths.push_back(current_path);
						current_path.m_edges.resize(0);
					}

					int	num_move_bits = in->read_uint(5);
					int	move_x = in->read_sint(num_move_bits);
					int	move_y = in->read_sint(num_move_bits);

					x = (float) move_x;
					y = (float) move_y;

					// Set the beginning of the path.
					current_path.m_ax = x;
					current_path.m_ay = y;

					if (SHAPE_LOG) IF_VERBOSE_PARSE(myprintf("  shape_character read: moveto %4g %4g\n", x, y));
				}
				if ((flags & 0x02)
					&& num_fill_bits > 0)
				{
					// fill_style_0_change = 1;
					if (! current_path.is_empty())
					{
						m_paths.push_back(current_path);
						current_path.m_edges.resize(0);
						current_path.m_ax = x;
						current_path.m_ay = y;
					}
					int	style = in->read_uint(num_fill_bits);
					if (style > 0)
					{
						style += fill_base;
					}
					current_path.m_fill0 = style;
					if (SHAPE_LOG) IF_VERBOSE_PARSE(myprintf("  shape_character read: fill0 = %d\n", current_path.m_fill0));
				}
				if ((flags & 0x04)
					&& num_fill_bits > 0)
				{
					// fill_style_1_change = 1;
					if (! current_path.is_empty())
					{
						m_paths.push_back(current_path);
						current_path.m_edges.resize(0);
						current_path.m_ax = x;
						current_path.m_ay = y;
					}
					int	style = in->read_uint(num_fill_bits);
					if (style > 0)
					{
						style += fill_base;
					}
					current_path.m_fill1 = style;
					if (SHAPE_LOG) IF_VERBOSE_PARSE(myprintf("  shape_character read: fill1 = %d\n", current_path.m_fill1));
				}
				if ((flags & 0x08)
					&& num_line_bits > 0)
				{
					// line_style_change = 1;
					if (! current_path.is_empty())
					{
						m_paths.push_back(current_path);
						current_path.m_edges.resize(0);
						current_path.m_ax = x;
						current_path.m_ay = y;
					}
					int	style = in->read_uint(num_line_bits);
					if (style > 0)
					{
						style += line_base;
					}
					current_path.m_line = style;
					if (SHAPE_LOG) IF_VERBOSE_PARSE(myprintf("  shape_character_read: line = %d\n", current_path.m_line));
				}
				if (flags & 0x10) {
					assert(tag_type >= 22);

					IF_VERBOSE_PARSE(myprintf("  shape_character read: more fill styles\n"));

					// Store the current path if any.
					if (! current_path.is_empty())
					{
						m_paths.push_back(current_path);
						current_path.m_edges.resize(0);

						// Clear styles.
						current_path.m_fill0 = -1;
						current_path.m_fill1 = -1;
						current_path.m_line = -1;
					}
					// Tack on an empty path signalling a new shape.
					// @@ need better understanding of whether this is correct??!?!!
					// @@ i.e., we should just start a whole new shape here, right?
					m_paths.push_back(path());
					m_paths.back().m_new_shape = true;

					fill_base = m_fill_styles.size();
					line_base = m_line_styles.size();
					read_fill_styles(&m_fill_styles, in, tag_type, m);
					read_line_styles(&m_line_styles, in, tag_type, m);
					num_fill_bits = in->read_uint(4);
					num_line_bits = in->read_uint(4);
				}
			}
			else
			{
				// EDGERECORD
				int	edge_flag = in->read_uint(1);
				if (edge_flag == 0)
				{
					// curved edge
					int num_bits = 2 + in->read_uint(4);
					float	cx = x + in->read_sint(num_bits);
					float	cy = y + in->read_sint(num_bits);
					float	ax = cx + in->read_sint(num_bits);
					float	ay = cy + in->read_sint(num_bits);

					if (SHAPE_LOG) IF_VERBOSE_PARSE(myprintf("  shape_character read: curved edge   = %4g %4g - %4g %4g - %4g %4g\n", x, y, cx, cy, ax, ay));

					current_path.m_edges.push_back(edge(cx, cy, ax, ay));

					x = ax;
					y = ay;
				}
				else
				{
					// straight edge
					int	num_bits = 2 + in->read_uint(4);
					int	line_flag = in->read_uint(1);
					float	dx = 0, dy = 0;
					if (line_flag)
					{
						// General line.
						dx = (float) in->read_sint(num_bits);
						dy = (float) in->read_sint(num_bits);
					}
					else
					{
						int	vert_flag = in->read_uint(1);
						if (vert_flag == 0) {
							// Horizontal line.
							dx = (float) in->read_sint(num_bits);
						} else {
							// Vertical line.
							dy = (float) in->read_sint(num_bits);
						}
					}

					if (SHAPE_LOG) IF_VERBOSE_PARSE(myprintf("  shape_character_read: straight edge = %4g %4g - %4g %4g\n", x, y, x + dx, y + dy));

					current_path.m_edges.push_back(edge(x + dx, y + dy, x + dx, y + dy));

					x += dx;
					y += dy;
				}
			}
		}

#ifdef PRE_TESSELATE_SHAPES
		// pre-tesselate
		float pixel_scale = 1;
		float max_scale = 10;
		float	object_space_max_error = 20.0f / max_scale / pixel_scale * s_curve_max_pixel_error;
		mesh_set*	mesh = new mesh_set(this, object_space_max_error * 0.75f);
		m_cached_meshes.push_back(mesh);
#endif
	}

	void	shape_character_def::load_bitmaps(array < smart_ptr<bitmap_info> >*	bitmap_hash)
	{
		for (int i = 0, n = m_fill_styles.size(); i < n; i++)
		{
			fill_style* fs = &m_fill_styles[i];
			bitmap_info* bi = fs->unzip_bitmap();
			if (bi)
			{
				bool add_item = true;
				for (int i = 0; i < bitmap_hash->size(); i++)
				{
					if (bitmap_hash->operator[](i).get() == bi)
					{
						add_item = false;
						break;
					}
				}

				if (add_item)
				{
					// myprintf("unzip bitmap: %dx%d\n", bi->get_width(), bi->get_height());
					bitmap_hash->push_back(bi);
					bi->upload();
				}
			}
		}
	}

	void	shape_character_def::unzip_bitmaps()
	{
		for (int i = 0, n = m_fill_styles.size(); i < n; i++)
		{
			fill_style* fs = &m_fill_styles[i];
			fs->unzip_bitmap();
		}
	}

	void	shape_character_def::clear_bitmaps()
	{
		for (int i = 0, n = m_fill_styles.size(); i < n; i++)
		{
			fill_style* fs = &m_fill_styles[i];
			fs->clear_bitmap();
		}
	}

	void	shape_character_def::display(character* inst)
	// Draw the shape using our own inherent styles.
	{
		matrix	mat;
		inst->get_world_matrix(&mat);
		cxform	cx;
		inst->get_world_cxform(&cx);

		float	pixel_scale = inst->get_parent()->get_pixel_scale();
		rgba* color = inst->get_rgba();

		switch(inst->get_blend_mode())
		{
		case 0:
		case 1:
			display(mat, cx, pixel_scale, m_fill_styles, m_line_styles, render_handler::BLEND_NORMAL, color);
			break;
		case 2:
			display(mat, cx, pixel_scale, m_fill_styles, m_line_styles, render_handler::BLEND_LAYER, color);
			break;
		case 3:
			display(mat, cx, pixel_scale, m_fill_styles, m_line_styles, render_handler::BLEND_MULTIPLY, color);
			break;
		case 4:
			display(mat, cx, pixel_scale, m_fill_styles, m_line_styles, render_handler::BLEND_SCREEN, color);
			break;
		case 5:
			display(mat, cx, pixel_scale, m_fill_styles, m_line_styles, render_handler::BLEND_LIGHTEN, color);
			break;
		case 6:
			display(mat, cx, pixel_scale, m_fill_styles, m_line_styles, render_handler::BLEND_DARKEN, color);
			break;
		case 7:
			display(mat, cx, pixel_scale, m_fill_styles, m_line_styles, render_handler::BLEND_DIFFERENCE, color);
			break;
		case 8:
			display(mat, cx, pixel_scale, m_fill_styles, m_line_styles, render_handler::BLEND_ADD, color);
			break;
		case 9:
			display(mat, cx, pixel_scale, m_fill_styles, m_line_styles, render_handler::BLEND_SUBTRACT, color);
			break;
		case 10:
			display(mat, cx, pixel_scale, m_fill_styles, m_line_styles, render_handler::BLEND_INVERT, color);
			break;
		case 11:
			display(mat, cx, pixel_scale, m_fill_styles, m_line_styles, render_handler::BLEND_ALPHA, color);
			break;
		case 12:
			display(mat, cx, pixel_scale, m_fill_styles, m_line_styles, render_handler::BLEND_ERASE, color);
			break;
		case 13:
			display(mat, cx, pixel_scale, m_fill_styles, m_line_styles, render_handler::BLEND_OVERLAY, color);
			break;
		case 14:
			display(mat, cx, pixel_scale, m_fill_styles, m_line_styles, render_handler::BLEND_HARDLIGHT, color);
			break;
		default:
			display(mat, cx, pixel_scale, m_fill_styles, m_line_styles, render_handler::BLEND_NORMAL, color);
			break;
		}
	}


#ifdef DEBUG_DISPLAY_SHAPE_PATHS

	static void	point_normalize(point* p)
	{
		float	mag2 = p->m_x * p->m_x + p->m_y * p->m_y;
		if (mag2 < 1e-9f)
		{
			// Very short vector.
			// @@ log error

			// Arbitrary unit vector.
			p->m_x = 1;
			p->m_y = 0;
		}

		float	inv_mag = 1.0f / sqrtf(mag2);
		p->m_x *= inv_mag;
		p->m_y *= inv_mag;
	}


	static void	show_fill_number(const point& p, int fill_number)
	{
		// We're inside a glBegin(GL_LINES)

		// Eh, let's do it in binary, least sig four bits...
		float	x = p.m_x;
		float	y = p.m_y;

		int	mask = 8;
		while (mask)
		{
			if (mask & fill_number)
			{
				// Vert line --> 1.
				glVertex2f(x, y - 40.0f);
				glVertex2f(x, y + 40.0f);
			}
			else
			{
				// Rectangle --> 0.
				glVertex2f(x - 10.0f, y - 40.0f);
				glVertex2f(x + 10.0f, y - 40.0f);

				glVertex2f(x + 10.0f, y - 40.0f);
				glVertex2f(x + 10.0f, y + 40.0f);

				glVertex2f(x - 10.0f, y + 40.0f);
				glVertex2f(x + 10.0f, y + 40.0f);

				glVertex2f(x - 10.0f, y - 40.0f);
				glVertex2f(x - 10.0f, y + 40.0f);
			}
			x += 40.0f;
			mask >>= 1;
		}
	}


	static void	debug_display_shape_paths(
		const matrix& mat,
		float object_space_max_error,
		const array<path>& paths,
		const array<fill_style>& fill_styles,
		const array<line_style>& line_styles)
	{
// Useful for debugging.  TODO: make a cleaner interface to this.
// 		// xxxxxxxx
// 		// Dump shape info.
// 		myprintf("\n# shape %x\n", (uint32) &paths);
// 		for (int i = 0; i < paths.size(); i++) {
// 			const path&	p = paths[i];
// 			myprintf("# path %d, fill0 = %d, fill1 = %d\n", i, p.m_fill0, p.m_fill1);
// 			myprintf("%d\n", p.m_edges.size() * 2 + 1);
// 			myprintf("%f %f\n", p.m_ax, p.m_ay);
// 			for (int j = 0; j < p.m_edges.size(); j++) {
// 				myprintf("%f %f\n", p.m_edges[j].m_cx, p.m_edges[j].m_cy);
// 				myprintf("%f %f\n", p.m_edges[j].m_ax, p.m_edges[j].m_ay);
// 			}
// 		}
// 		// xxxxxxx
		
		for (int i = 0; i < paths.size(); i++)
		{
			const path&	p = paths[i];

			if (p.m_fill0 == 0 && p.m_fill1 == 0)
			{
				continue;
			}

			bakeinflash::render::set_matrix(mat);

			// Color the line according to which side has
			// fills.
			if (p.m_fill0 == 0) glColor4f(0.25f * p.m_fill1, 0, 0, 0.5);
			else if (p.m_fill1 == 0) glColor4f(0, 0.25f * p.m_fill0, 0, 0.5);
			else glColor4f(0.25f * p.m_fill1, 0.25f * p.m_fill0, 0, 0.5);

			// Offset according to which loop we are.
			float	offset_x = (i & 1) * 80.0f;
			float	offset_y = ((i & 2) >> 1) * 80.0f;
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glTranslatef(offset_x, offset_y, 0.f);

			point	pt;

			glBegin(GL_LINE_STRIP);

			mat.transform(&pt, point(p.m_ax, p.m_ay));
			glVertex2f(pt.m_x, pt.m_y);

			for (int j = 0; j < p.m_edges.size(); j++)
			{
				mat.transform(&pt, point(p.m_edges[j].m_cx, p.m_edges[j].m_cy));
				glVertex2f(pt.m_x, pt.m_y);
				mat.transform(&pt, point(p.m_edges[j].m_ax, p.m_edges[j].m_ay));
				glVertex2f(pt.m_x, pt.m_y);
			}

			glEnd();

			// Draw arrowheads.
			point	dir, right, p0, p1;
			glBegin(GL_LINES);
			{for (int j = 0; j < p.m_edges.size(); j++)
			{
				mat.transform(&p0, point(p.m_edges[j].m_cx, p.m_edges[j].m_cy));
				mat.transform(&p1, point(p.m_edges[j].m_ax, p.m_edges[j].m_ay));
				dir = point(p1.m_x - p0.m_x, p1.m_y - p0.m_y);
				point_normalize(&dir);
				right = point(-dir.m_y, dir.m_x);	// perpendicular

				const float	ARROW_MAG = 60.0f;	// TWIPS?
				if (p.m_fill0 != 0)
				{
					glColor4f(0, 1, 0, 0.5);
					glVertex2f(p0.m_x,
						   p0.m_y);
					glVertex2f(p0.m_x - dir.m_x * ARROW_MAG - right.m_x * ARROW_MAG,
						   p0.m_y - dir.m_y * ARROW_MAG - right.m_y * ARROW_MAG);

					show_fill_number(point(p0.m_x - right.m_x * ARROW_MAG * 4,
							       p0.m_y - right.m_y * ARROW_MAG * 4),
							 p.m_fill0);
				}
				if (p.m_fill1 != 0)
				{
					glColor4f(1, 0, 0, 0.5);
					glVertex2f(p0.m_x,
						   p0.m_y);
					glVertex2f(p0.m_x - dir.m_x * ARROW_MAG + right.m_x * ARROW_MAG,
						   p0.m_y - dir.m_y * ARROW_MAG + right.m_y * ARROW_MAG);

					show_fill_number(point(p0.m_x + right.m_x * ARROW_MAG * 4,
							       p0.m_y + right.m_y * ARROW_MAG * 4),
							 p.m_fill1);
				}
			}}
			glEnd();

			glPopMatrix();
		}
	}
#endif // DEBUG_DISPLAY_SHAPE_PATHS


	void	shape_character_def::display( const matrix& mat, const cxform& cx, float pixel_scale, const array<fill_style>& fill_styles,
		const array<line_style>& line_styles, render_handler::bitmap_blend_mode bm, rgba* color) const
	// Display our shape.  Use the fill_styles arg to
	// override our default set of fill styles (e.g. when
	// rendering text).
	{
		// Compute the error tolerance in object-space.
		float	max_scale = mat.get_max_scale();
		if (fabsf(max_scale) < 1e-6f)
		{
			// Scale is essentially zero.
			return;
		}

// Useful for debugging.  TODO: make a cleaner interface to this.
// 		//xxxxxxxx
// 		//
// 		// If we want the tesselator to dump shapes, then
// 		// flush our cache so the shapes go through the
// 		// tesselator.
// 		if (bakeinflash_tesselate_dump_shape) {
// 			for (int i = 0; i < m_cached_meshes.size(); i++) {
// 				delete m_cached_meshes[i];
// 			}
// 			m_cached_meshes.resize(0);
// 		}

		float	object_space_max_error = 20.0f / max_scale / pixel_scale * s_curve_max_pixel_error;

#ifdef DEBUG_DISPLAY_SHAPE_PATHS
		// Render a debug view of shape path outlines, instead
		// of the tesselated shapes themselves.
		if (bakeinflash_debug_show_paths)
		{
			debug_display_shape_paths(mat, object_space_max_error, m_paths, fill_styles, line_styles);

			return;
		}
#endif // DEBUG_DISPLAY_SHAPE_PATHS

		// See if we have an acceptable mesh available; if so then render with it.
		for (int i = 0, n = m_cached_meshes.size(); i < n; i++)
		{
			const mesh_set*	candidate = m_cached_meshes[i];

			if (object_space_max_error > candidate->get_error_tolerance() * 3.0f)
			{
				// Mesh is too high-res; the remaining meshes are higher res,
				// so stop searching and build an appropriately scaled mesh.
#ifdef PRE_TESSELATE_SHAPES
				candidate->display(mat, cx, fill_styles, line_styles, bm);
				return;
#else
				break;
#endif
			}

			if (object_space_max_error > candidate->get_error_tolerance())
			{
				// Do it.
				candidate->display(mat, cx, fill_styles, line_styles, bm, color);
				return;
			}
		}

		// Construct a new mesh to handle this error tolerance.
		mesh_set*	m = new mesh_set(this, object_space_max_error * 0.75f);
		m_cached_meshes.push_back(m);
		m->display(mat, cx, fill_styles, line_styles, bm, color);
		
		sort_and_clean_meshes();
	}


	static int	sort_by_decreasing_error(const void* A, const void* B)
	{
		const mesh_set*	a = *(const mesh_set**) A;
		const mesh_set*	b = *(const mesh_set**) B;

		if (a->get_error_tolerance() < b->get_error_tolerance())
		{
			return 1;
		}
		else if (a->get_error_tolerance() > b->get_error_tolerance())
		{
			return -1;
		}
		else
		{
			return 0;
		}
	}


	void	shape_character_def::sort_and_clean_meshes() const
	// Maintain cached meshes.  Clean out mesh_sets that haven't
	// been used recently, and make sure they're sorted from high
	// error to low error.
	{
		// Re-sort.
		if (m_cached_meshes.size() > 0)
		{
			qsort(
				&m_cached_meshes[0],
				m_cached_meshes.size(),
				sizeof(m_cached_meshes[0]),
				sort_by_decreasing_error);

			// Check to make sure the sort worked as intended.
			#ifndef NDEBUG
			for (int i = 0, n = m_cached_meshes.size() - 1; i < n; i++)
			{
				const mesh_set*	a = m_cached_meshes[i];
				const mesh_set*	b = m_cached_meshes[i + 1];

				assert(a->get_error_tolerance() > b->get_error_tolerance());
			}
			#endif // not NDEBUG
		}
	}

		
	void	shape_character_def::tesselate(float error_tolerance, tesselate::trapezoid_accepter* accepter) const
	// Push our shape data through the tesselator.
	{
		tesselate::begin_shape(accepter, error_tolerance);
		for (int i = 0; i < m_paths.size(); i++)
		{
			if (m_paths[i].m_new_shape == true)
			{
				// Hm; should handle separate sub-shapes in a less lame way.
				tesselate::end_shape();
				tesselate::begin_shape(accepter, error_tolerance);
			}
			else
			{
				m_paths[i].tesselate();
			}
		}
		tesselate::end_shape();
	}


	void	shape_character_def::tesselate_new(float error_tolerance, tesselate_new::mesh_accepter* accepter) const
	// Push our shape data through the tesselator.
	{
		tesselate_new::begin_shape(accepter, error_tolerance);
		for (int i = 0; i < m_paths.size(); i++)
		{
			if (m_paths[i].m_new_shape == true)
			{
				// Hm; should handle separate sub-shapes in a less lame way.
				tesselate_new::end_shape();
				tesselate_new::begin_shape(accepter, error_tolerance);
			}
			else
			{
				m_paths[i].tesselate_new();
			}
		}
		tesselate_new::end_shape();
	}


	bool	shape_character_def::point_test_local(float x, float y)
	// Return true if the specified point is on the interior of our shape.
	// Incoming coords are local coords.
	{
		if (m_bound.point_test(x, y) == false)
		{
			// Early out.
			return false;
		}

		// test point in shape
		return point_test(x,y);
	}


	bool shape_character_def::point_test(float x,  float y)
	{
		// later we will need non-zero for glyphs... (TODO)
		bool even_odd = true;  

		int npaths = m_paths.size();
		int counter = 0;

		// browse all paths
		for (int pno = 0; pno < npaths; pno++)
		{
			const path& pth = m_paths[pno];
			unsigned nedges = pth.m_edges.size();

			float next_pen_x = pth.m_ax;
			float next_pen_y = pth.m_ay;
			float pen_x, pen_y;

			if (pth.m_new_shape)
			{
				if (( even_odd && (counter % 2) != 0) || (!even_odd && (counter != 0)) )
				{
					// the point is inside the previous subshape, so exit now
					return true;
				}
				counter=0;
			}

			if (pth.is_empty()) continue;

			// TODO
			// If the path has a line style, check for strokes there
/*			if (pth.m_line != 0 )
			{
				//   assert(lineStyles.size() >= pth.m_line);
				const line_style& ls = m_line_styles[pth.m_line-1];
				double thickness = ls.get_width();
				if (!thickness)
				{
					thickness = 20; // at least ONE PIXEL thick.
				}
				if (!ls.get_no_vscale_flag() &&	!ls.get_no_hscale_flag())
				{
					// TODO: pass the SWFMatrix to withinSquareDistance instead ?
					//	double xScale = m.get_x_scale();
					//	double yScale = m.get_y_scale();
					//	thickness *= max(xScale, yScale);
					thickness = 20; // at least ONE PIXEL thick.
				}
				else
					if (ls.get_no_vscale_flag() != ls.get_no_vscale_flag())
					{
						IF_VERBOSE_ACTION(myprintf("Collision detection for unidirectionally scaled strokes")));
					}

					double dist = thickness / 2.0;
					double sqdist = dist * dist;
					if (pth.withinSquareDistance(pt, sqdist))
					{
						return true;
					}
			}
			*/

			// browse all edges of the path
			for (unsigned eno=0; eno<nedges; eno++)
			{
				const edge& edg = pth.m_edges[eno];
				pen_x = next_pen_x;
				pen_y = next_pen_y;
				next_pen_x = edg.m_ax;
				next_pen_y = edg.m_ay;

				float cross1 = 0.0, cross2 = 0.0;
				int dir1 = 0, dir2 = 0; // +1 = downward, -1 = upward
				int crosscount = 0;

				if (edg.is_straight())
				{
					// ignore horizontal lines
					// TODO: better check for small difference?
					if (edg.m_ay == pen_y)  
					{
						continue;
					}

					// does this line cross the Y coordinate?
					if ( ((pen_y <= y) && (edg.m_ay >= y)) || ((pen_y >= y) && (edg.m_ay <= y)) )
					{
						// calculate X crossing
						cross1 = pen_x + (edg.m_ax - pen_x) *	(y - pen_y) / (edg.m_ay - pen_y);

						if (pen_y > edg.m_ay)
							dir1 = -1;  // upward
						else
							dir1 = +1;  // downward

						crosscount = 1;
					}
					else
					{
						// no crossing found
						crosscount = 0;
					}
				}
				else
				{
					// ==> curve case
					crosscount = curve_x_crossings(pen_x, pen_y, edg.m_ax, edg.m_ay, edg.m_cx, edg.m_cy, y, cross1, cross2);
					dir1 = pen_y > y ? -1 : +1;
					dir2 = dir1 * (-1); // second crossing always in opposite dir.
				} // curve

				// ==> we have now:
				//  - one (cross1) or two (cross1, cross2) ray crossings (X
				//    coordinate)
				//  - dir1/dir2 tells the direction of the crossing
				//    (+1 = downward, -1 = upward)
				//  - crosscount tells the number of crossings

				// need at least one crossing
				if (crosscount == 0)
				{
					continue;
				}

				// check first crossing
				if (cross1 <= x)
				{
					if (pth.m_fill0 > 0) counter += dir1;
					if (pth.m_fill1 > 0) counter -= dir1;
				}

				// check optional second crossing (only possible with curves)
				if ( (crosscount > 1) && (cross2 <= x) )
				{
					if (pth.m_fill0 > 0) counter += dir2;
					if (pth.m_fill1 > 0) counter -= dir2;
				}

			}// for edge
		} // for path

		return ( (even_odd && (counter % 2) != 0) || (!even_odd && (counter != 0)) );
	}

	// TODO: this should be moved to libgeometry or something
	// Finds the quadratic bezier curve crossings with the line Y.
	// The function can have zero, one or two solutions (cross1, cross2). The
	// return value of the function is the number of solutions.
	// x0, y0 = start point of the curve
	// x1, y1 = end point of the curve (anchor, aka ax|ay)
	// cx, cy = control point of the curve
	// If there are two crossings, cross1 is the nearest to x0|y0 on the curve.
	int shape_character_def::curve_x_crossings(float x0, float y0, float x1, float y1,  float cx, float cy, float y, float& cross1, float& cross2)
	{
		int count=0;

		// check if any crossings possible
		if ( ((y0 < y) && (y1 < y) && (cy < y)) || ((y0 > y) && (y1 > y) && (cy > y)) )
		{
			// all above or below -- no possibility of crossing
			return 0;
		}

		// Quadratic bezier is:
		// p = (1-t)^2 * a0 + 2t(1-t) * c + t^2 * a1

		// We need to solve for x at y. Use the quadratic formula.

		// Numerical Recipes suggests this variation:
		// q = -0.5 [b +sgn(b) sqrt(b^2 - 4ac)]
		// x1 = q/a;  x2 = c/q;

		float A = y1 + y0 - 2 * cy;
		float B = 2 * (cy - y0);
		float C = y0 - y;

		float rad = B * B - 4 * A * C;
		if (rad < 0) 
		{
			return 0;
		}
		else
		{
			float q;
			float sqrt_rad = (float) sqrt(rad);
			if (B < 0)
			{
				q = -0.5f * (B - sqrt_rad);
			}
			else
			{
				q = -0.5f * (B + sqrt_rad);
			}

			// The old-school way.
			// float t0 = (-B + sqrt_rad) / (2 * A);
			// float t1 = (-B - sqrt_rad) / (2 * A);

			if (q != 0)
			{
				float t1 = C / q;

				if (t1 >= 0 && t1 < 1) 
				{
					float x_at_t1 =	x0 + 2 * (cx - x0) * t1 + (x1 + x0 - 2 * cx) * t1 * t1;

					count++;
					assert(count==1);
					cross1 = x_at_t1;             // order is important!
				}
			}

			if (A != 0)
			{
				float t0 = q / A;
				if (t0 >= 0 && t0 < 1) 
				{
					float x_at_t0 =	x0 + 2 * (cx - x0) * t0 + (x1 + x0 - 2 * cx) * t0 * t0;

					++count;
					// order is important!
					if (count == 2) cross2 = x_at_t0;
					else cross1 = x_at_t0;
				}
			}
		}
		return count;
	}

	void shape_character_def::get_bound(rect* bound)
	{
		*bound = m_bound;
	}

	void	shape_character_def::compute_bound(rect* r) const
	// Find the bounds of this shape, and store them in
	// the given rectangle.
	{
		r->m_x_min = FLT_MAX;
		r->m_y_min = FLT_MAX;
		r->m_x_max = -FLT_MAX;
		r->m_y_max = -FLT_MAX;

		for (int i = 0; i < m_paths.size(); i++)
		{
			const path&	p = m_paths[i];
			r->expand_to_point(p.m_ax, p.m_ay);
			for (int j = 0; j < p.m_edges.size(); j++)
			{
				r->expand_to_point(p.m_edges[j].m_ax, p.m_edges[j].m_ay);
//					r->expand_to_point(p.m_edges[j].m_cx, p.m_edges[j].m_cy);
			}
		}
	}

	void    shape_character_def::flush_cache()
	{
		for (int i = 0; i < m_cached_meshes.size(); i++)
		{
			delete m_cached_meshes[i];
		}
		m_cached_meshes.resize( 0 );
	}


	
}	// end namespace bakeinflash


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
