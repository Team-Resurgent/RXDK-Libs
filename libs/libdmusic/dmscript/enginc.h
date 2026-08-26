/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

// Common definitions shared across the AudioVBScript engine.

#pragma once

#include "oleaut.h"

const LCID lcidUSEnglish = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
const bool g_fUseOleAut = false;

// Constants built into the langauge.  These will be treated as global variables and given fixed values.
extern const char *g_rgszBuiltInConstants[];
extern const int g_cBuiltInConstants;

// Helpers for working with IDispatch
DISPID GetDispID(IDispatch *pIDispatch, const char *pszBase); // returns DISPID_UNKNOWN on failure.
HRESULT InvokeAttemptingNotToUseOleAut(
			IDispatch *pDisp,
			DISPID dispIdMember,
			WORD wFlags,
			DISPPARAMS *pDispParams,
			VARIANT *pVarResult,
			EXCEPINFO *pExcepInfo,
			UINT *puArgErr);
HRESULT SetDispatchProperty(IDispatch *pDisp, DISPID dispid, bool fSetRef, const VARIANT &v, EXCEPINFO *pExcepInfo);
HRESULT GetDispatchProperty(IDispatch *pDisp, DISPID dispid, VARIANT &v, EXCEPINFO *pExcepInfo);
void ConvertOleAutExceptionBSTRs(bool fCurrentlyUsesOleAut, bool fResultUsesOleAut, EXCEPINFO *pExcepInfo);

// Holds a variant value.  Inits it on construction and clears it on destruction.  Any failure on clearing is ignored.
class SmartVariant
{
public:
	SmartVariant() { DMS_VariantInit(g_fUseOleAut, &m_v); }
	~SmartVariant() { DMS_VariantClear(g_fUseOleAut, &m_v); }

	operator VARIANT &() { return m_v; }
	VARIANT *operator &() { return &m_v; }

    void Clear(){ DMS_VariantClear(g_fUseOleAut, &m_v); DMS_VariantInit(g_fUseOleAut, &m_v); }

private:
	VARIANT m_v;
};
