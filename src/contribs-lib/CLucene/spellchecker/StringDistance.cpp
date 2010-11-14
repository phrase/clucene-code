#include "StringDistance.h"

int minimum(int a, int b)
{
  if (a > b)
    return b;
  else
    return a;
}

int maximum(int a, int b)
{
  if (a < b)
    return b;
  else
    return a;
}
