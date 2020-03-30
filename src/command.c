#include "uart.h"
#include "mailbox.h"
#include "string.h"

void input_buffer_overflow_message ( char cmd[] )
{
    uart_puts("Follow command: \"");
    uart_puts(cmd);
    uart_puts("\"... is too long to process.\n");

    uart_puts("The maximum length of input is 64.");
}

void command_help ()
{
    uart_puts("\n");
    uart_puts("Valid Command:\n");
    uart_puts("\thelp:\t\tprint this help.\n");
    uart_puts("\thello:\t\tprint \"Hello World!\".\n");
    uart_puts("\ttimestamp:\tget current timestamp.\n");
    uart_puts("\n");
}

void command_hello ()
{
    uart_puts("Hello World!\n");
}

void command_timestamp ()
{
    unsigned long int cnt_freq, cnt_tpct;
    char str[20];

    asm volatile(
        "mrs %0, cntfrq_el0 \n\t"
        "mrs %1, cntpct_el0 \n\t"
        : "=r" (cnt_freq),  "=r" (cnt_tpct)
        :
    );

    ftoa( ((float)cnt_tpct) / cnt_freq, str, 6);

    uart_send('[');
    uart_puts(str);
    uart_puts("]\n");
}

void command_not_found (char * s) 
{
    uart_puts("Err: command ");
    uart_puts(s);
    uart_puts(" not found, try <help>\n");
}

void command_reboot ()
{
    uart_puts("Start Rebooting...\n");

    *PM_WDOG = PM_PASSWORD | 0x20;
    *PM_RSTC = PM_PASSWORD | 100;
    
	while(1);
}

void command_board_revision()
{
    char str[20];  
    uint32_t board_revision = mbox_get_board_revision ();

    uart_puts("Board Revision: ");
    if ( board_revision )
    {
        itohex_str(board_revision, sizeof(uint32_t), str);
        uart_puts(str);
        uart_puts("\n");
    }
    else
    {
        uart_puts("Unable to query serial!\n");
    }
}

void command_vc_base_addr()
{
    char str[20];  
    uint64_t vc_base_addr = mbox_get_VC_base_addr ();

    uart_puts("VC Core Memory:\n");
    if ( vc_base_addr )
    {
        uart_puts("    - Base Address: ");
        itohex_str((uint32_t)(vc_base_addr >> 32), sizeof(uint32_t), str);
        uart_puts(str);
        uart_puts(" (in bytes)\n");


        uart_puts("    - Size: ");
        itohex_str((uint32_t)(vc_base_addr & 0xffffffff), sizeof(uint32_t), str);
        uart_puts(str);
        uart_puts(" (in bytes)\n");
    }
    else
    {
        uart_puts("Unable to query serial!\n");
    }
}

void command_load_image ()
{
    int32_t size = 0;
    int32_t is_receive_success = 0;
    char *kernel = (char *)0x80000;
        
    uart_puts("Start Loading Kernel Image...\n");

    do {

        // start signal to receive image
        uart_send(3);
        uart_send(3);
        uart_send(3);

        // read the kernel's size
        size  = uart_getc();
        size |= uart_getc() << 8;
        size |= uart_getc() << 16;
        size |= uart_getc() << 24;

        // send negative or positive acknowledge
        if(size<64 || size>1024*1024)
        {
            // size error
            uart_send('S');
            uart_send('E');            
            
            continue;
        }
        uart_send('O');
        uart_send('K');

        // 從0x80000開始放
        while ( size-- ) 
        {
            *kernel++ = uart_getc();
        }

        is_receive_success = 1;

    } while ( !is_receive_success );

    // restore arguments and jump to the new kernel.
    asm volatile (
        // we must force an absolute address to branch to
        "mov x30, 0x80000;"
        "ret"
    );
}
