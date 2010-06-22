; RUN: opt  -O3 -loopsimplify -indvars -polly-scop-detect  -print-top-scop-only -analyze %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }
@A = common global [128 x [128 x [128 x double]]] zeroinitializer, align 32 ; <[128 x [128 x [128 x double]]]*> [#uses=8]
@C4 = common global [128 x [128 x double]] zeroinitializer, align 32 ; <[128 x [128 x double]]*> [#uses=4]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
@sum = common global [128 x [128 x [128 x double]]] zeroinitializer, align 32 ; <[128 x [128 x [128 x double]]]*> [#uses=4]
define void @scop_func() nounwind {
bb.nph50.bb.nph50.split_crit_edge:
  br label %bb11.preheader

bb5.us:                                           ; preds = %bb3.us
  store double %4, double* %scevgep54
  %0 = add nsw i64 %storemerge26.us, 1            ; <i64> [#uses=2]
  %exitcond52 = icmp eq i64 %0, 128               ; <i1> [#uses=1]
  br i1 %exitcond52, label %bb8, label %bb.nph.us

bb3.us:                                           ; preds = %bb.nph.us, %bb3.us
  %.tmp.0.us = phi double [ 0.000000e+00, %bb.nph.us ], [ %4, %bb3.us ] ; <double> [#uses=1]
  %storemerge45.us = phi i64 [ 0, %bb.nph.us ], [ %5, %bb3.us ] ; <i64> [#uses=3]
  %scevgep = getelementptr [128 x [128 x double]]* @C4, i64 0, i64 %storemerge45.us, i64 %storemerge26.us ; <double*> [#uses=1]
  %scevgep51 = getelementptr [128 x [128 x [128 x double]]]* @A, i64 0, i64 %storemerge30, i64 %storemerge113, i64 %storemerge45.us ; <double*> [#uses=1]
  %1 = load double* %scevgep51, align 8           ; <double> [#uses=1]
  %2 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %3 = fmul double %1, %2                         ; <double> [#uses=1]
  %4 = fadd double %.tmp.0.us, %3                 ; <double> [#uses=2]
  %5 = add nsw i64 %storemerge45.us, 1            ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %5, 128                 ; <i1> [#uses=1]
  br i1 %exitcond, label %bb5.us, label %bb3.us

bb.nph.us:                                        ; preds = %bb6.preheader, %bb5.us
  %storemerge26.us = phi i64 [ %0, %bb5.us ], [ 0, %bb6.preheader ] ; <i64> [#uses=3]
  %scevgep54 = getelementptr [128 x [128 x [128 x double]]]* @sum, i64 0, i64 %storemerge30, i64 %storemerge113, i64 %storemerge26.us ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep54, align 8
  br label %bb3.us

bb8:                                              ; preds = %bb8, %bb5.us
  %storemerge311 = phi i64 [ %7, %bb8 ], [ 0, %bb5.us ] ; <i64> [#uses=3]
  %scevgep56 = getelementptr [128 x [128 x [128 x double]]]* @A, i64 0, i64 %storemerge30, i64 %storemerge113, i64 %storemerge311 ; <double*> [#uses=1]
  %scevgep57 = getelementptr [128 x [128 x [128 x double]]]* @sum, i64 0, i64 %storemerge30, i64 %storemerge113, i64 %storemerge311 ; <double*> [#uses=1]
  %6 = load double* %scevgep57, align 8           ; <double> [#uses=1]
  store double %6, double* %scevgep56, align 8
  %7 = add nsw i64 %storemerge311, 1              ; <i64> [#uses=2]
  %exitcond55 = icmp eq i64 %7, 128               ; <i1> [#uses=1]
  br i1 %exitcond55, label %bb10, label %bb8

bb10:                                             ; preds = %bb8
  %8 = add nsw i64 %storemerge113, 1              ; <i64> [#uses=2]
  %9 = icmp slt i64 %8, 128                       ; <i1> [#uses=1]
  br i1 %9, label %bb6.preheader, label %bb12

bb6.preheader:                                    ; preds = %bb11.preheader, %bb10
  %storemerge113 = phi i64 [ %8, %bb10 ], [ 0, %bb11.preheader ] ; <i64> [#uses=5]
  br label %bb.nph.us

bb12:                                             ; preds = %bb10
  %10 = add nsw i64 %storemerge30, 1              ; <i64> [#uses=2]
  %11 = icmp slt i64 %10, 128                     ; <i1> [#uses=1]
  br i1 %11, label %bb11.preheader, label %return

bb11.preheader:                                   ; preds = %bb12, %bb.nph50.bb.nph50.split_crit_edge
  %storemerge30 = phi i64 [ 0, %bb.nph50.bb.nph50.split_crit_edge ], [ %10, %bb12 ] ; <i64> [#uses=5]
  br label %bb6.preheader

return:                                           ; preds = %bb12
  ret void
}

; CHECK: SCoP: bb11.preheader => return Parameters: (), Max Loop Depth: 4
