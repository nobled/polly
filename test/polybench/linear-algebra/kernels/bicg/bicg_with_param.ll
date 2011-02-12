; RUN: %opt -mem2reg -loop-simplify -indvars -polly-prepare -polly-cloog -analyze  %s | FileCheck %s
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }
@r = common global [8000 x double] zeroinitializer, align 32 ; <[8000 x double]*> [#uses=4]
@p = common global [8000 x double] zeroinitializer, align 32 ; <[8000 x double]*> [#uses=4]
@A = common global [8000 x [8000 x double]] zeroinitializer, align 32 ; <[8000 x [8000 x double]]*> [#uses=4]
@s = common global [8000 x double] zeroinitializer, align 32 ; <[8000 x double]*> [#uses=6]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=8]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
@q = common global [8000 x double] zeroinitializer, align 32 ; <[8000 x double]*> [#uses=5]
define void @scop_func(i64 %nx, i64 %ny) nounwind {
entry:
  %0 = icmp sgt i64 %ny, 0                        ; <i1> [#uses=2]
  br i1 %0, label %bb, label %bb7.preheader

bb:                                               ; preds = %bb, %entry
  %storemerge9 = phi i64 [ %1, %bb ], [ 0, %entry ] ; <i64> [#uses=2]
  %scevgep20 = getelementptr [8000 x double]* @s, i64 0, i64 %storemerge9 ; <double*> [#uses=1]
  store double 0.000000e+00, double* %scevgep20, align 8
  %1 = add nsw i64 %storemerge9, 1                ; <i64> [#uses=2]
  %exitcond19 = icmp eq i64 %1, %ny               ; <i1> [#uses=1]
  br i1 %exitcond19, label %bb7.preheader, label %bb

bb7.preheader:                                    ; preds = %bb, %entry
  %2 = icmp sgt i64 %nx, 0                        ; <i1> [#uses=1]
  br i1 %2, label %bb.nph8, label %return

bb.nph8:                                          ; preds = %bb7.preheader
  br i1 %0, label %bb.nph.us, label %bb6

bb6.us:                                           ; preds = %bb4.us
  store double %10, double* %scevgep15
  %3 = add nsw i64 %storemerge14.us, 1            ; <i64> [#uses=2]
  %exitcond13 = icmp eq i64 %3, %nx               ; <i1> [#uses=1]
  br i1 %exitcond13, label %return, label %bb.nph.us

bb4.us:                                           ; preds = %bb.nph.us, %bb4.us
  %.tmp.0.us = phi double [ 0.000000e+00, %bb.nph.us ], [ %10, %bb4.us ] ; <double> [#uses=1]
  %storemerge23.us = phi i64 [ 0, %bb.nph.us ], [ %11, %bb4.us ] ; <i64> [#uses=4]
  %scevgep11 = getelementptr [8000 x [8000 x double]]* @A, i64 0, i64 %storemerge14.us, i64 %storemerge23.us ; <double*> [#uses=1]
  %scevgep = getelementptr [8000 x double]* @p, i64 0, i64 %storemerge23.us ; <double*> [#uses=1]
  %scevgep12 = getelementptr [8000 x double]* @s, i64 0, i64 %storemerge23.us ; <double*> [#uses=2]
  %4 = load double* %scevgep12, align 8           ; <double> [#uses=1]
  %5 = load double* %scevgep11, align 8           ; <double> [#uses=2]
  %6 = fmul double %12, %5                        ; <double> [#uses=1]
  %7 = fadd double %4, %6                         ; <double> [#uses=1]
  store double %7, double* %scevgep12, align 8
  %8 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %9 = fmul double %5, %8                         ; <double> [#uses=1]
  %10 = fadd double %.tmp.0.us, %9                ; <double> [#uses=2]
  %11 = add nsw i64 %storemerge23.us, 1           ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %11, %ny                ; <i1> [#uses=1]
  br i1 %exitcond, label %bb6.us, label %bb4.us

bb.nph.us:                                        ; preds = %bb6.us, %bb.nph8
  %storemerge14.us = phi i64 [ %3, %bb6.us ], [ 0, %bb.nph8 ] ; <i64> [#uses=4]
  %scevgep15 = getelementptr [8000 x double]* @q, i64 0, i64 %storemerge14.us ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep15, align 8
  %scevgep16 = getelementptr [8000 x double]* @r, i64 0, i64 %storemerge14.us ; <double*> [#uses=1]
  %12 = load double* %scevgep16, align 8          ; <double> [#uses=1]
  br label %bb4.us

bb6:                                              ; preds = %bb6, %bb.nph8
  %indvar = phi i64 [ %indvar.next, %bb6 ], [ 0, %bb.nph8 ] ; <i64> [#uses=2]
  %scevgep18 = getelementptr [8000 x double]* @q, i64 0, i64 %indvar ; <double*> [#uses=1]
  store double 0.000000e+00, double* %scevgep18, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond17 = icmp eq i64 %indvar.next, %nx     ; <i1> [#uses=1]
  br i1 %exitcond17, label %return, label %bb6

return:                                           ; preds = %bb6, %bb6.us, %bb7.preheader
  ret void
}

; CHECK: for region: 'entry.split => return' in function 'scop_func':

