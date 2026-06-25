// ---------------------------------------------------------------------------------------
// vlan.h
//
// Virtual Lan Library
//
// Copyright (C) Microsoft Corporation
// ---------------------------------------------------------------------------------------

#ifndef __VLAN_H__
#define __VLAN_H__

BOOL __attribute__((__stdcall__)) VLanInit();
BOOL __attribute__((__stdcall__)) VLanDriver();
BOOL __attribute__((__stdcall__)) VLanAttach(char * pszLan, BYTE * pbEnet, void * pvArg);
void __attribute__((__stdcall__)) VLanRecv(BYTE * pb, UINT cb, void * pvArg);
BOOL __attribute__((__stdcall__)) VLanXmit(BYTE * pb, UINT cb);
BOOL __attribute__((__stdcall__)) VLanDetach(BYTE * pbEnet);
void __attribute__((__stdcall__)) VLanTerm();

#endif
