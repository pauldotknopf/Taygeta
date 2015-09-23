#pragma once 

#include "stdafx.h"
#include "IRenderable.h"
#include "D3D9RenderImpl.h"
#include "XA2Player.h"

#pragma comment(linker, "/export:GetImplementation=_GetImplementation@0")
#pragma comment(linker, "/export:GetSoundImplementation=_GetSoundImplementation@0")

extern "C" EXPORTLIB IRenderable* APIENTRY GetImplementation()
{
	return new D3D9RenderImpl();
}

extern "C" EXPORTLIB ISoundPlayer* APIENTRY GetSoundImplementation()
{
	return new XA2Player();
}