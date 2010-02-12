; RUN: opt -O3 -polly-print-scop -S -analyze < %s | FileCheck %s
; ModuleID = 'test.c'
target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:64-f32:32:32-f64:32:64-v64:64:64-v128:128:128-a0:0:64-f80:32:32-n8:16:32"
target triple = "i386-portbld-freebsd8.0"

@k = common global i32 0, align 4                 ; <i32*> [#uses=0]

define void @foo() nounwind {
entry:
  %i = alloca i32, align 4                        ; <i32*> [#uses=4]
  %j = alloca i32, align 4                        ; <i32*> [#uses=4]
  store i32 0, i32* %i
  br label %for.cond

for.cond:                                         ; preds = %for.inc6, %entry
  %tmp = load i32* %i                             ; <i32> [#uses=1]
  %cmp = icmp slt i32 %tmp, 50                    ; <i1> [#uses=1]
  br i1 %cmp, label %for.body, label %for.end9

for.body:                                         ; preds = %for.cond
  store i32 0, i32* %j
  br label %for.cond1

for.cond1:                                        ; preds = %for.inc, %for.body
  %tmp2 = load i32* %j                            ; <i32> [#uses=1]
  %cmp3 = icmp slt i32 %tmp2, 200                 ; <i1> [#uses=1]
  br i1 %cmp3, label %for.body4, label %for.end

for.body4:                                        ; preds = %for.cond1
  %call = call i32 (...)* @bar3()                 ; <i32> [#uses=0]
  br label %for.inc

for.inc:                                          ; preds = %for.body4
  %tmp5 = load i32* %j                            ; <i32> [#uses=1]
  %inc = add nsw i32 %tmp5, 1                     ; <i32> [#uses=1]
  store i32 %inc, i32* %j
  br label %for.cond1

for.end:                                          ; preds = %for.cond1
  br label %for.inc6

for.inc6:                                         ; preds = %for.end
  %tmp7 = load i32* %i                            ; <i32> [#uses=1]
  %inc8 = add nsw i32 %tmp7, 1                    ; <i32> [#uses=1]
  store i32 %inc8, i32* %i
  br label %for.cond

for.end9:                                         ; preds = %for.cond
  ret void
}

declare i32 @bar3(...)
; CHECK: for (s1=0;s1<=199;s1++) {
; CHECK:   S0(s1);
; CHECK: }
; CHECK: for (s1=0;s1<=49;s1++) {
; CHECK:   S{{[0-3]}}(s1);
; CHECK:   for (s3=0;s3<=199;s3++) {
; CHECK:     S{{[[0-3]}}(s1,s3);
; CHECK:   }
; CHECK:   S{{[0-3]}}(s1);
; CHECK: }

