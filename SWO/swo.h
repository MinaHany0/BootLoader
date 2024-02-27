#ifndef __SWO_H
#define __SWO_H

#include <stdio.h>

FILE __stdout;
FILE __stdin;

int fputc(int ch, FILE *f);

int fputc(int ch, FILE *f)
{
  ITM_SendChar(ch);
  return(ch);
}

#endif /*__SWO_H*/