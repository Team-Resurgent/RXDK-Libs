typedef unsigned long DWORD;
extern const int _tls_array;
__declspec(thread) DWORD XapiLastErrorCode = 0;
void SetLastError(DWORD x) { XapiLastErrorCode = x; }
