; RUN: opt -O3 -polly-print-scop -S -analyze < %s | FileCheck %s
; ModuleID = 'pluto-matmul.c'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

@C = common global [2048 x [2061 x double]] zeroinitializer, align 8 ; <[2048 x [2061 x double]]*> [#uses=1]
@A = common global [2048 x [2061 x double]] zeroinitializer, align 8 ; <[2048 x [2061 x double]]*> [#uses=1]
@B = common global [2048 x [2061 x double]] zeroinitializer, align 8 ; <[2048 x [2061 x double]]*> [#uses=1]

define i32 @main() nounwind {
bb.nph65.bb.nph65.split_crit_edge:
  tail call void (...)* @init_array() nounwind
  br label %bb.nph55.bb.nph55.split_crit_edge

bb.nph55.bb.nph55.split_crit_edge:                ; preds = %for.inc44, %bb.nph65.bb.nph65.split_crit_edge
  %indvar66 = phi i64 [ 0, %bb.nph65.bb.nph65.split_crit_edge ], [ %indvar.next67, %for.inc44 ] ; <i64> [#uses=3]
  br label %bb.nph

bb.nph:                                           ; preds = %bb.nph55.bb.nph55.split_crit_edge, %for.inc40
  %indvar68 = phi i64 [ 0, %bb.nph55.bb.nph55.split_crit_edge ], [ %indvar.next69, %for.inc40 ] ; <i64> [#uses=3]
  %arrayidx12 = getelementptr [2048 x [2061 x double]]* @C, i64 0, i64 %indvar66, i64 %indvar68 ; <double*> [#uses=2]
  %arrayidx12.promoted = load double* %arrayidx12 ; <double> [#uses=1]
  br label %for.body8

for.body8:                                        ; preds = %for.body8, %bb.nph
  %indvar = phi i64 [ 0, %bb.nph ], [ %indvar.next, %for.body8 ] ; <i64> [#uses=3]
  %arrayidx12.tmp.0 = phi double [ %arrayidx12.promoted, %bb.nph ], [ %add, %for.body8 ] ; <double> [#uses=1]
  %arrayidx20 = getelementptr [2048 x [2061 x double]]* @A, i64 0, i64 %indvar66, i64 %indvar ; <double*> [#uses=1]
  %arrayidx29 = getelementptr [2048 x [2061 x double]]* @B, i64 0, i64 %indvar, i64 %indvar68 ; <double*> [#uses=1]
  %tmp21 = load double* %arrayidx20               ; <double> [#uses=1]
  %tmp30 = load double* %arrayidx29               ; <double> [#uses=1]
  %mul31 = fmul double %tmp21, %tmp30             ; <double> [#uses=1]
  %add = fadd double %arrayidx12.tmp.0, %mul31    ; <double> [#uses=2]
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 2048      ; <i1> [#uses=1]
  br i1 %exitcond, label %for.inc40, label %for.body8

for.inc40:                                        ; preds = %for.body8
  store double %add, double* %arrayidx12
  %indvar.next69 = add i64 %indvar68, 1           ; <i64> [#uses=2]
  %exitcond70 = icmp eq i64 %indvar.next69, 2048  ; <i1> [#uses=1]
  br i1 %exitcond70, label %for.inc44, label %bb.nph

for.inc44:                                        ; preds = %for.inc40
  %indvar.next67 = add i64 %indvar66, 1           ; <i64> [#uses=2]
  %exitcond71 = icmp eq i64 %indvar.next67, 2048  ; <i1> [#uses=1]
  br i1 %exitcond71, label %for.end47, label %bb.nph55.bb.nph55.split_crit_edge

for.end47:                                        ; preds = %for.inc44
  tail call void (...)* @print_array() nounwind
  ret i32 0
}

declare void @init_array(...)

declare void @print_array(...)
; CHECK:  for (s1=0;s1<=2047;s1++) {
; CHECK:    S1(s1);
; CHECK:    for (s3=0;s3<=2047;s3++) {
; CHECK:      S0(s1,s3);
; CHECK:      for (s5=0;s5<=2047;s5++) {
; CHECK:        S2(s1,s3,s5);
; CHECK:      }
; CHECK:      S3(s1,s3);
; CHECK:    }
; CHECK:    S4(s1);
; CHECK:  }
