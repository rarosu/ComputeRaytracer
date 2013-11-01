//--------------------------------------------------------------------------------------
// Real-Time JPEG Compression using DirectCompute - Demo
//
// Copyright (c) Stefan Petersson 2012. All rights reserved.
//--------------------------------------------------------------------------------------
#include "ComputeHelp.h"
#include <cstdio>

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib") 

ComputeShader::ComputeShader()
	: mD3DDevice(nullptr), mD3DDeviceContext(nullptr), mShader(nullptr)
{

}

ComputeShader::~ComputeShader()
{
	SAFE_RELEASE(mShader);
	//SAFE_RELEASE(mD3DDevice);
	//SAFE_RELEASE(mD3DDeviceContext);
}

bool ComputeShader::Init(TCHAR* shaderFile, TCHAR* blobFileAppendix, char* pFunctionName, D3D10_SHADER_MACRO* pDefines,
	ID3D11Device* d3dDevice, ID3D11DeviceContext*d3dContext)
{
	HRESULT hr = S_OK;
	mD3DDevice = d3dDevice;
	mD3DDeviceContext = d3dContext;

	ID3DBlob* pCompiledShader = nullptr;
	ID3DBlob* pErrorBlob = nullptr;

	FILE* fShaderBlob = nullptr;

	TCHAR blobFilename[300];
	if(blobFileAppendix != nullptr)
	{
		size_t l1 = _tcslen(shaderFile);
		size_t l2 = _tcslen(_tcsrchr(shaderFile, _T('.')));
		_tcsncpy_s(blobFilename, shaderFile, l1 - l2);
		
		_tcscat_s(blobFilename, _T("_"));
		_tcscat_s(blobFilename, blobFileAppendix);
		_tcscat_s(blobFilename, _T(".blob"));

		//MessageBox(0, blobFilename, L"", 0);

		_tfopen_s(&fShaderBlob, blobFilename, _T("rb"));
	}

	DWORD dwShaderFlags =	D3DCOMPILE_ENABLE_STRICTNESS |
							D3DCOMPILE_IEEE_STRICTNESS |
							//D3DCOMPILE_WARNINGS_ARE_ERRORS |
							D3DCOMPILE_PREFER_FLOW_CONTROL;
	
#if defined(DEBUG) || defined(_DEBUG)
	dwShaderFlags |= D3DCOMPILE_DEBUG;
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	dwShaderFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

	if(fShaderBlob == nullptr)
	{
		//hr = D3DX11CompileFromFile(shaderFile, pDefines, nullptr, pFunctionName, "cs_5_0", 
			//dwShaderFlags, nullptr, nullptr, &pCompiledShader, &pErrorBlob, nullptr);

		hr = D3DCompileFromFile(shaderFile, pDefines, nullptr, pFunctionName, "cs_5_0", 
			dwShaderFlags, NULL, &pCompiledShader, &pErrorBlob);

		if(hr == S_OK)
		{
			if(blobFileAppendix != nullptr)
			{
				_tfopen_s(&fShaderBlob, blobFilename, _T("wb"));

				if(fShaderBlob != nullptr)
				{
					size_t size = pCompiledShader->GetBufferSize();
					fwrite(&size, sizeof(size_t), 1, fShaderBlob);
					fwrite(pCompiledShader->GetBufferPointer(), size, 1, fShaderBlob);
					fclose(fShaderBlob);
				}
			}
		}
	}
	else
	{
		int size = 0;
		fread_s(&size, sizeof(int), sizeof(int), 1, fShaderBlob);

		if(D3DCreateBlob(size, &pCompiledShader) == S_OK)
		{
			fread_s(pCompiledShader->GetBufferPointer(), size, size, 1, fShaderBlob);
		}

		fclose(fShaderBlob);
	}
	if (pErrorBlob)
	{
		OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
	}

	if(hr == S_OK)
	{
		hr = mD3DDevice->CreateComputeShader(pCompiledShader->GetBufferPointer(),
			pCompiledShader->GetBufferSize(), nullptr, &mShader);
	}

	SAFE_RELEASE(pErrorBlob);
	SAFE_RELEASE(pCompiledShader);

    return (hr == S_OK);
}

void ComputeShader::Set()
{
	mD3DDeviceContext->CSSetShader( mShader, nullptr, 0 );
}

void ComputeShader::Unset()
{
	mD3DDeviceContext->CSSetShader( nullptr, nullptr, 0 );
}

ComputeBuffer* ComputeWrap::CreateBuffer(COMPUTE_BUFFER_TYPE uType,
	UINT uElementSize, UINT uCount, bool bSRV, bool bUAV, VOID* pInitData, bool bCreateStaging, char* debugName)
{
	ComputeBuffer* buffer = new ComputeBuffer();
	buffer->_D3DContext = mD3DDeviceContext;

	if(uType == STRUCTURED_BUFFER)
		buffer->_Resource = CreateStructuredBuffer(uElementSize, uCount, bSRV, bUAV, pInitData);
	else if(uType == RAW_BUFFER)
		buffer->_Resource = CreateRawBuffer(uElementSize * uCount, pInitData);

	if(buffer->_Resource != nullptr)
	{
		if(bSRV)
			buffer->_ResourceView = CreateBufferSRV(buffer->_Resource);
		if(bUAV)
			buffer->_UnorderedAccessView = CreateBufferUAV(buffer->_Resource);
		
		if(bCreateStaging)
			buffer->_Staging = CreateStagingBuffer(uElementSize * uCount);
	}

	if(debugName)
	{
		if(buffer->_Resource)				SetDebugName(buffer->_Resource, debugName);
		if(buffer->_Staging)				SetDebugName(buffer->_Staging, debugName);
		if(buffer->_ResourceView)			SetDebugName(buffer->_ResourceView, debugName);
		if(buffer->_UnorderedAccessView)	SetDebugName(buffer->_UnorderedAccessView, debugName);
	}

	return buffer; //return shallow copy
}

ID3D11Buffer* ComputeWrap::CreateStructuredBuffer(UINT uElementSize, UINT uCount,
									bool bSRV, bool bUAV, VOID* pInitData)
{
    ID3D11Buffer* pBufOut = nullptr;

    D3D11_BUFFER_DESC desc;
    ZeroMemory( &desc, sizeof(desc) );
    desc.BindFlags = 0;
	
	if(bUAV)	desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
	if(bSRV)	desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
    
	UINT bufferSize = uElementSize * uCount;
	desc.ByteWidth = bufferSize < 16 ? 16 : bufferSize;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = uElementSize;

    if ( pInitData )
    {
        D3D11_SUBRESOURCE_DATA InitData;
        InitData.pSysMem = pInitData;
		mD3DDevice->CreateBuffer( &desc, &InitData, &pBufOut);
    }
	else
	{
		mD3DDevice->CreateBuffer(&desc, nullptr, &pBufOut);
	}

	return pBufOut;
}

ID3D11Buffer* ComputeWrap::CreateRawBuffer(UINT uSize, VOID* pInitData)
{
    ID3D11Buffer* pBufOut = nullptr;

    D3D11_BUFFER_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_INDEX_BUFFER | D3D11_BIND_VERTEX_BUFFER;
    desc.ByteWidth = uSize;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

    if ( pInitData )
    {
        D3D11_SUBRESOURCE_DATA InitData;
        InitData.pSysMem = pInitData;
        mD3DDevice->CreateBuffer(&desc, &InitData, &pBufOut);
    }
	else
	{
        mD3DDevice->CreateBuffer(&desc, nullptr, &pBufOut);
	}

	return pBufOut;
}

ID3D11ShaderResourceView* ComputeWrap::CreateBufferSRV(ID3D11Buffer* pBuffer)
{
	ID3D11ShaderResourceView* pSRVOut = nullptr;

    D3D11_BUFFER_DESC descBuf;
    ZeroMemory(&descBuf, sizeof(descBuf));
    pBuffer->GetDesc(&descBuf);

    D3D11_SHADER_RESOURCE_VIEW_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
    desc.BufferEx.FirstElement = 0;

    if(descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
    {
        // This is a Raw Buffer
        desc.Format = DXGI_FORMAT_R32_TYPELESS;
        desc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
        desc.BufferEx.NumElements = descBuf.ByteWidth / 4;
    }
	else if(descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED)
    {
        // This is a Structured Buffer
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.BufferEx.NumElements = descBuf.ByteWidth / descBuf.StructureByteStride;
    }
	else
	{
		return nullptr;
	}

    mD3DDevice->CreateShaderResourceView(pBuffer, &desc, &pSRVOut);

	return pSRVOut;
}

ID3D11UnorderedAccessView* ComputeWrap::CreateBufferUAV(ID3D11Buffer* pBuffer)
{
	ID3D11UnorderedAccessView* pUAVOut = nullptr;

	D3D11_BUFFER_DESC descBuf;
    ZeroMemory(&descBuf, sizeof(descBuf));
    pBuffer->GetDesc(&descBuf);
        
    D3D11_UNORDERED_ACCESS_VIEW_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    desc.Buffer.FirstElement = 0;

    if (descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
    {
        // This is a Raw Buffer
        desc.Format = DXGI_FORMAT_R32_TYPELESS; // Format must be DXGI_FORMAT_R32_TYPELESS, when creating Raw Unordered Access View
        desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
        desc.Buffer.NumElements = descBuf.ByteWidth / 4; 
    }
	else if(descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED)
    {
        // This is a Structured Buffer
        desc.Format = DXGI_FORMAT_UNKNOWN;      // Format must be must be DXGI_FORMAT_UNKNOWN, when creating a View of a Structured Buffer
        desc.Buffer.NumElements = descBuf.ByteWidth / descBuf.StructureByteStride; 
    }
	else
	{
		return nullptr;
	}
    
	mD3DDevice->CreateUnorderedAccessView(pBuffer, &desc, &pUAVOut);

	return pUAVOut;
}

ID3D11Buffer* ComputeWrap::CreateStagingBuffer(UINT uSize)
{
    ID3D11Buffer* debugbuf = nullptr;

    D3D11_BUFFER_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
	desc.ByteWidth = uSize;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.MiscFlags = 0;
    
	mD3DDevice->CreateBuffer(&desc, nullptr, &debugbuf);

    return debugbuf;
}



// TEXTURE FUNCTIONS
ComputeTexture* ComputeWrap::CreateTexture(DXGI_FORMAT dxFormat, UINT uWidth,
	UINT uHeight, UINT uRowPitch, VOID* pInitData, bool bCreateStaging, char* debugName)
{
	ComputeTexture* texture = new ComputeTexture();
	texture->_D3DContext = mD3DDeviceContext;

	texture->_Resource = CreateTextureResource(dxFormat, uWidth, uHeight, uRowPitch, pInitData);

	if(texture->_Resource != nullptr)
	{
		texture->_ResourceView = CreateTextureSRV(texture->_Resource);
		texture->_UnorderedAccessView = CreateTextureUAV(texture->_Resource);
		
		if(bCreateStaging)
			texture->_Staging = CreateStagingTexture(texture->_Resource);
	}

	if(debugName)
	{
		if(texture->_Resource)				SetDebugName(texture->_Resource, debugName);
		if(texture->_Staging)				SetDebugName(texture->_Staging, debugName);
		if(texture->_ResourceView)			SetDebugName(texture->_ResourceView, debugName);
		if(texture->_UnorderedAccessView)	SetDebugName(texture->_UnorderedAccessView, debugName);
	}

	return texture;
}

ComputeTexture* ComputeWrap::CreateTexture(TCHAR* textureFilename, char* debugName)
{
	MessageBoxA(0, "ComputeWrap::CreateTexture is not implemented anymore!", "Error", 0);
	return nullptr;

	/*
	ComputeTexture* texture = new ComputeTexture();
	texture->_D3DContext = mD3DDeviceContext;

	if(SUCCEEDED(D3DX11CreateTextureFromFile(mD3DDevice, textureFilename, nullptr, nullptr, (ID3D11Resource**)&texture->_Resource, nullptr)))
	{
		texture->_ResourceView = CreateTextureSRV(texture->_Resource);
		
		if(debugName)
		{
			if(texture->_Resource)				SetDebugName(texture->_Resource, debugName);
			if(texture->_Staging)				SetDebugName(texture->_Staging, debugName);
			if(texture->_ResourceView)			SetDebugName(texture->_ResourceView, debugName);
			if(texture->_UnorderedAccessView)	SetDebugName(texture->_UnorderedAccessView, debugName);
		}
	}
	return texture;
	*/
}

ID3D11Texture2D* ComputeWrap::CreateTextureResource(DXGI_FORMAT dxFormat,
	UINT uWidth, UINT uHeight, UINT uRowPitch, VOID* pInitData)
{
	ID3D11Texture2D* pTexture = nullptr;

	D3D11_TEXTURE2D_DESC desc;
	desc.Width = uWidth;
	desc.Height = uHeight;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = dxFormat;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = pInitData;
	data.SysMemPitch = uRowPitch; //uWidth * 4;

	if(FAILED(mD3DDevice->CreateTexture2D( &desc, pInitData ? &data : nullptr, &pTexture )))
	{

	}

	return pTexture;
}

ID3D11ShaderResourceView* ComputeWrap::CreateTextureSRV(ID3D11Texture2D* pTexture)
{
	ID3D11ShaderResourceView* pSRV = nullptr;

	D3D11_TEXTURE2D_DESC td;
	pTexture->GetDesc(&td);

	//init view description
	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc; 
	ZeroMemory( &viewDesc, sizeof(viewDesc) ); 
	
	viewDesc.Format					= td.Format;
	viewDesc.ViewDimension			= D3D11_SRV_DIMENSION_TEXTURE2D;
	viewDesc.Texture2D.MipLevels	= td.MipLevels;

	if(FAILED(mD3DDevice->CreateShaderResourceView(pTexture, &viewDesc, &pSRV)))
	{
		//MessageBox(0, "Unable to create shader resource view", "Error!", 0);
	}

	return pSRV;
}

ID3D11UnorderedAccessView* ComputeWrap::CreateTextureUAV(ID3D11Texture2D* pTexture)
{
	ID3D11UnorderedAccessView* pUAV = nullptr;

	mD3DDevice->CreateUnorderedAccessView( pTexture, nullptr, &pUAV );
	pTexture->Release();

	return pUAV;
}

ID3D11Texture2D* ComputeWrap::CreateStagingTexture(ID3D11Texture2D* pTexture)
{
    ID3D11Texture2D* pStagingTex = nullptr;

    D3D11_TEXTURE2D_DESC desc;
	pTexture->GetDesc(&desc);
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.MiscFlags = 0;
    
	mD3DDevice->CreateTexture2D(&desc, nullptr, &pStagingTex);

    return pStagingTex;
}

ID3D11Buffer* ComputeWrap::CreateConstantBuffer(UINT uSize, VOID* pInitData, char* debugName)
{
	ID3D11Buffer* pBuffer = nullptr;

	// setup creation information
	D3D11_BUFFER_DESC cbDesc;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	bool addMod = uSize % 16 != 0 ? true : false;
	cbDesc.ByteWidth = uSize + (addMod ? (16 - uSize % 16) : 0);
	cbDesc.CPUAccessFlags = 0;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;
	cbDesc.Usage = D3D11_USAGE_DEFAULT;

    if(pInitData)
    {
        D3D11_SUBRESOURCE_DATA InitData;
        InitData.pSysMem = pInitData;
        mD3DDevice->CreateBuffer(&cbDesc, &InitData, &pBuffer);
    }
	else
	{
        mD3DDevice->CreateBuffer(&cbDesc, nullptr, &pBuffer);
	}

	if(debugName && pBuffer)
	{
		SetDebugName(pBuffer, debugName);
	}

	return pBuffer;
}

void ComputeWrap::SetDebugName(ID3D11DeviceChild* object, char* debugName)
{
#if defined( DEBUG ) || defined( _DEBUG )
	// Only works if device is created with the D3D10 or D3D11 debug layer, or when attached to PIX for Windows
	object->SetPrivateData( WKPDID_D3DDebugObjectName, (unsigned int)strlen(debugName), debugName );
#endif
}

ComputeShader* ComputeWrap::CreateComputeShader(TCHAR* shaderFile, TCHAR* blobFileAppendix, char* pFunctionName, D3D10_SHADER_MACRO* pDefines)
{
	ComputeShader* cs = new ComputeShader();

	if(cs && !cs->Init(
		shaderFile,
		blobFileAppendix,
		pFunctionName,
		pDefines,
		mD3DDevice,
		mD3DDeviceContext))
	{
		SAFE_DELETE(cs);
	}

	return cs;
}