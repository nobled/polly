; RUN: opt -O3 -indvars -polly-scop-extract -polly-print-temp-scop-in-detail -print-top-scop-only  -analyze %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }
@alpha = common global double 0.000000e+00        ; <double*> [#uses=3]
@beta = common global double 0.000000e+00         ; <double*> [#uses=3]
@u1 = common global [4000 x double] zeroinitializer, align 32 ; <[4000 x double]*> [#uses=3]
@u2 = common global [4000 x double] zeroinitializer, align 32 ; <[4000 x double]*> [#uses=3]
@v1 = common global [4000 x double] zeroinitializer, align 32 ; <[4000 x double]*> [#uses=3]
@v2 = common global [4000 x double] zeroinitializer, align 32 ; <[4000 x double]*> [#uses=3]
@y = common global [4000 x double] zeroinitializer, align 32 ; <[4000 x double]*> [#uses=3]
@z = common global [4000 x double] zeroinitializer, align 32 ; <[4000 x double]*> [#uses=3]
@x = common global [4000 x double] zeroinitializer, align 32 ; <[4000 x double]*> [#uses=5]
@w = common global [4000 x double] zeroinitializer, align 32 ; <[4000 x double]*> [#uses=5]
@A = common global [4000 x [4000 x double]] zeroinitializer, align 32 ; <[4000 x [4000 x double]]*> [#uses=5]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
@B = common global [4000 x [4000 x double]] zeroinitializer, align 32 ; <[4000 x [4000 x double]]*> [#uses=0]
define void @scop_func() nounwind {
bb.nph31.bb.nph31.split_crit_edge:
  br label %bb.nph26

bb.nph26:                                         ; preds = %bb3, %bb.nph31.bb.nph31.split_crit_edge
  %storemerge27 = phi i64 [ 0, %bb.nph31.bb.nph31.split_crit_edge ], [ %10, %bb3 ] ; <i64> [#uses=4]
  %scevgep53 = getelementptr [4000 x double]* @u2, i64 0, i64 %storemerge27 ; <double*> [#uses=1]
  %scevgep52 = getelementptr [4000 x double]* @u1, i64 0, i64 %storemerge27 ; <double*> [#uses=1]
  %0 = load double* %scevgep52, align 8           ; <double> [#uses=1]
  %1 = load double* %scevgep53, align 8           ; <double> [#uses=1]
  br label %bb1

bb1:                                              ; preds = %bb1, %bb.nph26
  %storemerge625 = phi i64 [ 0, %bb.nph26 ], [ %9, %bb1 ] ; <i64> [#uses=4]
  %scevgep47 = getelementptr [4000 x [4000 x double]]* @A, i64 0, i64 %storemerge27, i64 %storemerge625 ; <double*> [#uses=2]
  %scevgep48 = getelementptr [4000 x double]* @v1, i64 0, i64 %storemerge625 ; <double*> [#uses=1]
  %scevgep49 = getelementptr [4000 x double]* @v2, i64 0, i64 %storemerge625 ; <double*> [#uses=1]
  %2 = load double* %scevgep47, align 8           ; <double> [#uses=1]
  %3 = load double* %scevgep48, align 8           ; <double> [#uses=1]
  %4 = fmul double %0, %3                         ; <double> [#uses=1]
  %5 = fadd double %2, %4                         ; <double> [#uses=1]
  %6 = load double* %scevgep49, align 8           ; <double> [#uses=1]
  %7 = fmul double %1, %6                         ; <double> [#uses=1]
  %8 = fadd double %5, %7                         ; <double> [#uses=1]
  store double %8, double* %scevgep47, align 8
  %9 = add nsw i64 %storemerge625, 1              ; <i64> [#uses=2]
  %exitcond46 = icmp eq i64 %9, 4000              ; <i1> [#uses=1]
  br i1 %exitcond46, label %bb3, label %bb1

bb3:                                              ; preds = %bb1
  %10 = add nsw i64 %storemerge27, 1              ; <i64> [#uses=2]
  %exitcond50 = icmp eq i64 %10, 4000             ; <i1> [#uses=1]
  br i1 %exitcond50, label %bb.nph24.bb.nph24.split_crit_edge, label %bb.nph26

bb.nph16:                                         ; preds = %bb.nph24.bb.nph24.split_crit_edge, %bb9
  %storemerge120 = phi i64 [ 0, %bb.nph24.bb.nph24.split_crit_edge ], [ %17, %bb9 ] ; <i64> [#uses=3]
  %scevgep45 = getelementptr [4000 x double]* @x, i64 0, i64 %storemerge120 ; <double*> [#uses=2]
  %.promoted17 = load double* %scevgep45          ; <double> [#uses=1]
  br label %bb7

bb7:                                              ; preds = %bb7, %bb.nph16
  %.tmp.018 = phi double [ %.promoted17, %bb.nph16 ], [ %15, %bb7 ] ; <double> [#uses=1]
  %storemerge515 = phi i64 [ 0, %bb.nph16 ], [ %16, %bb7 ] ; <i64> [#uses=3]
  %scevgep42 = getelementptr [4000 x [4000 x double]]* @A, i64 0, i64 %storemerge515, i64 %storemerge120 ; <double*> [#uses=1]
  %scevgep41 = getelementptr [4000 x double]* @y, i64 0, i64 %storemerge515 ; <double*> [#uses=1]
  %11 = load double* %scevgep42, align 8          ; <double> [#uses=1]
  %12 = fmul double %11, %18                      ; <double> [#uses=1]
  %13 = load double* %scevgep41, align 8          ; <double> [#uses=1]
  %14 = fmul double %12, %13                      ; <double> [#uses=1]
  %15 = fadd double %.tmp.018, %14                ; <double> [#uses=2]
  %16 = add nsw i64 %storemerge515, 1             ; <i64> [#uses=2]
  %exitcond40 = icmp eq i64 %16, 4000             ; <i1> [#uses=1]
  br i1 %exitcond40, label %bb9, label %bb7

bb9:                                              ; preds = %bb7
  store double %15, double* %scevgep45
  %17 = add nsw i64 %storemerge120, 1             ; <i64> [#uses=2]
  %exitcond43 = icmp eq i64 %17, 4000             ; <i1> [#uses=1]
  br i1 %exitcond43, label %bb12, label %bb.nph16

bb.nph24.bb.nph24.split_crit_edge:                ; preds = %bb3
  %18 = load double* @beta, align 8               ; <double> [#uses=1]
  br label %bb.nph16

bb12:                                             ; preds = %bb12, %bb9
  %storemerge213 = phi i64 [ %22, %bb12 ], [ 0, %bb9 ] ; <i64> [#uses=3]
  %scevgep37 = getelementptr [4000 x double]* @z, i64 0, i64 %storemerge213 ; <double*> [#uses=1]
  %scevgep38 = getelementptr [4000 x double]* @x, i64 0, i64 %storemerge213 ; <double*> [#uses=2]
  %19 = load double* %scevgep38, align 8          ; <double> [#uses=1]
  %20 = load double* %scevgep37, align 8          ; <double> [#uses=1]
  %21 = fadd double %19, %20                      ; <double> [#uses=1]
  store double %21, double* %scevgep38, align 8
  %22 = add nsw i64 %storemerge213, 1             ; <i64> [#uses=2]
  %exitcond36 = icmp eq i64 %22, 4000             ; <i1> [#uses=1]
  br i1 %exitcond36, label %bb.nph12.bb.nph12.split_crit_edge, label %bb12

bb.nph:                                           ; preds = %bb.nph12.bb.nph12.split_crit_edge, %bb18
  %storemerge38 = phi i64 [ 0, %bb.nph12.bb.nph12.split_crit_edge ], [ %29, %bb18 ] ; <i64> [#uses=3]
  %scevgep35 = getelementptr [4000 x double]* @w, i64 0, i64 %storemerge38 ; <double*> [#uses=2]
  %.promoted = load double* %scevgep35            ; <double> [#uses=1]
  br label %bb16

bb16:                                             ; preds = %bb16, %bb.nph
  %.tmp.0 = phi double [ %.promoted, %bb.nph ], [ %27, %bb16 ] ; <double> [#uses=1]
  %storemerge47 = phi i64 [ 0, %bb.nph ], [ %28, %bb16 ] ; <i64> [#uses=3]
  %scevgep32 = getelementptr [4000 x [4000 x double]]* @A, i64 0, i64 %storemerge38, i64 %storemerge47 ; <double*> [#uses=1]
  %scevgep = getelementptr [4000 x double]* @x, i64 0, i64 %storemerge47 ; <double*> [#uses=1]
  %23 = load double* %scevgep32, align 8          ; <double> [#uses=1]
  %24 = fmul double %23, %30                      ; <double> [#uses=1]
  %25 = load double* %scevgep, align 8            ; <double> [#uses=1]
  %26 = fmul double %24, %25                      ; <double> [#uses=1]
  %27 = fadd double %.tmp.0, %26                  ; <double> [#uses=2]
  %28 = add nsw i64 %storemerge47, 1              ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %28, 4000               ; <i1> [#uses=1]
  br i1 %exitcond, label %bb18, label %bb16

bb18:                                             ; preds = %bb16
  store double %27, double* %scevgep35
  %29 = add nsw i64 %storemerge38, 1              ; <i64> [#uses=2]
  %exitcond33 = icmp eq i64 %29, 4000             ; <i1> [#uses=1]
  br i1 %exitcond33, label %return, label %bb.nph

bb.nph12.bb.nph12.split_crit_edge:                ; preds = %bb12
  %30 = load double* @alpha, align 8              ; <double> [#uses=1]
  br label %bb.nph

return:                                           ; preds = %bb18
  ret void
}

; CHECK: SCoP: bb.nph => return Parameters: (), Max Loop Depth: 2
; CHECK: Bounds of Loop: bb.nph:        { 1 * {0,+,1}<%bb.nph> + 0 >= 0, -1 * {0,+,1}<%bb.nph> + 3999 >= 0}
; CHECK:   BB: bb.nph{
; CHECK:     Reads @w[8 * {0,+,1}<%bb.nph> + 0]
; CHECK:     Writes %.promoted.scalar[0]
; CHECK:   }
; CHECK:   Bounds of Loop: bb16:        { 1 * {0,+,1}<%bb16> + 0 >= 0, -1 * {0,+,1}<%bb16> + 3999 >= 0}
; CHECK:     BB: bb16{
; CHECK:       Reads %.promoted.scalar[0]
; CHECK:       Reads %.scalar64[0]
; CHECK:       Reads @A[8 * {0,+,1}<%bb16> + 32000 * {0,+,1}<%bb.nph> + 0]
; CHECK:       Reads %.scalar67[0]
; CHECK:       Reads @x[8 * {0,+,1}<%bb16> + 0]
; CHECK:       Writes %.scalar64[0]
; CHECK:     }
; CHECK:   BB: bb18{
; CHECK:     Reads %.scalar64[0]
; CHECK:     Writes @w[8 * {0,+,1}<%bb.nph> + 0]
; CHECK:   }
