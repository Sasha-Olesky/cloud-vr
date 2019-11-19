
#include "IUnityInterface.h"

typedef void (*FuncPtr) (const char *);
FuncPtr Debug = 0;

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetDebugFunction(FuncPtr fp) {
	Debug = fp;
}

void DebugMessage(const char * message) {
	if(Debug){
		Debug(message);
	}
}

extern "C" void DebugMessageC(const char * message){
	DebugMessage(message);
}

