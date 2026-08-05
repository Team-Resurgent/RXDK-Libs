#include "host_prelude.h"
#include <d3d8types.h>
#include <stdio.h>
int main(void){
#include SHADER_INL
  DWORD*p=(DWORD*)&psd;
  for(int i=0;i<60;i++) printf("%08lx\n",p[i]); return 0; }
