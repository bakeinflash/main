// bakeinflash_render_handler_ogles.cpp	-- Willem Kokke <willem@mindparity.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// OpenGL ES based video handler for mobile units

#include <string.h>	// for memset()

#include <DirectXHelper.h>
#include <directxmath.h>
#include <D3D11_1.h>
#include <agile.h>

#include "base/tu_config.h"
#include "bakeinflash/bakeinflash.h"
#include "bakeinflash/bakeinflash_types.h"
#include "bakeinflash/bakeinflash_log.h"
#include "base/image.h"
#include "base/utility.h"
#include "base/png_helper.h"
#include "renderer.h"
#include "bakeinflash.h"

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;

// Cached renderer properties.
extern Windows::Foundation::Rect m_windowBounds;

// Direct3D Objects.
extern Microsoft::WRL::ComPtr<ID3D11Device1> m_d3dDevice;
extern Microsoft::WRL::ComPtr<ID3D11DeviceContext1> m_d3dContext;
extern Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;

extern Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
extern Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
extern Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
extern Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShaderTex;
//extern Microsoft::WRL::ComPtr<ID3D11Buffer> m_constantBuffer;

extern	int s_viewport_width;
extern	int s_viewport_height;

//#define DEBUG_BITMAPS

namespace render_handler_d3d
{

	HRESULT create_texture(DXGI_FORMAT format, int w, int h, void* data, int bpp, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv)
	{
		// for profiling texture memory
		//static float s_texture_memory = 0;
		//s_texture_memory += w * h* 4;
		//myprintf("mem=%dM, w=%d, h=%d\n", int(s_texture_memory/1024.0f/1024.0f), w, h);

		D3D11_TEXTURE2D_DESC sTexDesc;
		sTexDesc.Width = w;
		sTexDesc.Height = h;
		sTexDesc.MipLevels = 1;
		sTexDesc.ArraySize = 1;
		sTexDesc.Format = format;
		sTexDesc.SampleDesc.Count = 1;
		sTexDesc.SampleDesc.Quality = 0;
		sTexDesc.Usage = D3D11_USAGE_IMMUTABLE;
		sTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		sTexDesc.CPUAccessFlags = 0;
		sTexDesc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA sSubData;
		sSubData.pSysMem = data;
		sSubData.SysMemPitch = (UINT) (w * bpp);	// rowPitch 
		sSubData.SysMemSlicePitch = (UINT) (w * h * bpp);	// imageSize 

		ID3D11Texture2D* pTexture = NULL;
		HRESULT rc = m_d3dDevice->CreateTexture2D(&sTexDesc, &sSubData, &pTexture);
		if (pTexture)
		{
			rc = m_d3dDevice->CreateShaderResourceView(pTexture, NULL, &srv);
			pTexture->Release();
		}
		return rc;
	}

	// bitmap_info_ogl declaration
	struct bitmap_info_ogl : public bakeinflash::bitmap_info
	{
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	m_texture_id;
		int m_width;
		int m_height;
		image::image_base* m_suspended_image;

		bitmap_info_ogl();
		bitmap_info_ogl(image::image_base* im);

		virtual void upload();

		virtual void save(const char* filename)
		{
			// bitmap grubber
#if TU_CONFIG_LINK_TO_LIBPNG
			if (get_data())
			{
				png_helper::write_rgba(filename, get_data(), m_width, m_height, get_bpp());
			}
#endif
		}

		// get byte per pixel
		virtual int get_bpp() const
		{
			if (m_suspended_image)
			{
				switch (m_suspended_image->m_type)
				{
					default: return 0;
					case image::image_base::RGB: return 3;
					case image::image_base::RGBA: return 4;
					case image::image_base::ALPHA: return 1;
				};
			}
			return 0;
		}

		virtual unsigned char* get_data() const
		{
			if (m_suspended_image)
			{
				return m_suspended_image->m_data;
			}
			return NULL;
		}

		~bitmap_info_ogl()
		{
#ifdef DEBUG_BITMAPS
			myprintf("delete bitmap tex=%d, %dx%d\n", m_texture_id, get_width(), get_height());
#endif
			if (m_texture_id != NULL)
			{
				m_d3dContext->DiscardView(m_texture_id.Get());
				m_texture_id = nullptr;
			}
			delete m_suspended_image;
		}

		virtual int get_width() const { return m_width; }
		virtual int get_height() const { return m_height; }

	};

	struct video_handler_ogles : public bakeinflash::video_handler
	{
		int m_texture;
		float m_scoord;
		float m_tcoord;
		bakeinflash::rgba m_background_color;

		video_handler_ogles():
			m_texture(0),
			m_scoord(0),
			m_tcoord(0),
			m_background_color(0,0,0,0)	// current background color
		{
		}

		~video_handler_ogles()
		{
			//	glDeleteTextures(1, &m_texture);
		}

		void display(Uint8* data, int width, int height, 
			const bakeinflash::matrix* m, const bakeinflash::rect* bounds, const bakeinflash::rgba& color)
		{

			// this can't be placed in constructor becuase opengl may not be accessible yet
			if (m_texture == 0)
			{
				/*		glEnable(GL_TEXTURE_2D);
				glGenTextures(1, &m_texture);
				glBindTexture(GL_TEXTURE_2D, m_texture);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	// GL_NEAREST ?
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);*/
			}

			//		glBindTexture(GL_TEXTURE_2D, m_texture);
			//	glEnable(GL_TEXTURE_2D);


			//		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			//		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

			// update texture from video frame
			if (data)
			{
				int w2p = p2(width);
				int h2p = p2(height);
				m_scoord = (float) width / w2p;
				m_tcoord = (float) height / h2p;

				if (m_clear_background)
				{
					// set video background color
					// assume left-top pixel of the first frame as background color
					if (m_background_color.m_a == 0)
					{
						m_background_color.m_a = 255;
						m_background_color.m_r = data[2];
						m_background_color.m_g = data[1];
						m_background_color.m_b = data[0];
					}

					// clear video background, input data has BGRA format
					Uint8* p = data;
					for (int y = 0; y < height; y++)
					{
						for (int x = 0; x < width; x++)
						{
							// calculate color distance, dist is in [0..195075]
							int r = m_background_color.m_r - p[2];
							int g = m_background_color.m_g - p[1];
							int b = m_background_color.m_b - p[0];
							float dist = (float) (r * r + g * g + b * b);

							static int s_min_dist = 3 * 64 * 64;	// hack
							Uint8 a = (dist < s_min_dist) ? (Uint8) (255 * (dist / s_min_dist)) : 255;

							p[3] = a;		// set alpha
							p += 4;
						}
					}
				}

				// don't use compressed texture for video, it slows down video
				//			ogl::create_texture(GL_RGBA, m_width2p, m_height2p, NULL);
				//		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w2p, h2p, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
				//		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
			}

			if (m_scoord == 0.0f && m_scoord == 0.0f)
			{
				// no data
				return;
			}

			bakeinflash::point a, b, c, d;
			m->transform(&a, bakeinflash::point(bounds->m_x_min, bounds->m_y_min));
			m->transform(&b, bakeinflash::point(bounds->m_x_max, bounds->m_y_min));
			m->transform(&c, bakeinflash::point(bounds->m_x_min, bounds->m_y_max));
			d.m_x = b.m_x + c.m_x - a.m_x;
			d.m_y = b.m_y + c.m_y - a.m_y;

			//		glColor4f(color.m_r / 255.0, color.m_g / 255.0, color.m_b / 255.0, color.m_a / 255.0);

			// this code is equal to code that above

			float squareVertices[8]; 
			squareVertices[0] = a.m_x;
			squareVertices[1] = a.m_y;
			squareVertices[2] = b.m_x;
			squareVertices[3] = b.m_y;
			squareVertices[4] = c.m_x;
			squareVertices[5] = c.m_y;
			squareVertices[6] = d.m_x;
			squareVertices[7] = d.m_y;

			float squareTextureCoords[8];
			squareTextureCoords[0] = 0;
			squareTextureCoords[1] = 0;
			squareTextureCoords[2] = m_scoord;
			squareTextureCoords[3] = 0;
			squareTextureCoords[4] = 0;
			squareTextureCoords[5] = m_tcoord;
			squareTextureCoords[6] = m_scoord;
			squareTextureCoords[7] = m_tcoord;

			/*	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2, GL_FLOAT, 0, squareTextureCoords);

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(2, GL_FLOAT, 0, squareVertices);

			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			glDisableClientState(GL_VERTEX_ARRAY);

			glDisable(GL_TEXTURE_2D);*/
		}

	};

	struct render_handler_d3d : public bakeinflash::render_handler
	{
		// Some renderer state.

		// Enable/disable antialiasing.
		bool	m_enable_antialias;

		// Output size.
		float	m_display_width;
		float	m_display_height;

		bakeinflash::matrix	m_current_matrix;
		bakeinflash::cxform	m_current_cxform;
		bakeinflash::rgba*	m_current_rgba;

		Uint32 m_mask_level;	// nested mask level

		// DXD11
		Microsoft::WRL::ComPtr<ID3D11SamplerState> m_texWrapMode;
		Microsoft::WRL::ComPtr<ID3D11Buffer> s_vertex_buffer;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depthStencilView;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState> stencilState1;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState> stencilState2;

		const int m_maxVertexCount;
		float shift_x;
		float shift_y;
		float scale_x;
		float scale_y;

		render_handler_d3d() :
			m_enable_antialias(false),
			m_display_width(0),
			m_display_height(0),
			m_mask_level(0),
			m_current_rgba(NULL),
			m_maxVertexCount(500),
			shift_x(0),
			shift_y(0),
			scale_x(0),
			scale_y(0)
		{

			// Create the texture sampler state.

			D3D11_SAMPLER_DESC samplerDesc;
			ZeroMemory(&samplerDesc, sizeof(D3D11_SAMPLER_DESC));
			samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
			samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
			samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
			samplerDesc.MipLODBias = 0.0f;
			samplerDesc.MaxAnisotropy = 1;
			samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
			samplerDesc.BorderColor[0] = 0;
			samplerDesc.BorderColor[1] = 0;
			samplerDesc.BorderColor[2] = 0;
			samplerDesc.BorderColor[3] = 0;
			samplerDesc.MinLOD = 0;
			samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
			m_d3dDevice->CreateSamplerState(&samplerDesc, &m_texWrapMode);

			// create vertex buffer

			CD3D11_BUFFER_DESC vertexBufferDescription;
			vertexBufferDescription.Usage               = D3D11_USAGE_DYNAMIC;
			vertexBufferDescription.BindFlags           = D3D11_BIND_VERTEX_BUFFER;
			vertexBufferDescription.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
			vertexBufferDescription.MiscFlags           = 0;
			vertexBufferDescription.StructureByteStride = 0;

			// This is run in the first frame. But what if new vertices are added to the scene?
			vertexBufferDescription.ByteWidth = sizeof(VertexPositionColor) * m_maxVertexCount; //stride * maxVertexCount;

			// used only to create the buffer 
			void* vertexbuf = new VertexPositionColor[m_maxVertexCount];

			D3D11_SUBRESOURCE_DATA resourceData;
			ZeroMemory(&resourceData, sizeof(resourceData));
			resourceData.pSysMem = vertexbuf;

			m_d3dDevice->CreateBuffer(&vertexBufferDescription, &resourceData, &s_vertex_buffer);
			assert(s_vertex_buffer);

			delete [] vertexbuf;

			//
			// Create the depth-stencil buffer using a texture resource.
			//
			
			ID3D11Texture2D* pDepthStencilTex = NULL;
			D3D11_TEXTURE2D_DESC descDepth;
			ZeroMemory(&descDepth, sizeof(D3D11_TEXTURE2D_DESC));
			descDepth.Width = (int) m_windowBounds.Width;
			descDepth.Height = (int) m_windowBounds.Height;
			descDepth.MipLevels = 1;
			descDepth.ArraySize = 1;
			descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			descDepth.SampleDesc.Count = 1;
			descDepth.SampleDesc.Quality = 0;
			descDepth.Usage = D3D11_USAGE_DEFAULT;
			descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			descDepth.CPUAccessFlags = 0;
			descDepth.MiscFlags = 0;
			m_d3dDevice->CreateTexture2D( &descDepth, NULL, &pDepthStencilTex );
			
			D3D11_DEPTH_STENCIL_VIEW_DESC ddesc;
			ZeroMemory(&ddesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
			ddesc.Format = descDepth.Format;
			ddesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			ddesc.Texture2D.MipSlice = 0;

			// Create the depth stencil view
			m_d3dDevice->CreateDepthStencilView(pDepthStencilTex, &ddesc, &m_depthStencilView );


			// Create Depth-Stencil State

			D3D11_DEPTH_STENCIL_DESC dsDesc;
			ZeroMemory(&dsDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));

			// Depth test parameters #1
			dsDesc.DepthEnable = true;
			dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			dsDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;

			// Stencil test parameters
			dsDesc.StencilEnable = true;
			dsDesc.StencilReadMask = 0xFF;
			dsDesc.StencilWriteMask = 0xFF;

			// Stencil operations if pixel is front-facing
			// set the stencil buffer to 'm_mask_level+1' 
			// where we draw any polygon and stencil buffer is 'm_mask_level'
			dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
			dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
			dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;

			// Stencil operations if pixel is back-facing
			// Since we do not care about back-facing pixels, always keep original value.
			dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
			dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_NEVER;

			// Create depth stencil state for begin_submit
			m_d3dDevice->CreateDepthStencilState(&dsDesc, &stencilState1);

			// Depth test parameters #2
			dsDesc.DepthEnable = true;
			dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			dsDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;

			// Stencil test parameters
			dsDesc.StencilEnable = true;
			dsDesc.StencilReadMask = 0xFF;
			dsDesc.StencilWriteMask = 0xFF;

			// Stencil operations if pixel is front-facing
			// draw only where the stencil is m_mask_level (where the current mask was drawn)
			dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
			dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL; //D3D11_COMPARISON_ALWAYS;

			// Stencil operations if pixel is back-facing
			// Since we do not care about back-facing pixels, always keep original value.
			dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
			dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_NEVER;

			// Create depth stencil state, for end_submit
			m_d3dDevice->CreateDepthStencilState(&dsDesc, &stencilState2);
		}

		~render_handler_d3d()
		{
			m_texWrapMode = nullptr;
			s_vertex_buffer = nullptr;

			m_d3dContext->DiscardView(m_depthStencilView.Get());
			m_depthStencilView = nullptr;
		}

		// This is where it copies the new vertices to the buffer.
		// but it's causing flickering in the entire screen...
		void write_to_buffer(VertexPositionColor* vertexbuf, int count)
		{
			assert(count < m_maxVertexCount);

			D3D11_MAPPED_SUBRESOURCE resource;
			m_d3dContext->Map(s_vertex_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
			memcpy(resource.pData, vertexbuf, sizeof(VertexPositionColor) * count); 
			m_d3dContext->Unmap(s_vertex_buffer.Get(), 0);

			UINT stride = sizeof(VertexPositionColor);
			UINT offset = 0;
			m_d3dContext->IASetVertexBuffers(0, 1, s_vertex_buffer.GetAddressOf(),	&stride, &offset);
		}

		void open()
		{
			// Scan for extensions used by bakeinflash
		}

		void set_antialiased(bool enable)
		{
			m_enable_antialias = enable;
		}

		struct fill_style
		{
			enum mode
			{
				INVALID,
				COLOR,
				BITMAP_WRAP,
				BITMAP_CLAMP,
				LINEAR_GRADIENT,
				RADIAL_GRADIENT,
			};
			mode	m_mode;
			mutable float	m_color[4];
			bakeinflash::bitmap_info*	m_bitmap_info;
			bakeinflash::matrix	m_bitmap_matrix;
			float m_width;	// for line style
			int m_caps_style;
			mutable float	pS[4];
			mutable float	pT[4];

			fill_style() :
				m_mode(INVALID)
			{
			}

			const float* get_color() const
			{
				return m_color;
			}

			ID3D11ShaderResourceView* get_tex() const
			{
				ID3D11ShaderResourceView* tex = NULL;
				if (m_mode == BITMAP_WRAP || m_mode == BITMAP_CLAMP)
				{
					apply();
					bitmap_info_ogl* b = (bitmap_info_ogl*) m_bitmap_info;	// hack
					return b->m_texture_id.Get();
				}
				return NULL;
			}

			float* gentexcoords(int primitive_type, const void* coords, int vertex_count) const
			{
				float* tcoord = NULL;
				if (m_mode == BITMAP_WRAP || m_mode == BITMAP_CLAMP)
				{
					tcoord = new float[2 * vertex_count];
					float* vcoord = (float*) coords;
					for (int i = 0, n = 2 * vertex_count; i < n; i = i + 2)
					{
						tcoord[i] = vcoord[i] * pS[0] + vcoord[i + 1] * pS[1] + pS[3];
						tcoord[i + 1] = vcoord[i] * pT[0] + vcoord[i + 1] * pT[1] + pT[3];
					}
				}
				return tcoord;
			}

			void	apply(/*const matrix& current_matrix*/) const
				// Push our style into OpenGL.
			{
				assert(m_mode != INVALID);

				if (m_mode == COLOR)
				{
					//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					apply_color(m_color);
					//	glDisable(GL_TEXTURE_2D);
				}
				else
					if (m_mode == BITMAP_WRAP || m_mode == BITMAP_CLAMP)
					{
						assert(m_bitmap_info != NULL);
						if (m_bitmap_info == NULL)
						{
							//	glDisable(GL_TEXTURE_2D);
						}
						else
						{
							//	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
							apply_color(m_color);

							m_bitmap_info->upload();
							if (m_mode == BITMAP_CLAMP)
							{	
								//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
								//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
							}
							else
							{
								assert(m_mode == BITMAP_WRAP);
								//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
								//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
							}

							// Set up the bitmap matrix for texgen.

							int	w = p2(m_bitmap_info->get_width());
							int	h = p2(m_bitmap_info->get_height());
							float	inv_width = 1.0f / w;
							float	inv_height = 1.0f / h;

							const bakeinflash::matrix&	m = m_bitmap_matrix;

							pS[0] = m.m_[0][0] * inv_width;
							pS[1] = m.m_[0][1] * inv_width;
							pS[2] = 0;
							pS[3] = m.m_[0][2] * inv_width;

							pT[0] = m.m_[1][0] * inv_height;
							pT[1] = m.m_[1][1] * inv_height;
							pT[2] = 0;
							pT[3] = m.m_[1][2] * inv_height;
						}
					}
			}

			void	disable() { m_mode = INVALID; }

			void	set_color(bakeinflash::rgba color)
			{
				m_mode = COLOR; 
				m_color[0] = color.m_r / 255.0f; 
				m_color[1] = color.m_g / 255.0f; 
				m_color[2] = color.m_b / 255.0f; 
				m_color[3] = color.m_a / 255.0f; 
			}

			void	set_bitmap(bakeinflash::bitmap_info* bi, const bakeinflash::matrix& m, bitmap_wrap_mode wm, const bakeinflash::cxform& color_transform)
			{
				m_mode = (wm == WRAP_REPEAT) ? BITMAP_WRAP : BITMAP_CLAMP;
				m_bitmap_info = bi;
				m_bitmap_matrix = m;

				m_color[0] = color_transform.m_[0][0];
				m_color[1] = color_transform.m_[1][0];
				m_color[2] = color_transform.m_[2][0];
				m_color[3] = color_transform.m_[3][0];

				//
				// корректировать цета
				//
				m_color[0] += color_transform.m_[0][1] / 255.0f;
				m_color[1] += color_transform.m_[1][1] / 255.0f;
				m_color[2] += color_transform.m_[2][1] / 255.0f;
				m_color[3] += color_transform.m_[3][1] / 255.0f;

				// так как используется GL_ONE надо вычислить компоненты цвета заранее применив ALFA
				m_color[0] *= m_color[3]; 
				m_color[1] *= m_color[3]; 
				m_color[2] *= m_color[3]; 
			}

			bool	is_valid() const { return m_mode != INVALID; }
		};


		// Style state.
		enum style_index
		{
			LEFT_STYLE = 0,
			RIGHT_STYLE,
			LINE_STYLE,

			STYLE_COUNT
		};
		fill_style	m_current_styles[STYLE_COUNT];


		bakeinflash::bitmap_info*	create_bitmap_info(image::image_base* im)
			// Given an image, returns a pointer to a bitmap_info struct
			// that can later be passed to fill_styleX_bitmap(), to set a
			// bitmap fill style.
		{
			if (im)
			{
				return new bitmap_info_ogl(im);
			}
			return new bitmap_info_ogl();
		}

		bakeinflash::video_handler*	create_video_handler()
		{
			return new video_handler_ogles();
		}

		void	begin_display(
			bakeinflash::rgba background_color,
			int viewport_x0, int viewport_y0,
			int viewport_width, int viewport_height,
			float x0, float x1, float y0, float y1)
			// Set up to render a full frame from a movie and fills the
			// background.	Sets up necessary transforms, to scale the
			// movie to fit within the given dimensions.  Call
			// end_display() when you're done.
			//
			// The rectangle (viewport_x0, viewport_y0, viewport_x0 +
			// viewport_width, viewport_y0 + viewport_height) defines the
			// window coordinates taken up by the movie.
			//
			// The rectangle (x0, y0, x1, y1) defines the pixel
			// coordinates of the movie that correspond to the viewport
			// bounds.
		{
			m_display_width = fabsf(x1 - x0);
			m_display_height = fabsf(y1 - y0);


			// move to center

			float viewport_scale_x = m_windowBounds.Width / m_display_width * 20;
			float viewport_scale_y = m_windowBounds.Height / m_display_height * 20;
			float viewport_scale = fmin(viewport_scale_x, viewport_scale_y);
			shift_x = m_windowBounds.Width - viewport_scale * m_display_width / 20;
			shift_x = shift_x / m_windowBounds.Width;
			shift_y = m_windowBounds.Height - viewport_scale * m_display_height / 20;
			shift_y = shift_y / m_windowBounds.Height;
			scale_x = viewport_scale / 20 / (m_windowBounds.Width / 2);
			scale_y = viewport_scale / 20 / (m_windowBounds.Height / 2);

			float bg[] = {background_color.m_r / 255.0f, background_color.m_g / 255.0f, background_color.m_b / 255.0f, background_color.m_a / 255.0f};
			m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(),	bg);
			m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(),	D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

			ModelViewProjectionConstantBuffer m_constantBufferData;
			float xScale=1;
			float yScale=1;
			m_constantBufferData.projection = DirectX::XMFLOAT4X4(
				xScale, 0.0f,    0.0f,  0.0f,
				0.0f,   yScale,  0.0f,  0.0f,
				0.0f,   0.0f,   1.0f, 0.0f,
				0.0f,   0.0f,   0.0f,  1.0f
				);

			m_d3dContext->OMSetRenderTargets(1,	m_renderTargetView.GetAddressOf(), NULL);
			//m_d3dContext->UpdateSubresource(m_constantBuffer.Get(),	0, NULL, &m_constantBufferData,	0, 0);

			// create blend state
			static ID3D11BlendState* D3DBlendState = NULL;
			if (D3DBlendState==NULL)
			{
				D3D11_BLEND_DESC omDesc;
				ZeroMemory( &omDesc, sizeof( D3D11_BLEND_DESC ) );
				omDesc.AlphaToCoverageEnable = true;
				omDesc.IndependentBlendEnable = false;
				omDesc.RenderTarget[0].BlendEnable = true;
				omDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
				omDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
				omDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
				omDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
				omDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA; //D3D11_BLEND_ZERO;
				omDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
				omDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
				m_d3dDevice->CreateBlendState(&omDesc, &D3DBlendState);
			}
			m_d3dContext->OMSetBlendState(D3DBlendState, NULL, 0xFFFFFFFF);

		}

		void	end_display()
			// Clean up after rendering a frame.  Client program is still
			// responsible for calling glSwapBuffers() or whatever.
		{
		}


		void	set_matrix(const bakeinflash::matrix& m)
			// Set the current transform for mesh & line-strip rendering.
		{
			m_current_matrix = m;
		}


		void	set_cxform(const bakeinflash::cxform& cx)
			// Set the current color transform for mesh & line-strip rendering.
		{
			m_current_cxform = cx;
		}

		void	set_rgba(bakeinflash::rgba* color)
		{
			m_current_rgba = color;
		}


		static void	apply_matrix(const bakeinflash::matrix& m)
			// multiply current matrix with opengl matrix
		{
			float	mat[16];
			memset(&mat[0], 0, sizeof(mat));
			mat[0] = m.m_[0][0];
			mat[1] = m.m_[1][0];
			mat[4] = m.m_[0][1];
			mat[5] = m.m_[1][1];
			mat[10] = 1;
			mat[12] = m.m_[0][2];
			mat[13] = m.m_[1][2];
			mat[15] = 1;
			//	glMultMatrixf(mat);
		}

		static void	apply_color(float c[4])
			// Set the given color.
		{
			//	glColor4f(c[0], c[1], c[2], c[3]);
		}

		static void	apply_color(const bakeinflash::rgba& c)
			// Set the given color.
		{
			//		glColor4f(c.m_r / 255.0f, c.m_g / 255.0f, c.m_b / 255.0f, c.m_a / 255.0f);
			//	glColor4ub(c.m_r, c.m_g, c.m_b, c.m_a);
		}

		void	fill_style_disable(int fill_side)
			// Don't fill on the {0 == left, 1 == right} side of a path.
		{
			assert(fill_side >= 0 && fill_side < 2);
			m_current_styles[fill_side].disable();
		}


		void	line_style_disable()
			// Don't draw a line on this path.
		{
			m_current_styles[LINE_STYLE].disable();
		}


		void	fill_style_color(int fill_side, const bakeinflash::rgba& color)
			// Set fill style for the left interior of the shape.  If
			// enable is false, turn off fill for the left interior.
		{
			assert(fill_side >= 0 && fill_side < 2);

			m_current_styles[fill_side].set_color(m_current_cxform.transform(color));
		}


		void	line_style_color(bakeinflash::rgba color)
			// Set the line style of the shape.  If enable is false, turn
			// off lines for following curve segments.
		{
			m_current_styles[LINE_STYLE].set_color(m_current_cxform.transform(color));
		}


		void	fill_style_bitmap(int fill_side, bakeinflash::bitmap_info* bi, const bakeinflash::matrix& m,
			bitmap_wrap_mode wm, bitmap_blend_mode bm)
		{
			assert(fill_side >= 0 && fill_side < 2);
			m_current_styles[fill_side].set_bitmap(bi, m, wm, m_current_cxform);
		}

		void	line_style_width(float width)
		{
			m_current_styles[LINE_STYLE].m_width = width;
		}

		void	line_style_caps(int caps_style)
		{
			m_current_styles[LINE_STYLE].m_caps_style = caps_style;
		}


		void	draw_mesh_primitive(D3D_PRIMITIVE_TOPOLOGY primitive_type, const void* coords, int vertex_count)
			// Helper for draw_mesh_strip and draw_triangle_list.
		{
			ID3D11ShaderResourceView* tex =  m_current_styles[LEFT_STYLE].get_tex();
			float* tcoord = tex ? m_current_styles[LEFT_STYLE].gentexcoords(primitive_type, coords, vertex_count) : NULL;
			const float* color = m_current_styles[LEFT_STYLE].get_color();

			VertexPositionColor* vertexbuf = NULL;
			float* co = (float*) coords;
			vertexbuf = new VertexPositionColor[vertex_count];

			// create vertex array

			for (int i = 0; i < vertex_count; i++)
			{
				bakeinflash::point pt(co[i * 2 + 0], co[i * 2 + 1]);
				bakeinflash::point result;
				m_current_matrix.transform(&result, pt);

				vertexbuf[i].pos.x = result.m_x * scale_x - 1;
				vertexbuf[i].pos.x -= shift_x;
				vertexbuf[i].pos.y = - (result.m_y * scale_y - 1);	// MINUS means flip
				vertexbuf[i].pos.y -= shift_y;

				vertexbuf[i].color.x = color[0];
				vertexbuf[i].color.y = color[1];
				vertexbuf[i].color.z = color[2];
				vertexbuf[i].color.w = color[3];
				vertexbuf[i].texcoord.x = tcoord ? tcoord[i * 2 + 0] : 0;
				vertexbuf[i].texcoord.y = tcoord ? tcoord[i * 2 + 1] : 0;
			}

			// copy vertex data
			write_to_buffer(vertexbuf, vertex_count);

			m_d3dContext->IASetPrimitiveTopology(primitive_type); //D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
			m_d3dContext->IASetInputLayout(m_inputLayout.Get());
			m_d3dContext->VSSetShader(m_vertexShader.Get(),	nullptr, 0);
		//	m_d3dContext->VSSetConstantBuffers(0,	1, m_constantBuffer.GetAddressOf());
			m_d3dContext->PSSetShader(tex ? m_pixelShaderTex.Get() : m_pixelShader.Get(), nullptr, 0);

			// tex mode
			ID3D11SamplerState* pss = m_texWrapMode.Get();
			m_d3dContext->PSSetSamplers(0, 1, &pss);

			// bind texture
			m_d3dContext->PSSetShaderResources( 0, 1, &tex );	// Draw the map to the square

			m_d3dContext->Draw(vertex_count,	0);

			delete [] vertexbuf;
			delete [] tcoord;
		}

		void draw_mesh_strip(const void* coords, int vertex_count)
		{
			draw_mesh_primitive(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, coords, vertex_count);
		}

		void	draw_triangle_list(const void* coords, int vertex_count)
		{
			draw_mesh_primitive(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, coords, vertex_count);
		}


		void	draw_line_strip(const void* coords, int vertex_count)
			// Draw the line strip formed by the sequence of points.
		{
			// Set up current style.
			const float* color = m_current_styles[LINE_STYLE].get_color();

			VertexPositionColor* vertexbuf = new VertexPositionColor[vertex_count];
			float* co = (float*) coords;

			// create vertex array

			for (int i = 0; i < vertex_count; i++)
			{
				bakeinflash::point pt(co[i * 2 + 0], co[i * 2 + 1]);
				bakeinflash::point result;
				m_current_matrix.transform(&result, pt);

				vertexbuf[i].pos.x = result.m_x * scale_x - 1;
				vertexbuf[i].pos.x -= shift_x;
				vertexbuf[i].pos.y = - (result.m_y * scale_y - 1);	// MINUS means flip
				vertexbuf[i].pos.y -= shift_y;

				vertexbuf[i].color.x = color[0];
				vertexbuf[i].color.y = color[1];
				vertexbuf[i].color.z = color[2];
				vertexbuf[i].color.w = color[3];
				vertexbuf[i].texcoord.x = 0;
				vertexbuf[i].texcoord.y = 0;
			}

			// copy vertex data
			write_to_buffer(vertexbuf, vertex_count);

			m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
			m_d3dContext->IASetInputLayout(m_inputLayout.Get());
			m_d3dContext->VSSetShader(m_vertexShader.Get(),	nullptr, 0);
			//m_d3dContext->VSSetConstantBuffers(0,	1, m_constantBuffer.GetAddressOf());
			m_d3dContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);

			m_d3dContext->Draw(vertex_count,	0);

			delete [] vertexbuf;
		}


		void	draw_bitmap(
			const bakeinflash::matrix& m,
			bakeinflash::bitmap_info* bi,
			const bakeinflash::rect& coords,
			const bakeinflash::rect& uv_coords,
			bakeinflash::rgba color)
			// Draw a rectangle textured with the given bitmap, with the
			// given color.	 Apply given transform; ignore any currently
			// set transforms.
			//
			// Intended for textured glyph rendering.
		{
			assert(bi);

			bi->upload();
			bitmap_info_ogl* bb = (bitmap_info_ogl*) bi;	// hack
			ID3D11ShaderResourceView* tex = bb->m_texture_id.Get();
			assert(tex);

			float tcoord[8];
			tcoord[0] = uv_coords.m_x_min;
			tcoord[1] = uv_coords.m_y_min;
			tcoord[2] = uv_coords.m_x_max;
			tcoord[3] = uv_coords.m_y_min;
			tcoord[4] = uv_coords.m_x_min;
			tcoord[5] = uv_coords.m_y_max;
			tcoord[6] = uv_coords.m_x_max;
			tcoord[7] = uv_coords.m_y_max;

			bakeinflash::point a, b, c, d;
			m.transform(&a, bakeinflash::point(coords.m_x_min, coords.m_y_min));
			m.transform(&b, bakeinflash::point(coords.m_x_max, coords.m_y_min));
			m.transform(&c, bakeinflash::point(coords.m_x_min, coords.m_y_max));
			d.m_x = b.m_x + c.m_x - a.m_x;
			d.m_y = b.m_y + c.m_y - a.m_y;

			float co[8];
			co[0] = (float) a.m_x;
			co[1] = (float) a.m_y;
			co[2] = (float) b.m_x;
			co[3] = (float) b.m_y;
			co[4] = (float) c.m_x;
			co[5] = (float) c.m_y;
			co[6] = (float) d.m_x;
			co[7] = (float) d.m_y;


			// create vertex array

			int vertex_count = 4;
			VertexPositionColor* vertexbuf = new VertexPositionColor[vertex_count];
			for (int i = 0; i < vertex_count; i++)
			{
				bakeinflash::point result(co[i * 2 + 0], co[i * 2 + 1]);

				vertexbuf[i].pos.x = result.m_x * scale_x - 1;
				vertexbuf[i].pos.x -= shift_x;
				vertexbuf[i].pos.y = - (result.m_y * scale_y - 1);	// MINUS means flip
				vertexbuf[i].pos.y -= shift_y;

				vertexbuf[i].color.x = color.m_r / 255.0f;
				vertexbuf[i].color.y = color.m_g / 255.0f;
				vertexbuf[i].color.z = color.m_b / 255.0f;
				vertexbuf[i].color.w = color.m_a / 255.0f;
				vertexbuf[i].texcoord.x = tcoord[i * 2 + 0];
				vertexbuf[i].texcoord.y = tcoord[i * 2 + 1];
			}

			// copy vertex data
			write_to_buffer(vertexbuf, vertex_count);

			m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
			m_d3dContext->IASetInputLayout(m_inputLayout.Get());
			m_d3dContext->VSSetShader(m_vertexShader.Get(),	nullptr, 0);
	//		m_d3dContext->VSSetConstantBuffers(0,	1, m_constantBuffer.GetAddressOf());
			m_d3dContext->PSSetShader(m_pixelShaderTex.Get(), nullptr, 0);

			// tex mode
			ID3D11SamplerState* pss = m_texWrapMode.Get();
			m_d3dContext->PSSetSamplers(0, 1, &pss);

			// bind texture
			m_d3dContext->PSSetShaderResources( 0, 1, &tex );	// Draw the map to the square

			m_d3dContext->Draw(vertex_count,	0);

			delete [] vertexbuf;
		}

		bool test_stencil_buffer(const bakeinflash::rect& bound, Uint8 pattern)
		{
			return true;
		}

		void begin_submit_mask()
		{
			if (m_mask_level == 0)
			{
				m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(),	D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
			}

			// Bind depth stencil state and set the stencil buffer to 'm_mask_level+1' 
			// draw any polygon and stencil buffer is 'm_mask_level'
			m_d3dContext->OMSetDepthStencilState(stencilState1.Get(), m_mask_level++);

			// disable framebuffer writes
			m_d3dContext->OMSetRenderTargets(0,	NULL, m_depthStencilView.Get());
		}

		// called after begin_submit_mask and the drawing of mask polygons
		void end_submit_mask()
		{	     
			// Bind depth stencil state #2
			m_d3dContext->OMSetDepthStencilState(stencilState2.Get(), m_mask_level);

			// enable framebuffer writes
			m_d3dContext->OMSetRenderTargets(1,	m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());
		}


		void disable_mask()
		{	  
			assert(m_mask_level > 0);

			if (--m_mask_level == 0)
			{
				// disable depth stencil state
				m_d3dContext->OMSetDepthStencilState(nullptr, 1);
				m_d3dContext->OMSetRenderTargets(1,	m_renderTargetView.GetAddressOf(), nullptr);
			}
			else
			{
				end_submit_mask();
			}
		}



		bool is_visible(const bakeinflash::rect& bound)
		{
			bakeinflash::rect viewport;
			viewport.m_x_min = 0;
			viewport.m_y_min = 0;
			viewport.m_x_max = m_display_width;
			viewport.m_y_max = m_display_height;
			return viewport.bound_test(bound);
		}

	};	// end struct render_handler_ogles


	// bitmap_info_ogl implementation

	bitmap_info_ogl::bitmap_info_ogl() :
		m_texture_id(NULL),
		m_width(0),
		m_height(0),
		m_suspended_image(0)
	{
	}

	bitmap_info_ogl::bitmap_info_ogl(image::image_base* im) :
		m_texture_id(0),
		m_width(im->m_width),
		m_height(im->m_height),
		m_suspended_image(im)
	{
#ifdef DEBUG_BITMAPS
		myprintf("new bitmap_info_ogl %dx%d\n", m_width, m_height);
#endif
	}

	// layout image to opengl texture memory
	void bitmap_info_ogl::upload()
	{
		if (m_texture_id == NULL)
		{
			if (m_suspended_image == NULL)
			{
				return;
			}

			m_width = m_suspended_image->m_width;
			m_height = m_suspended_image->m_height;

			switch (m_suspended_image->m_type)
			{
			case image::image_base::RGB:
				{
					int	w = p2(m_suspended_image->m_width);
					int	h = p2(m_suspended_image->m_height);
					if (w != m_suspended_image->m_width || h != m_suspended_image->m_height)
					{
						// Faster/simpler software bilinear rescale.
						//					software_resample(bpp, m_suspended_image->m_width, m_suspended_image->m_height,	m_suspended_image->m_pitch, m_suspended_image->m_data, w, h);
						image::image_base* dst = new image::rgba(w, h);
						image::zoom(m_suspended_image, dst);
						create_texture(DXGI_FORMAT_R8G8B8A8_UNORM, w, h, dst->m_data, 4, m_texture_id);
						delete dst;
					}
					else
					{
						// Use original image directly.
						assert(0);
						//create_texture(GL_RGB, w, h, m_suspended_image->m_data, 0);
					}
					break;
				}

			case image::image_base::RGBA:
				{
					int	w = p2(m_suspended_image->m_width);
					int	h = p2(m_suspended_image->m_height);
					if (w != m_suspended_image->m_width || h != m_suspended_image->m_height)
					{
						// Faster/simpler software bilinear rescale.
						//					software_resample(bpp, m_suspended_image->m_width, m_suspended_image->m_height,	m_suspended_image->m_pitch, m_suspended_image->m_data, w, h);
						image::image_base* dst = new image::rgba(w, h);
						image::zoom(m_suspended_image, dst);
						create_texture(DXGI_FORMAT_R8G8B8A8_UNORM, w, h, dst->m_data, 4, m_texture_id);
						delete dst;
					}
					else
					{
						// Use original image directly.
						create_texture(DXGI_FORMAT_R8G8B8A8_UNORM, w, h, m_suspended_image->m_data, 4, m_texture_id);
					}
					break;
				}

			case image::image_base::ALPHA:
				{
					int	w = m_suspended_image->m_width;
					int	h = m_suspended_image->m_height;
					create_texture(DXGI_FORMAT_A8_UNORM, w, h, m_suspended_image->m_data, 1, m_texture_id);
					break;
				}

			default:
				assert(0);
			}

#ifdef DEBUG_BITMAPS
			myprintf("upload tex=%d, %dx%d\n", m_texture_id, m_suspended_image->m_width, m_suspended_image->m_height);
#endif
			delete m_suspended_image;
			m_suspended_image = NULL;

		}
		else
		{
			//		glBindTexture(GL_TEXTURE_2D, m_texture_id);
			//		glEnable(GL_TEXTURE_2D);
		}
	}

}	// render_handler_ogles

namespace bakeinflash
{
	render_handler*	create_render_handler_d3d()
		// Factory.
	{
		return new render_handler_d3d::render_handler_d3d();
	}
}



// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
