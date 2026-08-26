/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

// RXDK UIX skin (.uix / "XSK0") runtime loader + IPluginSupport backing.
//
// The retail UIX built-in features render as plain text (see uix.cpp), but
// TITLE-supplied *extension* features (e.g. the on-screen keyboard in the
// UIXKeyboard sample) read all their geometry, key labels, and textures out of
// the .uix skin through IPluginSupport. This module parses the XSK0 skin and
// serves those resources, and provides the IPluginSupport object the engine
// hands to the feature and to the title's ITitleUIPlugin.
//
// The XSK0 format is RXDK's own (built by Rxdk.SkinBld); the UIXKeyboard sample
// skin is byte-identical to RXDK-Tools/reftest/skins/kbd.uix.

#ifndef RXDK_UIX_SKIN_H
#define RXDK_UIX_SKIN_H

#include <xtl.h>
#include <xobjbase.h>   // DECLARE_INTERFACE / STDMETHOD used by uix.h
#include <d3d8.h>
#include <uix.h>   // UIX_SKIN_LAYOUT_INFO, IPluginSupport

struct UIX_SKIN;   // opaque parsed skin

// Parse an in-memory .uix image. Returns NULL on a bad/unknown blob. The bytes
// are copied as needed; the caller may free its buffer after this returns.
UIX_SKIN *UixSkinLoad(const void *pData, DWORD cbData, DWORD LanguageID);
void      UixSkinFree(UIX_SKIN *pSkin);

// Resource lookups (all return NULL/E_* when the resID is absent).
const WCHAR *UixSkinGetString(UIX_SKIN *pSkin, DWORD StringResID);
HRESULT      UixSkinGetLayout(UIX_SKIN *pSkin, DWORD ScreenResID, DWORD ObjectResID,
                              UIX_SKIN_LAYOUT_INFO **ppLayout);
HRESULT      UixSkinGetImage(UIX_SKIN *pSkin, DWORD ImageResID,
                             IDirect3DTexture8 **ppTexture);
HRESULT      UixSkinGetScreenImage(UIX_SKIN *pSkin, DWORD ScreenResID, DWORD ImageResID,
                                   IDirect3DTexture8 **ppTexture);
// Audio resID -> the sound name the ITitleAudioPlugin expects (NULL if absent).
LPCSTR       UixSkinGetAudioName(UIX_SKIN *pSkin, DWORD AudioResID);
HRESULT      UixSkinGetWordLength(UIX_SKIN *pSkin, LPCWSTR pString, DWORD *pWordLength);

// Create/destroy the IPluginSupport object that fronts a loaded skin. The object
// is an opaque handle dispatched through the exported PluginSupport_* functions
// (declared in uix.h); free it with UixDestroyPluginSupport.
IPluginSupport *UixCreatePluginSupport(UIX_SKIN *pSkin);
void            UixDestroyPluginSupport(IPluginSupport *pSupport);

#endif // RXDK_UIX_SKIN_H
