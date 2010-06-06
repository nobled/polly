; RUN: opt -polly-scop-condition -analyze < %s | FileCheck %s
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-linux-gnu"

define void @f(i32 %N) nounwind {
entry:
  %0 = icmp sgt i32 %N, 0                         ; <i1> [#uses=1]
  br i1 %0, label %bb, label %return

bb:                                               ; preds = %entry, %bb
  %1 = phi i32 [ %3, %bb ], [ 0, %entry ]         ; <i32> [#uses=2]
  %2 = tail call i32 @rnd(i32 %1) nounwind        ; <i32> [#uses=0]
  %3 = add nsw i32 %1, 1                          ; <i32> [#uses=2]
  %exitcond = icmp eq i32 %3, %N                  ; <i1> [#uses=1]
  br i1 %exitcond, label %return, label %bb

return:                                           ; preds = %bb, %entry
  ret void
}

declare i32 @rnd(i32)

; CHECK: Condition of BB: entry is
; CHECK:   True
; CHECK: Condition of BB: bb is
; CHECK:   {%0,!%exitcond}<%bb>
; CHECK: Condition of BB: return is
; CHECK:   (!%0,||,(%0,&&,%exitcond))
