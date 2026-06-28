#ifndef RXDK_XAPI_TLS_LAYOUT_H
#define RXDK_XAPI_TLS_LAYOUT_H

/*
 * Byte offsets from the copied TLS image base (KeGetCurrentThread()->TlsData + 4).
 *
 * Clang places each __declspec(thread) object in .tls$$<name>; the linker orders
 * those sections alphabetically after _tls_start (1 byte, padded to 4).
 *
 * Keep these in sync with the template symbol names in:
 *   build/xapi_tls_image.c
 */

#define RXDK_TLS_IMAGE_OFF_CURRENT_FIBER      4
#define RXDK_TLS_IMAGE_OFF_THREAD_FIBER_DATA  8
#define RXDK_TLS_IMAGE_OFF_LAST_ERROR         24
#define RXDK_TLS_IMAGE_OFF_TLS_SLOTS          28

#define RXDK_TLS_IMAGE_SIZE                   284

#endif
