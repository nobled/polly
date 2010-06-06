; RUN: opt -polly-scop-condition -analyze < %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define i32 @f() nounwind {
entry:
  %retval = alloca i32                            ; <i32*> [#uses=2]
  %0 = alloca i32                                 ; <i32*> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %1 = call i32 @rnd(i32 1) nounwind              ; <i32> [#uses=1]
  %2 = icmp ne i32 %1, 0                          ; <i1> [#uses=1]
  br i1 %2, label %bb, label %bb4

bb:                                               ; preds = %entry
  %3 = call i32 @rnd(i32 2) nounwind              ; <i32> [#uses=1]
  %4 = icmp ne i32 %3, 0                          ; <i1> [#uses=1]
  br i1 %4, label %bb1, label %bb2

bb1:                                              ; preds = %bb
  %5 = call i32 @rnd(i32 3) nounwind              ; <i32> [#uses=0]
  br label %bb3

bb2:                                              ; preds = %bb
  %6 = call i32 @rnd(i32 6) nounwind              ; <i32> [#uses=0]
  br label %bb3

bb3:                                              ; preds = %bb2, %bb1
  %7 = call i32 @rnd(i32 7) nounwind              ; <i32> [#uses=0]
  br label %bb5

bb4:                                              ; preds = %entry
  %8 = call i32 @rnd(i32 4) nounwind              ; <i32> [#uses=0]
  br label %bb5

bb5:                                              ; preds = %bb4, %bb3
  %9 = call i32 @rnd(i32 5) nounwind              ; <i32> [#uses=1]
  store i32 %9, i32* %0, align 4
  %10 = load i32* %0, align 4                     ; <i32> [#uses=1]
  store i32 %10, i32* %retval, align 4
  br label %return

return:                                           ; preds = %bb5
  %retval6 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval6
}

declare i32 @rnd(i32)

; CHECK: Condition of BB: entry is
; CHECK:   True
; CHECK: Condition of BB: bb is
; CHECK:   %2
; CHECK: Condition of BB: bb1 is
; CHECK:   (%2,&&,%4)
; CHECK: Condition of BB: bb2 is
; CHECK:   (%2,&&,!%4)
; CHECK: Condition of BB: bb3 is
; CHECK:   %2
; CHECK: Condition of BB: bb4 is
; CHECK:   !%2
; CHECK: Condition of BB: bb5 is
; CHECK:   True
; CHECK: Condition of BB: return is
; CHECK:   True
