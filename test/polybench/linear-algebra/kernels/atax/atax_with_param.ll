; RUN: opt -mem2reg -loopsimplify -indvars -polly-prepare -polly-print -polly-allow-scalar-deps %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }
@x = common global [8000 x double] zeroinitializer, align 32 ; <[8000 x double]*> [#uses=4]
@A = common global [8000 x [8000 x double]] zeroinitializer, align 32 ; <[8000 x [8000 x double]]*> [#uses=6]
@y = common global [8000 x double] zeroinitializer, align 32 ; <[8000 x double]*> [#uses=6]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
@tmp = common global [8000 x double] zeroinitializer, align 32 ; <[8000 x double]*> [#uses=2]
define void @scop_func(i64 %nx, i64 %ny) nounwind {
entry:
  %0 = icmp sgt i64 %nx, 0                        ; <i1> [#uses=1]
  br i1 %0, label %bb, label %bb10.preheader

bb:                                               ; preds = %bb, %entry
  %storemerge15 = phi i64 [ %1, %bb ], [ 0, %entry ] ; <i64> [#uses=2]
  %scevgep26 = getelementptr [8000 x double]* @y, i64 0, i64 %storemerge15 ; <double*> [#uses=1]
  store double 0.000000e+00, double* %scevgep26, align 8
  %1 = add nsw i64 %storemerge15, 1               ; <i64> [#uses=2]
  %exitcond25 = icmp eq i64 %1, %nx               ; <i1> [#uses=1]
  br i1 %exitcond25, label %bb10.preheader, label %bb

bb10.preheader:                                   ; preds = %bb, %entry
  %2 = icmp sgt i64 %ny, 0                        ; <i1> [#uses=1]
  br i1 %2, label %bb.nph, label %return

bb.nph:                                           ; preds = %bb9, %bb10.preheader
  %storemerge17 = phi i64 [ %13, %bb9 ], [ 0, %bb10.preheader ] ; <i64> [#uses=4]
  %scevgep24 = getelementptr [8000 x double]* @tmp, i64 0, i64 %storemerge17 ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep24, align 8
  br label %bb4

bb4:                                              ; preds = %bb4, %bb.nph
  %.tmp.0 = phi double [ 0.000000e+00, %bb.nph ], [ %6, %bb4 ] ; <double> [#uses=1]
  %storemerge24 = phi i64 [ 0, %bb.nph ], [ %7, %bb4 ] ; <i64> [#uses=3]
  %scevgep17 = getelementptr [8000 x [8000 x double]]* @A, i64 0, i64 %storemerge17, i64 %storemerge24 ; <double*> [#uses=1]
  %scevgep = getelementptr [8000 x double]* @x, i64 0, i64 %storemerge24 ; <double*> [#uses=1]
  %3 = load double* %scevgep17, align 8           ; <double> [#uses=1]
  %4 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %5 = fmul double %3, %4                         ; <double> [#uses=1]
  %6 = fadd double %.tmp.0, %5                    ; <double> [#uses=3]
  %7 = add nsw i64 %storemerge24, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %7, %ny                 ; <i1> [#uses=1]
  br i1 %exitcond, label %bb8.loopexit, label %bb4

bb7:                                              ; preds = %bb8.loopexit, %bb7
  %storemerge35 = phi i64 [ %12, %bb7 ], [ 0, %bb8.loopexit ] ; <i64> [#uses=3]
  %scevgep19 = getelementptr [8000 x [8000 x double]]* @A, i64 0, i64 %storemerge17, i64 %storemerge35 ; <double*> [#uses=1]
  %scevgep20 = getelementptr [8000 x double]* @y, i64 0, i64 %storemerge35 ; <double*> [#uses=2]
  %8 = load double* %scevgep20, align 8           ; <double> [#uses=1]
  %9 = load double* %scevgep19, align 8           ; <double> [#uses=1]
  %10 = fmul double %9, %6                        ; <double> [#uses=1]
  %11 = fadd double %8, %10                       ; <double> [#uses=1]
  store double %11, double* %scevgep20, align 8
  %12 = add nsw i64 %storemerge35, 1              ; <i64> [#uses=2]
  %exitcond18 = icmp eq i64 %12, %ny              ; <i1> [#uses=1]
  br i1 %exitcond18, label %bb9, label %bb7

bb8.loopexit:                                     ; preds = %bb4
  store double %6, double* %scevgep24
  br label %bb7

bb9:                                              ; preds = %bb7
  %13 = add nsw i64 %storemerge17, 1              ; <i64> [#uses=2]
  %exitcond21 = icmp eq i64 %13, %ny              ; <i1> [#uses=1]
  br i1 %exitcond21, label %return, label %bb.nph

return:                                           ; preds = %bb9, %bb10.preheader
  ret void
}

; CHECK: In function: 'scop_func' SCoP: entry.split => return:
