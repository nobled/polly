void bar();

void foo(int a) {

  int i, j, k;
  int A[100];

  bar();

  for (int i = 0; i <= a; i++)
    A[i] = 0;

  bar();

  for (int i = 0; i <= a; i++)
    for (int j = 0; j <= 80; j++)
      A[i] += i * j;

  bar();
}
