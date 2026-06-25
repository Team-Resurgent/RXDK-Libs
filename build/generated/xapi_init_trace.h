#ifndef RXDK_XAPI_INIT_TRACE_H
#define RXDK_XAPI_INIT_TRACE_H

#ifndef RXDK_MU_TRACE
#define RXDK_MU_TRACE 0
#endif

#if RXDK_MU_TRACE

#ifdef __cplusplus
extern "C" {
#endif
unsigned long DbgPrint(const char *Format, ...);
#ifdef __cplusplus
}
#endif

#define RXDK_MU_TRACE_MSG(msg) DbgPrint("rxdk-mu: " msg "\n")
#define RXDK_MU_TRACE_MSG1(msg, a) DbgPrint("rxdk-mu: " msg "\n", (a))
#define RXDK_MU_TRACE_MSG2(msg, a, b) DbgPrint("rxdk-mu: " msg "\n", (a), (b))

#else

#define RXDK_MU_TRACE_MSG(msg) ((void)0)
#define RXDK_MU_TRACE_MSG1(msg, a) ((void)0)
#define RXDK_MU_TRACE_MSG2(msg, a, b) ((void)0)

#endif

#ifndef RXDK_XAPI_INIT_TRACE
#define RXDK_XAPI_INIT_TRACE 0
#endif

#if RXDK_XAPI_INIT_TRACE

#ifdef __cplusplus
extern "C" {
#endif
unsigned long DbgPrint(const char *Format, ...);
#ifdef __cplusplus
}
#endif

#define RXDK_INIT_TRACE(msg) DbgPrint("xapi-smoke: init " msg "\n")
#define RXDK_INIT_TRACE1(msg, a) DbgPrint("xapi-smoke: init " msg "\n", (a))
#define RXDK_INIT_TRACE2(msg, a, b) DbgPrint("xapi-smoke: init " msg "\n", (a), (b))

#else

#define RXDK_INIT_TRACE(msg) ((void)0)
#define RXDK_INIT_TRACE1(msg, a) ((void)0)
#define RXDK_INIT_TRACE2(msg, a, b) ((void)0)

#endif

#endif
