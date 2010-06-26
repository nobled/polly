; RUN: opt  -O3 -loopsimplify -indvars -polly-scop-extract  -print-top-scop-only -analyze %s | FileCheck %s
; XFAIL: *
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }
@float_n = global double 0x41B32863F6028F5C       ; <double*> [#uses=3]
@eps = global double 5.000000e-03                 ; <double*> [#uses=1]
@data = common global [501 x [501 x double]] zeroinitializer, align 32 ; <[501 x [501 x double]]*> [#uses=7]
@symmat = common global [501 x [501 x double]] zeroinitializer, align 32 ; <[501 x [501 x double]]*> [#uses=8]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
@mean = common global [501 x double] zeroinitializer, align 32 ; <[501 x double]*> [#uses=3]
@stddev = common global [501 x double] zeroinitializer, align 32 ; <[501 x double]*> [#uses=2]
define void @scop_func(i64 %m, i64 %n) nounwind {
entry:
  %0 = icmp slt i64 %n, 1                         ; <i1> [#uses=2]
  %1 = icmp slt i64 %m, 1                         ; <i1> [#uses=1]
  %or.cond = or i1 %0, %1                         ; <i1> [#uses=1]
  br i1 %or.cond, label %bb13.preheader, label %bb2.preheader

bb1:                                              ; preds = %bb2.preheader, %bb1
  %indvar52 = phi i64 [ %tmp63, %bb1 ], [ 0, %bb2.preheader ] ; <i64> [#uses=2]
  %tmp63 = add i64 %indvar52, 1                   ; <i64> [#uses=4]
  %scevgep59 = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %tmp62, i64 %tmp63 ; <double*> [#uses=3]
  %scevgep55 = getelementptr [501 x double]* @stddev, i64 0, i64 %tmp63 ; <double*> [#uses=1]
  %scevgep60 = getelementptr [501 x double]* @mean, i64 0, i64 %tmp63 ; <double*> [#uses=1]
  %2 = load double* %scevgep59, align 8           ; <double> [#uses=1]
  %3 = load double* %scevgep60, align 8           ; <double> [#uses=1]
  %4 = fsub double %2, %3                         ; <double> [#uses=2]
  store double %4, double* %scevgep59, align 8
  %5 = load double* @float_n, align 8             ; <double> [#uses=1]
  %6 = tail call double @sqrt(double %5) nounwind readonly ; <double> [#uses=1]
  %7 = load double* %scevgep55, align 8           ; <double> [#uses=1]
  %8 = fmul double %6, %7                         ; <double> [#uses=1]
  %9 = fdiv double %4, %8                         ; <double> [#uses=1]
  store double %9, double* %scevgep59, align 8
  %tmp61 = add i64 %indvar52, 2                   ; <i64> [#uses=1]
  %10 = icmp sgt i64 %tmp61, %m                   ; <i1> [#uses=1]
  br i1 %10, label %bb3, label %bb1

bb3:                                              ; preds = %bb1
  %11 = icmp sgt i64 %tmp65, %n                   ; <i1> [#uses=1]
  br i1 %11, label %bb13.preheader, label %bb2.preheader

bb2.preheader:                                    ; preds = %bb3, %entry
  %indvar56 = phi i64 [ %tmp62, %bb3 ], [ 0, %entry ] ; <i64> [#uses=2]
  %tmp62 = add i64 %indvar56, 1                   ; <i64> [#uses=2]
  %tmp65 = add i64 %indvar56, 2                   ; <i64> [#uses=1]
  br label %bb1

bb13.preheader:                                   ; preds = %bb3, %entry
  %12 = add nsw i64 %m, -1                        ; <i64> [#uses=2]
  %13 = icmp slt i64 %12, 1                       ; <i1> [#uses=1]
  br i1 %13, label %return, label %bb6

bb6:                                              ; preds = %bb12, %bb13.preheader
  %storemerge113 = phi i32 [ %33, %bb12 ], [ 1, %bb13.preheader ] ; <i32> [#uses=6]
  %14 = sext i32 %storemerge113 to i64            ; <i64> [#uses=7]
  %15 = getelementptr inbounds [501 x [501 x double]]* @symmat, i64 0, i64 %14, i64 %14 ; <double*> [#uses=1]
  store double 1.000000e+00, double* %15, align 8
  %storemerge27 = add i32 %storemerge113, 1       ; <i32> [#uses=3]
  %16 = sext i32 %storemerge27 to i64             ; <i64> [#uses=1]
  %17 = icmp sgt i64 %16, %m                      ; <i1> [#uses=1]
  br i1 %17, label %bb12, label %bb.nph12

bb.nph12:                                         ; preds = %bb6
  %tmp = add i32 %storemerge113, 2                ; <i32> [#uses=2]
  br i1 %0, label %bb10.us, label %bb.nph

bb10.us:                                          ; preds = %bb10.us, %bb.nph12
  %indvar = phi i32 [ %indvar.next, %bb10.us ], [ 0, %bb.nph12 ] ; <i32> [#uses=3]
  %storemerge115 = phi i32 [ %storemerge115, %bb10.us ], [ %storemerge113, %bb.nph12 ] ; <i32> [#uses=2]
  %storemerge2.us = add i32 %tmp, %indvar         ; <i32> [#uses=1]
  %storemerge28.us = add i32 %storemerge27, %indvar ; <i32> [#uses=1]
  %18 = sext i32 %storemerge28.us to i64          ; <i64> [#uses=2]
  %19 = getelementptr inbounds [501 x [501 x double]]* @symmat, i64 0, i64 %14, i64 %18 ; <double*> [#uses=1]
  store double 0.000000e+00, double* %19, align 8
  %20 = getelementptr inbounds [501 x [501 x double]]* @symmat, i64 0, i64 %18, i64 %14 ; <double*> [#uses=1]
  store double 0.000000e+00, double* %20, align 8
  %21 = sext i32 %storemerge2.us to i64           ; <i64> [#uses=1]
  %22 = icmp sgt i64 %21, %m                      ; <i1> [#uses=1]
  %indvar.next = add i32 %indvar, 1               ; <i32> [#uses=1]
  br i1 %22, label %bb12, label %bb10.us

bb.nph:                                           ; preds = %bb10, %bb.nph12
  %indvar41 = phi i32 [ %indvar.next42, %bb10 ], [ 0, %bb.nph12 ] ; <i32> [#uses=3]
  %storemerge28 = add i32 %storemerge27, %indvar41 ; <i32> [#uses=1]
  %storemerge2 = add i32 %tmp, %indvar41          ; <i32> [#uses=1]
  %tmp45 = sext i32 %storemerge28 to i64          ; <i64> [#uses=3]
  %23 = getelementptr inbounds [501 x [501 x double]]* @symmat, i64 0, i64 %14, i64 %tmp45 ; <double*> [#uses=2]
  store double 0.000000e+00, double* %23, align 8
  br label %bb8

bb8:                                              ; preds = %bb8, %bb.nph
  %indvar38 = phi i64 [ 0, %bb.nph ], [ %tmp40, %bb8 ] ; <i64> [#uses=2]
  %24 = phi double [ 0.000000e+00, %bb.nph ], [ %28, %bb8 ] ; <double> [#uses=1]
  %tmp40 = add i64 %indvar38, 1                   ; <i64> [#uses=3]
  %scevgep = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %tmp40, i64 %tmp45 ; <double*> [#uses=1]
  %scevgep47 = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %tmp40, i64 %14 ; <double*> [#uses=1]
  %25 = load double* %scevgep47, align 8          ; <double> [#uses=1]
  %26 = load double* %scevgep, align 8            ; <double> [#uses=1]
  %27 = fmul double %25, %26                      ; <double> [#uses=1]
  %28 = fadd double %24, %27                      ; <double> [#uses=3]
  %tmp48 = add i64 %indvar38, 2                   ; <i64> [#uses=1]
  %29 = icmp sgt i64 %tmp48, %n                   ; <i1> [#uses=1]
  br i1 %29, label %bb10, label %bb8

bb10:                                             ; preds = %bb8
  store double %28, double* %23
  %30 = getelementptr inbounds [501 x [501 x double]]* @symmat, i64 0, i64 %tmp45, i64 %14 ; <double*> [#uses=1]
  store double %28, double* %30, align 8
  %31 = sext i32 %storemerge2 to i64              ; <i64> [#uses=1]
  %32 = icmp sgt i64 %31, %m                      ; <i1> [#uses=1]
  %indvar.next42 = add i32 %indvar41, 1           ; <i32> [#uses=1]
  br i1 %32, label %bb12, label %bb.nph

bb12:                                             ; preds = %bb10, %bb10.us, %bb6
  %storemerge125 = phi i32 [ %storemerge113, %bb6 ], [ %storemerge115, %bb10.us ], [ %storemerge113, %bb10 ] ; <i32> [#uses=1]
  %33 = add nsw i32 %storemerge125, 1             ; <i32> [#uses=2]
  %34 = sext i32 %33 to i64                       ; <i64> [#uses=1]
  %35 = icmp sgt i64 %34, %12                     ; <i1> [#uses=1]
  br i1 %35, label %return, label %bb6

return:                                           ; preds = %bb12, %bb13.preheader
  ret void
}
declare double @sqrt(double) nounwind readonly
