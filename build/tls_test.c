typedef unsigned long DWORD;
__declspec(thread) DWORD XapiLastErrorCode = 0;
void SetLastError(DWORD x) { XapiLastErrorCode = x; }
