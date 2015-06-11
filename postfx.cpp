#include "skygfx.h"

// Credits: much of the code in this file was originally written by NTAuthority

struct QuadVertex
{
	RwReal      x, y, z;
	RwReal      rhw;
	RwUInt32    emissiveColor;
	RwReal      u, v;
	RwReal      u1, v1;
};

struct ScreenVertex
{
	RwReal      x, y, z;
	RwReal      rhw;
	RwReal      u, v;
};

void *postfxVS, *colorFilterPS, *radiosityPS;
void *quadVertexDecl, *screenVertexDecl;
RwRect smallRect;
static QuadVertex quadVertices[4];
static ScreenVertex screenVertices[4];
RwRaster *target1, *target2;

static QuadVertex vcsVertices[24];
RwRect vcsRect;
RwImVertexIndex vcsIndices1[] = {
	0, 1, 2, 1, 2, 3,
		4, 5, 2, 5, 2, 3,
	4, 5, 6, 5, 6, 7,
		8, 9, 6, 9, 6, 7,
	8, 9, 10, 9, 10, 11,
		12, 13, 10, 13, 10, 11,
	12, 13, 14, 13, 14, 15,
};

RwImVertexIndex radiosityIndices[] = {
	0, 1, 2, 1, 2, 3
};

static RwImVertexIndex* quadIndices = (RwImVertexIndex*)0x8D5174;
RwRaster *&CPostEffects__pRasterFrontBuffer = *(RwRaster**)0xC402D8;

WRAPPER void SpeedFX(float) { EAXJMP(0x7030A0); }

void
SpeedFX_Fix(float fStrength)
{
	// So we don't do useless work if no colour postfx was performed
	if(config->colorFilter < 2){
		RwCameraEndUpdate(Camera);
		RwRasterPushContext(CPostEffects__pRasterFrontBuffer);
		RwRasterRenderFast(RwCameraGetRaster(Camera), 0, 0);
		RwRasterPopContext();
		RwCameraBeginUpdate(Camera);
	}
	SpeedFX(fStrength);
}

void
CPostEffects__Radiosity_VCS(int intensityLimit, int filterPasses, int renderPasses, int intensity)
{
	RwRaster *vcsBuffer1, *vcsBuffer2;
	float radiosityColors[4];
	RwTexture tempTexture;
	RwRaster *camraster;
	int width, height;
	float uOffsets[] = { -1.0f, 1.0f, 0.0f, 0.0f };
	float vOffsets[] = { 0.0f, 0.0f, -1.0f, 1.0f };

	width = vcsRect.w = filterPasses ? RsGlobal->MaximumWidth*256/640 : RsGlobal->MaximumWidth;
	height = vcsRect.h = filterPasses ? RsGlobal->MaximumHeight*128/480 : RsGlobal->MaximumHeight;

	if(target1->width < width || target2->width < width ||
	   target1->height < height || target2->height < height || 
	   target1->width != target2->width || target1->height != target2->height){
		int width2 = 1, height2 = 1;
		while(width2 < width) width2 <<= 1;
		while(height2 < height) height2 <<= 1;
		RwRasterDestroy(target1);
		target1 = RwRasterCreate(width2, height2, CPostEffects__pRasterFrontBuffer->depth, 5);
		RwRasterDestroy(target2);
		target2 = RwRasterCreate(width2, height2, CPostEffects__pRasterFrontBuffer->depth, 5);
	}
	vcsBuffer1 = target1;
	vcsBuffer2 = target2;

	float rasterWidth = RwRasterGetWidth(target1);
	float rasterHeight = RwRasterGetHeight(target1);
	float halfU = 0.5 / rasterWidth;
	float halfV = 0.5 / rasterHeight;
	float uMax = width / rasterWidth;
	float vMax = height / rasterHeight;
	float xMax = -1.0f + 2*uMax;
	float yMax = 1.0f - 2*vMax;
	float xOffsetScale = config->scaleOffsets ? width/256.0f : 1.0f;
	float yOffsetScale = config->scaleOffsets ? height/128.0f : 1.0f;
	if(config->radiosityFilterUCorrection == 0.0f)
		xOffsetScale = yOffsetScale = 0.0f;

	vcsVertices[0].x = -1.0f;
	vcsVertices[0].y = 1.0f;
	vcsVertices[0].z = 0.0f;
	vcsVertices[0].rhw = 1.0f;
	vcsVertices[0].u = 0.0f + halfU;
	vcsVertices[0].v = 0.0f + halfV;
	vcsVertices[0].emissiveColor = 0xFFFFFFFF;

	vcsVertices[1].x = -1.0f;
	vcsVertices[1].y = yMax;
	vcsVertices[1].z = 0.0f;
	vcsVertices[1].rhw = 1.0f;
	vcsVertices[1].u = 0.0f + halfU;
	vcsVertices[1].v = vMax + halfV;
	vcsVertices[1].emissiveColor = 0xFFFFFFFF;

	vcsVertices[2].x = xMax;
	vcsVertices[2].y = 1.0f;
	vcsVertices[2].z = 0.0f;
	vcsVertices[2].rhw = 1.0f;
	vcsVertices[2].u = uMax + halfU;
	vcsVertices[2].v = 0.0f + halfV;
	vcsVertices[2].emissiveColor = 0xFFFFFFFF;

	vcsVertices[3].x = xMax;
	vcsVertices[3].y = yMax;
	vcsVertices[3].z = 0.0f;
	vcsVertices[3].rhw = 1.0f;
	vcsVertices[3].u = uMax + halfU;
	vcsVertices[3].v = vMax + halfV;
	vcsVertices[3].emissiveColor = 0xFFFFFFFF;

	vcsVertices[4].x = -1.0f;
	vcsVertices[4].y = 1.0f;
	vcsVertices[4].z = 0.0f;
	vcsVertices[4].rhw = 1.0f;
	vcsVertices[4].u = 0.0f + halfU;
	vcsVertices[4].v = 0.0f + halfV;
	vcsVertices[4].emissiveColor = 0xFFFFFFFF;

	vcsVertices[5].x = -1.0f;
	vcsVertices[5].y = -1.0f;
	vcsVertices[5].z = 0.0f;
	vcsVertices[5].rhw = 1.0f;
	vcsVertices[5].u = 0.0f + halfU;
	vcsVertices[5].v = vMax + halfV;
	vcsVertices[5].emissiveColor = 0xFFFFFFFF;

	vcsVertices[6].x = 1.0f;
	vcsVertices[6].y = 1.0f;
	vcsVertices[6].z = 0.0f;
	vcsVertices[6].rhw = 1.0f;
	vcsVertices[6].u = uMax + halfU;
	vcsVertices[6].v = 0.0f + halfV;
	vcsVertices[6].emissiveColor = 0xFFFFFFFF;

	vcsVertices[7].x = 1.0f;
	vcsVertices[7].y = -1.0f;
	vcsVertices[7].z = 0.0f;
	vcsVertices[7].rhw = 1.0f;
	vcsVertices[7].u = uMax + halfU;
	vcsVertices[7].v = vMax + halfV;
	vcsVertices[7].emissiveColor = 0xFFFFFFFF;

	RwUInt32 c = 0xFF000000 | 0x010101*config->trailsIntensity;
	for(int i = 8; i < 24; i++){
		int idx = (i-8)/4;
		vcsVertices[i].x = vcsVertices[i%4].x;
		vcsVertices[i].y = vcsVertices[i%4].y;
		vcsVertices[i].z = 0.0f;
		vcsVertices[i].rhw = 1.0f;
		switch(i%4){
		case 0:
			vcsVertices[i].u = 0.0f + xOffsetScale*uOffsets[idx]/rasterWidth + halfU;
			vcsVertices[i].v = 0.0f + yOffsetScale*vOffsets[idx]/rasterHeight + halfV;
			break;
		case 1:
			vcsVertices[i].u = 0.0f + xOffsetScale*uOffsets[idx]/rasterWidth + halfU;
			vcsVertices[i].v = vMax + yOffsetScale*vOffsets[idx]/rasterHeight + halfV;
			break;
		case 2:
			vcsVertices[i].u = uMax + xOffsetScale*uOffsets[idx]/rasterWidth + halfU;
			vcsVertices[i].v = 0.0f + yOffsetScale*vOffsets[idx]/rasterHeight + halfV;
			break;
		case 3:
			vcsVertices[i].u = uMax + xOffsetScale*uOffsets[idx]/rasterWidth + halfU;
			vcsVertices[i].v = vMax + yOffsetScale*vOffsets[idx]/rasterHeight + halfV;
			break;
		}
		vcsVertices[i].emissiveColor = c;
	}

	// First pass - render downsampled and darkened frame to buffer2

	RwCameraEndUpdate(Camera);
	RwRasterPushContext(vcsBuffer1);
	camraster = RwCameraGetRaster(Camera);
	RwRasterRenderScaled(camraster, &vcsRect);
	RwRasterPopContext();
	RwCameraSetRaster(Camera, vcsBuffer2);
	RwCameraBeginUpdate(Camera);

	tempTexture.raster = vcsBuffer1;
	RwD3D9SetTexture(&tempTexture, 0);

	RwD3D9SetVertexDeclaration(quadVertexDecl);

	RwD3D9SetVertexShader(postfxVS);
	RwD3D9SetPixelShader(radiosityPS);

	radiosityColors[0] = config->trailsLimit/255.0f;
	radiosityColors[1] = 1.0f;
	radiosityColors[2] = 1.0f;
	RwD3D9SetPixelShaderConstant(0, radiosityColors, 1);

	int blend, srcblend, destblend, ztest, zwrite;
	RwRenderStateGet(rwRENDERSTATEZTESTENABLE, &ztest);
	RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &zwrite);
	RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &blend);
	RwRenderStateGet(rwRENDERSTATESRCBLEND, &srcblend);
	RwRenderStateGet(rwRENDERSTATEDESTBLEND, &destblend);

	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)0);
	RwD3D9SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
	RwD3D9DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 6, 2, vcsIndices1, vcsVertices, sizeof(QuadVertex));

	// Second pass - render brightened buffer2 to buffer1 (then swap buffers)
	RwD3D9SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

	for(int i = 0; i < 4; i++){
		RwCameraEndUpdate(Camera);
		RwCameraSetRaster(Camera, vcsBuffer1);
		RwCameraBeginUpdate(Camera);

		RwD3D9SetPixelShader(radiosityPS);
		RwD3D9SetTexture(NULL, 0);
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)0);
		RwD3D9DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 6, 2, vcsIndices1, vcsVertices, sizeof(QuadVertex));

		RwD3D9SetPixelShader(NULL);
		tempTexture.raster = vcsBuffer2;
		RwD3D9SetTexture(&tempTexture, 0);
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
		RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
		RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
		RwD3D9DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 6*7, 2*7, vcsIndices1, vcsVertices+8, sizeof(QuadVertex));
		RwRaster *tmp = vcsBuffer1;
		vcsBuffer1 = vcsBuffer2;
		vcsBuffer2 = tmp;
	}

	RwCameraEndUpdate(Camera);
	RwCameraSetRaster(Camera, camraster);
	RwCameraBeginUpdate(Camera);

	tempTexture.raster = vcsBuffer2;
	RwD3D9SetTexture(&tempTexture, 0);
	RwD3D9DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 6, 2, vcsIndices1, vcsVertices+4, sizeof(QuadVertex));

	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)ztest);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)zwrite);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)blend);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)srcblend);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)destblend);
	RwD3D9SetVertexShader(NULL);
}

WRAPPER void CPostEffects__Radiosity(int intensityLimit, signed int filterPasses, signed int renderPasses, int intensity) { EAXJMP(0x702080); }

void
CPostEffects__Radiosity_PS2(int intensityLimit, int filterPasses, int renderPasses, int intensity)
{
	int width, height;
	RwRaster *buffer1, *buffer2, *camraster;
	float radiosityColors[4];
	RwTexture tempTexture;

//	*(int*)0xC40314 = RsGlobal->MaximumWidth;
//	*(int*)0xC40318 = RsGlobal->MaximumHeight;
//	*(char*)0x8D510B = 0;
//	CPostEffects__Radiosity(intensityLimit, 2, 4, intensity);
//	return;

	if(!config->doRadiosity)
		return;

	if(config->vcsTrails){
		CPostEffects__Radiosity_VCS(intensityLimit, filterPasses, renderPasses, intensity);
		return;
	}


	width = RsGlobal->MaximumWidth;
	height = RsGlobal->MaximumHeight;

	if(target1->width < width || target2->width < width ||
	   target1->height < height || target2->height < height || 
	   target1->width != target2->width || target1->height != target2->height){
		int width2 = 1, height2 = 1;
		while(width2 < width) width2 <<= 1;
		while(height2 < height) height2 <<= 1;
		RwRasterDestroy(target1);
		target1 = RwRasterCreate(width2, height2, CPostEffects__pRasterFrontBuffer->depth, 5);
		RwRasterDestroy(target2);
		target2 = RwRasterCreate(width2, height2, CPostEffects__pRasterFrontBuffer->depth, 5);
	}
	buffer1 = target1;
	buffer2 = target2;

	RwCameraEndUpdate(Camera);
	RwRasterPushContext(buffer1);
	camraster = RwCameraGetRaster(Camera);
	RwRasterRenderFast(RwCameraGetRaster(Camera), 0, 0);
	RwRasterPopContext();
	RwCameraBeginUpdate(Camera);

	float rasterWidth = RwRasterGetWidth(target1);
	float rasterHeight = RwRasterGetHeight(target1);
	float xOffsetScale = config->scaleOffsets ? width/640.0f : 1.0f;
	float yOffsetScale = config->scaleOffsets ? height/480.0f : 1.0f;

	screenVertices[0].z = 0.0f;
	screenVertices[0].rhw = 1.0f / Camera->nearPlane;
	screenVertices[1].z = 0.0f;
	screenVertices[1].rhw = 1.0f / Camera->nearPlane;
	screenVertices[2].z = 0.0f;
	screenVertices[2].rhw = 1.0f / Camera->nearPlane;
	screenVertices[3].z = 0.0f;
	screenVertices[3].rhw = 1.0f / Camera->nearPlane;

	RwD3D9SetVertexDeclaration(screenVertexDecl);
	RwD3D9SetVertexShader(NULL);
	RwD3D9SetPixelShader(NULL);

	int blend, srcblend, destblend, ztest, zwrite;
	RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &blend);
	RwRenderStateGet(rwRENDERSTATESRCBLEND, &srcblend);
	RwRenderStateGet(rwRENDERSTATEDESTBLEND, &destblend);
	RwRenderStateGet(rwRENDERSTATEZTESTENABLE, &ztest);
	RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &zwrite);

	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)0);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)0);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)0);
	RwD3D9SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	RwD3D9SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

	screenVertices[0].x = 0.0f;
	screenVertices[0].y = 0.0f;
	screenVertices[0].u = (config->radiosityFilterUCorrection*xOffsetScale + 0.5f) / rasterWidth;
	screenVertices[0].v = (config->radiosityFilterVCorrection*yOffsetScale + 0.5f) / rasterHeight;
	screenVertices[2].x = screenVertices[0].x;
	screenVertices[1].y = screenVertices[0].y;
	screenVertices[2].u = screenVertices[0].u;
	screenVertices[1].v = screenVertices[0].v;

	for(int i = 0; i < filterPasses; i++){
		screenVertices[3].x = width/2 + 1*xOffsetScale;
		screenVertices[3].y = height/2 + 1*yOffsetScale;
		screenVertices[3].u = (width + 0.5f) / rasterWidth;
		screenVertices[3].v = (height + 0.5f) / rasterHeight;

		screenVertices[1].x = screenVertices[3].x;
		screenVertices[2].y = screenVertices[3].y;
		screenVertices[1].u = screenVertices[3].u;
		screenVertices[2].v = screenVertices[3].v;

		width /= 2;
		height /= 2;

//		RwD3D9SetRenderTarget(0, buffer2);
		RwCameraEndUpdate(Camera);
		RwCameraSetRaster(Camera, buffer2);
		RwCameraBeginUpdate(Camera);

		tempTexture.raster = buffer1;
		RwD3D9SetTexture(&tempTexture, 0);
		RwD3D9DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 6, 2, radiosityIndices, screenVertices, sizeof(ScreenVertex));
		RwRaster *tmp = buffer1;
		buffer1 = buffer2;
		buffer2 = tmp;
	}

	screenVertices[0].x = 0.0f;
	screenVertices[0].y = 0.0f;
	screenVertices[0].u = 0.5f / rasterWidth;
	screenVertices[0].v = 0.5f / rasterHeight;

	screenVertices[3].x = RsGlobal->MaximumWidth;
	screenVertices[3].y = RsGlobal->MaximumHeight;
	screenVertices[3].u = (width+0.5f) / rasterWidth;
	screenVertices[3].v = (height+0.5f) / rasterHeight;

	screenVertices[1].x = screenVertices[3].x;
	screenVertices[2].y = screenVertices[3].y;
	screenVertices[1].u = screenVertices[3].u;
	screenVertices[2].v = screenVertices[3].v;
	screenVertices[2].x = screenVertices[0].x;
	screenVertices[1].y = screenVertices[0].y;
	screenVertices[2].u = screenVertices[0].u;
	screenVertices[1].v = screenVertices[0].v;

	RwCameraEndUpdate(Camera);
	RwCameraSetRaster(Camera, camraster);
	RwCameraBeginUpdate(Camera);

	tempTexture.raster = buffer1;
	RwD3D9SetTexture(&tempTexture, 0);
	RwD3D9SetPixelShader(radiosityPS);

	radiosityColors[0] = intensityLimit/255.0f;
	radiosityColors[1] = (intensity/255.0f)*renderPasses;
	radiosityColors[2] = 2.0f;
	RwD3D9SetPixelShaderConstant(0, radiosityColors, 1);

	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);

	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);

	RwD3D9DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 6, 2, radiosityIndices, screenVertices, sizeof(ScreenVertex));

	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)blend);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)srcblend);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)destblend);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)ztest);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)zwrite);

	RwD3D9SetVertexShader(NULL);
	RwD3D9SetPixelShader(NULL);
}

void
CPostEffects__ColourFilter_PS2(RwRGBA rgb1, RwRGBA rgb2)
{
	float rasterWidth = RwRasterGetWidth(CPostEffects__pRasterFrontBuffer);
	float rasterHeight = RwRasterGetHeight(CPostEffects__pRasterFrontBuffer);
	float halfU = 0.5 / rasterWidth;
	float halfV = 0.5 / rasterHeight;
	float uMax = RsGlobal->MaximumWidth / rasterWidth;
	float vMax = RsGlobal->MaximumHeight / rasterHeight;
	int i = 0;

	float leftOff, rightOff, topOff, bottomOff;
	float scale = config->scaleOffsets ? RsGlobal->MaximumWidth/640.0f : 1.0f;
	leftOff = (float)config->offLeft*scale / 16.0f / rasterWidth;
	rightOff = (float)config->offRight*scale / 16.0f / rasterWidth;
	topOff = (float)config->offTop*scale / 16.0f / rasterHeight;
	bottomOff = (float)config->offBottom*scale / 16.0f / rasterHeight;

	quadVertices[i].x = 1.0f;
	quadVertices[i].y = -1.0f;
	quadVertices[i].z = 0.0f;
	quadVertices[i].rhw = 1.0f;
	quadVertices[i].u = uMax + halfU;
	quadVertices[i].v = vMax + halfV;
	quadVertices[i].u1 = quadVertices[i].u + rightOff;
	quadVertices[i].v1 = quadVertices[i].v + bottomOff;
	i++;

	quadVertices[i].x = -1.0f;
	quadVertices[i].y = -1.0f;
	quadVertices[i].z = 0.0f;
	quadVertices[i].rhw = 1.0f;
	quadVertices[i].u = 0.0f + halfU;
	quadVertices[i].v = vMax + halfV;
	quadVertices[i].u1 = quadVertices[i].u + leftOff;
	quadVertices[i].v1 = quadVertices[i].v + bottomOff;
	i++;

	quadVertices[i].x = -1.0f;
	quadVertices[i].y = 1.0f;
	quadVertices[i].z = 0.0f;
	quadVertices[i].rhw = 1.0f;
	quadVertices[i].u = 0.0f + halfU;
	quadVertices[i].v = 0.0f + halfV;
	quadVertices[i].u1 = quadVertices[i].u + leftOff;
	quadVertices[i].v1 = quadVertices[i].v + topOff;
	i++;

	quadVertices[i].x = 1.0f;
	quadVertices[i].y = 1.0f;
	quadVertices[i].z = 0.0f;
	quadVertices[i].rhw = 1.0f;
	quadVertices[i].u = uMax + halfU;
	quadVertices[i].v = 0.0f + halfV;
	quadVertices[i].u1 = quadVertices[i].u + rightOff;
	quadVertices[i].v1 = quadVertices[i].v + topOff;
	
	RwTexture tempTexture;
	tempTexture.raster = CPostEffects__pRasterFrontBuffer;
	RwD3D9SetTexture(&tempTexture, 0);

	RwD3D9SetVertexDeclaration(quadVertexDecl);

	RwD3D9SetVertexShader(postfxVS);
	RwD3D9SetPixelShader(colorFilterPS);

	RwRGBAReal color, color2;
	RwRGBARealFromRwRGBA(&color, &rgb1);
	RwRGBARealFromRwRGBA(&color2, &rgb2);

	RwD3D9SetPixelShaderConstant(0, &color, 1);
	RwD3D9SetPixelShaderConstant(1, &color2, 1);

	RwD3D9SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
	RwD3D9SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	RwD3D9DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 6, 2, quadIndices, quadVertices, sizeof(QuadVertex));

	RwD3D9SetRenderState(D3DRS_ALPHABLENDENABLE, 1);

	RwD3D9SetVertexShader(NULL);
	RwD3D9SetPixelShader(NULL);
}

WRAPPER char ReloadPlantConfig(void) { EAXJMP(0x5DD780); }

void
CPostEffects__ColourFilter_switch(RwRGBA rgb1, RwRGBA rgb2)
{
	if(config->enableHotkeys){
		static bool keystate = false;
		if(GetAsyncKeyState(config->keys[0]) & 0x8000){
			if(!keystate){
				keystate = true;
				if(config == &configs[0])
					config++;
				else
					config = &configs[0];
				resetRadiosityValues();
				SetCloseFarAlphaDist(3.0, 1234.0); // second value overriden
			}
		}else
			keystate = false;
	}

	if(config->enableHotkeys){
		static bool keystate = false;
		if(GetAsyncKeyState(config->keys[1]) & 0x8000){
			if(!keystate){
				keystate = true;
				if(config == &configs[0])
					config++;
				else
					config = &configs[0];
				readIni();
				resetRadiosityValues();
				SetCloseFarAlphaDist(3.0, 1234.0); // second value overriden
			}
		}else
			keystate = false;
	}

	if(config->enableHotkeys){
		static bool keystate = false;
		if(GetAsyncKeyState(config->keys[2]) & 0x8000){
			if(!keystate){
				keystate = true;
				if(config == &configs[0])
					config++;
				else
					config = &configs[0];
				readIni();
				resetRadiosityValues();
				ReloadPlantConfig();
				SetCloseFarAlphaDist(3.0, 1234.0); // second value overriden
			}
		}else
			keystate = false;
	}

	if(config->colorFilter == 0)
		CPostEffects__ColourFilter_PS2(rgb1, rgb2);
	else if(config->colorFilter == 1)
		CPostEffects__ColourFilter(rgb1, rgb2);
}

void
CPostEffects__Init(void)
{
	HRSRC resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_POSTFXVS), RT_RCDATA);
	RwUInt32 *shader = (RwUInt32*)LoadResource(dllModule, resource);
	RwD3D9CreateVertexShader(shader, &postfxVS);
	FreeResource(shader);

	resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_FILTERPS), RT_RCDATA);
	shader = (RwUInt32*)LoadResource(dllModule, resource);
	RwD3D9CreatePixelShader(shader, &colorFilterPS);
	FreeResource(shader);

	resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_RADIOSITYPS), RT_RCDATA);
	shader = (RwUInt32*)LoadResource(dllModule, resource);
	RwD3D9CreatePixelShader(shader, &radiosityPS);
	FreeResource(shader);

	static const D3DVERTEXELEMENT9 vertexElements[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 16, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
		{ 0, 20, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		{ 0, 28, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
		D3DDECL_END()
	};
	RwD3D9CreateVertexDeclaration(vertexElements, &quadVertexDecl);

	static const D3DVERTEXELEMENT9 screenVertexElements[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 },
		{ 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};
	RwD3D9CreateVertexDeclaration(screenVertexElements, &screenVertexDecl);

	target1 = RwRasterCreate(4,
	                         4,
	                         CPostEffects__pRasterFrontBuffer->depth, 5);
	target2 = RwRasterCreate(4,
	                         4,
	                         CPostEffects__pRasterFrontBuffer->depth, 5);
	smallRect.x = 0;
	smallRect.y = 0;
	smallRect.w = Camera->frameBuffer->width/4;
	smallRect.h = Camera->frameBuffer->height/4;

	reflTex = RwRasterCreate(128,
	                         128,
	                         CPostEffects__pRasterFrontBuffer->depth, 5);
	vcsRect.x = 0;
	vcsRect.y = 0;
}

