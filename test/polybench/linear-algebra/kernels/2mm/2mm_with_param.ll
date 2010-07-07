; RUN: opt  -O3 -loopsimplify -indvars -polly-analyze-ir  -print-top-scop-only -analyze %s | FileCheck %s
; XFAIL: *
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }
@alpha1 = common global double 0.000000e+00       ; <double*> [#uses=1]
@beta1 = common global double 0.000000e+00        ; <double*> [#uses=1]
@alpha2 = common global double 0.000000e+00       ; <double*> [#uses=1]
@beta2 = common global double 0.000000e+00        ; <double*> [#uses=1]
@A = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=2]
@B = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=2]
@C = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=4]
@D = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=2]
@E = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=5]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
define void @scop_func(i64 %ni, i64 %nj, i64 %nk, i64 %nl) nounwind {
entry:
  %0 = icmp sgt i64 %ni, 0                        ; <i1> [#uses=2]
  br i1 %0, label %bb.nph50, label %return

bb.nph35:                                         ; preds = %bb.nph50, %bb6
  %storemerge37 = phi i64 [ %7, %bb6 ], [ 0, %bb.nph50 ] ; <i64> [#uses=4]
  br i1 %10, label %bb.nph27.us, label %bb4

bb4.us:                                           ; preds = %bb2.us
  store double %5, double* %scevgep64
  %1 = add nsw i64 %storemerge431.us, 1           ; <i64> [#uses=2]
  %exitcond62 = icmp eq i64 %1, %nj               ; <i1> [#uses=1]
  br i1 %exitcond62, label %bb6, label %bb.nph27.us

bb2.us:                                           ; preds = %bb.nph27.us, %bb2.us
  %.tmp.029.us = phi double [ 0.000000e+00, %bb.nph27.us ], [ %5, %bb2.us ] ; <double> [#uses=1]
  %storemerge526.us = phi i64 [ 0, %bb.nph27.us ], [ %6, %bb2.us ] ; <i64> [#uses=3]
  %scevgep60 = getelementptr [512 x [512 x double]]* @B, i64 0, i64 %storemerge526.us, i64 %storemerge431.us ; <double*> [#uses=1]
  %scevgep61 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %storemerge37, i64 %storemerge526.us ; <double*> [#uses=1]
  %2 = load double* %scevgep61, align 8           ; <double> [#uses=1]
  %3 = load double* %scevgep60, align 8           ; <double> [#uses=1]
  %4 = fmul double %2, %3                         ; <double> [#uses=1]
  %5 = fadd double %.tmp.029.us, %4               ; <double> [#uses=2]
  %6 = add nsw i64 %storemerge526.us, 1           ; <i64> [#uses=2]
  %exitcond59 = icmp eq i64 %6, %nk               ; <i1> [#uses=1]
  br i1 %exitcond59, label %bb4.us, label %bb2.us

bb.nph27.us:                                      ; preds = %bb4.us, %bb.nph35
  %storemerge431.us = phi i64 [ %1, %bb4.us ], [ 0, %bb.nph35 ] ; <i64> [#uses=3]
  %scevgep64 = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %storemerge37, i64 %storemerge431.us ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep64, align 8
  br label %bb2.us

bb4:                                              ; preds = %bb4, %bb.nph35
  %indvar67 = phi i64 [ %indvar.next68, %bb4 ], [ 0, %bb.nph35 ] ; <i64> [#uses=2]
  %storemerge44 = phi i64 [ %storemerge44, %bb4 ], [ %storemerge37, %bb.nph35 ] ; <i64> [#uses=3]
  %tmp71 = shl i64 %storemerge44, 9               ; <i64> [#uses=1]
  %scevgep70.sum = add i64 %indvar67, %tmp71      ; <i64> [#uses=1]
  %scevgep72 = getelementptr [512 x [512 x double]]* @C, i64 0, i64 0, i64 %scevgep70.sum ; <double*> [#uses=1]
  store double 0.000000e+00, double* %scevgep72, align 8
  %indvar.next68 = add i64 %indvar67, 1           ; <i64> [#uses=2]
  %exitcond69 = icmp eq i64 %indvar.next68, %nj   ; <i1> [#uses=1]
  br i1 %exitcond69, label %bb6, label %bb4

bb6:                                              ; preds = %bb4, %bb4.us
  %storemerge49 = phi i64 [ %storemerge37, %bb4.us ], [ %storemerge44, %bb4 ] ; <i64> [#uses=1]
  %7 = add nsw i64 %storemerge49, 1               ; <i64> [#uses=2]
  %8 = icmp slt i64 %7, %ni                       ; <i1> [#uses=1]
  br i1 %8, label %bb.nph35, label %bb16.preheader

bb.nph50:                                         ; preds = %entry
  %9 = icmp sgt i64 %nj, 0                        ; <i1> [#uses=1]
  %10 = icmp sgt i64 %nk, 0                       ; <i1> [#uses=1]
  br i1 %9, label %bb.nph35, label %bb16.preheader

bb16.preheader:                                   ; preds = %bb.nph50, %bb6
  br i1 %0, label %bb.nph25, label %return

bb.nph11:                                         ; preds = %bb.nph25, %bb15
  %storemerge112 = phi i64 [ %17, %bb15 ], [ 0, %bb.nph25 ] ; <i64> [#uses=4]
  br i1 %20, label %bb.nph.us, label %bb13

bb13.us:                                          ; preds = %bb11.us
  store double %15, double* %scevgep54
  %11 = add nsw i64 %storemerge27.us, 1           ; <i64> [#uses=2]
  %exitcond52 = icmp eq i64 %11, %nl              ; <i1> [#uses=1]
  br i1 %exitcond52, label %bb15, label %bb.nph.us

bb11.us:                                          ; preds = %bb.nph.us, %bb11.us
  %.tmp.0.us = phi double [ 0.000000e+00, %bb.nph.us ], [ %15, %bb11.us ] ; <double> [#uses=1]
  %storemerge36.us = phi i64 [ 0, %bb.nph.us ], [ %16, %bb11.us ] ; <i64> [#uses=3]
  %scevgep = getelementptr [512 x [512 x double]]* @D, i64 0, i64 %storemerge36.us, i64 %storemerge27.us ; <double*> [#uses=1]
  %scevgep51 = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %storemerge112, i64 %storemerge36.us ; <double*> [#uses=1]
  %12 = load double* %scevgep51, align 8          ; <double> [#uses=1]
  %13 = load double* %scevgep, align 8            ; <double> [#uses=1]
  %14 = fmul double %12, %13                      ; <double> [#uses=1]
  %15 = fadd double %.tmp.0.us, %14               ; <double> [#uses=2]
  %16 = add nsw i64 %storemerge36.us, 1           ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %16, %nj                ; <i1> [#uses=1]
  br i1 %exitcond, label %bb13.us, label %bb11.us

bb.nph.us:                                        ; preds = %bb13.us, %bb.nph11
  %storemerge27.us = phi i64 [ %11, %bb13.us ], [ 0, %bb.nph11 ] ; <i64> [#uses=3]
  %scevgep54 = getelementptr [512 x [512 x double]]* @E, i64 0, i64 %storemerge112, i64 %storemerge27.us ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep54, align 8
  br label %bb11.us

bb13:                                             ; preds = %bb13, %bb.nph11
  %indvar = phi i64 [ %indvar.next, %bb13 ], [ 0, %bb.nph11 ] ; <i64> [#uses=2]
  %storemerge119 = phi i64 [ %storemerge119, %bb13 ], [ %storemerge112, %bb.nph11 ] ; <i64> [#uses=3]
  %tmp = shl i64 %storemerge119, 9                ; <i64> [#uses=1]
  %scevgep56.sum = add i64 %indvar, %tmp          ; <i64> [#uses=1]
  %scevgep57 = getelementptr [512 x [512 x double]]* @E, i64 0, i64 0, i64 %scevgep56.sum ; <double*> [#uses=1]
  store double 0.000000e+00, double* %scevgep57, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond55 = icmp eq i64 %indvar.next, %nl     ; <i1> [#uses=1]
  br i1 %exitcond55, label %bb15, label %bb13

bb15:                                             ; preds = %bb13, %bb13.us
  %storemerge124 = phi i64 [ %storemerge112, %bb13.us ], [ %storemerge119, %bb13 ] ; <i64> [#uses=1]
  %17 = add nsw i64 %storemerge124, 1             ; <i64> [#uses=2]
  %18 = icmp slt i64 %17, %ni                     ; <i1> [#uses=1]
  br i1 %18, label %bb.nph11, label %return

bb.nph25:                                         ; preds = %bb16.preheader
  %19 = icmp sgt i64 %nl, 0                       ; <i1> [#uses=1]
  %20 = icmp sgt i64 %nj, 0                       ; <i1> [#uses=1]
  br i1 %19, label %bb.nph11, label %return

return:                                           ; preds = %bb.nph25, %bb15, %bb16.preheader, %entry
  ret void
}
; CHECK: SCoP: entry => <Function Return>        Parameters: (%n{{[i-l]}}, %n{{[i-l]}}, %n{{[i-l]}}, %n{{[i-l]}}, ), Max Loop Depth: 3
