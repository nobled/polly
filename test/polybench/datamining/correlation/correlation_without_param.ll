; RUN: opt  -O3 -loopsimplify -indvars -polly-scop-extract  -print-top-scop-only -analyze %s | FileCheck %s
; XFAIL: *
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }
@float_n = global double 0x41B32863F6028F5C       ; <double*> [#uses=3]
@eps = global double 5.000000e-03                 ; <double*> [#uses=1]
@data = common global [501 x [501 x double]] zeroinitializer, align 32 ; <[501 x [501 x double]]*> [#uses=7]
@symmat = common global [501 x [501 x double]] zeroinitializer, align 32 ; <[501 x [501 x double]]*> [#uses=6]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
@mean = common global [501 x double] zeroinitializer, align 32 ; <[501 x double]*> [#uses=3]
@stddev = common global [501 x double] zeroinitializer, align 32 ; <[501 x double]*> [#uses=2]
define void @scop_func() nounwind {
bb.nph33.bb.nph33.split_crit_edge:
  br label %bb2.preheader

bb1:                                              ; preds = %bb2.preheader, %bb1
  %indvar45 = phi i64 [ %tmp57, %bb1 ], [ 0, %bb2.preheader ] ; <i64> [#uses=1]
  %tmp57 = add i64 %indvar45, 1                   ; <i64> [#uses=5]
  %scevgep53 = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %tmp56, i64 %tmp57 ; <double*> [#uses=3]
  %scevgep49 = getelementptr [501 x double]* @stddev, i64 0, i64 %tmp57 ; <double*> [#uses=1]
  %scevgep54 = getelementptr [501 x double]* @mean, i64 0, i64 %tmp57 ; <double*> [#uses=1]
  %0 = load double* %scevgep53, align 8           ; <double> [#uses=1]
  %1 = load double* %scevgep54, align 8           ; <double> [#uses=1]
  %2 = fsub double %0, %1                         ; <double> [#uses=2]
  store double %2, double* %scevgep53, align 8
  %3 = load double* @float_n, align 8             ; <double> [#uses=1]
  %4 = tail call double @sqrt(double %3) nounwind readonly ; <double> [#uses=1]
  %5 = load double* %scevgep49, align 8           ; <double> [#uses=1]
  %6 = fmul double %4, %5                         ; <double> [#uses=1]
  %7 = fdiv double %2, %6                         ; <double> [#uses=1]
  store double %7, double* %scevgep53, align 8
  %exitcond47 = icmp eq i64 %tmp57, 500           ; <i1> [#uses=1]
  br i1 %exitcond47, label %bb3, label %bb1

bb3:                                              ; preds = %bb1
  %exitcond55 = icmp eq i64 %tmp56, 500           ; <i1> [#uses=1]
  br i1 %exitcond55, label %bb6, label %bb2.preheader

bb2.preheader:                                    ; preds = %bb3, %bb.nph33.bb.nph33.split_crit_edge
  %indvar50 = phi i64 [ 0, %bb.nph33.bb.nph33.split_crit_edge ], [ %tmp56, %bb3 ] ; <i64> [#uses=1]
  %tmp56 = add i64 %indvar50, 1                   ; <i64> [#uses=3]
  br label %bb1

bb6:                                              ; preds = %bb12, %bb3
  %storemerge113 = phi i32 [ %storemerge27, %bb12 ], [ 1, %bb3 ] ; <i32> [#uses=3]
  %8 = sext i32 %storemerge113 to i64             ; <i64> [#uses=5]
  %9 = getelementptr inbounds [501 x [501 x double]]* @symmat, i64 0, i64 %8, i64 %8 ; <double*> [#uses=1]
  store double 1.000000e+00, double* %9, align 8
  %storemerge27 = add i32 %storemerge113, 1       ; <i32> [#uses=4]
  %10 = icmp sgt i32 %storemerge27, 500           ; <i1> [#uses=1]
  br i1 %10, label %bb12, label %bb.nph12.bb.nph12.split_crit_edge

bb.nph12.bb.nph12.split_crit_edge:                ; preds = %bb6
  %tmp43 = add i32 %storemerge113, 2              ; <i32> [#uses=1]
  br label %bb.nph

bb.nph:                                           ; preds = %bb10, %bb.nph12.bb.nph12.split_crit_edge
  %indvar35 = phi i32 [ 0, %bb.nph12.bb.nph12.split_crit_edge ], [ %indvar.next36, %bb10 ] ; <i32> [#uses=3]
  %storemerge28 = add i32 %storemerge27, %indvar35 ; <i32> [#uses=1]
  %storemerge2 = add i32 %tmp43, %indvar35        ; <i32> [#uses=1]
  %tmp39 = sext i32 %storemerge28 to i64          ; <i64> [#uses=3]
  %11 = getelementptr inbounds [501 x [501 x double]]* @symmat, i64 0, i64 %8, i64 %tmp39 ; <double*> [#uses=2]
  store double 0.000000e+00, double* %11, align 8
  br label %bb8

bb8:                                              ; preds = %bb8, %bb.nph
  %indvar = phi i64 [ 0, %bb.nph ], [ %tmp, %bb8 ] ; <i64> [#uses=1]
  %12 = phi double [ 0.000000e+00, %bb.nph ], [ %16, %bb8 ] ; <double> [#uses=1]
  %tmp = add i64 %indvar, 1                       ; <i64> [#uses=4]
  %scevgep = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %tmp, i64 %tmp39 ; <double*> [#uses=1]
  %scevgep41 = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %tmp, i64 %8 ; <double*> [#uses=1]
  %13 = load double* %scevgep41, align 8          ; <double> [#uses=1]
  %14 = load double* %scevgep, align 8            ; <double> [#uses=1]
  %15 = fmul double %13, %14                      ; <double> [#uses=1]
  %16 = fadd double %12, %15                      ; <double> [#uses=3]
  %exitcond = icmp eq i64 %tmp, 500               ; <i1> [#uses=1]
  br i1 %exitcond, label %bb10, label %bb8

bb10:                                             ; preds = %bb8
  store double %16, double* %11
  %17 = getelementptr inbounds [501 x [501 x double]]* @symmat, i64 0, i64 %tmp39, i64 %8 ; <double*> [#uses=1]
  store double %16, double* %17, align 8
  %18 = icmp sgt i32 %storemerge2, 500            ; <i1> [#uses=1]
  %indvar.next36 = add i32 %indvar35, 1           ; <i32> [#uses=1]
  br i1 %18, label %bb12, label %bb.nph

bb12:                                             ; preds = %bb10, %bb6
  %19 = icmp sgt i32 %storemerge27, 499           ; <i1> [#uses=1]
  br i1 %19, label %return, label %bb6

return:                                           ; preds = %bb12
  ret void
}
declare double @sqrt(double) nounwind readonly
