#ifndef __IO_H__
#define __IO_H__

#include "map.h"
#include "mini_uart.h"

#define __print_as_number(type) \
    void __print_ ## type ## _as_number(type); \
    void _print_ ## type ## _as_number(type, int); \

#define UL unsigned long
#define __print_as_number_types char, int, long
#define extra_number_printing  long unsigned int: __print_UL_as_number
#define define__print_as_number_types() MAP(__print_as_number, __print_as_number_types) __print_as_number(UL)

define__print_as_number_types();

#define LABAL(x) x: __print_ ## x ## _as_number
#define print_as_number(x) _Generic((x), MAP_LIST(LABAL, __print_as_number_types), extra_number_printing)(x)

#define getchar(x) uart_recv();
#define putchar(x) uart_send(x);

#define __print(x) \
    _Generic((x), char: uart_send, char*: uart_send_string, \
            long: __print_long_as_number, int: __print_int_as_number, extra_number_printing)(x);

#define print(...) do{MAP(__print, __VA_ARGS__)}while(0)

#define println(...) print(__VA_ARGS__, "\n");
#define puts println

#endif
