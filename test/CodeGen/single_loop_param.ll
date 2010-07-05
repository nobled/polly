; RUN: opt -polly-print -S < %s | FileCheck %s
; RUN: opt -polly-codegen < %s | lli -
; ModuleID = 'single_loop_param.s'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-pc-linux-gnu"

@A = common global [1024 x i32] zeroinitializer, align 16 ; <[1024 x i32]*> [#uses=4]

define void @bar(i64 %n) nounwind {
; <label>:0
  call void @llvm.memory.barrier(i1 true, i1 true, i1 true, i1 true, i1 false)
  br label %1

; <label>:1                                       ; preds = %3, %0
  %i.0 = phi i64 [ 0, %0 ], [ %4, %3 ]            ; <i64> [#uses=3]
  %scevgep = getelementptr [1024 x i32]* @A, i64 0, i64 %i.0 ; <i32*> [#uses=1]
  %exitcond = icmp ne i64 %i.0, %n                ; <i1> [#uses=1]
  br i1 %exitcond, label %2, label %5

; <label>:2                                       ; preds = %1
  store i32 1, i32* %scevgep
  br label %3

; <label>:3                                       ; preds = %2
  %4 = add nsw i64 %i.0, 1                        ; <i64> [#uses=1]
  br label %1

; <label>:5                                       ; preds = %1
  call void @llvm.memory.barrier(i1 true, i1 true, i1 true, i1 true, i1 false)
  ret void
}

declare void @llvm.memory.barrier(i1, i1, i1, i1, i1) nounwind

define i32 @main() nounwind {
; <label>:0
  call void @llvm.memset.p0i8.i64(i8* bitcast ([1024 x i32]* @A to i8*), i8 0, i64 4096, i32 1, i1 false)
  call void @bar(i64 1024)
  br label %1

; <label>:1                                       ; preds = %8, %0
  %indvar = phi i64 [ %indvar.next, %8 ], [ 0, %0 ] ; <i64> [#uses=3]
  %scevgep = getelementptr [1024 x i32]* @A, i64 0, i64 %indvar ; <i32*> [#uses=1]
  %i.0 = trunc i64 %indvar to i32                 ; <i32> [#uses=1]
  %2 = icmp slt i32 %i.0, 1024                    ; <i1> [#uses=1]
  br i1 %2, label %3, label %9

; <label>:3                                       ; preds = %1
  %4 = load i32* %scevgep                         ; <i32> [#uses=1]
  %5 = icmp ne i32 %4, 1                          ; <i1> [#uses=1]
  br i1 %5, label %6, label %7

; <label>:6                                       ; preds = %3
  br label %10

; <label>:7                                       ; preds = %3
  br label %8

; <label>:8                                       ; preds = %7
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=1]
  br label %1

; <label>:9                                       ; preds = %1
  br label %10

; <label>:10                                      ; preds = %9, %6
  %.0 = phi i32 [ 1, %6 ], [ 0, %9 ]              ; <i32> [#uses=1]
  ret i32 %.0
}

declare void @llvm.memset.p0i8.i64(i8* nocapture, i8, i64, i32, i1) nounwind
; CHECK: if (p0 >= 1) {
; CHECK:     for (s1=0;s1<=p0-1;s1++) {
; CHECK:           S0(s1);
; CHECK:             }
; CHECK: }

