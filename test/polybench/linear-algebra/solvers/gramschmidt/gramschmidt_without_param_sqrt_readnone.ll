; RUN: opt  -O3 -loopsimplify -indvars -polly-analyze-ir  -print-top-scop-only -analyze %s | FileCheck %s
; XFAIL: *
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }
@A = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=8]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
@nrm = common global double 0.000000e+00          ; <double*> [#uses=2]
@R = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=2]
@Q = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=3]
define void @scop_func() nounwind {
bb.nph41:
  br label %bb.nph

bb.nph:                                           ; preds = %bb15, %bb.nph41
  %storemerge24 = phi i64 [ 0, %bb.nph41 ], [ %storemerge315, %bb15 ] ; <i64> [#uses=8]
  %tmp = mul i64 %storemerge24, 513               ; <i64> [#uses=2]
  %tmp67 = add i64 %tmp, 1                        ; <i64> [#uses=1]
  %storemerge315 = add i64 %storemerge24, 1       ; <i64> [#uses=4]
  %tmp56 = sub i64 511, %storemerge24             ; <i64> [#uses=1]
  %scevgep81 = getelementptr [512 x [512 x double]]* @R, i64 0, i64 0, i64 %tmp ; <double*> [#uses=1]
  store double 0.000000e+00, double* @nrm, align 8
  br label %bb1

bb1:                                              ; preds = %bb1, %bb.nph
  %nrm.tmp.0 = phi double [ 0.000000e+00, %bb.nph ], [ %2, %bb1 ] ; <double> [#uses=1]
  %storemerge17 = phi i64 [ 0, %bb.nph ], [ %3, %bb1 ] ; <i64> [#uses=2]
  %scevgep = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %storemerge17, i64 %storemerge24 ; <double*> [#uses=1]
  %0 = load double* %scevgep, align 8             ; <double> [#uses=2]
  %1 = fmul double %0, %0                         ; <double> [#uses=1]
  %2 = fadd double %1, %nrm.tmp.0                 ; <double> [#uses=3]
  %3 = add nsw i64 %storemerge17, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %3, 512                 ; <i1> [#uses=1]
  br i1 %exitcond, label %bb.nph9, label %bb1

bb.nph9:                                          ; preds = %bb1
  store double %2, double* @nrm
  %4 = tail call double @sqrt(double %2) nounwind readonly ; <double> [#uses=2]
  store double %4, double* %scevgep81, align 8
  br label %bb4

bb4:                                              ; preds = %bb4, %bb.nph9
  %storemerge28 = phi i64 [ 0, %bb.nph9 ], [ %7, %bb4 ] ; <i64> [#uses=3]
  %scevgep44 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %storemerge28, i64 %storemerge24 ; <double*> [#uses=1]
  %scevgep43 = getelementptr [512 x [512 x double]]* @Q, i64 0, i64 %storemerge28, i64 %storemerge24 ; <double*> [#uses=1]
  %5 = load double* %scevgep44, align 8           ; <double> [#uses=1]
  %6 = fdiv double %5, %4                         ; <double> [#uses=1]
  store double %6, double* %scevgep43, align 8
  %7 = add nsw i64 %storemerge28, 1               ; <i64> [#uses=2]
  %exitcond42 = icmp eq i64 %7, 512               ; <i1> [#uses=1]
  br i1 %exitcond42, label %bb14.loopexit, label %bb4

bb.nph11:                                         ; preds = %bb14.loopexit6, %bb14.loopexit
  %indvar = phi i64 [ %indvar.next, %bb14.loopexit6 ], [ 0, %bb14.loopexit ] ; <i64> [#uses=3]
  %tmp68 = add i64 %tmp67, %indvar                ; <i64> [#uses=1]
  %scevgep65 = getelementptr [512 x [512 x double]]* @R, i64 0, i64 0, i64 %tmp68 ; <double*> [#uses=2]
  %tmp71 = add i64 %storemerge315, %indvar        ; <i64> [#uses=2]
  store double 0.000000e+00, double* %scevgep65, align 8
  br label %bb8

bb8:                                              ; preds = %bb8, %bb.nph11
  %.tmp.0 = phi double [ 0.000000e+00, %bb.nph11 ], [ %11, %bb8 ] ; <double> [#uses=1]
  %storemerge410 = phi i64 [ 0, %bb.nph11 ], [ %12, %bb8 ] ; <i64> [#uses=3]
  %scevgep48 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %storemerge410, i64 %tmp71 ; <double*> [#uses=1]
  %scevgep49 = getelementptr [512 x [512 x double]]* @Q, i64 0, i64 %storemerge410, i64 %storemerge24 ; <double*> [#uses=1]
  %8 = load double* %scevgep49, align 8           ; <double> [#uses=1]
  %9 = load double* %scevgep48, align 8           ; <double> [#uses=1]
  %10 = fmul double %8, %9                        ; <double> [#uses=1]
  %11 = fadd double %.tmp.0, %10                  ; <double> [#uses=3]
  %12 = add nsw i64 %storemerge410, 1             ; <i64> [#uses=2]
  %exitcond46 = icmp eq i64 %12, 512              ; <i1> [#uses=1]
  br i1 %exitcond46, label %bb12.loopexit, label %bb8

bb11:                                             ; preds = %bb12.loopexit, %bb11
  %storemerge513 = phi i64 [ %17, %bb11 ], [ 0, %bb12.loopexit ] ; <i64> [#uses=3]
  %scevgep53 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %storemerge513, i64 %tmp71 ; <double*> [#uses=2]
  %scevgep54 = getelementptr [512 x [512 x double]]* @Q, i64 0, i64 %storemerge513, i64 %storemerge24 ; <double*> [#uses=1]
  %13 = load double* %scevgep53, align 8          ; <double> [#uses=1]
  %14 = load double* %scevgep54, align 8          ; <double> [#uses=1]
  %15 = fmul double %14, %11                      ; <double> [#uses=1]
  %16 = fsub double %13, %15                      ; <double> [#uses=1]
  store double %16, double* %scevgep53, align 8
  %17 = add nsw i64 %storemerge513, 1             ; <i64> [#uses=2]
  %exitcond50 = icmp eq i64 %17, 512              ; <i1> [#uses=1]
  br i1 %exitcond50, label %bb14.loopexit6, label %bb11

bb12.loopexit:                                    ; preds = %bb8
  store double %11, double* %scevgep65
  br label %bb11

bb14.loopexit:                                    ; preds = %bb4
  %18 = icmp slt i64 %storemerge315, 512          ; <i1> [#uses=1]
  br i1 %18, label %bb.nph11, label %bb15

bb14.loopexit6:                                   ; preds = %bb11
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond57 = icmp eq i64 %indvar.next, %tmp56  ; <i1> [#uses=1]
  br i1 %exitcond57, label %bb15, label %bb.nph11

bb15:                                             ; preds = %bb14.loopexit6, %bb14.loopexit
  %exitcond66 = icmp eq i64 %storemerge315, 512   ; <i1> [#uses=1]
  br i1 %exitcond66, label %return, label %bb.nph

return:                                           ; preds = %bb15
  ret void
}
declare double @sqrt(double) nounwind readonly
