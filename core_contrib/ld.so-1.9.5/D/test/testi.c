#include <stdio.h>

int main(void)
{
  int i;

  printf("Testing ...\n");

  for (i = 10; i >= 0; i--)
    printf("\t%d\n", i);

  printf("Boom!\n");

  return 0;
}
