; RUN: opt -polly-print-scop -analyze < %s | FileCheck %s

define void @f(double* %a) nounwind {
bb.nph:
  br label %for.body

for.body:                                         ; preds = %for.body, %bb.nph
  %storemerge2 = phi i32 [ 0, %bb.nph ], [ %inc, %for.body ] ; <i32> [#uses=3]
  %arrayidx = getelementptr double* %a, i32 %storemerge2 ; <double*> [#uses=2]
  %tmp = add i32 %storemerge2, 64                 ; <i32> [#uses=1]
  %arrayidx24 = getelementptr double* %a, i32 %tmp ; <double*> [#uses=2]
  %tmp3 = load double* %arrayidx                  ; <double> [#uses=1]
  %tmp7 = load double* %arrayidx24                ; <double> [#uses=2]
  %add8 = fadd double %tmp3, %tmp7                ; <double> [#uses=2]
  store double %add8, double* %arrayidx
  %sub = fsub double %add8, %tmp7                 ; <double> [#uses=1]
  store double %sub, double* %arrayidx24
  %inc = add nsw i32 %storemerge2, 1              ; <i32> [#uses=2]
  %exitcond = icmp eq i32 %inc, 64                ; <i1> [#uses=1]
  br i1 %exitcond, label %for.end, label %for.body

for.end:                                          ; preds = %for.body
  ret void
}
; CHECK:  for (s1=0;s1<=63;s1++) {
; CHECK:    S0(s1);
; CHECK:  }
; CHECK:  S{{[0-2]}}();
; CHECK:  for (s1=0;s1<=63;s1++) {
; CHECK:    S{{[0-2]}}(s1);
; CHECK:  }
; CHECK:  S{{[0-2]}}();
