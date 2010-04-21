; RUN: opt -polly-scop-info -print-loop-params -analyze %s | FileCheck %s
;void f(int a[], int N) {
;  int i;
;  for (i = 0; i < N; ++i)
;    ...
;}

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128"
target triple = "x86_64-unknown-linux-gnu"

define void @f(i32* nocapture %a, i32 %N) nounwind {
entry:
  %0 = icmp sgt i32 %N, 0                         ; <i1> [#uses=1]
  br i1 %0, label %bb.nph, label %return

bb.nph:                                           ; preds = %entry
  %tmp = zext i32 %N to i64                       ; <i64> [#uses=1]
  br label %bb

bb:                                               ; preds = %bb, %bb.nph
  %indvar = phi i64 [ 0, %bb.nph ], [ %indvar.next, %bb ] ; <i64> [#uses=3]
  %scevgep = getelementptr i32* %a, i64 %indvar   ; <i32*> [#uses=1]
  %indvar6 = trunc i64 %indvar to i32             ; <i32> [#uses=1]
  %tmp5 = add i32 %indvar6, %N                    ; <i32> [#uses=1]
  store i32 %tmp5, i32* %scevgep, align 4
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, %tmp      ; <i1> [#uses=1]
  br i1 %exitcond, label %return, label %bb

return:                                           ; preds = %bb, %entry
  ret void
}

; CHECK: Parameters used in Loop: bb:    (zext i32 %N to i64),
