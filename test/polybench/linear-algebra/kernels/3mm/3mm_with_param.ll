; RUN: opt -O3 -indvars -polly-scop-detect  -analyze %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }
@A = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=2]
@B = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=2]
@C = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=2]
@D = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=2]
@E = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=4]
@F = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=4]
@G = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=5]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
define void @scop_func(i64 %ni, i64 %nj, i64 %nk, i64 %nl, i64 %nm) nounwind {
entry:
  %0 = icmp sgt i64 %ni, 0                        ; <i1> [#uses=3]
  br i1 %0, label %bb.nph76.bb.nph76.split_crit_edge, label %return

bb.nph62:                                         ; preds = %bb.nph76.bb.nph76.split_crit_edge, %bb6
  %storemerge63 = phi i64 [ 0, %bb.nph76.bb.nph76.split_crit_edge ], [ %7, %bb6 ] ; <i64> [#uses=4]
  br i1 %9, label %bb.nph54.us, label %bb4

bb4.us:                                           ; preds = %bb2.us
  store double %5, double* %scevgep105
  %1 = add nsw i64 %storemerge758.us, 1           ; <i64> [#uses=2]
  %exitcond103 = icmp eq i64 %1, %ni              ; <i1> [#uses=1]
  br i1 %exitcond103, label %bb6, label %bb.nph54.us

bb2.us:                                           ; preds = %bb.nph54.us, %bb2.us
  %.tmp.056.us = phi double [ 0.000000e+00, %bb.nph54.us ], [ %5, %bb2.us ] ; <double> [#uses=1]
  %storemerge853.us = phi i64 [ 0, %bb.nph54.us ], [ %6, %bb2.us ] ; <i64> [#uses=3]
  %scevgep101 = getelementptr [512 x [512 x double]]* @B, i64 0, i64 %storemerge853.us, i64 %storemerge758.us ; <double*> [#uses=1]
  %scevgep102 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %storemerge63, i64 %storemerge853.us ; <double*> [#uses=1]
  %2 = load double* %scevgep102, align 8          ; <double> [#uses=1]
  %3 = load double* %scevgep101, align 8          ; <double> [#uses=1]
  %4 = fmul double %2, %3                         ; <double> [#uses=1]
  %5 = fadd double %.tmp.056.us, %4               ; <double> [#uses=2]
  %6 = add nsw i64 %storemerge853.us, 1           ; <i64> [#uses=2]
  %exitcond100 = icmp eq i64 %6, %nk              ; <i1> [#uses=1]
  br i1 %exitcond100, label %bb4.us, label %bb2.us

bb.nph54.us:                                      ; preds = %bb4.us, %bb.nph62
  %storemerge758.us = phi i64 [ %1, %bb4.us ], [ 0, %bb.nph62 ] ; <i64> [#uses=3]
  %scevgep105 = getelementptr [512 x [512 x double]]* @E, i64 0, i64 %storemerge63, i64 %storemerge758.us ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep105, align 8
  br label %bb2.us

bb4:                                              ; preds = %bb4, %bb.nph62
  %indvar108 = phi i64 [ %indvar.next109, %bb4 ], [ 0, %bb.nph62 ] ; <i64> [#uses=2]
  %storemerge70 = phi i64 [ %storemerge70, %bb4 ], [ %storemerge63, %bb.nph62 ] ; <i64> [#uses=3]
  %tmp112 = shl i64 %storemerge70, 9              ; <i64> [#uses=1]
  %scevgep111.sum = add i64 %indvar108, %tmp112   ; <i64> [#uses=1]
  %scevgep113 = getelementptr [512 x [512 x double]]* @E, i64 0, i64 0, i64 %scevgep111.sum ; <double*> [#uses=1]
  store double 0.000000e+00, double* %scevgep113, align 8
  %indvar.next109 = add i64 %indvar108, 1         ; <i64> [#uses=2]
  %exitcond110 = icmp eq i64 %indvar.next109, %ni ; <i1> [#uses=1]
  br i1 %exitcond110, label %bb6, label %bb4

bb6:                                              ; preds = %bb4, %bb4.us
  %storemerge75 = phi i64 [ %storemerge63, %bb4.us ], [ %storemerge70, %bb4 ] ; <i64> [#uses=1]
  %7 = add nsw i64 %storemerge75, 1               ; <i64> [#uses=2]
  %8 = icmp slt i64 %7, %ni                       ; <i1> [#uses=1]
  br i1 %8, label %bb.nph62, label %bb16.preheader

bb.nph76.bb.nph76.split_crit_edge:                ; preds = %entry
  %9 = icmp sgt i64 %nk, 0                        ; <i1> [#uses=1]
  br label %bb.nph62

bb16.preheader:                                   ; preds = %bb6
  br i1 %0, label %bb.nph52.bb.nph52.split_crit_edge, label %return

bb.nph38:                                         ; preds = %bb.nph52.bb.nph52.split_crit_edge, %bb15
  %storemerge139 = phi i64 [ 0, %bb.nph52.bb.nph52.split_crit_edge ], [ %16, %bb15 ] ; <i64> [#uses=4]
  br i1 %18, label %bb.nph30.us, label %bb13

bb13.us:                                          ; preds = %bb11.us
  store double %14, double* %scevgep90
  %10 = add nsw i64 %storemerge534.us, 1          ; <i64> [#uses=2]
  %exitcond88 = icmp eq i64 %10, %ni              ; <i1> [#uses=1]
  br i1 %exitcond88, label %bb15, label %bb.nph30.us

bb11.us:                                          ; preds = %bb.nph30.us, %bb11.us
  %.tmp.032.us = phi double [ 0.000000e+00, %bb.nph30.us ], [ %14, %bb11.us ] ; <double> [#uses=1]
  %storemerge629.us = phi i64 [ 0, %bb.nph30.us ], [ %15, %bb11.us ] ; <i64> [#uses=3]
  %scevgep86 = getelementptr [512 x [512 x double]]* @D, i64 0, i64 %storemerge629.us, i64 %storemerge534.us ; <double*> [#uses=1]
  %scevgep87 = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %storemerge139, i64 %storemerge629.us ; <double*> [#uses=1]
  %11 = load double* %scevgep87, align 8          ; <double> [#uses=1]
  %12 = load double* %scevgep86, align 8          ; <double> [#uses=1]
  %13 = fmul double %11, %12                      ; <double> [#uses=1]
  %14 = fadd double %.tmp.032.us, %13             ; <double> [#uses=2]
  %15 = add nsw i64 %storemerge629.us, 1          ; <i64> [#uses=2]
  %exitcond85 = icmp eq i64 %15, %nk              ; <i1> [#uses=1]
  br i1 %exitcond85, label %bb13.us, label %bb11.us

bb.nph30.us:                                      ; preds = %bb13.us, %bb.nph38
  %storemerge534.us = phi i64 [ %10, %bb13.us ], [ 0, %bb.nph38 ] ; <i64> [#uses=3]
  %scevgep90 = getelementptr [512 x [512 x double]]* @F, i64 0, i64 %storemerge139, i64 %storemerge534.us ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep90, align 8
  br label %bb11.us

bb13:                                             ; preds = %bb13, %bb.nph38
  %indvar93 = phi i64 [ %indvar.next94, %bb13 ], [ 0, %bb.nph38 ] ; <i64> [#uses=2]
  %storemerge146 = phi i64 [ %storemerge146, %bb13 ], [ %storemerge139, %bb.nph38 ] ; <i64> [#uses=3]
  %tmp97 = shl i64 %storemerge146, 9              ; <i64> [#uses=1]
  %scevgep96.sum = add i64 %indvar93, %tmp97      ; <i64> [#uses=1]
  %scevgep98 = getelementptr [512 x [512 x double]]* @F, i64 0, i64 0, i64 %scevgep96.sum ; <double*> [#uses=1]
  store double 0.000000e+00, double* %scevgep98, align 8
  %indvar.next94 = add i64 %indvar93, 1           ; <i64> [#uses=2]
  %exitcond95 = icmp eq i64 %indvar.next94, %ni   ; <i1> [#uses=1]
  br i1 %exitcond95, label %bb15, label %bb13

bb15:                                             ; preds = %bb13, %bb13.us
  %storemerge151 = phi i64 [ %storemerge139, %bb13.us ], [ %storemerge146, %bb13 ] ; <i64> [#uses=1]
  %16 = add nsw i64 %storemerge151, 1             ; <i64> [#uses=2]
  %17 = icmp slt i64 %16, %ni                     ; <i1> [#uses=1]
  br i1 %17, label %bb.nph38, label %bb25.preheader

bb.nph52.bb.nph52.split_crit_edge:                ; preds = %bb16.preheader
  %18 = icmp sgt i64 %nk, 0                       ; <i1> [#uses=1]
  br label %bb.nph38

bb25.preheader:                                   ; preds = %bb15
  br i1 %0, label %bb.nph28.bb.nph28.split_crit_edge, label %return

bb.nph14:                                         ; preds = %bb.nph28.bb.nph28.split_crit_edge, %bb24
  %storemerge215 = phi i64 [ 0, %bb.nph28.bb.nph28.split_crit_edge ], [ %25, %bb24 ] ; <i64> [#uses=4]
  br i1 %27, label %bb.nph.us, label %bb22

bb22.us:                                          ; preds = %bb20.us
  store double %23, double* %scevgep80
  %19 = add nsw i64 %storemerge310.us, 1          ; <i64> [#uses=2]
  %exitcond78 = icmp eq i64 %19, %ni              ; <i1> [#uses=1]
  br i1 %exitcond78, label %bb24, label %bb.nph.us

bb20.us:                                          ; preds = %bb.nph.us, %bb20.us
  %.tmp.0.us = phi double [ 0.000000e+00, %bb.nph.us ], [ %23, %bb20.us ] ; <double> [#uses=1]
  %storemerge49.us = phi i64 [ 0, %bb.nph.us ], [ %24, %bb20.us ] ; <i64> [#uses=3]
  %scevgep = getelementptr [512 x [512 x double]]* @F, i64 0, i64 %storemerge49.us, i64 %storemerge310.us ; <double*> [#uses=1]
  %scevgep77 = getelementptr [512 x [512 x double]]* @E, i64 0, i64 %storemerge215, i64 %storemerge49.us ; <double*> [#uses=1]
  %20 = load double* %scevgep77, align 8          ; <double> [#uses=1]
  %21 = load double* %scevgep, align 8            ; <double> [#uses=1]
  %22 = fmul double %20, %21                      ; <double> [#uses=1]
  %23 = fadd double %.tmp.0.us, %22               ; <double> [#uses=2]
  %24 = add nsw i64 %storemerge49.us, 1           ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %24, %nk                ; <i1> [#uses=1]
  br i1 %exitcond, label %bb22.us, label %bb20.us

bb.nph.us:                                        ; preds = %bb22.us, %bb.nph14
  %storemerge310.us = phi i64 [ %19, %bb22.us ], [ 0, %bb.nph14 ] ; <i64> [#uses=3]
  %scevgep80 = getelementptr [512 x [512 x double]]* @G, i64 0, i64 %storemerge215, i64 %storemerge310.us ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep80, align 8
  br label %bb20.us

bb22:                                             ; preds = %bb22, %bb.nph14
  %indvar = phi i64 [ %indvar.next, %bb22 ], [ 0, %bb.nph14 ] ; <i64> [#uses=2]
  %storemerge222 = phi i64 [ %storemerge222, %bb22 ], [ %storemerge215, %bb.nph14 ] ; <i64> [#uses=3]
  %tmp = shl i64 %storemerge222, 9                ; <i64> [#uses=1]
  %scevgep82.sum = add i64 %indvar, %tmp          ; <i64> [#uses=1]
  %scevgep83 = getelementptr [512 x [512 x double]]* @G, i64 0, i64 0, i64 %scevgep82.sum ; <double*> [#uses=1]
  store double 0.000000e+00, double* %scevgep83, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond81 = icmp eq i64 %indvar.next, %ni     ; <i1> [#uses=1]
  br i1 %exitcond81, label %bb24, label %bb22

bb24:                                             ; preds = %bb22, %bb22.us
  %storemerge227 = phi i64 [ %storemerge215, %bb22.us ], [ %storemerge222, %bb22 ] ; <i64> [#uses=1]
  %25 = add nsw i64 %storemerge227, 1             ; <i64> [#uses=2]
  %26 = icmp slt i64 %25, %ni                     ; <i1> [#uses=1]
  br i1 %26, label %bb.nph14, label %return

bb.nph28.bb.nph28.split_crit_edge:                ; preds = %bb25.preheader
  %27 = icmp sgt i64 %nk, 0                       ; <i1> [#uses=1]
  br label %bb.nph14

return:                                           ; preds = %bb24, %bb25.preheader, %bb16.preheader, %entry
  ret void
}

; CHECK: SCoP: entry => <Function Return>        Parameters: (%ni, %nk, ), Max Loop Depth: 3
