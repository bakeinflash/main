// bakeinflash_canvas.cpp	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Drawing API implementation

#include "bakeinflash_canvas.h"
#include "bakeinflash/bakeinflash_tesselate.h"

namespace bakeinflash
{

	canvas::canvas() :
		m_current_x(FLT_MIN),
		m_current_y(FLT_MIN),
		m_current_fill(0),
		m_current_line(0),
		m_current_path(-1)
	{
	}

	canvas::~canvas()
	{
	}

	void canvas::begin_fill(const rgba& color)
	{

		// default fill style is solid
		fill_style fs;
		fs.m_type = 0x00; 
		fs.m_color = color;
		m_fill_styles.push_back(fs);
		m_current_fill = m_fill_styles.size();
	}

	void canvas::end_fill()
	{
		if (m_current_path >= 0)
		{
			close_path();
		}
		m_current_path = -1;
		m_current_fill = 0;
	}

	void canvas::close_path()
	{
		path& p = m_paths[m_current_path];
		array<edge>& edges = m_paths[m_current_path].m_edges;
		if (edges.size() > 0)
		{
			// Close it with a straight edge if needed
			const edge& ed = edges.back();
			if (ed.m_ax != p.m_ax	|| ed.m_ay != p.m_ay)
			{
				edge new_edge(p.m_ax, p.m_ay, p.m_ax, p.m_ay);
				edges.push_back(new_edge);
			}
		}
	}

	void canvas::add_path(bool new_path)
	{
		if (m_current_path >= 0 && m_current_fill > 0)
		{
			close_path();
		}

		// Flash uses the left fill style
		path p(m_current_x, m_current_y, m_current_fill, 0, m_current_line);
		m_paths.push_back(p);
		m_current_path = m_paths.size() - 1;
		
		flush_cache();
	}

	void canvas::set_line_style(Uint16 width, const rgba& color, const tu_string& caps_style)
	{
		line_style ls;
		ls.m_color = color;
		ls.m_width = width;

		if (caps_style == "round")
		{
			ls.m_caps_style = line_style::ROUND;
		}
		else
		if (caps_style == "square")
		{
			ls.m_caps_style = line_style::SQUARE;
		}
		else
		if (caps_style == "ball")			// extension
		{
			ls.m_caps_style = line_style::BALL;
		}
		else
		if (caps_style == "point")		// extension
		{
			ls.m_caps_style = line_style::POINT;
		}
		else
		{
			ls.m_caps_style = line_style::NONE;
		}

		m_line_styles.push_back(ls);
		m_current_line = m_line_styles.size();
		add_path(false);
	}

	void canvas::move_to(float x, float y)
	{
		if (x != m_current_x || y != m_current_y)
		{
			m_current_x = x;
			m_current_y = y;
			m_bound.expand_to_point(x, y);
			add_path(false);
		}
	}

	void canvas::line_to(float x, float y)
	{
		if (x != m_current_x || y != m_current_y)
		{
			if (m_current_path < 0)
			{
				add_path(true);
			}

			m_current_x = x;
			m_current_y = y;
			m_bound.expand_to_point(x, y);

			edge ed(x, y, x, y);
			m_paths[m_current_path].m_edges.push_back(ed); 
			flush_cache();
		}
	}

	void canvas::curve_to(float cx, float cy, float ax, float ay)
	{
		if (m_current_path >= 0)
		{
			add_path(true); 
		}

		m_current_x = ax;
		m_current_y = ay;
		m_bound.expand_to_point(ax, ay);

		m_paths[m_current_path].m_edges.push_back(edge(cx, cy, ax, ay)); 
		flush_cache();
	}

	void canvas::get_bound(rect* bound)
	{
		compute_bound(bound);
	}

	void	canvas::tesselate(float error_tolerance, tesselate::trapezoid_accepter* accepter) const
	// Push our shape data through the tesselator.
	{
		for (int i = 0; i < m_paths.size(); i++)
		{
			tesselate::begin_shape(accepter, error_tolerance);
			m_paths[i].tesselate();
			tesselate::end_shape();
		}
	}

}	// end namespace bakeinflash
