#include <d3d11.h>

#pragma comment (lib, "d3d11.lib")

#include "graphics.h"


#include "IUnityInterface.h"
#include "IUnityGraphics.h"
#include "IUnityGraphicsD3D11.h"

#include "nvcapture.h"


#include <stdio.h>


ID3D11Texture2D* pLeftTexturePtr;
ID3D11Texture2D* pRightTexturePtr;
ID3D11Texture2D* pTopTexturePtr;
ID3D11Texture2D* pBottomTexturePtr;
ID3D11Texture2D* pFrontTexturePtr;
ID3D11Texture2D* pBackTexturePtr;

ID3D11Texture2D* pLargeTexturePtr = NULL;

ID3D11Texture2D *pMyDevTexture = NULL;
ID3D11RenderTargetView *pMyDevRenderTargetView = NULL;

IUnityInterfaces* s_UnityInterfaces = NULL;
IUnityGraphics* s_Graphics = NULL;
UnityGfxRenderer s_DeviceType = kUnityGfxRendererNull;

ID3D11Device *unityDev;
ID3D11DeviceContext *unityDevCon;

ID3D11Device *dev;
ID3D11DeviceContext *devcon;

D3D11_TEXTURE2D_DESC desc;

extern "C" void	UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
	s_UnityInterfaces = unityInterfaces;
	s_Graphics = s_UnityInterfaces->Get<IUnityGraphics>();
	s_DeviceType = s_Graphics->GetRenderer();

	if(s_DeviceType == kUnityGfxRendererD3D11){
		IUnityGraphicsD3D11* d3d = s_UnityInterfaces->Get<IUnityGraphicsD3D11>();
		unityDev = d3d->GetDevice();
		unityDev->GetImmediateContext((ID3D11DeviceContext **)&unityDevCon);
	} 
}

void SetTexturePtrs(void* leftTexturePtr, void* rightTexturePtr, void* topTexturePtr, void* bottomTexturePtr, void* frontTexturePtr, void* backTexturePtr){
	
	pLeftTexturePtr = (ID3D11Texture2D*) leftTexturePtr;
	pRightTexturePtr = (ID3D11Texture2D*) rightTexturePtr;
	pTopTexturePtr = (ID3D11Texture2D*) topTexturePtr;
	pBottomTexturePtr = (ID3D11Texture2D*) bottomTexturePtr;
	pFrontTexturePtr = (ID3D11Texture2D*) frontTexturePtr;
	pBackTexturePtr = (ID3D11Texture2D*) backTexturePtr;

	DebugMessage("SetTexturePtrs has been called !!!");
}

static void CreateLargeTexture(UINT width, UINT height){
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM ;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET ;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

	HRESULT hResult = unityDev->CreateTexture2D( &desc, NULL, &pLargeTexturePtr );
}

void CopyResourcesIntoLargeResource()
{
	unityDevCon->CopySubresourceRegion(pLargeTexturePtr, 0, 0, 0, 0, pBackTexturePtr, 0, NULL);
	unityDevCon->CopySubresourceRegion(pLargeTexturePtr, 0, desc.Width, 0, 0, pTopTexturePtr, 0, NULL);
	unityDevCon->CopySubresourceRegion(pLargeTexturePtr, 0, 2*desc.Width, 0, 0, pBottomTexturePtr, 0, NULL);
	unityDevCon->CopySubresourceRegion(pLargeTexturePtr, 0, 0, desc.Height, 0, pRightTexturePtr, 0, NULL);
	unityDevCon->CopySubresourceRegion(pLargeTexturePtr, 0, desc.Width, desc.Height, 0, pFrontTexturePtr, 0, NULL);
	unityDevCon->CopySubresourceRegion(pLargeTexturePtr, 0, 2*desc.Width, desc.Height, 0, pLeftTexturePtr, 0, NULL);

}


void InitGraphics()
{
	if(pLeftTexturePtr){
		if(s_DeviceType == kUnityGfxRendererD3D11){
	
			pLeftTexturePtr->GetDesc(&desc);
			CreateLargeTexture(3*desc.Width, 2*desc.Height);

			HRESULT result = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, unityDev->GetCreationFlags(), NULL, NULL, D3D11_SDK_VERSION, &dev, NULL, &devcon);

			//char message[250];
			//sprintf(message,"D3D11CreateDevice  result: %d",result);
			//DebugMessage(message);

			IDXGIResource* pOtherResource(NULL);
			HRESULT hr = pLargeTexturePtr->QueryInterface( __uuidof(IDXGIResource), (void**)&pOtherResource );
			HANDLE sharedHandle;
			pOtherResource->GetSharedHandle(&sharedHandle);
			dev->OpenSharedResource(sharedHandle, __uuidof(ID3D11Texture2D), (LPVOID*)&pMyDevTexture);

			dev->CreateRenderTargetView(pMyDevTexture, NULL, &pMyDevRenderTargetView);
			devcon->OMSetRenderTargets(1, &pMyDevRenderTargetView, NULL);
			
			InitNvCapture(dev);

			CopyResourcesIntoLargeResource(); //optionally
		}
	}


}

void RenderFrame()
{
	if(pLargeTexturePtr){
		if(s_DeviceType == kUnityGfxRendererD3D11){
			CopyResourcesIntoLargeResource();

			TransferNvCapture();
		}
	}
}

static void CleanD3D()
{
	if(pLargeTexturePtr){
		pLargeTexturePtr->Release();
		pLargeTexturePtr = NULL;
	}
	if(pMyDevRenderTargetView){
		pMyDevRenderTargetView->Release();
		pMyDevRenderTargetView = NULL;
	}
	if(pMyDevTexture){
		pMyDevTexture->Release();
		pMyDevTexture = NULL;
	}
	if(pMyDevTexture){
		pMyDevTexture->Release();
		pMyDevTexture = NULL;
	}

	if(dev){
		dev->Release();
		devcon->Release();

		dev = NULL;
		devcon = NULL;
	}
}


void ReleaseGraphics(){
	ReleaseNv();
	CleanD3D();
}

