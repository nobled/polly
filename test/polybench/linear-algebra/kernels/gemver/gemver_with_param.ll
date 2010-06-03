; RUN: opt -O3 -indvars -polly-scop-detect -polly-print-temp-scop-in-detail -print-top-scop-only  -analyze %s | FileCheck %s
; XFAIL: *
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
define void @scop_func(i64 %n) nounwind {
entry:
  %0 = icmp sgt i64 %n, 0                         ; <i1> [#uses=4]
  br i1 %0, label %bb.nph40, label %return

bb.nph40:                                         ; preds = %bb3, %entry
  %i.041 = phi i64 [ %11, %bb3 ], [ 0, %entry ]   ; <i64> [#uses=4]
  %scevgep67 = getelementptr [4000 x double]* @u2, i64 0, i64 %i.041 ; <double*> [#uses=1]
  %scevgep66 = getelementptr [4000 x double]* @u1, i64 0, i64 %i.041 ; <double*> [#uses=1]
  %1 = load double* %scevgep66, align 8           ; <double> [#uses=1]
  %2 = load double* %scevgep67, align 8           ; <double> [#uses=1]
  br label %bb1

bb1:                                              ; preds = %bb1, %bb.nph40
  %j.039 = phi i64 [ 0, %bb.nph40 ], [ %10, %bb1 ] ; <i64> [#uses=4]
  %scevgep63 = getelementptr [4000 x [4000 x double]]* @A, i64 0, i64 %i.041, i64 %j.039 ; <double*> [#uses=2]
  %scevgep61 = getelementptr [4000 x double]* @v1, i64 0, i64 %j.039 ; <double*> [#uses=1]
  %scevgep62 = getelementptr [4000 x double]* @v2, i64 0, i64 %j.039 ; <double*> [#uses=1]
  %3 = load double* %scevgep63, align 8           ; <double> [#uses=1]
  %4 = load double* %scevgep61, align 8           ; <double> [#uses=1]
  %5 = fmul double %1, %4                         ; <double> [#uses=1]
  %6 = fadd double %3, %5                         ; <double> [#uses=1]
  %7 = load double* %scevgep62, align 8           ; <double> [#uses=1]
  %8 = fmul double %2, %7                         ; <double> [#uses=1]
  %9 = fadd double %6, %8                         ; <double> [#uses=1]
  store double %9, double* %scevgep63, align 8
  %10 = add nsw i64 %j.039, 1                     ; <i64> [#uses=2]
  %exitcond60 = icmp eq i64 %10, %n               ; <i1> [#uses=1]
  br i1 %exitcond60, label %bb3, label %bb1

bb3:                                              ; preds = %bb1
  %11 = add nsw i64 %i.041, 1                     ; <i64> [#uses=2]
  %exitcond64 = icmp eq i64 %11, %n               ; <i1> [#uses=1]
  br i1 %exitcond64, label %bb10.preheader, label %bb.nph40

bb10.preheader:                                   ; preds = %bb3
  br i1 %0, label %bb.nph38.bb.nph38.split_crit_edge, label %return

bb.nph30:                                         ; preds = %bb.nph38.bb.nph38.split_crit_edge, %bb9
  %i.134 = phi i64 [ 0, %bb.nph38.bb.nph38.split_crit_edge ], [ %18, %bb9 ] ; <i64> [#uses=3]
  %scevgep59 = getelementptr [4000 x double]* @x, i64 0, i64 %i.134 ; <double*> [#uses=2]
  %.promoted31 = load double* %scevgep59          ; <double> [#uses=1]
  br label %bb7

bb7:                                              ; preds = %bb7, %bb.nph30
  %.tmp.032 = phi double [ %.promoted31, %bb.nph30 ], [ %16, %bb7 ] ; <double> [#uses=1]
  %j.129 = phi i64 [ 0, %bb.nph30 ], [ %17, %bb7 ] ; <i64> [#uses=3]
  %scevgep56 = getelementptr [4000 x [4000 x double]]* @A, i64 0, i64 %j.129, i64 %i.134 ; <double*> [#uses=1]
  %scevgep55 = getelementptr [4000 x double]* @y, i64 0, i64 %j.129 ; <double*> [#uses=1]
  %12 = load double* %scevgep56, align 8          ; <double> [#uses=1]
  %13 = fmul double %12, %19                      ; <double> [#uses=1]
  %14 = load double* %scevgep55, align 8          ; <double> [#uses=1]
  %15 = fmul double %13, %14                      ; <double> [#uses=1]
  %16 = fadd double %.tmp.032, %15                ; <double> [#uses=2]
  %17 = add nsw i64 %j.129, 1                     ; <i64> [#uses=2]
  %exitcond54 = icmp eq i64 %17, %n               ; <i1> [#uses=1]
  br i1 %exitcond54, label %bb9, label %bb7

bb9:                                              ; preds = %bb7
  store double %16, double* %scevgep59
  %18 = add nsw i64 %i.134, 1                     ; <i64> [#uses=2]
  %exitcond57 = icmp eq i64 %18, %n               ; <i1> [#uses=1]
  br i1 %exitcond57, label %bb13.preheader, label %bb.nph30

bb.nph38.bb.nph38.split_crit_edge:                ; preds = %bb10.preheader
  %19 = load double* @beta, align 8               ; <double> [#uses=1]
  br label %bb.nph30

bb13.preheader:                                   ; preds = %bb9
  br i1 %0, label %bb12, label %return

bb12:                                             ; preds = %bb12, %bb13.preheader
  %i.227 = phi i64 [ %23, %bb12 ], [ 0, %bb13.preheader ] ; <i64> [#uses=3]
  %scevgep51 = getelementptr [4000 x double]* @x, i64 0, i64 %i.227 ; <double*> [#uses=2]
  %scevgep52 = getelementptr [4000 x double]* @z, i64 0, i64 %i.227 ; <double*> [#uses=1]
  %20 = load double* %scevgep51, align 8          ; <double> [#uses=1]
  %21 = load double* %scevgep52, align 8          ; <double> [#uses=1]
  %22 = fadd double %20, %21                      ; <double> [#uses=1]
  store double %22, double* %scevgep51, align 8
  %23 = add nsw i64 %i.227, 1                     ; <i64> [#uses=2]
  %exitcond50 = icmp eq i64 %23, %n               ; <i1> [#uses=1]
  br i1 %exitcond50, label %bb19.preheader, label %bb12

bb19.preheader:                                   ; preds = %bb12
  br i1 %0, label %bb.nph26.bb.nph26.split_crit_edge, label %return

bb.nph:                                           ; preds = %bb.nph26.bb.nph26.split_crit_edge, %bb18
  %i.322 = phi i64 [ 0, %bb.nph26.bb.nph26.split_crit_edge ], [ %30, %bb18 ] ; <i64> [#uses=3]
  %scevgep49 = getelementptr [4000 x double]* @w, i64 0, i64 %i.322 ; <double*> [#uses=2]
  %.promoted = load double* %scevgep49            ; <double> [#uses=1]
  br label %bb16

bb16:                                             ; preds = %bb16, %bb.nph
  %.tmp.0 = phi double [ %.promoted, %bb.nph ], [ %28, %bb16 ] ; <double> [#uses=1]
  %j.221 = phi i64 [ 0, %bb.nph ], [ %29, %bb16 ] ; <i64> [#uses=3]
  %scevgep46 = getelementptr [4000 x [4000 x double]]* @A, i64 0, i64 %i.322, i64 %j.221 ; <double*> [#uses=1]
  %scevgep = getelementptr [4000 x double]* @x, i64 0, i64 %j.221 ; <double*> [#uses=1]
  %24 = load double* %scevgep46, align 8          ; <double> [#uses=1]
  %25 = fmul double %24, %31                      ; <double> [#uses=1]
  %26 = load double* %scevgep, align 8            ; <double> [#uses=1]
  %27 = fmul double %25, %26                      ; <double> [#uses=1]
  %28 = fadd double %.tmp.0, %27                  ; <double> [#uses=2]
  %29 = add nsw i64 %j.221, 1                     ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %29, %n                 ; <i1> [#uses=1]
  br i1 %exitcond, label %bb18, label %bb16

bb18:                                             ; preds = %bb16
  store double %28, double* %scevgep49
  %30 = add nsw i64 %i.322, 1                     ; <i64> [#uses=2]
  %exitcond47 = icmp eq i64 %30, %n               ; <i1> [#uses=1]
  br i1 %exitcond47, label %return, label %bb.nph

bb.nph26.bb.nph26.split_crit_edge:                ; preds = %bb19.preheader
  %31 = load double* @alpha, align 8              ; <double> [#uses=1]
  br label %bb.nph

return:                                           ; preds = %bb18, %bb19.preheader, %bb13.preheader, %bb10.preheader, %entry
  ret void
}

; CHECK: SCoP: bb.nph => return.loopexit        Parameters: (%n, ), Max Loop Depth: 2
; CHECK: Bounds of Loop: bb.nph:        { 0, 1 * %n + -1}
; CHECK:   BB: bb.nph{
; CHECK:     Writes %.promoted[]
; CHECK:     Reads @w[8 * {0,+,1}<%bb.nph> + 0]
; CHECK:   }
; CHECK:   Bounds of Loop: bb16:        { 0, 1 * %n + -1}
; CHECK:     BB: bb16{
; CHECK:       Reads %.promoted[]
; CHECK:       Reads %28[]
; CHECK:       Reads @A[8 * {0,+,1}<%bb16> + 32000 * {0,+,1}<%bb.nph> + 0]
; CHECK:       Reads %31[]
; CHECK:       Reads @x[8 * {0,+,1}<%bb16> + 0]
; CHECK:       Writes %28[]
; CHECK:     }
; CHECK:   BB: bb18{
; CHECK:     Reads %28[]
; CHECK:     Writes @w[8 * {0,+,1}<%bb.nph> + 0]
; CHECK:   }
; CHECK: SCoP: bb12 => bb19.preheader   Parameters: (%n, ), Max Loop Depth: 1
; CHECK: Bounds of Loop: bb12:  { 0, 1 * %n + -1}
; CHECK:   BB: bb12{
; CHECK:     Reads @x[8 * {0,+,1}<%bb12> + 0]
; CHECK:     Reads @z[8 * {0,+,1}<%bb12> + 0]
; CHECK:     Writes @x[8 * {0,+,1}<%bb12> + 0]
; CHECK:   }
; CHECK: SCoP: bb.nph30 => bb13.preheader       Parameters: (%n, ), Max Loop Depth: 2
; CHECK: Bounds of Loop: bb.nph30:      { 0, 1 * %n + -1}
; CHECK:   BB: bb.nph30{
; CHECK:     Writes %.promoted31[]
; CHECK:     Reads @x[8 * {0,+,1}<%bb.nph30> + 0]
; CHECK:   }
; CHECK:   Bounds of Loop: bb7: { 0, 1 * %n + -1}
; CHECK:     BB: bb7{
; CHECK:       Reads %.promoted31[]
; CHECK:       Reads %16[]
; CHECK:       Reads @A[8 * {0,+,1}<%bb.nph30> + 32000 * {0,+,1}<%bb7> + 0]
; CHECK:       Reads %19[]
; CHECK:       Reads @y[8 * {0,+,1}<%bb7> + 0]
; CHECK:       Writes %16[]
; CHECK:     }
; CHECK:   BB: bb9{
; CHECK:     Reads %16[]
; CHECK:     Writes @x[8 * {0,+,1}<%bb.nph30> + 0]
; CHECK:   }
; CHECK: SCoP: bb.nph40 => bb10.preheader       Parameters: (%n, ), Max Loop Depth: 2
; CHECK: Bounds of Loop: bb.nph40:      { 0, 1 * %n + -1}
; CHECK:   BB: bb.nph40{
; CHECK:     Writes %1[]
; CHECK:     Reads @u1[8 * {0,+,1}<%bb.nph40> + 0]
; CHECK:     Writes %2[]
; CHECK:     Reads @u2[8 * {0,+,1}<%bb.nph40> + 0]
; CHECK:   }
; CHECK:   Bounds of Loop: bb1: { 0, 1 * %n + -1}
; CHECK:     BB: bb1{
; CHECK:       Reads @A[32000 * {0,+,1}<%bb.nph40> + 8 * {0,+,1}<%bb1> + 0]
; CHECK:       Reads @v1[8 * {0,+,1}<%bb1> + 0]
; CHECK:       Reads %1[]
; CHECK:       Reads @v2[8 * {0,+,1}<%bb1> + 0]
; CHECK:       Reads %2[]
; CHECK:       Writes @A[32000 * {0,+,1}<%bb.nph40> + 8 * {0,+,1}<%bb1> + 0]
; CHECK:     }

