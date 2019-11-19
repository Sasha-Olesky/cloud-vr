#pragma once

void InitNvCapture(ID3D11Device *unityDev);

void TransferNvCapture();

void ReleaseNv();

void DebugMessage(const char * message);