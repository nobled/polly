; RUN: %opt -polly-analyze-ir -analyze < %s 2>&1 | FileCheck %s
; ModuleID = 'reduction-with-added-immediate.s'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-pc-linux-gnu"

@A = common global [128 x i32] zeroinitializer, align 16
@R = common global i32 0, align 4

define i32 @not_a_reduction() nounwind {
bb:
  br label %bb1

bb1:                                              ; preds = %bb6, %bb
  %indvar = phi i64 [ %indvar.next, %bb6 ], [ 0, %bb ]
  %scevgep = getelementptr [128 x i32]* @A, i64 0, i64 %indvar
  %exitcond = icmp ne i64 %indvar, 128
  br i1 %exitcond, label %bb2, label %bb7

bb2:                                              ; preds = %bb1
  %tmp = load i32* %scevgep, align 4
  %tmp3 = add nsw i32 %tmp, 1
  %tmp4 = load i32* @R, align 4
  %tmp5 = add nsw i32 %tmp4, %tmp3
  store i32 %tmp5, i32* @R, align 4
  br label %bb6

bb6:                                              ; preds = %bb2
  %indvar.next = add i64 %indvar, 1
  br label %bb1

bb7:                                              ; preds = %bb1
  %tmp8 = load i32* @R, align 4
  ret i32 %tmp8
}

; CHECK:     Reduction
