;RUN: opt -polly-print -disable-output %s | FileCheck %s
@A = common global [1 x i32] zeroinitializer, align 4 ; <[1 x i32]*> [#uses=1]

define void @constant_condition() nounwind {
bb:
  %tmp = icmp eq i32 0, 0                         ; <i1> [#uses=0]
  br i1 true, label %bb1, label %bb2

bb1:                                              ; preds = %bb
  store i32 0, i32* getelementptr inbounds ([1 x i32]* @A, i32 0, i32 0)
  br label %bb3

bb2:                                              ; preds = %bb
  store i32 1, i32* getelementptr inbounds ([1 x i32]* @A, i32 0, i32 0)
  br label %bb3

bb3:                                              ; preds = %bb2, %bb1
  ret void
}

declare void @llvm.memory.barrier(i1, i1, i1, i1, i1) nounwind

define i32 @main() nounwind {
bb:
  store i32 2, i32* getelementptr inbounds ([1 x i32]* @A, i32 0, i32 0)
  call void @constant_condition()
  %tmp = load i32* getelementptr inbounds ([1 x i32]* @A, i32 0, i32 0) ; <i32> [#uses=1]
  ret i32 %tmp
}


; CHECK: %bb1();
