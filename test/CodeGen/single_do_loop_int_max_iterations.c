#define N 20
#include "limits.h"
volatile  int A[N];

int main () {
  int i;

  A[0] = 0;

  __sync_synchronize();

  i = 0;

  do {
    A[0] = i;
    ++i;
  } while (i < INT_MAX);

  __sync_synchronize();

  if (A[0] == INT_MAX - 1)
    return 0;
  else
    return 1;
}
