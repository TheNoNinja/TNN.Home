#include "hello_greet.h"

int main(void)
{
    const int printed = hello_greet("Clion");
    return printed > 0 ? 0 : 1;
}
