#include "ctype.h"
#include "stdlib.h"

long
strtol (const char *nptr, char **endptr, int base)
{
  long result = 0;
  int sign = 1;
  int digit;

  // Skip whitespace
  while (isspace (*nptr))
    nptr++;

  // Check for sign
  if (*nptr == '-')
    {
      sign = -1;
      nptr++;
    }
  else if (*nptr == '+')
    nptr++;

  // Check for base prefix
  if (base == 0)
    {
      if (*nptr == '0')
        {
          if ((*(nptr + 1) | 32) == 'x')
            { // Check for "0x" or "0X"
              base = 16;
              nptr += 2;
            }
          else
            base = 8;
        }
      else
        base = 10;
    }
  else if (base == 16 && (*nptr == '0' && (*(nptr + 1) | 32) == 'x'))
    nptr += 2;

  // Convert string to long
  while (1)
    {
      if (*nptr >= '0' && *nptr <= '9')
        digit = *nptr - '0';
      else if (*nptr >= 'A' && *nptr <= 'Z')
        digit = *nptr - 'A' + 10;
      else if (*nptr >= 'a' && *nptr <= 'z')
        digit = *nptr - 'a' + 10;
      else
        break;

      if (digit >= base)
        break;

      result = result * base + digit;
      nptr++;
    }

  // Set end pointer
  if (endptr != NULL)
    *endptr = (char *)nptr;

  return result * sign;
}
