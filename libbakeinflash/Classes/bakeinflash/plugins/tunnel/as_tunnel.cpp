// bakeinflash_3ds.cpp	-- Vitaly Alexeev <tishka92@yahoo.com>	2007

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Lib3ds plugin implementation for bakeinflash library

#include "base/tu_config.h"

#include "as_tunnel.h"
#include "base/tu_random.h"
#include <stdlib.h>
#include <time.h>

#define rand_max 2147483647

void gluPerspective(float fov, float aspect, float znear, float zfar);
void gluLookAt(float eyex, float eyey, float eyez, float centerx, float centery, float centerz, float upx, float upy, float upz);

extern int screen_height;
extern int screen_width;
extern int fullscreen_mode;

extern PFNGLACTIVETEXTUREPROC glActiveTexture;

namespace bakeinflash
{

	void	as_tunnel_ctor(const fn_call& fn)
	// Constructor for ActionScript class
	{
		if (fn.nargs >= 1)
		{
			sprite_instance* parent = cast_to<sprite_instance>(fn.arg(0).to_object());
			as_tunnel* obj = new as_tunnel(parent);
			fn.result->set_as_object(obj);
		}
	}

	void	as_tunnel_get_bitmapdata(const fn_call& fn)
	{
		as_tunnel* obj = cast_to<as_tunnel>(fn.this_ptr);
		assert(obj);
		fn.result->set_as_object(obj->m_bitmapdata);
	}

	void	as_tunnel_set_bitmapdata(const fn_call& fn)
	{
		as_tunnel* obj = cast_to<as_tunnel>(fn.this_ptr);
		if (obj && fn.nargs >= 1)
		{
			obj->m_bitmapdata = cast_to<as_bitmapdata>(fn.arg(0).to_object()); 
		}
	}

	void	as_tunnel_get_speed(const fn_call& fn)
	{
		as_tunnel* obj = cast_to<as_tunnel>(fn.this_ptr);
		assert(obj);
		fn.result->set_double(obj->m_speed);
	}

	void	as_tunnel_set_speed(const fn_call& fn)
	{
		as_tunnel* obj = cast_to<as_tunnel>(fn.this_ptr);
		if (obj && fn.nargs >= 1)
		{
			obj->m_speed = fn.arg(0).to_float();
		}
	}

	void	as_tunnel_get_diameter(const fn_call& fn)
	{
		as_tunnel* obj = cast_to<as_tunnel>(fn.this_ptr);
		assert(obj);
		fn.result->set_double(obj->m_diameter);
	}

	void	as_tunnel_set_diameter(const fn_call& fn)
	{
		as_tunnel* obj = cast_to<as_tunnel>(fn.this_ptr);
		if (obj && fn.nargs >= 1)
		{
			obj->m_diameter = fn.arg(0).to_float();
		}
	}

	void	as_tunnel_get_red(const fn_call& fn)
	{
		as_tunnel* obj = cast_to<as_tunnel>(fn.this_ptr);
		assert(obj);
		fn.result->set_double(obj->m_colour.m_r);
	}
	void	as_tunnel_set_red(const fn_call& fn)
	{
		as_tunnel* obj = cast_to<as_tunnel>(fn.this_ptr);
		if (obj && fn.nargs >= 1)
		{
			obj->m_colour.m_r = (Uint8) fn.arg(0).to_float();
		}
	}

	void	as_tunnel_get_sharp(const fn_call& fn)
	{
		as_tunnel* obj = cast_to<as_tunnel>(fn.this_ptr);
		assert(obj);
		fn.result->set_double(obj->m_sharp);
	}
	void	as_tunnel_set_sharp(const fn_call& fn)
	{
		as_tunnel* obj = cast_to<as_tunnel>(fn.this_ptr);
		if (obj && fn.nargs >= 1)
		{
			obj->m_sharp = fn.arg(0).to_float();
		}
	}

	void	as_tunnel_get_smooth(const fn_call& fn)
	{
		as_tunnel* obj = cast_to<as_tunnel>(fn.this_ptr);
		assert(obj);
		fn.result->set_double(obj->m_smooth);
	}
	void	as_tunnel_set_smooth(const fn_call& fn)
	{
		as_tunnel* obj = cast_to<as_tunnel>(fn.this_ptr);
		if (obj && fn.nargs >= 1)
		{
			obj->m_smooth = fn.arg(0).to_float();
		}
	}

	void	as_tunnel_get_green(const fn_call& fn)
	{
		as_tunnel* obj = cast_to<as_tunnel>(fn.this_ptr);
		assert(obj);
		fn.result->set_double(obj->m_colour.m_g);
	}
	void	as_tunnel_set_green(const fn_call& fn)
	{
		as_tunnel* obj = cast_to<as_tunnel>(fn.this_ptr);
		if (obj && fn.nargs >= 1)
		{
			obj->m_colour.m_g = (Uint8) fn.arg(0).to_float();
		}
	}

	void	as_tunnel_get_hue(const fn_call& fn)
	{
		as_tunnel* obj = cast_to<as_tunnel>(fn.this_ptr);
		assert(obj);
		fn.result->set_double(obj->m_hue.m_r);
	}
	void	as_tunnel_set_hue(const fn_call& fn)
	{
		as_tunnel* obj = cast_to<as_tunnel>(fn.this_ptr);
		if (obj && fn.nargs >= 1)
		{
			obj->m_hue.m_r = (Uint8) fn.arg(0).to_float();
		}
	}

	void	as_tunnel_get_saturation(const fn_call& fn)
	{
		as_tunnel* obj = cast_to<as_tunnel>(fn.this_ptr);
		assert(obj);
		fn.result->set_double(obj->m_hue.m_g);
	}
	void	as_tunnel_set_saturation(const fn_call& fn)
	{
		as_tunnel* obj = cast_to<as_tunnel>(fn.this_ptr);
		if (obj && fn.nargs >= 1)
		{
			obj->m_hue.m_g = (Uint8) fn.arg(0).to_float();
		}
	}
	void	as_tunnel_get_brightness(const fn_call& fn)
	{
		as_tunnel* obj = cast_to<as_tunnel>(fn.this_ptr);
		assert(obj);
		fn.result->set_double(obj->m_hue.m_b);
	}
	void	as_tunnel_set_brightness(const fn_call& fn)
	{
		as_tunnel* obj = cast_to<as_tunnel>(fn.this_ptr);
		if (obj && fn.nargs >= 1)
		{
			obj->m_hue.m_b = (Uint8) fn.arg(0).to_float();
		}
	}


	void	as_tunnel_get_blue(const fn_call& fn)
	{
		as_tunnel* obj = cast_to<as_tunnel>(fn.this_ptr);
		assert(obj);
		fn.result->set_double(obj->m_colour.m_b);
	}
	void	as_tunnel_set_blue(const fn_call& fn)
	{
		as_tunnel* obj = cast_to<as_tunnel>(fn.this_ptr);
		if (obj && fn.nargs >= 1)
		{
			obj->m_colour.m_b = (Uint8) fn.arg(0).to_float();
		}
	}

	as_tunnel::as_tunnel(sprite_instance* parent) :
		character(parent, 0),
		m_texture(0),
		m_speed(1),
		m_diameter(0),
		m_sharp(50),
		m_smooth(0)
	{
		builtin_member("bitmapData", as_value(as_tunnel_get_bitmapdata, as_tunnel_set_bitmapdata));
		builtin_member("speed", as_value(as_tunnel_get_speed, as_tunnel_set_speed));
		builtin_member("diameter", as_value(as_tunnel_get_diameter, as_tunnel_set_diameter));
		builtin_member("red", as_value(as_tunnel_get_red, as_tunnel_set_red));
		builtin_member("green", as_value(as_tunnel_get_green, as_tunnel_set_green));
		builtin_member("blue", as_value(as_tunnel_get_blue, as_tunnel_set_blue));
		builtin_member("hue", as_value(as_tunnel_get_hue, as_tunnel_set_hue));
		builtin_member("saturation", as_value(as_tunnel_get_saturation, as_tunnel_set_saturation));
		builtin_member("brightness", as_value(as_tunnel_get_brightness, as_tunnel_set_brightness));
		builtin_member("sharp", as_value(as_tunnel_get_sharp, as_tunnel_set_sharp));
		builtin_member("smooth", as_value(as_tunnel_get_smooth, as_tunnel_set_smooth));
		
		m_colour.m_a = 100;
		m_colour.m_r = 100;
		m_colour.m_g = 100;
		m_colour.m_b = 100;

		m_hue.m_a = 100;
		m_hue.m_r = 100;
		m_hue.m_g = 100;
		m_hue.m_b = 100;

		int toruses = 36;
		int sectors = 40;
		float torus_radius = 3.5f;
		float cross_section_radius = 0.75f;
		makeTorus(toruses, sectors, torus_radius, cross_section_radius);

		cxform cx;
		matrix m;
		int depth = parent->get_highest_depth() + 1;
		m_def = new as_tunnel_character_def();
		parent->m_display_list.add_display_object(this, depth, true, cx, m, 0.0f, 0, 0); 

		const char *vs =
		"varying vec2 texCoord;\n"
		"void main()\n"
		"{ \n"
		"	gl_Position =  gl_ModelViewProjectionMatrix * gl_Vertex; \n"
		"	texCoord = gl_MultiTexCoord0.xy;\n"
		"} \n";

		const char *fs =
			"varying vec2 texCoord;	\n"
			"uniform sampler2D tex0;	\n"
			"uniform vec3 mycolor;	\n"
			"uniform vec3 myhue;	\n"

			"vec3 rgb2hsv(vec3 c)\n"
			"{\n"
    			"vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);\n"
			"    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));\n"
			"    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));\n"
			"    float d = q.x - min(q.w, q.y);\n"
			"    float e = 1.0e-10;\n"
			"    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);\n"
			"}\n"

			"vec3 hsv2rgb(vec3 c)\n"
			"{\n"
			"    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);\n"
			"    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);\n"
			"    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);\n"
			"}\n"

			"void main(void)	\n"
			"{  \n"
			"		vec4 textureColor = texture2D(tex0, texCoord); \n"
			"    vec3 fragRGB = textureColor.rgb; \n"
			"    fragRGB.r *=	mycolor.r;	\n"
			"    fragRGB.g *= mycolor.g;	\n"
			"    fragRGB.b *= mycolor.b;	\n"

			"    vec3 fragHSV = rgb2hsv(fragRGB).xyz; \n"
			"    fragHSV.x *= myhue.r; \n"
			"    fragHSV.y *= myhue.g; // saturate; \n"
			"    fragHSV.z *= myhue.b; //brightness; \n"
			"    fragRGB = hsv2rgb(fragHSV); \n"
			"    vec4 xx = vec4(fragRGB, textureColor.w); \n"

//			"    vec2 screenDim_; screenDim_.x=800.0; screenDim_.y=800.0; "
//			"    float circleRadius_ = 0.02; "
//			"    vec2 midpoint = screenDim_.xy * 0.5; midpoint.x += 22.0; \n"
//			"    float radius = min(screenDim_.x, screenDim_.y) * circleRadius_; \n"
//			"    float dist = length(gl_FragCoord.xy - midpoint); \n"
//			"    float circle = smoothstep(radius - 40.0, radius + 40.0, dist); \n"
//			"    xx.a = circle; \n"

			"    gl_FragColor = xx; \n"
			"}\n";
			
		m_colour_shader.compile(vs, fs);

		const char *fs_sharpen =
			" uniform sampler2D tex0; \n"
		//	" uniform vec2 offset[9]; \n"
			" uniform float kernel[9]; \n"
			" void main() \n"
			" { \n"
			" vec4 sum = vec4(0.0); \n"
			" int i; \n"
			" for (i = 0; i < 9; i++) { \n"
//			" vec4 color = texture2D(tex0, gl_TexCoord[0].st + offset[i]); \n"
			" vec4 color = texture2D(tex0, gl_TexCoord[0].st); \n"
			" sum += color * kernel[i]; \n"
			" } \n"
			" gl_FragColor = sum; \n"
			" } \n";
		m_sharpen_shader.compile(NULL, fs_sharpen);
	}

	as_tunnel::~as_tunnel()
	{
		if (m_texture > 0)
		{
			glDeleteTextures(1, &m_texture);
		}
	}

	void as_tunnel::read_params()
	{
		tu_string path = get_workdir();
		path += "voxel.conf";
		FILE* fi = fopen(path.c_str(), "rb");
		if (fi)
		{
			//read line by line
			char line[1024];
			while (fgets(line, sizeof(line), fi) != NULL)  
			{
				//printf(line);
				tu_string s(line);
				array<tu_string> a;
				s.split('=', &a);
				if (a.size() == 2)
				{
					float val = (float) atof(a[1].c_str());
					val = fclamp(val, 0, 100);
					if (a[0] == "speed")
					{
						m_speed = val;
					}
					else
					if (a[0] == "diameter")
					{
						m_diameter = val;
					}
					else
					if (a[0] == "red")
					{
						m_colour.m_r = (Uint8) val;
					}
					else
					if (a[0] == "green")
					{
						m_colour.m_g = (Uint8) val;
					}
					else
					if (a[0] == "blue")
					{
						m_colour.m_b = (Uint8) val;
					}
					else
					if (a[0] == "alpha")
					{
						m_colour.m_a = (Uint8) val;
					}
					else
					if (a[0] == "hue")
					{
						m_hue.m_r = (Uint8) val;
					}
					else
					if (a[0] == "saturation")
					{
						m_hue.m_g = (Uint8) val;
					}
					else
					if (a[0] == "brightness")
					{
						m_hue.m_b = (Uint8) val;
					}
					else
					if (a[0] == "sharp")
					{
						m_sharp = val;
					}
					else
					if (a[0] == "smooth")
					{
						m_smooth = val;
					}
					else
					{
						printf("invalid parameter: %s\n", a[0].c_str());
					}
				}
			}
			fclose(fi);
		}
	}

	bool as_tunnel::get_member(const tu_string& name, as_value* val)
	{
		return character::get_member(name, val);
	}

	bool as_tunnel::set_member(const tu_string& name, const as_value& val)
	{
		return character::set_member(name, val);
	}

	void as_tunnel::save_ogl()
	{
		// save GL state
		glPushAttrib (GL_ALL_ATTRIB_BITS);	
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
	}

	void as_tunnel::restore_ogl()
	// restore openGL state
	{
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glPopAttrib();
	}

	void as_tunnel::makeTorus(int toruses, int sectors, float radius, float cs_radius)
		//This code is written keeping OpenGLES in mind too.. so not using unsupported features
	{
		assert(360.0f / toruses == int(360 / toruses));
		assert(360.0f / sectors == int(360 / sectors));

		float degree_to_radian = (float) M_PI / 180.0f;

		m_vertices.resize(toruses * sectors);
		m_texels.resize(toruses * sectors);

		//calculating the vertex array
		int cs_angleincs = 0;
		float currentradius, zval;
		int k = 0;
		int m = 0;
		for (int j = 0; j < sectors; j++)
		{
			currentradius = radius + (cs_radius * cosf(cs_angleincs * degree_to_radian));
			zval = cs_radius * sinf(cs_angleincs * degree_to_radian);

			int n = 0;
			int angleincs = 0;
			for (int i = 0; i < toruses; i++)
			{
				m_vertices[k].x = currentradius * cosf(angleincs * degree_to_radian);
				m_vertices[k].y = currentradius * sinf(angleincs * degree_to_radian);
				m_vertices[k].z = zval;

				m_texels[k].u = i * 1.0f / (toruses - 1);	//n / 2.0f;
				m_texels[k].v = j * 1.0f / (sectors - 1); // m / 45.0f;

				k++;
				n++;
				angleincs += 360 / toruses;
			}
			m++;
			cs_angleincs += 360 / sectors;

		}

		//calculating the index array
		k = 0;
		m_indices.resize(sectors * 2 * (toruses + 1));
		for (int i = 0; i < sectors; i++)
		{
			for (int j = 0; j < toruses; j++)
			{
				m_indices[k++] = i * toruses + j;
				m_indices[k++] = ((i + 1) % sectors) * toruses + j;
			}

			m_indices[k++] = i * toruses;
			m_indices[k++] = ((i + 1) % sectors) * toruses;
//			m_indices.push_back(((i + 1) % sectors) * sides);
		}
	}
	
	void	as_tunnel::display()
	{
		read_params();

		save_ogl();

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		// Первый параметр это охват в градусах от 0 до 180. 
		// Второй параметр это угол поворота по оси Y. 
		gluPerspective(100.0f, 1.0f, 0.01f, 100.0f);

		//	//comment when camera inside
		//	glLoadIdentity();
		//	glOrthof(-6, 6, -9, 9, -100, 100);

		// apply bakeinflash matrix
		bakeinflash::rect bound;
		get_parent()->get_bound(&bound);

//		matrix m;
//		get_parent()->get_world_matrix(&m);
//		m.transform(&bound);

		// get viewport size
		GLint vp[4]; 
		glGetIntegerv(GL_VIEWPORT, vp); 
		int vp_width = vp[2];
		int vp_height = vp[3];

//		bound.twips_to_pixels();
//		int w = (int) (bound.m_x_max - bound.m_x_min);
//		int h = (int) (bound.m_y_max - bound.m_y_min);
//		int x = (int) bound.m_x_min;
//		int y = (int) bound.m_y_min;

		glViewport(0, 0, vp_width, vp_height);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		float scale = 0.5f * (m_diameter + 1) / 10.0f;
		glScalef(scale, scale, 1.0);

//		glTranslatef(m_diameter/100, m_diameter/100, 0.0);

		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);
		if (m_texture == 0 && m_bitmapdata != NULL)
		{
			int width = m_bitmapdata->get_width();
			int height = m_bitmapdata->get_height();
			int p2w = p2(width);
			int p2h = p2(height);

			glGenTextures(1, &m_texture);
			glBindTexture(GL_TEXTURE_2D, m_texture);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, p2w, p2h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, m_bitmapdata->get_data());
		}
		glBindTexture(GL_TEXTURE_2D, m_texture);

/*		if (m_bitmapdata != NULL)
		{
			int width = m_bitmapdata->get_width();
			int height = m_bitmapdata->get_height();
			Uint8* b = m_bitmapdata->get_data();

			int k = tu_random::next_random() % (width * height*4);
			b[k] = tu_random::next_random() % 255;
			b[k+1] = tu_random::next_random() % 255;
			b[k+2] = tu_random::next_random() % 255;

			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, m_bitmapdata->get_data());
		}
		*/

		glEnable(GL_BLEND);
		glColor4f(m_colour.m_r / 100.0f, m_colour.m_g / 100.0f, m_colour.m_b / 100.0f, 1.0f); //color.m_a / 255.0f);
		glPointSize(2.0f);

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_DEPTH_TEST);

		if (m_colour.m_r < 100 || m_colour.m_g < 100 || m_colour.m_b < 100 || m_hue.m_r < 100 || m_hue.m_g < 100 || m_hue.m_b < 100)
		{
			m_colour_shader.attach();
			m_colour_shader.set_uniform("mycolor", fclamp(m_colour.m_r / 100.0f, 0, 1), fclamp(m_colour.m_g / 100.0f, 0, 1), fclamp(m_colour.m_b / 100.0f, 0, 1));
			m_colour_shader.set_uniform("myhue", fclamp(m_hue.m_r / 100.0f, 0, 1), fclamp(m_hue.m_g / 100.0f, 0, 1), fclamp(m_hue.m_g / 100.0f, 0, 1));
		}

		if (m_sharp != 50)
		{
			m_sharpen_shader.attach();
			float kernel[9] = {0, -1, 0, -1, 5 + (m_sharp - 50) / 100.0f, -1, 0, -1, 0};
	//vv		m_sharpen_shader.set_uniformv("kernel", TU_ARRAYSIZE(kernel), kernel);
		}


		glClear(GL_DEPTH_BUFFER_BIT);

		if (m_smooth < 50)
		{
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 1);
		}
		else
		{
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
		}

		float degree_to_radian = (float) M_PI / 180.0f;
		static float	counter = 0;
		counter += degree_to_radian / 2.0f;
		counter += m_speed / 1000.0f;		// adjustment

		if (counter > 360)
		{
			counter = 0;
		}

		//glTranslatef(0.0f, 0.0f, -6.0f);
		xyz eye_coords;		// откуда смортим
		eye_coords.x = -3.5f; eye_coords.y = 0; eye_coords.z = 0;
		xyz scene_center_coords;  // куда смотрим
		scene_center_coords.x = -3.5f; scene_center_coords.y = 1; scene_center_coords.z = 0;
		xyz view_vector;
		view_vector.x = sinf(counter * degree_to_radian); view_vector.y = 0; view_vector.z = cosf(counter * degree_to_radian);
		gluLookAt(eye_coords.x, eye_coords.y, eye_coords.z, scene_center_coords.x, scene_center_coords.y, scene_center_coords.z, view_vector.x, view_vector.y, view_vector.z);

		glRotatef(counter*2, 0.0f, 0.0f, 1.0f);
		glVertexPointer(3, GL_FLOAT, 0, &m_vertices.front());
		glTexCoordPointer(2, GL_FLOAT, 0, &m_texels.front());

		glDrawElements(GL_TRIANGLE_STRIP, m_indices.size(), GL_UNSIGNED_SHORT, &m_indices.front());	
		//glDrawArrays(GL_TRIANGLE_STRIP, 0, m_vertices.size());

		/*

		int circles = 36;
		int sectors = 40;

		int k=0;
		float r,g,b,a;
		r=g=b=a= 0.0f;
		r=1;
		a=1;

		// sectors = сепкторов в круге
		for (int j = 0; j<sectors;j++)
		{
			for (int i = 0; i<circles;i++)
			{
				int k1 = j * circles + i;
				int k2 = (i < circles - 1) ? k1 + 1 : j * circles;
				int kk1 = (k1+circles) % m_vertices.size();
				int kk2 = (k2+circles) % m_vertices.size();

			//	glColor4f(r,g,b,a);
				r +=0.1; if (r>1) r=0.1;
				g +=0.1; if (g>1) g=0.1;
				b +=0.1; if (b>1) b=0.1;
				glBegin(GL_QUADS);
				glTexCoord2f(m_texels[k1].u,m_texels[k1].v);
				glVertex3f( m_vertices[k1].x, m_vertices[k1].y, m_vertices[k1].z);
				glTexCoord2f(m_texels[k1].u,m_texels[k1].v);
				glVertex3f( m_vertices[k2].x, m_vertices[k2].y, m_vertices[k2].z);
				glTexCoord2f(m_texels[k1].u,m_texels[k1].v);
				glVertex3f( m_vertices[kk2].x, m_vertices[kk2].y, m_vertices[kk2].z);
				glTexCoord2f(m_texels[k1].u,m_texels[k1].v);
				glVertex3f( m_vertices[kk1].x, m_vertices[kk1].y, m_vertices[kk1].z);
				glEnd();

				k++;
			}
		}
		*/

		m_sharpen_shader.detach();
		m_colour_shader.detach();

		restore_ogl();
	}


} // end of namespace bakeinflash
