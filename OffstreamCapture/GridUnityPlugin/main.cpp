
#include "IUnityInterface.h"
#include "IUnityGraphics.h"
#include "graphics.h"

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SendTextureIdsToPlugin(void* leftTexturePtr, void* rightTexturePtr, void* topTexturePtr, void* bottomTexturePtr, void* frontTexturePtr, void* backTexturePtr){
	SetTexturePtrs(leftTexturePtr, rightTexturePtr, topTexturePtr, bottomTexturePtr, frontTexturePtr, backTexturePtr);
}

static void UNITY_INTERFACE_API OnInitGraphics(int eventID){
	InitGraphics();
}

extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API InitGraphicsEventFunc(){
	return OnInitGraphics;
}

static void UNITY_INTERFACE_API OnRenderEvent(int eventID){
	RenderFrame();
}

extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetRenderEventFunc(){
	return OnRenderEvent;
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API StopApplication(){
	ReleaseGraphics();
}

