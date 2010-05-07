; RUN: opt -indvars -polly-print-scop -S -analyze < %s | FileCheck %s
; RUN: opt -indvars -polly-codegen < %s
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

@C = common global [2048 x [2061 x double]] zeroinitializer, align 8 ; <[2048 x [2061 x double]]*> [#uses=2]
@A = common global [2048 x [2061 x double]] zeroinitializer, align 8 ; <[2048 x [2061 x double]]*> [#uses=2]
@B = common global [2048 x [2061 x double]] zeroinitializer, align 8 ; <[2048 x [2061 x double]]*> [#uses=2]

define i32 @main() nounwind {
entry:
  call void (...)* @init_array()
  br label %for.cond

for.cond:                                         ; preds = %for.inc44, %entry
  %indvar3 = phi i64 [ %indvar.next4, %for.inc44 ], [ 0, %entry ] ; <i64> [#uses=4]
  %exitcond6 = icmp ne i64 %indvar3, 2048         ; <i1> [#uses=1]
  br i1 %exitcond6, label %for.cond1.preheader, label %for.end47

for.cond1.preheader:                              ; preds = %for.cond
  br label %for.cond1

for.cond1:                                        ; preds = %for.cond1.preheader, %for.inc40
  %indvar1 = phi i64 [ 0, %for.cond1.preheader ], [ %indvar.next2, %for.inc40 ] ; <i64> [#uses=4]
  %arrayidx12 = getelementptr [2048 x [2061 x double]]* @C, i64 0, i64 %indvar3, i64 %indvar1 ; <double*> [#uses=2]
  %exitcond5 = icmp ne i64 %indvar1, 2048         ; <i1> [#uses=1]
  br i1 %exitcond5, label %for.cond5.preheader, label %for.inc44

for.cond5.preheader:                              ; preds = %for.cond1
  br label %for.cond5

for.cond5:                                        ; preds = %for.cond5.preheader, %for.body8
  %indvar = phi i64 [ 0, %for.cond5.preheader ], [ %indvar.next, %for.body8 ] ; <i64> [#uses=4]
  %arrayidx20 = getelementptr [2048 x [2061 x double]]* @A, i64 0, i64 %indvar3, i64 %indvar ; <double*> [#uses=1]
  %arrayidx29 = getelementptr [2048 x [2061 x double]]* @B, i64 0, i64 %indvar, i64 %indvar1 ; <double*> [#uses=1]
  %exitcond = icmp ne i64 %indvar, 2048           ; <i1> [#uses=1]
  br i1 %exitcond, label %for.body8, label %for.inc40

for.body8:                                        ; preds = %for.cond5
  %tmp13 = load double* %arrayidx12               ; <double> [#uses=1]
  %mul = fmul double 1.000000e+00, %tmp13         ; <double> [#uses=1]
  %tmp21 = load double* %arrayidx20               ; <double> [#uses=1]
  %mul22 = fmul double 1.000000e+00, %tmp21       ; <double> [#uses=1]
  %tmp30 = load double* %arrayidx29               ; <double> [#uses=1]
  %mul31 = fmul double %mul22, %tmp30             ; <double> [#uses=1]
  %add = fadd double %mul, %mul31                 ; <double> [#uses=1]
  store double %add, double* %arrayidx12
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=1]
  br label %for.cond5

for.inc40:                                        ; preds = %for.cond5
  %indvar.next2 = add i64 %indvar1, 1             ; <i64> [#uses=1]
  br label %for.cond1

for.inc44:                                        ; preds = %for.cond1
  %indvar.next4 = add i64 %indvar3, 1             ; <i64> [#uses=1]
  br label %for.cond

for.end47:                                        ; preds = %for.cond
  call void (...)* @print_array()
  ret i32 0
}

declare void @init_array(...)

declare void @print_array(...)
; CHECK:  for (s1=0;s1<=2048;s1++) {
; CHECK:    S{{[0-7]}}(s1);
; CHECK:    S{{[0-7]}}(s1);
; CHECK:    for (s3=0;s3<=2048;s3++) {
; CHECK:      S{{[0-7]}}(s1,s3);
; CHECK:      S{{[0-7]}}(s1,s3);
; CHECK:      for (s5=0;s5<=2048;s5++) {
; CHECK:        S{{[0-7]}}(s1,s3,s5);
; CHECK:        S{{[0-7]}}(s1,s3,s5);
; CHECK:      }
; CHECK:      S{{[0-7]}}(s1,s3);
; CHECK:    }
; CHECK:    S{{[0-7]}}(s1);
; CHECK:  }
