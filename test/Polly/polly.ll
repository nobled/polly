; RUN: opt -polly-codegen -S < %s | FileCheck %s
define void @foo() nounwind {
start:
  br label %end

end:
  ret void
}

; CHECK: foo
