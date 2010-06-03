; RUN: opt -O3 -indvars -polly-scop-detect -polly-print-temp-scop-in-detail -print-top-scop-only -analyze %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }
@A = common global [1024 x [1024 x double]] zeroinitializer, align 32 ; <[1024 x [1024 x double]]*> [#uses=14]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
define void @scop_func() nounwind {
bb.nph28:
  br label %bb2.preheader

bb1:                                              ; preds = %bb2.preheader, %bb1
  %indvar = phi i64 [ %indvar.next, %bb1 ], [ 0, %bb2.preheader ] ; <i64> [#uses=2]
  %tmp65 = add i64 %tmp59, %indvar                ; <i64> [#uses=1]
  %scevgep = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 0, i64 %tmp65 ; <double*> [#uses=2]
  %0 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %1 = load double* %scevgep68, align 8           ; <double> [#uses=1]
  %2 = fdiv double %0, %1                         ; <double> [#uses=1]
  store double %2, double* %scevgep, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, %tmp29    ; <i1> [#uses=1]
  br i1 %exitcond, label %bb8.loopexit, label %bb1

bb5:                                              ; preds = %bb6.preheader, %bb5
  %indvar33 = phi i64 [ %indvar.next34, %bb5 ], [ 0, %bb6.preheader ] ; <i64> [#uses=2]
  %tmp60 = add i64 %tmp59, %indvar33              ; <i64> [#uses=2]
  %scevgep44 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp56, i64 %tmp60 ; <double*> [#uses=2]
  %scevgep45 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 0, i64 %tmp60 ; <double*> [#uses=1]
  %3 = load double* %scevgep44, align 8           ; <double> [#uses=1]
  %4 = load double* %scevgep54, align 8           ; <double> [#uses=1]
  %5 = load double* %scevgep45, align 8           ; <double> [#uses=1]
  %6 = fmul double %4, %5                         ; <double> [#uses=1]
  %7 = fsub double %3, %6                         ; <double> [#uses=1]
  store double %7, double* %scevgep44, align 8
  %indvar.next34 = add i64 %indvar33, 1           ; <i64> [#uses=2]
  %exitcond37 = icmp eq i64 %indvar.next34, %tmp29 ; <i1> [#uses=1]
  br i1 %exitcond37, label %bb8.loopexit4, label %bb5

bb8.loopexit:                                     ; preds = %bb1
  br i1 %8, label %bb6.preheader, label %bb9

bb8.loopexit4:                                    ; preds = %bb5
  %exitcond48 = icmp eq i64 %tmp56, %tmp29        ; <i1> [#uses=1]
  br i1 %exitcond48, label %bb9, label %bb6.preheader

bb6.preheader:                                    ; preds = %bb8.loopexit4, %bb8.loopexit
  %indvar38 = phi i64 [ %tmp56, %bb8.loopexit4 ], [ 0, %bb8.loopexit ] ; <i64> [#uses=1]
  %tmp56 = add i64 %indvar38, 1                   ; <i64> [#uses=4]
  %scevgep54 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp56, i64 %tmp57 ; <double*> [#uses=1]
  br label %bb5

bb9:                                              ; preds = %bb2.preheader, %bb8.loopexit4, %bb8.loopexit
  %exitcond55 = icmp eq i64 %storemerge15, 1024   ; <i1> [#uses=1]
  br i1 %exitcond55, label %return, label %bb2.preheader

bb2.preheader:                                    ; preds = %bb9, %bb.nph28
  %storemerge17 = phi i64 [ 0, %bb.nph28 ], [ %storemerge15, %bb9 ] ; <i64> [#uses=3]
  %tmp57 = mul i64 %storemerge17, 1025            ; <i64> [#uses=3]
  %tmp59 = add i64 %tmp57, 1                      ; <i64> [#uses=2]
  %tmp29 = sub i64 1023, %storemerge17            ; <i64> [#uses=3]
  %storemerge15 = add i64 %storemerge17, 1        ; <i64> [#uses=3]
  %scevgep68 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 0, i64 %tmp57 ; <double*> [#uses=1]
  %8 = icmp slt i64 %storemerge15, 1024           ; <i1> [#uses=2]
  br i1 %8, label %bb1, label %bb9

return:                                           ; preds = %bb9
  ret void
}

; CHECK: SCoP: bb1 => bb8.loopexit      Parameters: ({0,+,1}<%bb2.preheader>, ), Max Loop Depth: 1
; CHECK: Bounds of Loop: bb1:   { 0, -1 * {0,+,1}<%bb2.preheader> + 1022}
; CHECK:   BB: bb1{
; CHECK:     Reads @A[8200 * {0,+,1}<%bb2.preheader> + 8 * {0,+,1}<%bb1> + 8]
; CHECK:     Reads @A[8200 * {0,+,1}<%bb2.preheader> + 0]
; CHECK:     Writes @A[8200 * {0,+,1}<%bb2.preheader> + 8 * {0,+,1}<%bb1> + 8]
; CHECK:   }
; CHECK: SCoP: bb6.preheader => bb9.loopexit    Parameters: ({0,+,1}<%bb2.preheader>, ), Max Loop Depth: 2
; CHECK: Bounds of Loop: bb6.preheader: { 0, -1 * {0,+,1}<%bb2.preheader> + 1022}
; CHECK:   Bounds of Loop: bb5: { 0, -1 * {0,+,1}<%bb2.preheader> + 1022}
; CHECK:     BB: bb5{
; CHECK:       Reads @A[8 * {0,+,1}<%bb5> + 8200 * {0,+,1}<%bb2.preheader> + 8192 * {0,+,1}<%bb6.preheader> + 8200]
; CHECK:       Reads @A[8200 * {0,+,1}<%bb2.preheader> + 8192 * {0,+,1}<%bb6.preheader> + 8192]
; CHECK:       Reads @A[8 * {0,+,1}<%bb5> + 8200 * {0,+,1}<%bb2.preheader> + 8]
; CHECK:       Writes @A[8 * {0,+,1}<%bb5> + 8200 * {0,+,1}<%bb2.preheader> + 8192 * {0,+,1}<%bb6.preheader> + 8200]
; CHECK:     }
