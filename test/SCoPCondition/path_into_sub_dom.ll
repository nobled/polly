; RUN: opt -polly-scop-condition -analyze < %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-linux-gnu"

define i32 @f() nounwind {
entry:
  %0 = tail call i32 @rnd(i32 1) nounwind         ; <i32> [#uses=1]
  %1 = icmp eq i32 %0, 0                          ; <i1> [#uses=1]
  br i1 %1, label %bb3, label %bb

bb:                                               ; preds = %entry
  %2 = tail call i32 @rnd(i32 2) nounwind         ; <i32> [#uses=1]
  %3 = icmp eq i32 %2, 0                          ; <i1> [#uses=1]
  br i1 %3, label %bb2, label %bb1

bb1:                                              ; preds = %bb
  %4 = tail call i32 @rnd(i32 3) nounwind         ; <i32> [#uses=0]
  br label %bb4

bb2:                                              ; preds = %bb
  %5 = tail call i32 @rnd(i32 5) nounwind         ; <i32> [#uses=0]
  br label %bb4

bb3:                                              ; preds = %entry
  %6 = tail call i32 @rnd(i32 4) nounwind         ; <i32> [#uses=0]
  br label %bb4

bb4:                                              ; preds = %bb3, %bb2, %bb1
  %7 = tail call i32 @rnd(i32 5) nounwind         ; <i32> [#uses=1]
  ret i32 %7
}

declare i32 @rnd(i32)

; CHECK: Condition of BB: entry is
; CHECK:   True
; CHECK: Condition of BB: bb is
; CHECK:   !%1
; CHECK: Condition of BB: bb1 is
; CHECK:   (!%1,&&,!%3)
; CHECK: Condition of BB: bb2 is
; CHECK:   (!%1,&&,%3)
; CHECK: Condition of BB: bb3 is
; CHECK:   %1
; CHECK: Condition of BB: bb4 is
; CHECK:   (%1,||,(!%1,&&,%3),||,(!%1,&&,!%3))
