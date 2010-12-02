; RUN: %opt -mem2reg -indvars -polly-prepare -polly-detect -analyze  %s | FileCheck %s
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }
@A = common global [1024 x [1024 x double]] zeroinitializer, align 32 ; <[1024 x [1024 x double]]*> [#uses=10]
@B = common global [1024 x [1024 x double]] zeroinitializer, align 32 ; <[1024 x [1024 x double]]*> [#uses=4]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
define void @scop_func(i64 %n) nounwind {
bb.nph35:
  %0 = add nsw i64 %n, -1                         ; <i64> [#uses=1]
  %1 = icmp sgt i64 %0, 2                         ; <i1> [#uses=2]
  %tmp = add i64 %n, -3                           ; <i64> [#uses=4]
  br label %bb5.preheader

bb.nph:                                           ; preds = %bb5.preheader, %bb4
  %indvar36 = phi i64 [ %tmp50, %bb4 ], [ 0, %bb5.preheader ] ; <i64> [#uses=3]
  %tmp50 = add i64 %indvar36, 1                   ; <i64> [#uses=3]
  %tmp53 = add i64 %indvar36, 3                   ; <i64> [#uses=1]
  %tmp55 = add i64 %indvar36, 2                   ; <i64> [#uses=4]
  %scevgep40.phi.trans.insert = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp55, i64 2 ; <double*> [#uses=1]
  %.pre = load double* %scevgep40.phi.trans.insert, align 16 ; <double> [#uses=1]
  br label %bb2

bb2:                                              ; preds = %bb2, %bb.nph
  %2 = phi double [ %.pre, %bb.nph ], [ %5, %bb2 ] ; <double> [#uses=1]
  %indvar = phi i64 [ 0, %bb.nph ], [ %tmp58, %bb2 ] ; <i64> [#uses=3]
  %tmp51 = add i64 %indvar, 2                     ; <i64> [#uses=3]
  %scevgep44 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp50, i64 %tmp51 ; <double*> [#uses=1]
  %scevgep42 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp53, i64 %tmp51 ; <double*> [#uses=1]
  %tmp56 = add i64 %indvar, 3                     ; <i64> [#uses=1]
  %scevgep48 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp55, i64 %tmp56 ; <double*> [#uses=1]
  %tmp58 = add i64 %indvar, 1                     ; <i64> [#uses=3]
  %scevgep46 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp55, i64 %tmp58 ; <double*> [#uses=1]
  %scevgep = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %tmp55, i64 %tmp51 ; <double*> [#uses=1]
  %3 = load double* %scevgep46, align 8           ; <double> [#uses=1]
  %4 = fadd double %2, %3                         ; <double> [#uses=1]
  %5 = load double* %scevgep48, align 8           ; <double> [#uses=2]
  %6 = fadd double %4, %5                         ; <double> [#uses=1]
  %7 = load double* %scevgep42, align 8           ; <double> [#uses=1]
  %8 = fadd double %6, %7                         ; <double> [#uses=1]
  %9 = load double* %scevgep44, align 8           ; <double> [#uses=1]
  %10 = fadd double %8, %9                        ; <double> [#uses=1]
  %11 = fmul double %10, 2.000000e-01             ; <double> [#uses=1]
  store double %11, double* %scevgep, align 8
  %exitcond = icmp eq i64 %tmp58, %tmp            ; <i1> [#uses=1]
  br i1 %exitcond, label %bb4, label %bb2

bb4:                                              ; preds = %bb2
  %exitcond49 = icmp eq i64 %tmp50, %tmp          ; <i1> [#uses=1]
  br i1 %exitcond49, label %bb11.loopexit, label %bb.nph

bb8:                                              ; preds = %bb9.preheader, %bb8
  %indvar62 = phi i64 [ %indvar.next63, %bb8 ], [ 0, %bb9.preheader ] ; <i64> [#uses=2]
  %tmp73 = add i64 %indvar62, 2                   ; <i64> [#uses=2]
  %scevgep70 = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %tmp72, i64 %tmp73 ; <double*> [#uses=1]
  %scevgep69 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp72, i64 %tmp73 ; <double*> [#uses=1]
  %12 = load double* %scevgep70, align 8          ; <double> [#uses=1]
  store double %12, double* %scevgep69, align 8
  %indvar.next63 = add i64 %indvar62, 1           ; <i64> [#uses=2]
  %exitcond64 = icmp eq i64 %indvar.next63, %tmp  ; <i1> [#uses=1]
  br i1 %exitcond64, label %bb10, label %bb8

bb10:                                             ; preds = %bb8
  %indvar.next66 = add i64 %indvar65, 1           ; <i64> [#uses=2]
  %exitcond71 = icmp eq i64 %indvar.next66, %tmp  ; <i1> [#uses=1]
  br i1 %exitcond71, label %bb12, label %bb9.preheader

bb11.loopexit:                                    ; preds = %bb4
  br i1 %1, label %bb9.preheader, label %bb12

bb9.preheader:                                    ; preds = %bb11.loopexit, %bb10
  %indvar65 = phi i64 [ %indvar.next66, %bb10 ], [ 0, %bb11.loopexit ] ; <i64> [#uses=2]
  %tmp72 = add i64 %indvar65, 2                   ; <i64> [#uses=2]
  br label %bb8

bb12:                                             ; preds = %bb5.preheader, %bb11.loopexit, %bb10
  %13 = add nsw i64 %storemerge20, 1              ; <i64> [#uses=2]
  %exitcond76 = icmp eq i64 %13, 20               ; <i1> [#uses=1]
  br i1 %exitcond76, label %return, label %bb5.preheader

bb5.preheader:                                    ; preds = %bb12, %bb.nph35
  %storemerge20 = phi i64 [ 0, %bb.nph35 ], [ %13, %bb12 ] ; <i64> [#uses=1]
  br i1 %1, label %bb.nph, label %bb12

return:                                           ; preds = %bb12
  ret void
}

; CHECK: Valid Region for SCoP: bb5.preheader => return

