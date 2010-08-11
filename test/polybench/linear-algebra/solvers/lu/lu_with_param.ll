; RUN: opt  -mem2reg -loopsimplify -indvars -polly-code-prep -polly-detect -analyze %s | FileCheck %s
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }
@A = common global [1024 x [1024 x double]] zeroinitializer, align 32 ; <[1024 x [1024 x double]]*> [#uses=14]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
define void @scop_func(i64 %n) nounwind {
entry:
  %0 = icmp sgt i64 %n, 0                         ; <i1> [#uses=1]
  br i1 %0, label %bb.nph28, label %return

bb1:                                              ; preds = %bb2.preheader, %bb1
  %indvar = phi i64 [ %indvar.next, %bb1 ], [ 0, %bb2.preheader ] ; <i64> [#uses=2]
  %tmp66 = add i64 %tmp60, %indvar                ; <i64> [#uses=1]
  %scevgep = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 0, i64 %tmp66 ; <double*> [#uses=2]
  %1 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %2 = load double* %scevgep69, align 8           ; <double> [#uses=1]
  %3 = fdiv double %1, %2                         ; <double> [#uses=1]
  store double %3, double* %scevgep, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, %tmp30    ; <i1> [#uses=1]
  br i1 %exitcond, label %bb8.loopexit, label %bb1

bb5:                                              ; preds = %bb6.preheader, %bb5
  %indvar34 = phi i64 [ %indvar.next35, %bb5 ], [ 0, %bb6.preheader ] ; <i64> [#uses=2]
  %tmp61 = add i64 %tmp60, %indvar34              ; <i64> [#uses=2]
  %scevgep45 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp57, i64 %tmp61 ; <double*> [#uses=2]
  %scevgep46 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 0, i64 %tmp61 ; <double*> [#uses=1]
  %4 = load double* %scevgep45, align 8           ; <double> [#uses=1]
  %5 = load double* %scevgep55, align 8           ; <double> [#uses=1]
  %6 = load double* %scevgep46, align 8           ; <double> [#uses=1]
  %7 = fmul double %5, %6                         ; <double> [#uses=1]
  %8 = fsub double %4, %7                         ; <double> [#uses=1]
  store double %8, double* %scevgep45, align 8
  %indvar.next35 = add i64 %indvar34, 1           ; <i64> [#uses=2]
  %exitcond38 = icmp eq i64 %indvar.next35, %tmp30 ; <i1> [#uses=1]
  br i1 %exitcond38, label %bb8.loopexit4, label %bb5

bb8.loopexit:                                     ; preds = %bb1
  br i1 %9, label %bb6.preheader, label %bb9

bb8.loopexit4:                                    ; preds = %bb5
  %exitcond49 = icmp eq i64 %tmp57, %tmp30        ; <i1> [#uses=1]
  br i1 %exitcond49, label %bb9, label %bb6.preheader

bb6.preheader:                                    ; preds = %bb8.loopexit4, %bb8.loopexit
  %indvar39 = phi i64 [ %tmp57, %bb8.loopexit4 ], [ 0, %bb8.loopexit ] ; <i64> [#uses=1]
  %tmp57 = add i64 %indvar39, 1                   ; <i64> [#uses=4]
  %scevgep55 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp57, i64 %tmp58 ; <double*> [#uses=1]
  br label %bb5

bb9:                                              ; preds = %bb2.preheader, %bb8.loopexit4, %bb8.loopexit
  %exitcond56 = icmp eq i64 %storemerge15, %n     ; <i1> [#uses=1]
  br i1 %exitcond56, label %return, label %bb2.preheader

bb.nph28:                                         ; preds = %entry
  %tmp29 = add i64 %n, -1                         ; <i64> [#uses=1]
  br label %bb2.preheader

bb2.preheader:                                    ; preds = %bb.nph28, %bb9
  %storemerge17 = phi i64 [ 0, %bb.nph28 ], [ %storemerge15, %bb9 ] ; <i64> [#uses=3]
  %tmp58 = mul i64 %storemerge17, 1025            ; <i64> [#uses=3]
  %tmp60 = add i64 %tmp58, 1                      ; <i64> [#uses=2]
  %tmp30 = sub i64 %tmp29, %storemerge17          ; <i64> [#uses=3]
  %storemerge15 = add i64 %storemerge17, 1        ; <i64> [#uses=3]
  %scevgep69 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 0, i64 %tmp58 ; <double*> [#uses=1]
  %9 = icmp slt i64 %storemerge15, %n             ; <i1> [#uses=2]
  br i1 %9, label %bb1, label %bb9

return:                                           ; preds = %bb9, %entry
  ret void
}

; CHECK: Valid Region for SCoP: entry => return

