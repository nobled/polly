; RUN: %opt -polly-codegen %s
define void @Reflection_coefficients(i16* %r) nounwind {
bb20:
  %indvar3.lcssa20.reload = load i64* undef
  %tmp = mul i64 %indvar3.lcssa20.reload, -1
  %tmp5 = add i64 %tmp, 8
  br label %bb22

bb21:                                             ; preds = %bb22
  %r_addr.1.moved.to.bb21 = getelementptr i16* %r, i64 0
  store i16 0, i16* %r_addr.1.moved.to.bb21, align 2
  %indvar.next = add i64 %indvar, 1
  br label %bb22

bb22:                                             ; preds = %bb21, %bb20
  %indvar = phi i64 [ %indvar.next, %bb21 ], [ 0, %bb20 ]
  %exitcond = icmp ne i64 %indvar, %tmp5
  br i1 %exitcond, label %bb21, label %return

return:                                           ; preds = %bb22
  ret void
}
