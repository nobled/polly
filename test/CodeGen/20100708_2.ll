; RUN: opt -polly-codegen < %s
; ModuleID = 'bugpoint-reduced-simplified.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-pc-linux-gnu"

define void @init_array() nounwind {
  br label %1

; <label>:1                                       ; preds = %5, %0
  br i1 undef, label %2, label %6

; <label>:2                                       ; preds = %3, %1
  %indvar = phi i64 [ %indvar.next, %3 ], [ 0, %1 ] ; <i64> [#uses=1]
  %tmp3 = trunc i64 undef to i32                  ; <i32> [#uses=1]
  br i1 false, label %3, label %5

; <label>:3                                       ; preds = %2
  %4 = srem i32 %tmp3, 1024                       ; <i32> [#uses=0]
  store double undef, double* undef
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=1]
  br label %2

; <label>:5                                       ; preds = %2
  br label %1

; <label>:6                                       ; preds = %1
  ret void
}
