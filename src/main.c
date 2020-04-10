#include "../include/uart.h"
#include "../include/power.h"
#include "../include/time.h"
#include "../include/info.h"
#include "../include/lfb.h"
#include "../include/loadimg.h"
#include "../include/cmdline.h"
#include "../include/interrupt.h"
//#include "../include/exception.h"


int SPLASH_ON = 0;
int LOADIMG = 0;
unsigned int KERNEL_ADDR = 0x100000;
unsigned int CORE_TIMER_COUNT = 0;
unsigned int LOCAL_TIMER_COUNT = 0;

// #define CORE0_TIMER_IRQ_CTRL 0x40000040
// #define EXPIRE_PERIOD 0xfffffff

// void core_timer_enable()
// {
//     asm volatile(
//         "mov x0, #1\n\t"
//         "msr cntp_ctl_el0, x0\n\t" // enable timer
//         "mov x0, #2\n\t"
//     );
//     asm volatile("ldr x1, %1"::"i"(CORE0_TIMER_IRQ_CTRL));
//     asm volatile(
//         "str x0, [x1]\n\t" // enable timer interrupt
//     );
//     uart_puts("core timer is enable.\n");
// }    

// void core_timer_handler()
// {
//     asm volatile ("mov x0, %1"::"i"(EXPIRE_PERIOD));
//     asm volatile(
//         "msr cntp_tval_el0, x0\n\t"
//     );

// }
void main()
{
    // set up serial console and linear frame buffer
    uart_init();
    // char *int1 = 0;
    // uart_atoi(int1, 123456);
    // uart_puts(int1);
    // char *int2 = 0;
    // uart_atoi(int2, -13456);
    // uart_puts(int1);

    // display a pixmap
    if (SPLASH_ON == 1) {
        lfb_init();
        lfb_showpicture();
    }   

    // get hardware info
    get_serial();
    get_board_revision();
    get_vccore_addr();

    while (1) { 

        // get user input
        // command line
        char user_input[256];
        char tmp;
        int i = 0;
        uart_send('>');
        while (i < 10) {
            tmp = uart_getc();
            if (tmp == '\n') break;
            uart_send(tmp);
            user_input[i++] = tmp;
        }
        user_input[i] = '\0';
        uart_puts("\n");
        //uart_send(uart_i2c(i));
        if (i == 0) {
            continue;
        }
        //read_cmdline(user_input);
        /*
         ** <hello> Echo hello
         */
        if (uart_strncmp(user_input, "hello", 5) == 0) {
            uart_puts("Hello World!\n");
            continue;
        }
        /*
         ** <reboot> reboot in given cpu ticks
         */
        if (uart_strncmp(user_input, "reboot", 6) == 0) {
            get_time();
            uart_puts("reboot in 10 ticks.\n\n");
            reset(10);
            continue;
        }
        /*
         ** <time> get the timestamp
         */
        if (uart_strncmp(user_input, "time", 4) == 0) {
            get_time();
            continue;    
        }
        /*
         ** <loadimg> listen for raspbootcom and load the img from UART
         */
        if (uart_strncmp(user_input, "loadimg", 7) == 0) {
            uart_puts("loading image from uart...\n");
            loadimg(KERNEL_ADDR);
            LOADIMG = 1;
            break;    
        }
        /*
         ** <help> list the existed commands
         */
        if (uart_strncmp(user_input, "help", 4) == 0) {
            uart_puts("hello: print hello world.\n");
            uart_puts("help: help.\n");
            uart_puts("reboot: restart.\n");
            uart_puts("time: show timestamp.\n");
            uart_puts("loadimg: reload image from UART.\n");
            uart_puts("exc_svc: execute `svc #1`.\n");
            uart_puts("exc_brk: execute `brk #0`.\n");
            uart_puts("irq_core: core timer interrupt.\n");
            uart_puts("irq_local: local timer interrupt.\n");
            continue;
        }
        /*
         ** <exc> test exception
         */
        if (uart_strncmp(user_input, "exc_svc", 7) == 0) {
            uart_puts("execute `svc #1`\n");
            asm volatile ("svc #1");
            uart_puts("done\n");
            continue;
        }
        if (uart_strncmp(user_input, "exc_brk", 7) == 0) {
            uart_puts("execute `brk #0`\n");
            asm volatile ("brk #0");
            uart_puts("done\n");
            continue;
        }
        /*
         ** <irq> test interrupt
         */
        if (uart_strncmp(user_input, "irq", 3) == 0) {
            uart_puts("this is irq test\n");
            set_HCR_EL2_IMO();
            enable_irq();
            core_timer_enable();
            local_timer_init();
            continue;
        }
        /*
         ** Invalid command
         */
        uart_puts("Error: command ");
        uart_puts(user_input);
        uart_puts(" not found, try <help>.\n");
    }
    if (LOADIMG == 1) {
        unsigned int stack_pointer;; 
        asm volatile ("mov %0, sp" : "=r"(stack_pointer));
        uart_puts("stack pointer: 0x");
        uart_hex(stack_pointer);
        uart_puts("\n");

        unsigned int program_counter;; 
        asm volatile ("mov %0, x30" : "=r"(program_counter));
        uart_puts("program counter: 0x");
        uart_hex(program_counter);
        uart_puts("\n");
        /*
        ** Start to load image
        */
        asm volatile (
            // we must force an absolute address to branch to
            "mov x30, %0; ret" :: "r"(KERNEL_ADDR)
        );
    }
    
}
