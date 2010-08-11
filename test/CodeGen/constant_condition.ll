;RUN: opt -polly-print -disable-output %s | FileCheck %s
@A = common global [1 x i32] zeroinitializer, align 4 ; <[1 x i32]*> [#uses=1]

define void @constant_condition() nounwind {
  %1 = icmp eq i32 0, 0                           ; <i1> [#uses=1]
  br i1 true, label %2, label %3

; <label>:2                                       ; preds = %0
  store i32 0, i32* getelementptr inbounds ([1 x i32]* @A, i32 0, i32 0)
  br label %4

; <label>:3                                       ; preds = %0
  store i32 1, i32* getelementptr inbounds ([1 x i32]* @A, i32 0, i32 0)
  br label %4

; <label>:4                                       ; preds = %3, %2
  ret void
}

declare void @llvm.memory.barrier(i1, i1, i1, i1, i1) nounwind

define i32 @main() nounwind {
  store i32 2, i32* getelementptr inbounds ([1 x i32]* @A, i32 0, i32 0)
  call void @constant_condition()
  %1 = load i32* getelementptr inbounds ([1 x i32]* @A, i32 0, i32 0) ; <i32> [#uses=1]
  ret i32 %1
}

; CHECK: %2();
