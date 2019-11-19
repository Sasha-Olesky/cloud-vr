#pragma once

void SetTexturePtrs(void* leftTexturePtr, void* rightTexturePtr, void* topTexturePtr, void* bottomTexturePtr, void* frontTexturePtr, void* backTexturePtr);

void InitGraphics();

void RenderFrame();

void ReleaseGraphics();