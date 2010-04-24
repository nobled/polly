; ModuleID = 'test.c'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define void @foo(i32 %a) nounwind {
entry:
  %A = alloca [100 x i32], align 4                ; <[100 x i32]*> [#uses=2]
  call void (...)* @bar() nounwind
  %cmp41 = icmp slt i32 %a, 0                     ; <i1> [#uses=2]
  br i1 %cmp41, label %for.end.thread, label %for.body

for.end.thread:                                   ; preds = %entry
  call void (...)* @bar() nounwind
  br label %for.end32

for.body:                                         ; preds = %entry, %for.body
  %indvar51 = phi i64 [ %tmp, %for.body ], [ 0, %entry ] ; <i64> [#uses=2]
  %arrayidx = getelementptr [100 x i32]* %A, i64 0, i64 %indvar51 ; <i32*> [#uses=1]
  store i32 0, i32* %arrayidx
  %tmp = add i64 %indvar51, 1                     ; <i64> [#uses=2]
  %inc = trunc i64 %tmp to i32                    ; <i32> [#uses=1]
  %cmp = icmp sgt i32 %inc, %a                    ; <i1> [#uses=1]
  br i1 %cmp, label %for.end, label %for.body

for.end:                                          ; preds = %for.body
  call void (...)* @bar() nounwind
  br i1 %cmp41, label %for.end32, label %for.inc29

for.inc29:                                        ; preds = %for.end, %for.inc29
  %indvar = phi i64 [ %tmp49, %for.inc29 ], [ 0, %for.end ] ; <i64> [#uses=3]
  %tmp49 = add i64 %indvar, 1                     ; <i64> [#uses=2]
  %inc31 = trunc i64 %tmp49 to i32                ; <i32> [#uses=1]
  %arrayidx23 = getelementptr [100 x i32]* %A, i64 0, i64 %indvar ; <i32*> [#uses=2]
  %tmp46 = mul i64 %indvar, 3240                  ; <i64> [#uses=1]
  %tmp47 = trunc i64 %tmp46 to i32                ; <i32> [#uses=1]
  %arrayidx23.promoted = load i32* %arrayidx23    ; <i32> [#uses=1]
  %tmp44 = add i32 %arrayidx23.promoted, %tmp47   ; <i32> [#uses=1]
  store i32 %tmp44, i32* %arrayidx23
  %cmp10 = icmp sgt i32 %inc31, %a                ; <i1> [#uses=1]
  br i1 %cmp10, label %for.end32, label %for.inc29

for.end32:                                        ; preds = %for.inc29, %for.end.thread, %for.end
  call void (...)* @bar() nounwind
  ret void
}

declare void @bar(...)
