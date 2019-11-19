
#include <Windows.h>

#include "NvIFRToSys.h"
#include "NvIFRHWEnc.h"

#include <d3d11.h>

#include "nvcapture.h"


#include <tchar.h>

#include <stdio.h>

//#include <time.h>


extern "C"{
	#include "mp4muxer.h"
}

#define NUMFRAMESINFLIGHT = 1

NvIFRToSys* toSys = NULL;

unsigned char *buffer;
HANDLE gpuEvent;

//hw enc
INvIFRToHWEncoder* toHWEnc = NULL;
#define N_BUFFERS 1
unsigned char * g_pBitStreamBuffer[N_BUFFERS] = {NULL};
HANDLE g_EncodeCompletionEvents[N_BUFFERS] = {NULL};


DWORD WINAPI ThreadFunction(LPVOID lpParam);
HANDLE hThread = NULL;
DWORD dwThreadId = 0;

HANDLE hFile = NULL;

boolean releasedCalled = false;

HANDLE ghMutex; 

int pkt_counter = 0;

#define WRITE_H264 0 //if 1, write h264 not mp4

HINSTANCE handleNVIFR = NULL;

static void SetUpTargetBuffers(){
	
	NVIFR_TOSYS_SETUP_PARAMS params = {0};
	params.dwVersion = NVIFR_TOSYS_SETUP_PARAMS_VER; 
	params.eFormat = NVIFR_FORMAT_RGB; 
	params.eSysStereoFormat = NVIFR_SYS_STEREO_NONE; 
	params.dwNBuffers = 1; 
	params.ppPageLockedSysmemBuffers = &buffer; 
	params.ppTransferCompletionEvents = &gpuEvent;

	NVIFRRESULT result = toSys->NvIFRSetUpTargetBufferToSys(&params);
}

static void SetUpHwEncTargetBuffers(){
	
	NVIFR_HW_ENC_SETUP_PARAMS params = {0};
	params.dwVersion = NVIFR_HW_ENC_SETUP_PARAMS_VER;
	//params.configParams.eCodec = NV_HW_ENC_H264;
	params.dwNBuffers = N_BUFFERS;
	params.dwBSMaxSize = 2048*1024;

	//g_pBitStreamBuffer[0] = new unsigned char[params.dwBSMaxSize];

	params.ppPageLockedBitStreamBuffers = g_pBitStreamBuffer;//(unsigned char **) (&g_pBitStreamBuffer);

	//g_EncodeCompletionEvents[0] = CreateEvent(NULL, TRUE, FALSE, TEXT("Sipvay"));
	params.ppEncodeCompletionEvents = g_EncodeCompletionEvents;

	NV_HW_ENC_CONFIG_PARAMS encodeConfig = {0}; 
	encodeConfig.dwVersion = NV_HW_ENC_CONFIG_PARAMS_VER; 
	encodeConfig.eCodec    = NV_HW_ENC_H264;
    encodeConfig.eStereoFormat = NV_HW_ENC_STEREO_NONE;
	encodeConfig.dwProfile = 100; 
	encodeConfig.dwAvgBitRate = 8000000;//TODO calculate the corect value
	encodeConfig.dwFrameRateDen = 1; 
	encodeConfig.dwFrameRateNum = 30; 
	encodeConfig.dwPeakBitRate = (encodeConfig.dwAvgBitRate * 12/10); // +20% 
	encodeConfig.dwGOPLength = 75; 
	encodeConfig.dwQP = 26 ; 
	encodeConfig.eRateControl = NV_HW_ENC_PARAMS_RC_CBR; 
	encodeConfig.bRecordTimeStamps = TRUE;
	//encodeConfig.ePresetConfig= NV_HW_ENC_PRESET_LOW_LATENCY_HQ; 
	
	//encodeConfig.bEnableAdaptiveQuantization = true;
	//! Set Encode Config params.configParams = encodeConfig;
	params.configParams = encodeConfig;
	
	NVIFRRESULT res = toHWEnc->NvIFRSetUpHWEncoder(&params);

	char message[250];
	sprintf(message,"NvIFRSetUpHWEncoder res: %d",res);
	DebugMessage(message);
}

void InitNvCapture(ID3D11Device *device){
	
	if(handleNVIFR == NULL){
		handleNVIFR = ::LoadLibrary(_T("NVIFR64.dll"));
	}
	
	if(handleNVIFR == NULL){
		char message[250];
		sprintf(message,"Couldn't load library NVIFR64.dll !!! error: %d",GetLastError());
		DebugMessage(message);
	} else {
				
		NvIFR_CreateFunctionExType pfnNVIFR_Create = NULL;		pfnNVIFR_Create = (NvIFR_CreateFunctionExType) GetProcAddress(handleNVIFR, "NvIFR_CreateEx");		if(pfnNVIFR_Create == NULL){			char message[250];
			sprintf(message,"Couldn't get NVIFR_CreateEx address !!! error: %d",GetLastError());
			DebugMessage(message);		} else {

			pkt_counter = 0;
			NVIFR_CREATE_PARAMS params = {0}; 
			params.dwVersion = NVIFR_CREATE_PARAMS_VER; 
			//params.dwInterfaceType = NVIFR_TOSYS; 
			params.dwInterfaceType = NVIFR_TO_HWENCODER; 
			params.pDevice = device;

			NVIFRRESULT res = pfnNVIFR_Create(&params);
			if (res == NVIFR_SUCCESS) { 
				//toSys = (NvIFRToSys *)params.pNvIFR; 
				toHWEnc = (INvIFRToHWEncoder *)params.pNvIFR;
				//SetUpTargetBuffers();
				//SetUpHwEncTargetBuffers();

				ghMutex = CreateMutex( NULL, FALSE, NULL);             
				
				if(g_pBitStreamBuffer[0] == 0){
					SetUpHwEncTargetBuffers();
				}
		  
#if WRITE_H264
				hFile = CreateFile("outputVideoH264", GENERIC_WRITE, 0,NULL,CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				if(hFile == INVALID_HANDLE_VALUE){
					hFile = NULL;
				}
#else
				mp4_init();
#endif

				hThread = CreateThread(NULL, 0, ThreadFunction, NULL, 0, &dwThreadId);

			}
		}
	}
}


void TransferNvCapture(){

	if(toSys != NULL){
		NVIFRRESULT result = toSys->NvIFRTransferRenderTargetToSys(0);
		
	} else if(toHWEnc != NULL) {
		
		NV_HW_ENC_PIC_PARAMS encodePicParams = {0}; 
		encodePicParams.dwVersion = NV_HW_ENC_PIC_PARAMS_VER;

		//NvU64 timestamp = GetTickCount64();
		
		encodePicParams.ulCaptureTimeStamp = pkt_counter++;

		char message[250];
		sprintf(message,"TransferNvCapture() %ull",encodePicParams.ulCaptureTimeStamp);
		DebugMessage(message);

		NVIFR_HW_ENC_TRANSFER_RT_TO_HW_ENC_PARAMS params = {0}; 
		params.dwVersion = NVIFR_HW_ENC_TRANSFER_RT_TO_HW_ENC_PARAMS_VER; 
		int i = 0;//temp
		params.dwBufferIndex = i; 
		params.encodePicParams = encodePicParams;
				
		NVIFRRESULT res = toHWEnc->NvIFRTransferRenderTargetToHWEncoder(&params);

					/*char message[250];
			sprintf(message,"NvIFRTransferRenderTargetToHWEncoder res: %d",res);
			DebugMessage(message);*/
	}
}

void ReleaseNv(){
	releasedCalled = true;
	//get mutex
	WaitForSingleObject( ghMutex, INFINITE);

	if(toSys != NULL){
		toSys->NvIFRRelease();
		toSys = NULL;
	} else if(toHWEnc != NULL) {
		toHWEnc->NvIFRRelease();
		toHWEnc = NULL;
	}

	if(g_pBitStreamBuffer[0] != NULL) {
		//delete(g_pBitStreamBuffer[0]);
		g_pBitStreamBuffer[0] = NULL;
	}

	if(ghMutex != NULL){
		CloseHandle(ghMutex);
		ghMutex = NULL;
	}

	if (hThread != NULL) {
		CloseHandle(hThread);
		hThread = NULL;
	}

	if(hFile != NULL){
		CloseHandle(hFile);
		hFile = NULL;
	}

	//leave mutex
	ReleaseMutex(ghMutex);

#if !WRITE_H264
	mp4_release();
#endif

}

DWORD WINAPI ThreadFunction(LPVOID lpParam){

	//get mutex
	WaitForSingleObject( ghMutex, INFINITE);

	if(toSys != NULL){//not implemented(not needed)
		WaitForSingleObject(gpuEvent, INFINITE);
	} else if(toHWEnc != NULL) {
		int i = 0;//temp,only one buffer
				
		while(true){
			if(releasedCalled){
				//stop ...
				releasedCalled = false;
				break;
			}
			DWORD dwRet = WaitForSingleObject(g_EncodeCompletionEvents[i], 100);
			
			ResetEvent(g_EncodeCompletionEvents[i]);
					
			if(dwRet == WAIT_OBJECT_0){

				DebugMessage("All good on g_EncodeCompletionEvents!!!");
				NVIFR_HW_ENC_GET_BITSTREAM_PARAMS params = {0}; 
				params.dwVersion = NVIFR_HW_ENC_GET_BITSTREAM_PARAMS_VER; 
				params.bitStreamParams.dwVersion = NV_HW_ENC_GET_BIT_STREAM_PARAMS_VER; 
				params.dwBufferIndex = i; 
				NVIFRRESULT res = toHWEnc->NvIFRGetStatsFromHWEncoder(&params);
				if(res == NVIFR_SUCCESS ) {
					//copy to disk params.bitStreamParams.dwByteSize;
#if WRITE_H264
					if(hFile != NULL){
#endif
						if(params.bitStreamParams.dwByteSize > 0){
							DWORD byteSize = 0; 

#if WRITE_H264
							WriteFile(hFile, g_pBitStreamBuffer[0], params.bitStreamParams.dwByteSize, &byteSize, NULL);
#else

							mp4_write_data(g_pBitStreamBuffer[0], params.bitStreamParams.dwByteSize);
#endif

							char message[250];
							sprintf(message,"params.bitStreamParams.dwByteSize: %d byteSize: %d timestamp: %ull ",params.bitStreamParams.dwByteSize,byteSize,params.bitStreamParams.ulTimeStamp);
							DebugMessage(message);
						}
#if WRITE_H264
					}
#endif
				}
			} else if(dwRet == WAIT_FAILED){
				char message[250];
				sprintf(message,"WaitForSingleObject  error: %d",GetLastError());
				DebugMessage(message);
			} else if(dwRet == WAIT_TIMEOUT){
				DebugMessage("WaitForSingleObject time out !!!");
			}

		}
				
	}
	//leave mutex
	ReleaseMutex(ghMutex);

	return 1;
}