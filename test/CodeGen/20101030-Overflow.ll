; RUN: %opt -polly-codegen %s

define void @compdecomp() nounwind {
entry:
  %max = alloca i64
  %i = load i64* undef
  br label %bb37

bb37:                                             ; preds = %bb36, %bb28
  %tmp = icmp ugt i64 %i, 0
  br i1 %tmp, label %bb38, label %bb39

bb38:                                             ; preds = %bb37
  store i64 %i, i64* %max
  br label %bb39

bb39:                                             ; preds = %bb38, %bb37
  unreachable

}
