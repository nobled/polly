; RUN: opt -O3 -indvars -polly-scop-extract -polly-print-temp-scop-in-detail -print-top-scop-only  -analyze %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }
@alpha = common global double 0.000000e+00        ; <double*> [#uses=3]
@beta = common global double 0.000000e+00         ; <double*> [#uses=3]
@x = common global [4000 x double] zeroinitializer, align 32 ; <[4000 x double]*> [#uses=4]
@A = common global [4000 x [4000 x double]] zeroinitializer, align 32 ; <[4000 x [4000 x double]]*> [#uses=4]
@y = common global [4000 x double] zeroinitializer, align 32 ; <[4000 x double]*> [#uses=4]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
@tmp = common global [4000 x double] zeroinitializer, align 32 ; <[4000 x double]*> [#uses=2]
@B = common global [4000 x [4000 x double]] zeroinitializer, align 32 ; <[4000 x [4000 x double]]*> [#uses=2]
define void @scop_func() nounwind {
bb.nph10.split.us:
  %0 = load double* @alpha, align 8               ; <double> [#uses=1]
  %1 = load double* @beta, align 8                ; <double> [#uses=1]
  br label %bb.nph.us

bb3.us:                                           ; preds = %bb1.us
  store double %9, double* %scevgep17
  %2 = fmul double %9, %0                         ; <double> [#uses=1]
  %3 = fmul double %12, %1                        ; <double> [#uses=1]
  %4 = fadd double %2, %3                         ; <double> [#uses=1]
  store double %4, double* %scevgep18, align 8
  %5 = add nsw i64 %storemerge6.us, 1             ; <i64> [#uses=2]
  %exitcond14 = icmp eq i64 %5, 4000              ; <i1> [#uses=1]
  br i1 %exitcond14, label %return, label %bb.nph.us

bb1.us:                                           ; preds = %bb.nph.us, %bb1.us
  %.tmp3.0.us = phi double [ 0.000000e+00, %bb.nph.us ], [ %12, %bb1.us ] ; <double> [#uses=1]
  %.tmp.0.us = phi double [ 0.000000e+00, %bb.nph.us ], [ %9, %bb1.us ] ; <double> [#uses=1]
  %storemerge12.us = phi i64 [ 0, %bb.nph.us ], [ %13, %bb1.us ] ; <i64> [#uses=4]
  %scevgep13 = getelementptr [4000 x [4000 x double]]* @A, i64 0, i64 %storemerge6.us, i64 %storemerge12.us ; <double*> [#uses=1]
  %scevgep = getelementptr [4000 x [4000 x double]]* @B, i64 0, i64 %storemerge6.us, i64 %storemerge12.us ; <double*> [#uses=1]
  %scevgep12 = getelementptr [4000 x double]* @x, i64 0, i64 %storemerge12.us ; <double*> [#uses=1]
  %6 = load double* %scevgep13, align 8           ; <double> [#uses=1]
  %7 = load double* %scevgep12, align 8           ; <double> [#uses=2]
  %8 = fmul double %6, %7                         ; <double> [#uses=1]
  %9 = fadd double %8, %.tmp.0.us                 ; <double> [#uses=3]
  %10 = load double* %scevgep, align 8            ; <double> [#uses=1]
  %11 = fmul double %10, %7                       ; <double> [#uses=1]
  %12 = fadd double %11, %.tmp3.0.us              ; <double> [#uses=2]
  %13 = add nsw i64 %storemerge12.us, 1           ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %13, 4000               ; <i1> [#uses=1]
  br i1 %exitcond, label %bb3.us, label %bb1.us

bb.nph.us:                                        ; preds = %bb3.us, %bb.nph10.split.us
  %storemerge6.us = phi i64 [ 0, %bb.nph10.split.us ], [ %5, %bb3.us ] ; <i64> [#uses=5]
  %scevgep17 = getelementptr [4000 x double]* @tmp, i64 0, i64 %storemerge6.us ; <double*> [#uses=2]
  %scevgep18 = getelementptr [4000 x double]* @y, i64 0, i64 %storemerge6.us ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep17, align 8
  store double 0.000000e+00, double* %scevgep18, align 8
  br label %bb1.us

return:                                           ; preds = %bb3.us
  ret void
}

; CHECK: SCoP: bb.nph.us => return      Parameters: (), Max Loop Depth: 2
; CHECK: Bounds of Loop: bb.nph.us:     { 1 * {0,+,1}<%bb.nph.us> + 0 >= 0, -1 * {0,+,1}<%bb.nph.us> + 3999 >= 0}
; CHECK:   BB: bb.nph.us{
; CHECK:     Writes @tmp[8 * {0,+,1}<%bb.nph.us> + 0]
; CHECK:     Writes @y[8 * {0,+,1}<%bb.nph.us> + 0]
; CHECK:   }
; CHECK:   Bounds of Loop: bb1.us:      { 1 * {0,+,1}<%bb1.us> + 0 >= 0, -1 * {0,+,1}<%bb1.us> + 3999 >= 0}
; CHECK:     BB: bb1.us{
; CHECK:       Reads %.scalar28[0]
; CHECK:       Reads %.scalar25[0]
; CHECK:       Reads @A[8 * {0,+,1}<%bb1.us> + 32000 * {0,+,1}<%bb.nph.us> + 0]
; CHECK:       Reads @x[8 * {0,+,1}<%bb1.us> + 0]
; CHECK:       Writes %.scalar25[0]
; CHECK:       Reads @B[8 * {0,+,1}<%bb1.us> + 32000 * {0,+,1}<%bb.nph.us> + 0]
; CHECK:       Writes %.scalar28[0]
; CHECK:     }
; CHECK:   BB: bb3.us{
; CHECK:     Reads %.scalar28[0]
; CHECK:     Reads %.scalar25[0]
; CHECK:     Writes @tmp[8 * {0,+,1}<%bb.nph.us> + 0]
; CHECK:     Reads %.scalar[0]
; CHECK:     Reads %.scalar23[0]
; CHECK:     Writes @y[8 * {0,+,1}<%bb.nph.us> + 0]
; CHECK:   }
