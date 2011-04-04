; RUN: opt %loadPolly %polybenchOpts %defaultOpts -polly-detect -analyze  %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }
@A = common global [1024 x [1024 x double]] zeroinitializer, align 32 ; <[1024 x [1024 x double]]*> [#uses=24]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
define void @scop_func() nounwind {
bb.nph20.bb.nph20.split_crit_edge:
  br label %bb5.preheader

bb.nph:                                           ; preds = %bb5.preheader, %bb4
  %indvar21 = phi i64 [ %tmp40, %bb4 ], [ 0, %bb5.preheader ] ; <i64> [#uses=5]
  %tmp40 = add i64 %indvar21, 1                   ; <i64> [#uses=6]
  %tmp44 = add i64 %indvar21, 2                   ; <i64> [#uses=3]
  %scevgep26.phi.trans.insert = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar21, i64 1 ; <double*> [#uses=1]
  %.pre = load double* %scevgep26.phi.trans.insert, align 8 ; <double> [#uses=1]
  %scevgep25.phi.trans.insert = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp40, i64 1 ; <double*> [#uses=1]
  %.pre49 = load double* %scevgep25.phi.trans.insert, align 8 ; <double> [#uses=1]
  %scevgep.phi.trans.insert = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp44, i64 1 ; <double*> [#uses=1]
  %.pre50 = load double* %scevgep.phi.trans.insert, align 8 ; <double> [#uses=1]
  %scevgep30.phi.trans.insert = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp40, i64 0 ; <double*> [#uses=1]
  %.pre51 = load double* %scevgep30.phi.trans.insert, align 32 ; <double> [#uses=1]
  br label %bb2

bb2:                                              ; preds = %bb2, %bb.nph
  %0 = phi double [ %.pre51, %bb.nph ], [ %17, %bb2 ] ; <double> [#uses=1]
  %1 = phi double [ %.pre50, %bb.nph ], [ %15, %bb2 ] ; <double> [#uses=1]
  %2 = phi double [ %.pre49, %bb.nph ], [ %10, %bb2 ] ; <double> [#uses=1]
  %3 = phi double [ %.pre, %bb.nph ], [ %6, %bb2 ] ; <double> [#uses=1]
  %indvar = phi i64 [ 0, %bb.nph ], [ %tmp38, %bb2 ] ; <i64> [#uses=4]
  %tmp35 = add i64 %indvar, 2                     ; <i64> [#uses=3]
  %scevgep29 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar21, i64 %tmp35 ; <double*> [#uses=1]
  %scevgep27 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar21, i64 %indvar ; <double*> [#uses=1]
  %tmp38 = add i64 %indvar, 1                     ; <i64> [#uses=3]
  %scevgep31 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp40, i64 %tmp35 ; <double*> [#uses=1]
  %scevgep25 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp40, i64 %tmp38 ; <double*> [#uses=1]
  %scevgep33 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp44, i64 %tmp35 ; <double*> [#uses=1]
  %scevgep32 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp44, i64 %indvar ; <double*> [#uses=1]
  %4 = load double* %scevgep27, align 8           ; <double> [#uses=1]
  %5 = fadd double %4, %3                         ; <double> [#uses=1]
  %6 = load double* %scevgep29, align 8           ; <double> [#uses=2]
  %7 = fadd double %5, %6                         ; <double> [#uses=1]
  %8 = fadd double %7, %0                         ; <double> [#uses=1]
  %9 = fadd double %8, %2                         ; <double> [#uses=1]
  %10 = load double* %scevgep31, align 8          ; <double> [#uses=2]
  %11 = fadd double %9, %10                       ; <double> [#uses=1]
  %12 = load double* %scevgep32, align 8          ; <double> [#uses=1]
  %13 = fadd double %11, %12                      ; <double> [#uses=1]
  %14 = fadd double %13, %1                       ; <double> [#uses=1]
  %15 = load double* %scevgep33, align 8          ; <double> [#uses=2]
  %16 = fadd double %14, %15                      ; <double> [#uses=1]
  %17 = fdiv double %16, 9.000000e+00             ; <double> [#uses=2]
  store double %17, double* %scevgep25, align 8
  %exitcond = icmp eq i64 %tmp38, 1022            ; <i1> [#uses=1]
  br i1 %exitcond, label %bb4, label %bb2

bb4:                                              ; preds = %bb2
  %exitcond34 = icmp eq i64 %tmp40, 1022          ; <i1> [#uses=1]
  br i1 %exitcond34, label %bb6, label %bb.nph

bb6:                                              ; preds = %bb4
  %18 = add nsw i64 %storemerge9, 1               ; <i64> [#uses=2]
  %exitcond48 = icmp eq i64 %18, 20               ; <i1> [#uses=1]
  br i1 %exitcond48, label %return, label %bb5.preheader

bb5.preheader:                                    ; preds = %bb6, %bb.nph20.bb.nph20.split_crit_edge
  %storemerge9 = phi i64 [ 0, %bb.nph20.bb.nph20.split_crit_edge ], [ %18, %bb6 ] ; <i64> [#uses=1]
  br label %bb.nph

return:                                           ; preds = %bb6
  ret void
}

; CHECK: Valid Region for Scop: bb5.preheader => return


