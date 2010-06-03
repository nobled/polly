; RUN: opt  -O3 -loopsimplify -indvars -polly-scop-detect  -print-top-scop-only -analyze %s | FileCheck %s
; XFAIL: *
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }
@A = common global [128 x [128 x [128 x double]]] zeroinitializer, align 32 ; <[128 x [128 x [128 x double]]]*> [#uses=8]
@C4 = common global [128 x [128 x double]] zeroinitializer, align 32 ; <[128 x [128 x double]]*> [#uses=4]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
@sum = common global [128 x [128 x [128 x double]]] zeroinitializer, align 32 ; <[128 x [128 x [128 x double]]]*> [#uses=4]
define void @scop_func(i64 %nr, i64 %nq, i64 %np) nounwind {
entry:
  %0 = icmp sgt i64 %nr, 0                        ; <i1> [#uses=1]
  br i1 %0, label %bb.nph50, label %return

bb5.us:                                           ; preds = %bb3.us
  store double %5, double* %scevgep54
  %1 = add nsw i64 %storemerge26.us, 1            ; <i64> [#uses=2]
  %exitcond52 = icmp eq i64 %1, %np               ; <i1> [#uses=1]
  br i1 %exitcond52, label %bb9.loopexit, label %bb.nph.us

bb3.us:                                           ; preds = %bb.nph.us, %bb3.us
  %.tmp.0.us = phi double [ 0.000000e+00, %bb.nph.us ], [ %5, %bb3.us ] ; <double> [#uses=1]
  %storemerge45.us = phi i64 [ 0, %bb.nph.us ], [ %6, %bb3.us ] ; <i64> [#uses=3]
  %scevgep = getelementptr [128 x [128 x double]]* @C4, i64 0, i64 %storemerge45.us, i64 %storemerge26.us ; <double*> [#uses=1]
  %scevgep51 = getelementptr [128 x [128 x [128 x double]]]* @A, i64 0, i64 %storemerge43, i64 %storemerge113, i64 %storemerge45.us ; <double*> [#uses=1]
  %2 = load double* %scevgep51, align 8           ; <double> [#uses=1]
  %3 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %4 = fmul double %2, %3                         ; <double> [#uses=1]
  %5 = fadd double %.tmp.0.us, %4                 ; <double> [#uses=2]
  %6 = add nsw i64 %storemerge45.us, 1            ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %6, %np                 ; <i1> [#uses=1]
  br i1 %exitcond, label %bb5.us, label %bb3.us

bb.nph.us:                                        ; preds = %bb6.preheader, %bb5.us
  %storemerge26.us = phi i64 [ %1, %bb5.us ], [ 0, %bb6.preheader ] ; <i64> [#uses=3]
  %scevgep54 = getelementptr [128 x [128 x [128 x double]]]* @sum, i64 0, i64 %storemerge43, i64 %storemerge113, i64 %storemerge26.us ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep54, align 8
  br label %bb3.us

bb8:                                              ; preds = %bb9.loopexit, %bb8
  %storemerge311 = phi i64 [ %8, %bb8 ], [ 0, %bb9.loopexit ] ; <i64> [#uses=3]
  %scevgep61 = getelementptr [128 x [128 x [128 x double]]]* @A, i64 0, i64 %storemerge43, i64 %storemerge113, i64 %storemerge311 ; <double*> [#uses=1]
  %scevgep62 = getelementptr [128 x [128 x [128 x double]]]* @sum, i64 0, i64 %storemerge43, i64 %storemerge113, i64 %storemerge311 ; <double*> [#uses=1]
  %7 = load double* %scevgep62, align 8           ; <double> [#uses=1]
  store double %7, double* %scevgep61, align 8
  %8 = add nsw i64 %storemerge311, 1              ; <i64> [#uses=2]
  %exitcond60 = icmp eq i64 %8, %np               ; <i1> [#uses=1]
  br i1 %exitcond60, label %bb10, label %bb8

bb9.loopexit:                                     ; preds = %bb5.us
  br i1 %14, label %bb8, label %bb10

bb10:                                             ; preds = %bb6.preheader, %bb9.loopexit, %bb8
  %storemerge12566 = phi i64 [ %storemerge113, %bb9.loopexit ], [ %storemerge113, %bb8 ], [ %storemerge113, %bb6.preheader ] ; <i64> [#uses=1]
  %storemerge4464 = phi i64 [ %storemerge43, %bb9.loopexit ], [ %storemerge43, %bb8 ], [ %storemerge43, %bb6.preheader ] ; <i64> [#uses=2]
  %9 = add nsw i64 %storemerge12566, 1            ; <i64> [#uses=2]
  %10 = icmp slt i64 %9, %nq                      ; <i1> [#uses=1]
  br i1 %10, label %bb6.preheader, label %bb12

bb6.preheader:                                    ; preds = %bb.nph50, %bb12, %bb10
  %storemerge43 = phi i64 [ %storemerge4464, %bb10 ], [ %11, %bb12 ], [ 0, %bb.nph50 ] ; <i64> [#uses=7]
  %storemerge113 = phi i64 [ %9, %bb10 ], [ 0, %bb.nph50 ], [ 0, %bb12 ] ; <i64> [#uses=7]
  br i1 %14, label %bb.nph.us, label %bb10

bb12:                                             ; preds = %bb10
  %11 = add nsw i64 %storemerge4464, 1            ; <i64> [#uses=2]
  %12 = icmp slt i64 %11, %nr                     ; <i1> [#uses=1]
  br i1 %12, label %bb6.preheader, label %return

bb.nph50:                                         ; preds = %entry
  %13 = icmp sgt i64 %nq, 0                       ; <i1> [#uses=1]
  %14 = icmp sgt i64 %np, 0                       ; <i1> [#uses=2]
  br i1 %13, label %bb6.preheader, label %return

return:                                           ; preds = %bb.nph50, %bb12, %entry
  ret void
}

; CHECK: SCoP: entry => <Function Return>        Parameters: (%np, %nr, %nq, ), Max Loop Depth: 4
