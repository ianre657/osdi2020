#include "uart.h"
#include "utility.h"
#include "mailbox.h"
#include "bootloader.h"
#include "framebuffer.h"

void processCommand()
{
    uart_puts("# ");
    char command[1024] = {0};
    int commandIndex = 0;
    int isbooting = 1;
    while (1) {
        char c = uart_getc();
        if (isbooting) {
            isbooting = 0;
            continue;
        }
        if (c == '\n') {
            uart_puts("\n");
            commandSwitcher(command);
            uart_puts("# ");
            memset(command, 0, sizeof(command));
            commandIndex = 0;
        } else {
            uart_send(c);
            command[commandIndex] = c;
            commandIndex ++;
        }
    }
}

void processTestCommand()
{
    uart_puts("# ");
    char command[1024] = {0};
    int commandIndex = 0;
    int isbooting = 1;
    while (1) {
        char c = uart_getc();
        if (isbooting) {
            isbooting = 0;
            continue;
        }
        if (c == '\n') {
            uart_puts("\n");
            if (strEqual(command, "loadimg")) {
                command_load_image();
            } else if (strEqual(command, "help")) {
                uart_puts("loadimg: load kernel image\n");
            }
            uart_puts("# ");
            memset(command, 0, sizeof(command));
            commandIndex = 0;
        } else {
            uart_send(c);
            command[commandIndex] = c;
            commandIndex ++;
        }
    }
}

void commandSwitcher(char *comm)
{
    if (strEqual(comm, "help")) {
        command_help();
    } else if (strEqual(comm, "hello")) {
        uart_puts("Hello World!\n");
    } else if (strEqual(comm, "reboot")) {
        command_reboot();
        return;
    } else if (strEqual(comm, "timestamp")) {
        command_timestamp();
    } else if (strEqual(comm, "info")) {
        command_hardware_info();
    } else if (strEqual(comm, "loadimg")) {
        command_load_image();
    } else if (strEqual(comm, "framebuffer")) {
        command_frame_buffer(); 
    } else if (strlen(comm) != 0){
        uart_puts("Error: command ");
        uart_puts(comm);
        uart_puts(" not found\n");
    }
}

void command_help()
{
    uart_puts("hello: print hello word\n");
    uart_puts("help: help\n");
    uart_puts("reboot: reboot pi\n");
    uart_puts("timestamp: get current timestamp\n");
    uart_puts("loadimg: load kernel image\n");
}

void command_reboot()
{
    reboot(0);
}

void command_timestamp()
{
    char str[1024] = {0};
    doubleToStr(getTimestamp(), str);
    uart_puts("[");
    uart_puts(str);
    uart_puts("]");
    uart_puts("\n");
}

void command_hardware_info()
{
    uart_puts("Serial number: : ");
    print_serial_number();
    uart_puts("\nBoard revision: ");
    print_board_revision();
    uart_puts("\nVC memory base address: ");
    print_vc_base_address();
    uart_puts("\nVC memory size: ");
    print_vc_size();
    uart_puts("\n");
}

void command_load_image()
{
    // uart_puts("Please input kernel load address: (default: 0x80000): ");
    char address[1024] = {0};
    // uart_get_string(address);
    // uart_puts(address);
    uart_puts("Please input kernel size: ");
    uart_puts("\n");

    char size[1024] = {0};
    uart_get_string(size);
    load_image(0x80000, strToInt(size));
}

void command_frame_buffer() 
{
    write_buf(framebuffer_init());
}