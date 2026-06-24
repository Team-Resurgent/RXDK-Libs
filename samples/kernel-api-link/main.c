#include "xbox/kernel.h"
#include <stdio.h>

unsigned kernel_api_function_count(void);
unsigned kernel_api_data_count(void);
void kernel_api_link_touch(void);

int main(void)
{
    kernel_api_link_touch();
    printf(
        "RXDK-LibsZig kernel-api-link OK funcs=%u data=%u\n",
        kernel_api_function_count(),
        kernel_api_data_count()
    );
    for (;;)
        ;
}
