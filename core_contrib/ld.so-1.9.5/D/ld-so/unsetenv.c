void unsetenv(char *symbol)
{
  extern char **__environ;
  char **envp = __environ;
  char *pnt;
  char *pnt1;
  char **newenvp = envp;
  for (pnt = *envp; pnt; pnt = *++envp) {
    pnt1 = symbol;
    while (*pnt && *pnt == *pnt1)
      pnt1++, pnt++;
    if(!*pnt || *pnt != '=' || *pnt1)
      *newenvp++ = *envp;
  }
  *newenvp++ = *envp;
  return;
}
