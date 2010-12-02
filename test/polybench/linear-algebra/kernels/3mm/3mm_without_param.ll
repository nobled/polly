; RUN: %opt  -mem2reg -loopsimplify -indvars -polly-prepare -polly-detect -analyze  %s | FileCheck %s
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }
@A = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=2]
@B = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=2]
@C = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=2]
@D = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=2]
@E = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=3]
@F = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=3]
@G = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=4]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
define void @scop_func() nounwind {
bb.nph76.bb.nph76.split_crit_edge:
  br label %bb5.preheader

bb4.us:                                           ; preds = %bb2.us
  store double %4, double* %scevgep94
  %0 = add nsw i64 %storemerge758.us, 1           ; <i64> [#uses=2]
  %exitcond92 = icmp eq i64 %0, 512               ; <i1> [#uses=1]
  br i1 %exitcond92, label %bb6, label %bb.nph54.us

bb2.us:                                           ; preds = %bb.nph54.us, %bb2.us
  %.tmp.056.us = phi double [ 0.000000e+00, %bb.nph54.us ], [ %4, %bb2.us ] ; <double> [#uses=1]
  %storemerge853.us = phi i64 [ 0, %bb.nph54.us ], [ %5, %bb2.us ] ; <i64> [#uses=3]
  %scevgep90 = getelementptr [512 x [512 x double]]* @B, i64 0, i64 %storemerge853.us, i64 %storemerge758.us ; <double*> [#uses=1]
  %scevgep91 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %storemerge63, i64 %storemerge853.us ; <double*> [#uses=1]
  %1 = load double* %scevgep91, align 8           ; <double> [#uses=1]
  %2 = load double* %scevgep90, align 8           ; <double> [#uses=1]
  %3 = fmul double %1, %2                         ; <double> [#uses=1]
  %4 = fadd double %.tmp.056.us, %3               ; <double> [#uses=2]
  %5 = add nsw i64 %storemerge853.us, 1           ; <i64> [#uses=2]
  %exitcond89 = icmp eq i64 %5, 512               ; <i1> [#uses=1]
  br i1 %exitcond89, label %bb4.us, label %bb2.us

bb.nph54.us:                                      ; preds = %bb5.preheader, %bb4.us
  %storemerge758.us = phi i64 [ %0, %bb4.us ], [ 0, %bb5.preheader ] ; <i64> [#uses=3]
  %scevgep94 = getelementptr [512 x [512 x double]]* @E, i64 0, i64 %storemerge63, i64 %storemerge758.us ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep94, align 8
  br label %bb2.us

bb6:                                              ; preds = %bb4.us
  %6 = add nsw i64 %storemerge63, 1               ; <i64> [#uses=2]
  %7 = icmp slt i64 %6, 512                       ; <i1> [#uses=1]
  br i1 %7, label %bb5.preheader, label %bb14.preheader

bb5.preheader:                                    ; preds = %bb6, %bb.nph76.bb.nph76.split_crit_edge
  %storemerge63 = phi i64 [ 0, %bb.nph76.bb.nph76.split_crit_edge ], [ %6, %bb6 ] ; <i64> [#uses=3]
  br label %bb.nph54.us

bb13.us:                                          ; preds = %bb11.us
  store double %12, double* %scevgep87
  %8 = add nsw i64 %storemerge534.us, 1           ; <i64> [#uses=2]
  %exitcond85 = icmp eq i64 %8, 512               ; <i1> [#uses=1]
  br i1 %exitcond85, label %bb15, label %bb.nph30.us

bb11.us:                                          ; preds = %bb.nph30.us, %bb11.us
  %.tmp.032.us = phi double [ 0.000000e+00, %bb.nph30.us ], [ %12, %bb11.us ] ; <double> [#uses=1]
  %storemerge629.us = phi i64 [ 0, %bb.nph30.us ], [ %13, %bb11.us ] ; <i64> [#uses=3]
  %scevgep83 = getelementptr [512 x [512 x double]]* @D, i64 0, i64 %storemerge629.us, i64 %storemerge534.us ; <double*> [#uses=1]
  %scevgep84 = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %storemerge139, i64 %storemerge629.us ; <double*> [#uses=1]
  %9 = load double* %scevgep84, align 8           ; <double> [#uses=1]
  %10 = load double* %scevgep83, align 8          ; <double> [#uses=1]
  %11 = fmul double %9, %10                       ; <double> [#uses=1]
  %12 = fadd double %.tmp.032.us, %11             ; <double> [#uses=2]
  %13 = add nsw i64 %storemerge629.us, 1          ; <i64> [#uses=2]
  %exitcond82 = icmp eq i64 %13, 512              ; <i1> [#uses=1]
  br i1 %exitcond82, label %bb13.us, label %bb11.us

bb.nph30.us:                                      ; preds = %bb14.preheader, %bb13.us
  %storemerge534.us = phi i64 [ %8, %bb13.us ], [ 0, %bb14.preheader ] ; <i64> [#uses=3]
  %scevgep87 = getelementptr [512 x [512 x double]]* @F, i64 0, i64 %storemerge139, i64 %storemerge534.us ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep87, align 8
  br label %bb11.us

bb15:                                             ; preds = %bb13.us
  %14 = add nsw i64 %storemerge139, 1             ; <i64> [#uses=2]
  %15 = icmp slt i64 %14, 512                     ; <i1> [#uses=1]
  br i1 %15, label %bb14.preheader, label %bb23.preheader

bb14.preheader:                                   ; preds = %bb15, %bb6
  %storemerge139 = phi i64 [ %14, %bb15 ], [ 0, %bb6 ] ; <i64> [#uses=3]
  br label %bb.nph30.us

bb22.us:                                          ; preds = %bb20.us
  store double %20, double* %scevgep80
  %16 = add nsw i64 %storemerge310.us, 1          ; <i64> [#uses=2]
  %exitcond78 = icmp eq i64 %16, 512              ; <i1> [#uses=1]
  br i1 %exitcond78, label %bb24, label %bb.nph.us

bb20.us:                                          ; preds = %bb.nph.us, %bb20.us
  %.tmp.0.us = phi double [ 0.000000e+00, %bb.nph.us ], [ %20, %bb20.us ] ; <double> [#uses=1]
  %storemerge49.us = phi i64 [ 0, %bb.nph.us ], [ %21, %bb20.us ] ; <i64> [#uses=3]
  %scevgep = getelementptr [512 x [512 x double]]* @F, i64 0, i64 %storemerge49.us, i64 %storemerge310.us ; <double*> [#uses=1]
  %scevgep77 = getelementptr [512 x [512 x double]]* @E, i64 0, i64 %storemerge215, i64 %storemerge49.us ; <double*> [#uses=1]
  %17 = load double* %scevgep77, align 8          ; <double> [#uses=1]
  %18 = load double* %scevgep, align 8            ; <double> [#uses=1]
  %19 = fmul double %17, %18                      ; <double> [#uses=1]
  %20 = fadd double %.tmp.0.us, %19               ; <double> [#uses=2]
  %21 = add nsw i64 %storemerge49.us, 1           ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %21, 512                ; <i1> [#uses=1]
  br i1 %exitcond, label %bb22.us, label %bb20.us

bb.nph.us:                                        ; preds = %bb23.preheader, %bb22.us
  %storemerge310.us = phi i64 [ %16, %bb22.us ], [ 0, %bb23.preheader ] ; <i64> [#uses=3]
  %scevgep80 = getelementptr [512 x [512 x double]]* @G, i64 0, i64 %storemerge215, i64 %storemerge310.us ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep80, align 8
  br label %bb20.us

bb24:                                             ; preds = %bb22.us
  %22 = add nsw i64 %storemerge215, 1             ; <i64> [#uses=2]
  %23 = icmp slt i64 %22, 512                     ; <i1> [#uses=1]
  br i1 %23, label %bb23.preheader, label %return

bb23.preheader:                                   ; preds = %bb24, %bb15
  %storemerge215 = phi i64 [ %22, %bb24 ], [ 0, %bb15 ] ; <i64> [#uses=3]
  br label %bb.nph.us

return:                                           ; preds = %bb24
  ret void
}

; CHECK: Valid Region for SCoP: bb5.preheader => return

