#include <stdio.h>
#include <math.h>

int main(void)
{
  int i;

  printf("Testing ...\n");

  for (i = 10; i >= 0; i--)
    printf("\t%g\n", pow(10.0, i));

  printf("Boom!\n");

  return 0;
}
