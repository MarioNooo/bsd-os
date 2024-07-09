/* Parse comma-separated name/value pairs */

#include <stdlib.h>
#include <string.h>


/* Parse comma-separated suboption from '*pOptions' and match 
   against strings in 'names'.  

   If found return index and set '*pValue' to optional value 
   following an equal sign.  

   If the suboption is not found in 'names' return in '*pValue'
   the unknown suboption.  

   On exit '*pOptions' is set to the beginning of the next token 
   or at the terminating NUL character.  */
int 
get_sub_option (pOptions, names, pValue)
     char **pOptions;
     const char *const *names;
     char **pValue;
{
  char *pNamStart, *pNamEnd, *pValStart, *pValEnd;
  int nameX, nameLen, valLen;

  if (pOptions == NULL || *pOptions == NULL || **pOptions == '\0')
    return -1;

  /* Get current name  */
  pNamStart = *pOptions;
  for (pNamEnd = pNamStart; 
       *pNamEnd != '\0' && *pNamEnd != '=' && *pNamEnd != ',';  
	   ++pNamEnd);

  /* Skip any leading blanks before the name */
  while (*pNamStart != '\0' && *pNamStart == ' ')
    pNamStart++;

  nameLen = pNamEnd - pNamStart;

  /* Get current value (if any) */

  pValStart = *pNamEnd == '=' ? pNamEnd : NULL;
  if (pValStart != NULL)
    {
	  pValStart++;
      pValEnd = strchr (pValStart, ',');
      if (pValEnd == NULL)
        pValEnd = strchr (pValStart, '\0');

	  valLen = pValEnd - pValStart;
    }
  else
	{
    pValEnd = pValStart;
    valLen = 0;
	}

  /* See if the name matches */
  for (nameX = 0; names[nameX] != NULL; ++nameX)
    if (memcmp (pNamStart, names[nameX], nameLen) == 0
        && names[nameX] [nameLen] == '\0')
      {
        /* We have a name match */
        if (pValStart != NULL)
          {
          *pValue = pValStart;
          *pOptions = *pValEnd != '\0' ? pValEnd + 1 : pValEnd;
		  *(pValStart + valLen) = '\0';
          }
        else
          {
          *pValue = NULL;
          *pOptions = *pNamEnd != '\0' ? pNamEnd + 1 : pNamEnd;
          }
        return nameX;
      }

  /* We did not find a name match */
  *pValue = *pOptions;

  if (pValStart == NULL)
    pValEnd = pNamEnd;
  else
	*(pValStart + valLen) = '\0';

  *pOptions = *pValEnd != '\0' ? pValEnd + 1 : pValEnd;

  return -1;
}
