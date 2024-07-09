#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

#define IPSEC 1

#if IPSEC
int nrl_strtoreq(char *str, void **req, int *reqlen)
{
  int request[4];
  int *cur = NULL;

  memset(&request, 0, sizeof(request));

  while(*str) {
    switch(*str) {
      case 'A':
        cur = &request[1];
        break;
      case 'E':
        cur = &request[2];
        break;
      case 'N':
      case 'T':
        cur = &request[3];
        break;
      case '0':
      case 'd':
        if (!cur)
          return -1;
        *cur = 0;
        break;
      case '1':
      case 'u':
        if (!cur)
          return -1;
        *cur = 1;
        break;
      case '2':
      case 'r':
        if (!cur)
          return -1;
        *cur = 2;
        break;
      case '3':
      case 'q':
        if (!cur)
          return -1;
        *cur = 3;
        break;
      case '-':
        if (*(++str) != '1')
          return -1;
      case 'n':
        if (!cur)
          return -1;
        *cur = -1;
        break;
      default:
        return -1;
    };
    str++;
  };

  if (!(*req = malloc(sizeof(request))))
    return -1;

  memcpy(*req, request, sizeof(request));
  *reqlen = sizeof(request);

  return 0;
};

static int level(int value)
{
  switch(value) {
    case -1:
      return 'n';
    case 0:
      return 'd';
    case 1:
      return 'u';
    case 2:
      return 'r';
    case 3:
      return 'q';
    default:
      return -1;
  };
};

int nrl_reqtostr(void *req, int reqlen, char *str, int strsize)
{
  int i;

  if (strsize < 7)
    return -1;

  str[0] = 'A';
  if ((i = level(((int *)req)[1])) < 0)
    return -1;
  str[1] = i;

  str[2] = 'E';
  if ((i = level(((int *)req)[2])) < 0)
    return -1;
  str[3] = i;

  str[4] = 'T';
  if ((i = level(((int *)req)[3])) < 0)
    return -1;
  str[5] = i;

  str[6] = 0;

  return 0;
};
#endif /* IPSEC */
