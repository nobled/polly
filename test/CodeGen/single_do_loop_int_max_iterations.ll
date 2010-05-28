; RUN: opt -polly-print-scop -S -analyze < %s | FileCheck %s
; ModuleID = 'single_do_loop_int_max_iterations.s'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define i32 @main() nounwind {
entry:
  %A = alloca [20 x i32], align 4                 ; <[20 x i32]*> [#uses=3]
  %arraydecay = getelementptr inbounds [20 x i32]* %A, i32 0, i32 0 ; <i32*> [#uses=1]
  %arrayidx = getelementptr inbounds i32* %arraydecay, i64 0 ; <i32*> [#uses=1]
  store i32 0, i32* %arrayidx
  call void @llvm.memory.barrier(i1 true, i1 true, i1 true, i1 true, i1 false)
  br label %do.body

do.body:                                          ; preds = %do.cond, %entry
  %0 = phi i32 [ 0, %entry ], [ %inc, %do.cond ]  ; <i32> [#uses=2]
  %arraydecay1 = getelementptr inbounds [20 x i32]* %A, i32 0, i32 0 ; <i32*> [#uses=1]
  %arrayidx2 = getelementptr inbounds i32* %arraydecay1, i64 0 ; <i32*> [#uses=1]
  store i32 %0, i32* %arrayidx2
  %inc = add nsw i32 %0, 1                        ; <i32> [#uses=2]
  br label %do.cond

do.cond:                                          ; preds = %do.body
  %exitcond = icmp ne i32 %inc, 2147483647        ; <i1> [#uses=1]
  br i1 %exitcond, label %do.body, label %do.end

do.end:                                           ; preds = %do.cond
  call void @llvm.memory.barrier(i1 true, i1 true, i1 true, i1 true, i1 false)
  %arraydecay5 = getelementptr inbounds [20 x i32]* %A, i32 0, i32 0 ; <i32*> [#uses=1]
  %arrayidx6 = getelementptr inbounds i32* %arraydecay5, i64 0 ; <i32*> [#uses=1]
  %tmp7 = load i32* %arrayidx6                    ; <i32> [#uses=1]
  %cmp8 = icmp eq i32 %tmp7, 2147483646           ; <i1> [#uses=1]
  br i1 %cmp8, label %if.then, label %if.else

if.then:                                          ; preds = %do.end
  br label %return

if.else:                                          ; preds = %do.end
  br label %return

return:                                           ; preds = %if.else, %if.then
  %retval.0 = phi i32 [ 0, %if.then ], [ 1, %if.else ] ; <i32> [#uses=1]
  ret i32 %retval.0
}

declare void @llvm.memory.barrier(i1, i1, i1, i1, i1) nounwind
; CHECK:for (s1=0;s1<=2147483646;s1++) {
