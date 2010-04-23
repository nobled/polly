; RUN: opt -polly-scop-info  -analyze %s | FileCheck %s

;void f(int a[], int N) {
;  int i;
;  for (i = 0; i < 128; ++i)
;    ...
;}

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128"
target triple = "x86_64-unknown-linux-gnu"

define void @f(i32* nocapture %a, i32 %N) nounwind {
entry:
  br label %bb

bb:                                               ; preds = %bb, %entry
  %indvar = phi i64 [ 0, %entry ], [ %indvar.next, %bb ] ; <i64> [#uses=3]
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 128       ; <i1> [#uses=1]
  br i1 %exitcond, label %return, label %bb

return:                                           ; preds = %bb
  ret void
}

; CHECK: SCoP: entry => <Function Return>        Parameters: {}
