; RUN: opt -O3 -polly-print-scop -S -analyze < %s | FileCheck %s
int bar1();
int bar2();
int bar3();
int k;

void foo () {
  int i, j;

  for (i = 0; i < 50; i++) {

      bar2();

      for (j = 0; j < 200; j++)
        bar3();
  }

  return;
}
; CHECK: for (s1=0;s1<=199;s1++) {
; CHECK:   S0();
; CHECK: }
; CHECK: for (s1=0;s1<=49;s1++) {
; CHECK:       S0(s1);
; CHECK:         for (s3=0;s3<=199;s3++) {
; CHECK:                 S1(s1,s3);
; CHECK:                   }
; CHECK:           S2(s1);
; CHECK: }

