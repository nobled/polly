; RUN: opt %loadPolly %polybenchOpts %defaultOpts -polly-analyze-ir  -print-top-scop-only -analyze %s | FileCheck %s
; Unknown failure reason.
; XFAIL: *
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }
@A = common global [1024 x [1024 x double]] zeroinitializer, align 32 ; <[1024 x [1024 x double]]*> [#uses=24]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
define void @scop_func(i64 %n) nounwind {
bb.nph20:
  %0 = add nsw i64 %n, -2                         ; <i64> [#uses=3]
  %1 = icmp slt i64 %0, 1                         ; <i1> [#uses=2]
  br i1 %1, label %return, label %bb.nph8

bb.nph:                                           ; preds = %bb.nph8, %bb4
  %indvar21 = phi i64 [ %tmp39, %bb4 ], [ 0, %bb.nph8 ] ; <i64> [#uses=5]
  %tmp39 = add i64 %indvar21, 1                   ; <i64> [#uses=5]
  %tmp43 = add i64 %indvar21, 2                   ; <i64> [#uses=4]
  %scevgep26.phi.trans.insert = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar21, i64 1 ; <double*> [#uses=1]
  %.pre = load double* %scevgep26.phi.trans.insert, align 8 ; <double> [#uses=1]
  %scevgep25.phi.trans.insert = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp39, i64 1 ; <double*> [#uses=1]
  %.pre47 = load double* %scevgep25.phi.trans.insert, align 8 ; <double> [#uses=1]
  %scevgep.phi.trans.insert = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp43, i64 1 ; <double*> [#uses=1]
  %.pre48 = load double* %scevgep.phi.trans.insert, align 8 ; <double> [#uses=1]
  %scevgep30.phi.trans.insert = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp39, i64 0 ; <double*> [#uses=1]
  %.pre49 = load double* %scevgep30.phi.trans.insert, align 32 ; <double> [#uses=1]
  br label %bb2

bb2:                                              ; preds = %bb2, %bb.nph
  %2 = phi double [ %.pre49, %bb.nph ], [ %19, %bb2 ] ; <double> [#uses=1]
  %3 = phi double [ %.pre48, %bb.nph ], [ %17, %bb2 ] ; <double> [#uses=1]
  %4 = phi double [ %.pre47, %bb.nph ], [ %12, %bb2 ] ; <double> [#uses=1]
  %5 = phi double [ %.pre, %bb.nph ], [ %8, %bb2 ] ; <double> [#uses=1]
  %indvar = phi i64 [ 0, %bb.nph ], [ %tmp37, %bb2 ] ; <i64> [#uses=4]
  %tmp34 = add i64 %indvar, 2                     ; <i64> [#uses=4]
  %scevgep29 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar21, i64 %tmp34 ; <double*> [#uses=1]
  %scevgep27 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar21, i64 %indvar ; <double*> [#uses=1]
  %tmp37 = add i64 %indvar, 1                     ; <i64> [#uses=2]
  %scevgep31 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp39, i64 %tmp34 ; <double*> [#uses=1]
  %scevgep25 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp39, i64 %tmp37 ; <double*> [#uses=1]
  %scevgep33 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp43, i64 %tmp34 ; <double*> [#uses=1]
  %scevgep32 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp43, i64 %indvar ; <double*> [#uses=1]
  %6 = load double* %scevgep27, align 8           ; <double> [#uses=1]
  %7 = fadd double %6, %5                         ; <double> [#uses=1]
  %8 = load double* %scevgep29, align 8           ; <double> [#uses=2]
  %9 = fadd double %7, %8                         ; <double> [#uses=1]
  %10 = fadd double %9, %2                        ; <double> [#uses=1]
  %11 = fadd double %10, %4                       ; <double> [#uses=1]
  %12 = load double* %scevgep31, align 8          ; <double> [#uses=2]
  %13 = fadd double %11, %12                      ; <double> [#uses=1]
  %14 = load double* %scevgep32, align 8          ; <double> [#uses=1]
  %15 = fadd double %13, %14                      ; <double> [#uses=1]
  %16 = fadd double %15, %3                       ; <double> [#uses=1]
  %17 = load double* %scevgep33, align 8          ; <double> [#uses=2]
  %18 = fadd double %16, %17                      ; <double> [#uses=1]
  %19 = fdiv double %18, 9.000000e+00             ; <double> [#uses=2]
  store double %19, double* %scevgep25, align 8
  %20 = icmp slt i64 %0, %tmp34                   ; <i1> [#uses=1]
  br i1 %20, label %bb4, label %bb2

bb4:                                              ; preds = %bb2
  %21 = icmp slt i64 %0, %tmp43                   ; <i1> [#uses=1]
  br i1 %21, label %bb6, label %bb.nph

bb.nph8:                                          ; preds = %bb6, %bb.nph20
  %storemerge9 = phi i64 [ %22, %bb6 ], [ 0, %bb.nph20 ] ; <i64> [#uses=1]
  br i1 %1, label %bb6, label %bb.nph

bb6:                                              ; preds = %bb.nph8, %bb4
  %22 = add nsw i64 %storemerge9, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %22, 20                 ; <i1> [#uses=1]
  br i1 %exitcond, label %return, label %bb.nph8

return:                                           ; preds = %bb6, %bb.nph20
  ret void
}
