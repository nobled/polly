; RUN: opt -O3 -indvars -polly-scop-detect  -analyze %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }
@alpha = common global double 0.000000e+00        ; <double*> [#uses=3]
@beta = common global double 0.000000e+00         ; <double*> [#uses=3]
@A = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=4]
@B = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=4]
@C = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=7]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
define void @scop_func(i64 %ni, i64 %nj, i64 %nk) nounwind {
entry:
  %0 = icmp sgt i64 %ni, 0                        ; <i1> [#uses=1]
  br i1 %0, label %bb.nph26, label %return

bb.nph8:                                          ; preds = %bb.nph26, %bb6
  %storemerge9 = phi i64 [ %12, %bb6 ], [ 0, %bb.nph26 ] ; <i64> [#uses=4]
  br i1 %16, label %bb.nph.us, label %bb4

bb4.us:                                           ; preds = %bb2.us
  store double %6, double* %scevgep30
  %1 = add nsw i64 %storemerge14.us, 1            ; <i64> [#uses=2]
  %exitcond28 = icmp eq i64 %1, %nj               ; <i1> [#uses=1]
  br i1 %exitcond28, label %bb6, label %bb.nph.us

bb2.us:                                           ; preds = %bb.nph.us, %bb2.us
  %.tmp.0.us = phi double [ %9, %bb.nph.us ], [ %6, %bb2.us ] ; <double> [#uses=1]
  %storemerge23.us = phi i64 [ 0, %bb.nph.us ], [ %7, %bb2.us ] ; <i64> [#uses=3]
  %scevgep = getelementptr [512 x [512 x double]]* @B, i64 0, i64 %storemerge23.us, i64 %storemerge14.us ; <double*> [#uses=1]
  %scevgep27 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %storemerge9, i64 %storemerge23.us ; <double*> [#uses=1]
  %2 = load double* %scevgep27, align 8           ; <double> [#uses=1]
  %3 = fmul double %2, %17                        ; <double> [#uses=1]
  %4 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %5 = fmul double %3, %4                         ; <double> [#uses=1]
  %6 = fadd double %.tmp.0.us, %5                 ; <double> [#uses=2]
  %7 = add nsw i64 %storemerge23.us, 1            ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %7, %nk                 ; <i1> [#uses=1]
  br i1 %exitcond, label %bb4.us, label %bb2.us

bb.nph.us:                                        ; preds = %bb4.us, %bb.nph8
  %storemerge14.us = phi i64 [ %1, %bb4.us ], [ 0, %bb.nph8 ] ; <i64> [#uses=3]
  %scevgep30 = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %storemerge9, i64 %storemerge14.us ; <double*> [#uses=3]
  %8 = load double* %scevgep30, align 8           ; <double> [#uses=1]
  %9 = fmul double %8, %15                        ; <double> [#uses=2]
  store double %9, double* %scevgep30, align 8
  br label %bb2.us

bb4:                                              ; preds = %bb4, %bb.nph8
  %indvar = phi i64 [ %indvar.next, %bb4 ], [ 0, %bb.nph8 ] ; <i64> [#uses=2]
  %storemerge19 = phi i64 [ %storemerge19, %bb4 ], [ %storemerge9, %bb.nph8 ] ; <i64> [#uses=3]
  %tmp34 = shl i64 %storemerge19, 9               ; <i64> [#uses=1]
  %scevgep32.sum = add i64 %indvar, %tmp34        ; <i64> [#uses=1]
  %scevgep35 = getelementptr [512 x [512 x double]]* @C, i64 0, i64 0, i64 %scevgep32.sum ; <double*> [#uses=2]
  %10 = load double* %scevgep35, align 8          ; <double> [#uses=1]
  %11 = fmul double %10, %15                      ; <double> [#uses=1]
  store double %11, double* %scevgep35, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond31 = icmp eq i64 %indvar.next, %nj     ; <i1> [#uses=1]
  br i1 %exitcond31, label %bb6, label %bb4

bb6:                                              ; preds = %bb4, %bb4.us
  %storemerge25 = phi i64 [ %storemerge9, %bb4.us ], [ %storemerge19, %bb4 ] ; <i64> [#uses=1]
  %12 = add nsw i64 %storemerge25, 1              ; <i64> [#uses=2]
  %13 = icmp slt i64 %12, %ni                     ; <i1> [#uses=1]
  br i1 %13, label %bb.nph8, label %return

bb.nph26:                                         ; preds = %entry
  %14 = icmp sgt i64 %nj, 0                       ; <i1> [#uses=1]
  %15 = load double* @beta, align 8               ; <double> [#uses=2]
  %16 = icmp sgt i64 %nk, 0                       ; <i1> [#uses=1]
  %17 = load double* @alpha, align 8              ; <double> [#uses=1]
  br i1 %14, label %bb.nph8, label %return

return:                                           ; preds = %bb.nph26, %bb6, %entry
  ret void
}

; CHECK: SCoP: entry => <Function Return>        Parameters: (%nj, %nk, %ni, ), Max Loop Depth: 3
