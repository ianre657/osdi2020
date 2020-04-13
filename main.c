#include "uart.h"
#include "pcsh.h"
#include "screen.h"
#include "string.h"

#include "syscall.h"
#include "irq.h"
#include "bottom_half.h"

#define INPUT_BUFFER_SIZE 1024

void system_start()
{
    uart_print("-------------------------\n");
    uart_print("Raspberry Pi 3B+ is start\n");
    uart_print("-------------------------\n");
}

void make_exc()
{
    uart_puts("exception happen!!!!!\n");
    unsigned int r;
    r = *((volatile unsigned int *)0xFFFFFFFFFF000000);

    r++;
}

void bottom_half_0()
{
    for (int i = 0; i < 5; i++)
    {
        uart_puts(".");
        // very very very slow in real rpi3, but very very very fast in qemu
        asm volatile(
            "mov  x0, #0xffffff\n"
            "bottom_half_0_l: subs  x0, x0, #1\n"
            "bne   bottom_half_0_l\n");
    }
    uart_send('\n');
}

int main()
{
    //_irq_init();

    *ENABLE_IRQ2 = 1 << 25;

    // set uart
    uart_init();

    system_start();

    get_board_revision();
    get_vc_memory();

    get_frame_buffer();
    showpicture();

    bottom_half_t n = {
        0,
        bottom_half_0};
    bottom_half_enroll(n);
    //test
    //    make_exc();
    //svc(1);

    //brk(1);

    /* You can't know Current EL, if you in EL0 */
    /*
    uart_puts("Current EL: ");
    uart_send_int(get_current_el());
    uart_puts("\n");
    */

    while (1)
    {
        uart_puts("=Shell Start=\n");
        pcsh();
        uart_puts("=Shell End=\n");
    }

    return 0;
}
