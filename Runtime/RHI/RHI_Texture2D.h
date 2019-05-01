/*
Copyright(c) 2016-2019 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

//= INCLUDES ===========
#include "RHI_Texture.h"
//======================

namespace Spartan
{
	class SPARTAN_CLASS RHI_Texture2D : public RHI_Texture
	{
	public:
		// Creates an empty texture intended for deferred loading
		RHI_Texture2D(Context* context, bool generate_mipmaps = true) : RHI_Texture(context) 
		{ 
			m_resource_type = Resource_Texture2d;
			m_has_mipmaps	= generate_mipmaps; 
		}

		// Creates a texture with mimaps. If only the first mipmap is available, the rest will automatically generated
		RHI_Texture2D(Context* context, unsigned int width, unsigned int height, unsigned int channels, RHI_Format format, const std::vector<std::vector<std::byte>>& data) : RHI_Texture(context)
		{
			m_resource_type = Resource_Texture2d;
			m_width			= width;
			m_height		= height;
			m_channels		= channels;
			m_format		= format;		
			m_has_mipmaps	= true;
			m_data			= data;

			CreateResourceGpu();
		}

		// Creates a texture without any mipmaps
		RHI_Texture2D(Context* context, unsigned int width, unsigned int height, unsigned int channels, RHI_Format format, const std::vector<std::byte>& data) : RHI_Texture(context)
		{
			m_resource_type = Resource_Texture2d;
			m_width			= width;
			m_height		= height;
			m_channels		= channels;
			m_format		= format;
			m_has_mipmaps	= false;
			m_data.emplace_back(data);

			CreateResourceGpu();
		}

		// Creates a texture without any data, intended for usage as a render target
		RHI_Texture2D(Context* context, unsigned int width, unsigned int height, RHI_Format format, bool is_render_target, bool is_depth_stencil_buffer, unsigned int array_size = 1) : RHI_Texture(context)
		{
			m_resource_type				= Resource_Texture2d;
			m_width						= width;
			m_height					= height;
			m_format					= format;
			m_is_render_target			= is_render_target;
			m_is_depth_stencil_buffer	= is_depth_stencil_buffer;
			m_array_size				= array_size;

			CreateResourceGpu();
		}

		~RHI_Texture2D();

		// RHI_Texture
		bool CreateResourceGpu() override;

	private:
		bool m_is_render_target			= false;
		bool m_is_depth_stencil_buffer	= false;
		unsigned int m_array_size		= 1;
	};
}