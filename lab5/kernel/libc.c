#include "libc.h"
#include "sys.h"

extern void uart_puts(char *s);
extern void uart_send(unsigned int c);
extern void halt();

char *itoa(int val, int base) {
  static char buf[32] = {0};
  int i = 30;
  if (val == 0) {
    buf[i] = '0';
    return &buf[i];
  }

  for (; val && i; --i, val /= base)
    buf[i] = "0123456789abcdef"[val % base];
  return &buf[i + 1];
}

size_t strlen(const char *str) {
  const char *s;

  for (s = str; *s; ++s) ;
  return (s - str);
}

char *strcat(char *dest, const char *src) {
  char *p = dest + strlen(dest);
  while (*src != 0) {
    *p++ = *src++;
  }
  *p = 0;
  return dest;
}

int sstrcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return *s1 - *s2;
}

void *memset(void *s, int c, size_t n) {
  unsigned char *p = (unsigned char *)s;

  while (n--)
    *p++ = (unsigned char)c;

  return s;
}

void memcpy(void *dest, const void *src, size_t num) {
  uint8_t *dest8 = (uint8_t *)dest;
  const uint8_t *src8 = (const uint8_t *)src;
  while (num--) {
    *dest8++ = *src8++;
  }
}

/* void __attribute__((noreturn)) abort() { */
/*   uart_puts("System aborted\r\n"); */
/*   halt(); */
/* } */

void uart_print(char *format, ...) {
  char print_buf[1024] = {
                          0,
  };
  int buf_i = 0;
  unsigned int i;
  unsigned int n;
  char *s;

  va_list arg;
  va_start(arg, format);

  for (char *traverse = format; *traverse != 0; traverse++) {
    while (*traverse != '%' && *traverse != 0) {
      /* uart_send(*traverse++); */
      print_buf[buf_i++] = *traverse++;
    }

    if (*traverse == 0)
      break;

    /* move to the hole */
    traverse++;
    /* actions */
    switch (*traverse) {
    case 'c':
      i = va_arg(arg, int);
      print_buf[buf_i++] = i;
      // uart_send(i);
      break;
    case 'd':
      i = va_arg(arg, int);
      if (i < 0) {
        i = -i;
        print_buf[buf_i++] = '-';
        // uart_send('-');
      }
      char *x = itoa(i, 10);
      while (*x != 0) {
        print_buf[buf_i++] = *x++;
      }
      // uart_puts(itoa(i, 10));
      break;
    case 'x':
      i = va_arg(arg, unsigned int);
      for (int c = 28; c >= 0; c -= 4) {
        /* get highest tetrad */
        n = (i >> c) & 0xF;
        /* 0-9 => '0'-'9', 10-15 => 'A'-'F' */
        n += (n > 9) ? 0x37 : 0x30;
        print_buf[buf_i++] = n;
        // uart_send(n);
      }
      break;
    case 's':
      s = va_arg(arg, char *);
      while (*s != 0) {
        print_buf[buf_i++] = *s++;
      }
      // uart_puts(s);
      break;
    default:
      print_buf[buf_i++] = '%';
      print_buf[buf_i++] = *traverse;
      // uart_send('%');
      // uart_send(*traverse);
      break;
    }
  }

  print_buf[buf_i] = 0;
  /* call_sys_write(print_buf); */
  uart_puts(print_buf);
  buf_i = 0;

  va_end(arg);
}

void sys_print(char *format, ...) {
  char print_buf[1024] = {0,};
  int buf_i = 0;
  unsigned int i;
  unsigned int n;
  char *s;

  va_list arg;
  va_start(arg, format);

  for (char *traverse = format; *traverse != 0; traverse++) {
    while (*traverse != '%' && *traverse != 0) {
      /* uart_send(*traverse++); */
      print_buf[buf_i++] = *traverse++;
    }

    if (*traverse == 0)
      break;

    /* move to the hole */
    traverse++;
    /* actions */
    switch (*traverse) {
    case 'c':
      i = va_arg(arg, int);
      print_buf[buf_i++] = i;
      // uart_send(i);
      break;
    case 'd':
      i = va_arg(arg, int);
      if (i < 0) {
        i = -i;
        print_buf[buf_i++] = '-';
        // uart_send('-');
      }
      char *x = itoa(i, 10);
      while (*x != 0) {
        print_buf[buf_i++] = *x++;
      }
      // uart_puts(itoa(i, 10));
      break;
    case 'x':
      i = va_arg(arg, unsigned int);
      for (int c = 28; c >= 0; c -= 4) {
        /* get highest tetrad */
        n = (i >> c) & 0xF;
        /* 0-9 => '0'-'9', 10-15 => 'A'-'F' */
        n += (n > 9) ? 0x37 : 0x30;
        print_buf[buf_i++] = n;
        // uart_send(n);
      }
      break;
    case 's':
      s = va_arg(arg, char *);
      while (*s != 0) {
        print_buf[buf_i++] = *s++;
      }
      // uart_puts(s);
      break;
    default:
      print_buf[buf_i++] = '%';
      print_buf[buf_i++] = *traverse;
      // uart_send('%');
      // uart_send(*traverse);
      break;
    }
  }

  print_buf[buf_i] = 0;
  /* call_sys_write(print_buf); */
  call_sys_write(print_buf);
  buf_i = 0;

  va_end(arg);
}

void uart_println(char *format, ...) {
  uart_print(format);
  uart_puts("\r\n");
}

void sys_println(char *format, ...) {
  char print_buf[1024] = {
      0,
  };
  int buf_i = 0;
  unsigned int i;
  unsigned int n;
  char *s;

  va_list arg;
  va_start(arg, format);

  for (char *traverse = format; *traverse != 0; traverse++) {
    while (*traverse != '%' && *traverse != 0) {
      /* uart_send(*traverse++); */
      print_buf[buf_i++] = *traverse++;
    }

    if (*traverse == 0)
      break;

    /* move to the hole */
    traverse++;
    /* actions */
    switch (*traverse) {
    case 'c':
      i = va_arg(arg, int);
      print_buf[buf_i++] = i;
      // uart_send(i);
      break;
    case 'd':
      i = va_arg(arg, int);
      if (i < 0) {
        i = -i;
        print_buf[buf_i++] = '-';
        // uart_send('-');
      }
      char *x = itoa(i, 10);
      while (*x != 0) {
        print_buf[buf_i++] = *x++;
      }
      // uart_puts(itoa(i, 10));
      break;
    case 'x':
      i = va_arg(arg, unsigned int);
      for (int c = 28; c >= 0; c -= 4) {
        /* get highest tetrad */
        n = (i >> c) & 0xF;
        /* 0-9 => '0'-'9', 10-15 => 'A'-'F' */
        n += (n > 9) ? 0x37 : 0x30;
        print_buf[buf_i++] = n;
        // uart_send(n);
      }
      break;
    case 's':
      s = va_arg(arg, char *);
      while (*s != 0) {
        print_buf[buf_i++] = *s++;
      }
      // uart_puts(s);
      break;
    default:
      print_buf[buf_i++] = '%';
      print_buf[buf_i++] = *traverse;
      // uart_send('%');
      // uart_send(*traverse);
      break;
    }
  }

  print_buf[buf_i++] ='\r';
  print_buf[buf_i++] ='\n';
  print_buf[buf_i] = 0;
  call_sys_write(print_buf);

  buf_i = 0;

  va_end(arg);

}
