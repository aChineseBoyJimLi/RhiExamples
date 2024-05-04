#include "Log.h"

int main()
{
    uint8_t a[5] {1, 2, 3, 4, 5};

    uint8_t* ptr = a;

    Log::Info("%d", *(ptr + 4));
    return 0;
}