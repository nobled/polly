; RUN: opt  -O3 -loopsimplify -indvars -polly-scop-detect  -print-top-scop-only -analyze %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }
@A = common global [1024 x [1024 x double]] zeroinitializer, align 32 ; <[1024 x [1024 x double]]*> [#uses=10]
@B = common global [1024 x [1024 x double]] zeroinitializer, align 32 ; <[1024 x [1024 x double]]*> [#uses=4]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
define void @scop_func() nounwind {
bb.nph35:
  br label %bb5.preheader

bb.nph:                                           ; preds = %bb5.preheader, %bb4
  %indvar36 = phi i64 [ %tmp49, %bb4 ], [ 0, %bb5.preheader ] ; <i64> [#uses=3]
  %tmp49 = add i64 %indvar36, 1                   ; <i64> [#uses=3]
  %tmp52 = add i64 %indvar36, 3                   ; <i64> [#uses=1]
  %tmp54 = add i64 %indvar36, 2                   ; <i64> [#uses=4]
  %scevgep39.phi.trans.insert = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp54, i64 2 ; <double*> [#uses=1]
  %.pre = load double* %scevgep39.phi.trans.insert, align 16 ; <double> [#uses=1]
  br label %bb2

bb2:                                              ; preds = %bb2, %bb.nph
  %0 = phi double [ %.pre, %bb.nph ], [ %3, %bb2 ] ; <double> [#uses=1]
  %indvar = phi i64 [ 0, %bb.nph ], [ %tmp57, %bb2 ] ; <i64> [#uses=3]
  %tmp50 = add i64 %indvar, 2                     ; <i64> [#uses=3]
  %scevgep43 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp49, i64 %tmp50 ; <double*> [#uses=1]
  %scevgep41 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp52, i64 %tmp50 ; <double*> [#uses=1]
  %tmp55 = add i64 %indvar, 3                     ; <i64> [#uses=1]
  %scevgep47 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp54, i64 %tmp55 ; <double*> [#uses=1]
  %tmp57 = add i64 %indvar, 1                     ; <i64> [#uses=3]
  %scevgep45 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp54, i64 %tmp57 ; <double*> [#uses=1]
  %scevgep = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %tmp54, i64 %tmp50 ; <double*> [#uses=1]
  %1 = load double* %scevgep45, align 8           ; <double> [#uses=1]
  %2 = fadd double %0, %1                         ; <double> [#uses=1]
  %3 = load double* %scevgep47, align 8           ; <double> [#uses=2]
  %4 = fadd double %2, %3                         ; <double> [#uses=1]
  %5 = load double* %scevgep41, align 8           ; <double> [#uses=1]
  %6 = fadd double %4, %5                         ; <double> [#uses=1]
  %7 = load double* %scevgep43, align 8           ; <double> [#uses=1]
  %8 = fadd double %6, %7                         ; <double> [#uses=1]
  %9 = fmul double %8, 2.000000e-01               ; <double> [#uses=1]
  store double %9, double* %scevgep, align 8
  %exitcond = icmp eq i64 %tmp57, 1021            ; <i1> [#uses=1]
  br i1 %exitcond, label %bb4, label %bb2

bb4:                                              ; preds = %bb2
  %exitcond48 = icmp eq i64 %tmp49, 1021          ; <i1> [#uses=1]
  br i1 %exitcond48, label %bb9.preheader, label %bb.nph

bb8:                                              ; preds = %bb9.preheader, %bb8
  %indvar61 = phi i64 [ %indvar.next62, %bb8 ], [ 0, %bb9.preheader ] ; <i64> [#uses=2]
  %tmp71 = add i64 %indvar61, 2                   ; <i64> [#uses=2]
  %scevgep68 = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %tmp70, i64 %tmp71 ; <double*> [#uses=1]
  %scevgep67 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp70, i64 %tmp71 ; <double*> [#uses=1]
  %10 = load double* %scevgep68, align 8          ; <double> [#uses=1]
  store double %10, double* %scevgep67, align 8
  %indvar.next62 = add i64 %indvar61, 1           ; <i64> [#uses=2]
  %exitcond63 = icmp eq i64 %indvar.next62, 1021  ; <i1> [#uses=1]
  br i1 %exitcond63, label %bb10, label %bb8

bb10:                                             ; preds = %bb8
  %indvar.next65 = add i64 %indvar64, 1           ; <i64> [#uses=2]
  %exitcond69 = icmp eq i64 %indvar.next65, 1021  ; <i1> [#uses=1]
  br i1 %exitcond69, label %bb12, label %bb9.preheader

bb9.preheader:                                    ; preds = %bb10, %bb4
  %indvar64 = phi i64 [ %indvar.next65, %bb10 ], [ 0, %bb4 ] ; <i64> [#uses=2]
  %tmp70 = add i64 %indvar64, 2                   ; <i64> [#uses=2]
  br label %bb8

bb12:                                             ; preds = %bb10
  %11 = add nsw i64 %storemerge20, 1              ; <i64> [#uses=2]
  %exitcond74 = icmp eq i64 %11, 20               ; <i1> [#uses=1]
  br i1 %exitcond74, label %return, label %bb5.preheader

bb5.preheader:                                    ; preds = %bb12, %bb.nph35
  %storemerge20 = phi i64 [ 0, %bb.nph35 ], [ %11, %bb12 ] ; <i64> [#uses=1]
  br label %bb.nph

return:                                           ; preds = %bb12
  ret void
}

; CHECK: SCoP: bb5.preheader => return  Parameters: (), Max Loop Depth: 3
