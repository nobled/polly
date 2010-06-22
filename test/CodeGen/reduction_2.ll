; RUN: opt -polly-print-scop -S -analyze < %s | FileCheck %s
; RUN: opt -polly-codegen < %s | lli
; ModuleID = 'reduction_2.s'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define i32 @main() nounwind {
entry:
  %A = alloca [1021 x i32], align 4               ; <[1021 x i32]*> [#uses=6]
  %RED = alloca [1 x i32], align 4                ; <[1 x i32]*> [#uses=3]
  %arraydecay = getelementptr inbounds [1021 x i32]* %A, i32 0, i32 0 ; <i32*> [#uses=1]
  %conv = bitcast i32* %arraydecay to i8*         ; <i8*> [#uses=1]
  call void @llvm.memset.p0i8.i64(i8* %conv, i8 0, i64 4084, i32 1, i1 false)
  %arraydecay1 = getelementptr inbounds [1021 x i32]* %A, i32 0, i32 0 ; <i32*> [#uses=1]
  %arrayidx = getelementptr inbounds i32* %arraydecay1, i64 0 ; <i32*> [#uses=1]
  store i32 1, i32* %arrayidx
  %arraydecay2 = getelementptr inbounds [1021 x i32]* %A, i32 0, i32 0 ; <i32*> [#uses=1]
  %arrayidx3 = getelementptr inbounds i32* %arraydecay2, i64 1 ; <i32*> [#uses=1]
  store i32 1, i32* %arrayidx3
  %arraydecay4 = getelementptr inbounds [1 x i32]* %RED, i32 0, i32 0 ; <i32*> [#uses=1]
  %arrayidx5 = getelementptr inbounds i32* %arraydecay4, i64 0 ; <i32*> [#uses=1]
  store i32 0, i32* %arrayidx5
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %indvar = phi i64 [ %indvar.next, %for.inc ], [ 0, %entry ] ; <i64> [#uses=5]
  %arrayidx15 = getelementptr [1021 x i32]* %A, i64 0, i64 %indvar ; <i32*> [#uses=2]
  %tmp = add i64 %indvar, 2                       ; <i64> [#uses=1]
  %arrayidx20 = getelementptr [1021 x i32]* %A, i64 0, i64 %tmp ; <i32*> [#uses=1]
  %tmp1 = add i64 %indvar, 1                      ; <i64> [#uses=1]
  %arrayidx9 = getelementptr [1021 x i32]* %A, i64 0, i64 %tmp1 ; <i32*> [#uses=1]
  %exitcond = icmp ne i64 %indvar, 1019           ; <i1> [#uses=1]
  br i1 %exitcond, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %tmp10 = load i32* %arrayidx9                   ; <i32> [#uses=1]
  %tmp16 = load i32* %arrayidx15                  ; <i32> [#uses=1]
  %add = add nsw i32 %tmp10, %tmp16               ; <i32> [#uses=1]
  store i32 %add, i32* %arrayidx20
  %tmp26 = load i32* %arrayidx15                  ; <i32> [#uses=1]
  %arraydecay27 = getelementptr inbounds [1 x i32]* %RED, i32 0, i32 0 ; <i32*> [#uses=1]
  %arrayidx28 = getelementptr inbounds i32* %arraydecay27, i64 0 ; <i32*> [#uses=2]
  %tmp29 = load i32* %arrayidx28                  ; <i32> [#uses=1]
  %add30 = add nsw i32 %tmp29, %tmp26             ; <i32> [#uses=1]
  store i32 %add30, i32* %arrayidx28
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=1]
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %arraydecay32 = getelementptr inbounds [1 x i32]* %RED, i32 0, i32 0 ; <i32*> [#uses=1]
  %arrayidx33 = getelementptr inbounds i32* %arraydecay32, i64 0 ; <i32*> [#uses=1]
  %tmp34 = load i32* %arrayidx33                  ; <i32> [#uses=1]
  %cmp35 = icmp ne i32 %tmp34, 382399368          ; <i1> [#uses=1]
  br i1 %cmp35, label %if.then, label %if.end

if.then:                                          ; preds = %for.end
  br label %if.end

if.end:                                           ; preds = %if.then, %for.end
  %retval.0 = phi i32 [ 1, %if.then ], [ 0, %for.end ] ; <i32> [#uses=1]
  ret i32 %retval.0
}

declare void @llvm.memset.p0i8.i64(i8* nocapture, i8, i64, i32, i1) nounwind

; CHECK: for (s1=0;s1<=1018;s1++) {
; CHECK:     S0(s1);
; CHECK: }
