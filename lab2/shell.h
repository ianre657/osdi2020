#ifndef SHELL
#define SHELL

#define CMD_SIZE 0x20

void shell_interactive ();
double get_time ();
void reset (int tick);
void cancel_reset ();
void hardware ();
void picture ();
void ftoa (double val, char *buf);
void loadimg (unsigned long address)
  __attribute__ ((section (".bootloader")));
void loadimg_jmp (void *address, unsigned long img_size)
  __attribute__ ((section (".bootloader")));
#endif /* ifndef SHELL */
