#include "include/uart.h"
#include "include/utils.h"
#include "include/reboot.h"
#include "include/string.h"
#include "include/mbox.h"
#include "include/framebuffer.h"
#include "include/irq.h"
#include "include/timer.h"
#include "include/sys.h"
#include "include/fork.h"
#include "include/scheduler.h"
#include "include/printf.h"

int check_string(char * str){
	char* cmd_help = "help";
	char* cmd_hello = "hello";
	char* cmd_reboot = "reboot";
	char* cmd_exc = "exc";
	char* cmd_irq = "irq";

	if(strcmp(str,cmd_help)==0){
	// print all available commands
		uart_send_string("\r\nhello : print Hello World!\r\n");
		uart_send_string("help : help\r\n");
		uart_send_string("exc: trigger an exception instruction\r\n");
		uart_send_string("irq: enable core timer and system timer\r\n");
		uart_send_string("reboot: reboot rpi3\r\n");
	}
	else if(strcmp(str,cmd_hello)==0){
	// print hello
		uart_send_string("\r\nHello World!\r\n");
	}
	else if(strcmp(str,cmd_reboot)==0){
		uart_send_string("\r\nBooting......\r\n");
		reset(10000);
		return 1;	
	}
	else if(strcmp(str,cmd_exc)==0){		
		uart_send_string("\r\n");
		// issue exception                	
		asm volatile(
			"mov x8,#10;" // invalid system call will return back
			"svc #1;"
    		);

		/*
		asm volatile(
			"brk #1;"
		);*/
	}
	else if(strcmp(str,cmd_irq)==0){
		uart_send_string("\r\n");	
		call_core_timer();
		sys_timer_init();//don't need system call for this!
	}
	else{
		uart_send_string("\r\nErr:command ");
		uart_send_string(str);
		uart_send_string(" not found, try <help>\r\n");
	}
	return 0;
}

void get_board_revision_info(){
  mbox[0] = 7 * 4; // buffer size in bytes
  mbox[1] = REQUEST_CODE;
  // tags begin
  mbox[2] = GET_BOARD_REVISION; // tag identifier
  mbox[3] = 4; // response value buffer's length.
  mbox[4] = 0; // request value buffer's length
  mbox[5] = 0; // value buffer
  // tags end
  mbox[6] = END_TAG;

  if(mbox_call(8)){
	 uart_send_string("### Board Revision:"); 
 	 uart_hex(mbox[5]); // it should be 0xa020d3 for rpi3 b+
  	 uart_send_string("\r\n");
  }
  else{
  	uart_send_string("Unable to query\r\n");
  }
}


void get_VC_core_base_addr(){
  mbox[0] = 8 * 4; // buffer size in bytes
  mbox[1] = REQUEST_CODE;
  // tags begin
  mbox[2] = GET_VC_MEMORY; // tag identifier
  mbox[3] = 8; // response value buffer's length.
  mbox[4] = 0; // request value buffer's length
  mbox[5] = 0; // value buffer
  mbox[6] = 0;
  // tags end
  mbox[7] = END_TAG;

  if(mbox_call(8)){
	 uart_send_string("### VC core base address:"); 
	 uart_hex(mbox[5]); //base address in bytes:3B400000
	 uart_send_string(", size ");
	 uart_hex(mbox[6]); //size:4C00000
	 uart_send_string("\r\n");
  }
  else{
  	uart_send_string("Unable to query\n");
  }
}


void foo(){
  int tmp = 5;
  printf("Task %d after exec, tmp address 0x%x, tmp value %d\n", get_taskid(), &tmp, tmp);
  exit(0);
}

void idle(){
  while(1){
    	schedule();
    	delay(1000000);
  }
  printf("Test finished\n");
  while(1);
}

void test() {
  	int cnt = 1;
  	if (fork() == 0) {
    		fork();
    		delay(100000);
    		fork();

    		while(cnt < 10) {
      			printf("Task id: %d, cnt: %d\n", get_taskid(), cnt);
      			delay(100000);
      			++cnt;
    		}
   	 	exit(0);
    		printf("Should not be printed\n");
  	} else {
    		printf("Task %d before exec, cnt address 0x%x, cnt value %d\n", get_taskid(), &cnt, cnt);
    		exec(foo);
  	}	
}

void kernel_process(){	
	int err = do_exec(test);
    	if (err < 0){
        	printf("Error while moving process to user mode\r\n");
    	}
}

void kernel_main(void)
{	
    uart_init();  
    init_printf(0, putc);
    
    printf("Hello, world!\r\n");

    //async_exc_routing(); //set HCR_EL2.IMO
                           // Do not set HCR_EL2.IMO if you want your interrupt directly goto kernel in EL1 
   
    enable_irq();        //clear PSTATE.DAIF
    core_timer_enable(); //enable core timer

    //fb_init();
    //fb_show();
   
    //get hardware information by mailbox
    get_board_revision_info();
    get_VC_core_base_addr();
   
    init_runQ(); 
   
    for(int num=0;num<1;num++){ 
    	int res = privilege_task_create(kernel_process);
    	if (res < 0) {
        	printf("error while starting process");
        	return;
    	}
    }	
   
    idle();
    
    /*
    // simple shell implement
    char str[128];
    
    while (1) {
    	uart_send_string(">>");
	uart_recv_string(128,str);	
	check_string(str); 
    }
    */
}
