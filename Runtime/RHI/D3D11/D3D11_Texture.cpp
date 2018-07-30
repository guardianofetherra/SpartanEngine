/*
Copyright(c) 2016-2018 Panos Karabelas

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

//= INCLUDES ======================
#include "D3D11_Texture.h"
#include "../IRHI_Implementation.h"
//=================================

//= NAMESPAECES ====
using namespace std;
//==================

namespace Directus
{
	D3D11_Texture::D3D11_Texture(Context* context) : IRHI_Texture(context)
	{
		m_rhiDevice				= context->GetSubsystem<RHI_Device>();
		m_shaderResourceView	= nullptr;	
		m_memoryUsage			= 0;
	}

	D3D11_Texture::~D3D11_Texture()
	{
		if (!m_shaderResourceView)
			return;

		m_shaderResourceView->Release();
		m_shaderResourceView = nullptr;
	}

	bool D3D11_Texture::CreateShaderResource(unsigned int width, unsigned int height, unsigned int channels, const vector<std::byte>& data, Texture_Format format)
	{
		if (!m_rhiDevice->GetDevice())
		{
			LOG_ERROR("D3D11_Texture::CreateShaderResource: Invalid device.");
			return false;
		}

		if (data.empty())
		{
			LOG_ERROR("D3D11_Texture::CreateShaderResource: Invalid parameters.");
			return false;
		}

		unsigned int mipLevels = 1;

		// ID3D11Texture2D
		D3D11_TEXTURE2D_DESC textureDesc;
		ZeroMemory(&textureDesc, sizeof(textureDesc));
		textureDesc.Width				= width;
		textureDesc.Height				= height;
		textureDesc.MipLevels			= mipLevels;
		textureDesc.ArraySize			= (unsigned int)1;
		textureDesc.Format				= d3d11_dxgi_format[format];
		textureDesc.SampleDesc.Count	= (unsigned int)1;
		textureDesc.SampleDesc.Quality	= (unsigned int)0;
		textureDesc.Usage				= D3D11_USAGE_IMMUTABLE;
		textureDesc.BindFlags			= D3D11_BIND_SHADER_RESOURCE;
		textureDesc.MiscFlags			= 0;
		textureDesc.CPUAccessFlags		= 0;

		// SUBRESROUCE DATA
		D3D11_SUBRESOURCE_DATA subresource;
		ZeroMemory(&subresource, sizeof(subresource));
		subresource.pSysMem = &data[0];
		subresource.SysMemPitch = (width * channels) * sizeof(std::byte);
		subresource.SysMemSlicePitch = (width * height * channels) * sizeof(std::byte);

		ID3D11Texture2D* texture = nullptr;
		HRESULT result = m_rhiDevice->GetDevice()->CreateTexture2D(&textureDesc, &subresource, &texture);
		if (FAILED(result))
		{
			LOG_ERROR("D3D11_Texture::CreateShaderResource: Failed to create ID3D11Texture2D. Invalid CreateTexture2D() parameters.");
			return false;
		}

		// SHADER RESOURCE VIEW
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = mipLevels;

		result = m_rhiDevice->GetDevice()->CreateShaderResourceView(texture, &srvDesc, &m_shaderResourceView);
		if (FAILED(result))
		{
			LOG_ERROR("D3D11_Texture::CreateShaderResource: Failed to create the ID3D11ShaderResourceView.");
			return false;
		}

		// Compute memory usage
		m_memoryUsage = (unsigned int)(sizeof(std::byte) * data.size());

		return true;
	}

	bool D3D11_Texture::CreateShaderResource(unsigned int width, unsigned int height, unsigned int channels, const vector<vector<std::byte>>& mipmaps, Texture_Format format)
	{
		if (!m_rhiDevice->GetDevice())
		{
			LOG_ERROR("D3D11_Texture::CreateFromMipmaps: Invalid device.");
			return false;
		}

		auto mipLevels = (unsigned int)mipmaps.size();

		vector<D3D11_SUBRESOURCE_DATA> subresourceData;
		vector<D3D11_TEXTURE2D_DESC> textureDescs;
		for (unsigned int i = 0; i < mipLevels; i++)
		{
			if (mipmaps[i].empty())
			{
				LOG_ERROR("D3D11_Texture::CreateShaderResource: Aborting creation of ID3D11Texture2D. Provided bits for mip level \"" + to_string(i) + "\" are empty.");
				return false;
			}

			// SUBRESROUCE DATA
			subresourceData.push_back(D3D11_SUBRESOURCE_DATA{});
			subresourceData.back().pSysMem			= &mipmaps[i][0];
			subresourceData.back().SysMemPitch		= (width * channels) * sizeof(std::byte);
			subresourceData.back().SysMemSlicePitch = (width * height * channels) * sizeof(std::byte);

			// ID3D11Texture2D
			textureDescs.push_back(D3D11_TEXTURE2D_DESC{});
			textureDescs.back().Width				= width;
			textureDescs.back().Height				= height;
			textureDescs.back().MipLevels			= mipLevels;
			textureDescs.back().ArraySize			= (unsigned int)1;
			textureDescs.back().Format				= d3d11_dxgi_format[format];
			textureDescs.back().SampleDesc.Count	= (unsigned int)1;
			textureDescs.back().SampleDesc.Quality	= (unsigned int)0;
			textureDescs.back().Usage				= D3D11_USAGE_IMMUTABLE;
			textureDescs.back().BindFlags			= D3D11_BIND_SHADER_RESOURCE;
			textureDescs.back().MiscFlags			= 0;
			textureDescs.back().CPUAccessFlags		= 0;

			width = max(width / 2, 1);
			height = max(height / 2, 1);

			// Compute memory usage
			m_memoryUsage += (unsigned int)(sizeof(std::byte) * mipmaps[i].size());
		}
		ID3D11Texture2D* texture = nullptr;
		HRESULT result = m_rhiDevice->GetDevice()->CreateTexture2D(textureDescs.data(), subresourceData.data(), &texture);
		if (FAILED(result))
		{
			LOG_ERROR("D3D11_Texture::CreateShaderResource: Failed to create ID3D11Texture2D. Invalid CreateTexture2D() parameters.");
			return false;
		}

		// SHADER RESOURCE VIEW
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format						= d3d11_dxgi_format[format];
		srvDesc.ViewDimension				= D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip	= 0;
		srvDesc.Texture2D.MipLevels			= mipLevels;

		result = m_rhiDevice->GetDevice()->CreateShaderResourceView(texture, &srvDesc, &m_shaderResourceView);
		if (FAILED(result))
		{
			LOG_ERROR("D3D11_Texture::CreateShaderResource: Failed to create the ID3D11ShaderResourceView.");
			return false;
		}

		return true;
	}

	bool D3D11_Texture::CreateShaderResource(unsigned int width, int height, unsigned int channels, const std::vector<std::byte>& data, Texture_Format format)
	{
		if (!m_rhiDevice->GetDevice())
		{
			LOG_ERROR("D3D11_Texture::CreateShaderResource: Invalid device.");
			return false;
		}

		unsigned int mipLevels = 7;

		// ID3D11Texture2D
		D3D11_TEXTURE2D_DESC textureDesc;
		ZeroMemory(&textureDesc, sizeof(textureDesc));
		textureDesc.Width				= width;
		textureDesc.Height				= height;
		textureDesc.MipLevels			= mipLevels;
		textureDesc.ArraySize			= (unsigned int)1;
		textureDesc.Format				= d3d11_dxgi_format[format];
		textureDesc.SampleDesc.Count	= (unsigned int)1;
		textureDesc.SampleDesc.Quality	= (unsigned int)0;
		textureDesc.Usage				= D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags			= D3D11_BIND_SHADER_RESOURCE;
		textureDesc.MiscFlags			= D3D11_RESOURCE_MISC_GENERATE_MIPS;
		textureDesc.CPUAccessFlags		= 0;

		// Create texture from description
		ID3D11Texture2D* texture = nullptr;
		HRESULT result = m_rhiDevice->GetDevice()->CreateTexture2D(&textureDesc, nullptr, &texture);
		if (FAILED(result))
		{
			LOG_ERROR("D3D11_Texture::CreateShaderResource: Failed to create ID3D11Texture2D. Invalid CreateTexture2D() parameters.");
			return false;
		}

		// SHADER RESOURCE VIEW
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = mipLevels;

		// Create shader resource view from description
		result = m_rhiDevice->GetDevice()->CreateShaderResourceView(texture, &srvDesc, &m_shaderResourceView);
		if (FAILED(result))
		{
			LOG_ERROR("D3D11_Texture::CreateShaderResource: Failed to create the ID3D11ShaderResourceView.");
			return false;
		}

		// SHADER RESOURCE VIEW
		D3D11_SUBRESOURCE_DATA subresource;
		ZeroMemory(&subresource, sizeof(subresource));
		subresource.pSysMem				= &data[0];
		subresource.SysMemPitch			= (width * channels) * sizeof(std::byte);
		subresource.SysMemSlicePitch	= (width * height * channels) * sizeof(std::byte);

		// Compute memory usage
		m_memoryUsage = (unsigned int)(sizeof(std::byte) * data.size());

		// Copy data from memory to the subresource created in non-mappable memory
		m_rhiDevice->GetDeviceContext()->UpdateSubresource(texture, 0, nullptr, subresource.pSysMem, subresource.SysMemPitch, 0);

		// Create mipmaps based on ID3D11ShaderResourveView
		m_rhiDevice->GetDeviceContext()->GenerateMips(m_shaderResourceView);

		return true;
	}

	unsigned int D3D11_Texture::GetMemoryUsage()
	{
		return IRHI_Texture::GetMemoryUsage() + m_memoryUsage;
	}
}