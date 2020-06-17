#include "timer.h"
#include "sched.h"

void local_timer_init(){
  unsigned int flag = 0x30000000; // enable timer and interrupt.
  unsigned int reload = 25000000;
  *LOCAL_TIMER_CONTROL_REG = flag | reload;
  return;
}

void local_timer_disable(){
  *LOCAL_TIMER_CONTROL_REG = 0;
  return;
}

void local_timer_handler(){
  *LOCAL_TIMER_IRQ_CLR = 0xc0000000; // clear interrupt and reload timer
  return;
}

void sched_core_timer_handler() {
  unsigned long r;
  asm volatile ("mov %0, #0x300000" : "=r"(r));
  asm volatile ("msr cntp_tval_el0, %0" : "=r"(r));
  timer_tick();
}
