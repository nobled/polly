; RUN: opt -polly-scop-info -S < %s | FileCheck %s
define void @foo() nounwind {
start:
  br label %end

end:
  ret void
}

; CHECK: foo
