/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Declares the CLSIDs for the standard stock property pages (font, color,
 * picture and related) provided by the property-page component.
 */
#ifndef _MS_STOCK_PROP_PAGES_H_

// {7EBDAAE0-8120-11cf-899F-00AA00688B10}
DEFINE_GUID(CLSID_StockFontPage, 0x7ebdaae0, 0x8120, 0x11cf, 0x89, 0x9f, 0x0, 0xaa, 0x0, 0x68, 0x8b, 0x10);

// {7EBDAAE1-8120-11cf-899F-00AA00688B10}
DEFINE_GUID(CLSID_StockColorPage, 0x7ebdaae1, 0x8120, 0x11cf, 0x89, 0x9f, 0x0, 0xaa, 0x0, 0x68, 0x8b, 0x10);

// {7EBDAAE2-8120-11cf-899F-00AA00688B10}
DEFINE_GUID(CLSID_StockPicturePage, 0x7ebdaae2, 0x8120, 0x11cf, 0x89, 0x9f, 0x0, 0xaa, 0x0, 0x68, 0x8b, 0x10);

#define _MS_STOCK_PROP_PAGES_H_
#endif // _MS_STOCK_PROP_PAGES_H_
