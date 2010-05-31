; RUN: opt -O3 -indvars -polly-scop-detect -polly-print-temp-scop-in-detail -print-top-scop-only -analyze %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }
@alpha1 = common global double 0.000000e+00       ; <double*> [#uses=1]
@beta1 = common global double 0.000000e+00        ; <double*> [#uses=1]
@alpha2 = common global double 0.000000e+00       ; <double*> [#uses=1]
@beta2 = common global double 0.000000e+00        ; <double*> [#uses=1]
@A = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=3]
@B = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=3]
@C = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=5]
@D = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=3]
@E = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=5]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
define void @scop_func() nounwind {
bb.nph50.bb.nph50.split_crit_edge:
  br label %bb5.preheader

bb4.us:                                           ; preds = %bb2.us
  store double %4, double* %scevgep61
  %0 = add nsw i64 %storemerge431.us, 1           ; <i64> [#uses=2]
  %exitcond59 = icmp eq i64 %0, 512               ; <i1> [#uses=1]
  br i1 %exitcond59, label %bb6, label %bb.nph27.us

bb2.us:                                           ; preds = %bb.nph27.us, %bb2.us
  %.tmp.029.us = phi double [ 0.000000e+00, %bb.nph27.us ], [ %4, %bb2.us ] ; <double> [#uses=1]
  %storemerge526.us = phi i64 [ 0, %bb.nph27.us ], [ %5, %bb2.us ] ; <i64> [#uses=3]
  %scevgep57 = getelementptr [512 x [512 x double]]* @B, i64 0, i64 %storemerge526.us, i64 %storemerge431.us ; <double*> [#uses=1]
  %scevgep58 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %storemerge37, i64 %storemerge526.us ; <double*> [#uses=1]
  %1 = load double* %scevgep58, align 8           ; <double> [#uses=1]
  %2 = load double* %scevgep57, align 8           ; <double> [#uses=1]
  %3 = fmul double %1, %2                         ; <double> [#uses=1]
  %4 = fadd double %.tmp.029.us, %3               ; <double> [#uses=2]
  %5 = add nsw i64 %storemerge526.us, 1           ; <i64> [#uses=2]
  %exitcond56 = icmp eq i64 %5, 512               ; <i1> [#uses=1]
  br i1 %exitcond56, label %bb4.us, label %bb2.us

bb.nph27.us:                                      ; preds = %bb5.preheader, %bb4.us
  %storemerge431.us = phi i64 [ %0, %bb4.us ], [ 0, %bb5.preheader ] ; <i64> [#uses=3]
  %scevgep61 = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %storemerge37, i64 %storemerge431.us ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep61, align 8
  br label %bb2.us

bb6:                                              ; preds = %bb4.us
  %6 = add nsw i64 %storemerge37, 1               ; <i64> [#uses=2]
  %7 = icmp slt i64 %6, 512                       ; <i1> [#uses=1]
  br i1 %7, label %bb5.preheader, label %bb14.preheader

bb5.preheader:                                    ; preds = %bb6, %bb.nph50.bb.nph50.split_crit_edge
  %storemerge37 = phi i64 [ 0, %bb.nph50.bb.nph50.split_crit_edge ], [ %6, %bb6 ] ; <i64> [#uses=3]
  br label %bb.nph27.us

bb13.us:                                          ; preds = %bb11.us
  store double %12, double* %scevgep54
  %8 = add nsw i64 %storemerge27.us, 1            ; <i64> [#uses=2]
  %exitcond52 = icmp eq i64 %8, 512               ; <i1> [#uses=1]
  br i1 %exitcond52, label %bb15, label %bb.nph.us

bb11.us:                                          ; preds = %bb.nph.us, %bb11.us
  %.tmp.0.us = phi double [ 0.000000e+00, %bb.nph.us ], [ %12, %bb11.us ] ; <double> [#uses=1]
  %storemerge36.us = phi i64 [ 0, %bb.nph.us ], [ %13, %bb11.us ] ; <i64> [#uses=3]
  %scevgep = getelementptr [512 x [512 x double]]* @D, i64 0, i64 %storemerge36.us, i64 %storemerge27.us ; <double*> [#uses=1]
  %scevgep51 = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %storemerge112, i64 %storemerge36.us ; <double*> [#uses=1]
  %9 = load double* %scevgep51, align 8           ; <double> [#uses=1]
  %10 = load double* %scevgep, align 8            ; <double> [#uses=1]
  %11 = fmul double %9, %10                       ; <double> [#uses=1]
  %12 = fadd double %.tmp.0.us, %11               ; <double> [#uses=2]
  %13 = add nsw i64 %storemerge36.us, 1           ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %13, 512                ; <i1> [#uses=1]
  br i1 %exitcond, label %bb13.us, label %bb11.us

bb.nph.us:                                        ; preds = %bb14.preheader, %bb13.us
  %storemerge27.us = phi i64 [ %8, %bb13.us ], [ 0, %bb14.preheader ] ; <i64> [#uses=3]
  %scevgep54 = getelementptr [512 x [512 x double]]* @E, i64 0, i64 %storemerge112, i64 %storemerge27.us ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep54, align 8
  br label %bb11.us

bb15:                                             ; preds = %bb13.us
  %14 = add nsw i64 %storemerge112, 1             ; <i64> [#uses=2]
  %15 = icmp slt i64 %14, 512                     ; <i1> [#uses=1]
  br i1 %15, label %bb14.preheader, label %return

bb14.preheader:                                   ; preds = %bb15, %bb6
  %storemerge112 = phi i64 [ %14, %bb15 ], [ 0, %bb6 ] ; <i64> [#uses=3]
  br label %bb.nph.us

return:                                           ; preds = %bb15
  ret void
}

; CHECK: SCoP: bb.nph50.bb.nph50.split_crit_edge => <Function Return>   Parameters: (), Max Loop Depth: 3
; CHECK: Bounds of Loop: bb5.preheader: { 0, 511}
; CHECK:   Bounds of Loop: bb.nph27.us: { 0, 511}
; CHECK:     BB: bb.nph27.us{
; CHECK:       Writes @C[4096 * {0,+,1}<%bb5.preheader> + 8 * {0,+,1}<%bb.nph27.us> + 0]
; CHECK:     }
; CHECK:     Bounds of Loop: bb2.us:    { 0, 511}
; CHECK:       BB: bb2.us{
; CHECK:         Reads %4[]
; CHECK:         Reads @A[4096 * {0,+,1}<%bb5.preheader> + 8 * {0,+,1}<%bb2.us> + 0]
; CHECK:         Reads @B[8 * {0,+,1}<%bb.nph27.us> + 4096 * {0,+,1}<%bb2.us> + 0]
; CHECK:         Writes %4[]
; CHECK:       }
; CHECK:     BB: bb4.us{
; CHECK:       Reads %4[]
; CHECK:       Writes @C[4096 * {0,+,1}<%bb5.preheader> + 8 * {0,+,1}<%bb.nph27.us> + 0]
; CHECK:     }
; CHECK: Bounds of Loop: bb14.preheader:        { 0, 511}
; CHECK:   Bounds of Loop: bb.nph.us:   { 0, 511}
; CHECK:     BB: bb.nph.us{
; CHECK:       Writes @E[4096 * {0,+,1}<%bb14.preheader> + 8 * {0,+,1}<%bb.nph.us> + 0]
; CHECK:     }
; CHECK:     Bounds of Loop: bb11.us:   { 0, 511}
; CHECK:       BB: bb11.us{
; CHECK:         Reads %11[]
; CHECK:         Reads @C[8 * {0,+,1}<%bb11.us> + 4096 * {0,+,1}<%bb14.preheader> + 0]
; CHECK:         Reads @D[4096 * {0,+,1}<%bb11.us> + 8 * {0,+,1}<%bb.nph.us> + 0]
; CHECK:         Writes %11[]
; CHECK:       }
; CHECK:     BB: bb13.us{
; CHECK:       Reads %11[]
; CHECK:       Writes @E[4096 * {0,+,1}<%bb14.preheader> + 8 * {0,+,1}<%bb.nph.us> + 0]
; CHECK:     }
