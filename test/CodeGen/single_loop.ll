; RUN: opt -O3 -polly-print-scop -S -analyze < %s | FileCheck %s
; RUN: opt -O3 -polly-codegen < %s | lli -
; XFAIL: *
; ModuleID = 'single_loop.c'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define i32 @main() nounwind {
entry:
  %retval = alloca i32, align 4                   ; <i32*> [#uses=4]
  %i = alloca i32, align 4                        ; <i32*> [#uses=10]
  %A = alloca [1024 x i32], align 4               ; <[1024 x i32]*> [#uses=3]
  store i32 0, i32* %retval
  %arraydecay = getelementptr inbounds [1024 x i32]* %A, i32 0, i32 0 ; <i32*> [#uses=1]
  %conv = bitcast i32* %arraydecay to i8*         ; <i8*> [#uses=1]
  call void @llvm.memset.i64(i8* %conv, i8 0, i64 4096, i32 1)
  store i32 0, i32* %i
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %tmp = load i32* %i                             ; <i32> [#uses=1]
  %cmp = icmp slt i32 %tmp, 1024                  ; <i1> [#uses=1]
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %tmp2 = load i32* %i                            ; <i32> [#uses=1]
  %arraydecay3 = getelementptr inbounds [1024 x i32]* %A, i32 0, i32 0 ; <i32*> [#uses=1]
  %idxprom = sext i32 %tmp2 to i64                ; <i64> [#uses=1]
  %arrayidx = getelementptr inbounds i32* %arraydecay3, i64 %idxprom ; <i32*> [#uses=1]
  store i32 1, i32* %arrayidx
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %tmp4 = load i32* %i                            ; <i32> [#uses=1]
  %inc = add nsw i32 %tmp4, 1                     ; <i32> [#uses=1]
  store i32 %inc, i32* %i
  br label %for.cond

for.end:                                          ; preds = %for.cond
  store i32 0, i32* %i
  br label %for.cond5

for.cond5:                                        ; preds = %for.inc17, %for.end
  %tmp6 = load i32* %i                            ; <i32> [#uses=1]
  %cmp7 = icmp slt i32 %tmp6, 1024                ; <i1> [#uses=1]
  br i1 %cmp7, label %for.body9, label %for.end20

for.body9:                                        ; preds = %for.cond5
  %tmp10 = load i32* %i                           ; <i32> [#uses=1]
  %arraydecay11 = getelementptr inbounds [1024 x i32]* %A, i32 0, i32 0 ; <i32*> [#uses=1]
  %idxprom12 = sext i32 %tmp10 to i64             ; <i64> [#uses=1]
  %arrayidx13 = getelementptr inbounds i32* %arraydecay11, i64 %idxprom12 ; <i32*> [#uses=1]
  %tmp14 = load i32* %arrayidx13                  ; <i32> [#uses=1]
  %cmp15 = icmp ne i32 %tmp14, 1                  ; <i1> [#uses=1]
  br i1 %cmp15, label %if.then, label %if.end

if.then:                                          ; preds = %for.body9
  store i32 1, i32* %retval
  br label %return

if.end:                                           ; preds = %for.body9
  br label %for.inc17

for.inc17:                                        ; preds = %if.end
  %tmp18 = load i32* %i                           ; <i32> [#uses=1]
  %inc19 = add nsw i32 %tmp18, 1                  ; <i32> [#uses=1]
  store i32 %inc19, i32* %i
  br label %for.cond5

for.end20:                                        ; preds = %for.cond5
  store i32 0, i32* %retval
  br label %return

return:                                           ; preds = %for.end20, %if.then
  %0 = load i32* %retval                          ; <i32> [#uses=1]
  ret i32 %0
}

declare void @llvm.memset.i64(i8* nocapture, i8, i64, i32) nounwind

; CHECK:for (s1=0;s1<=1023;s1++) {
; CHECK:    S0(s1);
; CHECK:}
