; RUN: opt  -indvars -polly-scop-detect -print-top-scop-only -polly-print-scop -analyze  %s | FileCheck %s
; XFAIL: *
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }

@alpha = common global double 0.000000e+00        ; <double*> [#uses=3]
@beta = common global double 0.000000e+00         ; <double*> [#uses=3]
@x = common global [4000 x double] zeroinitializer, align 32 ; <[4000 x double]*> [#uses=4]
@A = common global [4000 x [4000 x double]] zeroinitializer, align 32 ; <[4000 x [4000 x double]]*> [#uses=4]
@y = common global [4000 x double] zeroinitializer, align 32 ; <[4000 x double]*> [#uses=4]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
@tmp = common global [4000 x double] zeroinitializer, align 32 ; <[4000 x double]*> [#uses=2]
@B = common global [4000 x [4000 x double]] zeroinitializer, align 32 ; <[4000 x [4000 x double]]*> [#uses=2]

define void @init_array() nounwind inlinehint {
bb.nph6.split.us:
  store double 4.353200e+04, double* @alpha, align 8
  store double 1.231300e+04, double* @beta, align 8
  br label %bb.nph.us

bb3.us:                                           ; preds = %bb1.us
  %indvar.next8 = add i64 %indvar7, 1             ; <i64> [#uses=2]
  %exitcond9 = icmp eq i64 %indvar.next8, 4000    ; <i1> [#uses=1]
  br i1 %exitcond9, label %return, label %bb.nph.us

bb1.us:                                           ; preds = %bb.nph.us, %bb1.us
  %indvar = phi i64 [ 0, %bb.nph.us ], [ %indvar.next, %bb1.us ] ; <i64> [#uses=3]
  %scevgep = getelementptr [4000 x [4000 x double]]* @A, i64 0, i64 %indvar7, i64 %indvar ; <double*> [#uses=1]
  %j.01.us = trunc i64 %indvar to i32             ; <i32> [#uses=1]
  %0 = sitofp i32 %j.01.us to double              ; <double> [#uses=1]
  %1 = fmul double %3, %0                         ; <double> [#uses=1]
  %2 = fdiv double %1, 4.000000e+03               ; <double> [#uses=1]
  store double %2, double* %scevgep, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 4000      ; <i1> [#uses=1]
  br i1 %exitcond, label %bb3.us, label %bb1.us

bb.nph.us:                                        ; preds = %bb3.us, %bb.nph6.split.us
  %indvar7 = phi i64 [ %indvar.next8, %bb3.us ], [ 0, %bb.nph6.split.us ] ; <i64> [#uses=4]
  %i.02.us = trunc i64 %indvar7 to i32            ; <i32> [#uses=1]
  %scevgep11 = getelementptr [4000 x double]* @x, i64 0, i64 %indvar7 ; <double*> [#uses=1]
  %3 = sitofp i32 %i.02.us to double              ; <double> [#uses=2]
  %4 = fdiv double %3, 4.000000e+03               ; <double> [#uses=1]
  store double %4, double* %scevgep11, align 8
  br label %bb1.us

return:                                           ; preds = %bb3.us
  ret void
}

define void @print_array(i32 %argc, i8** nocapture %argv) nounwind inlinehint {
entry:
  %0 = icmp sgt i32 %argc, 42                     ; <i1> [#uses=1]
  br i1 %0, label %bb, label %return

bb:                                               ; preds = %entry
  %1 = load i8** %argv, align 1                   ; <i8*> [#uses=1]
  %2 = load i8* %1, align 1                       ; <i8> [#uses=1]
  %3 = icmp eq i8 %2, 0                           ; <i1> [#uses=1]
  br i1 %3, label %bb2, label %return

bb2:                                              ; preds = %bb4, %bb
  %indvar = phi i64 [ %indvar.next, %bb4 ], [ 0, %bb ] ; <i64> [#uses=3]
  %scevgep = getelementptr [4000 x double]* @y, i64 0, i64 %indvar ; <double*> [#uses=1]
  %i.01 = trunc i64 %indvar to i32                ; <i32> [#uses=1]
  %4 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %5 = load %struct._IO_FILE** @stderr, align 8   ; <%struct._IO_FILE*> [#uses=1]
  %6 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %5, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %4) nounwind ; <i32> [#uses=0]
  %7 = srem i32 %i.01, 80                         ; <i32> [#uses=1]
  %8 = icmp eq i32 %7, 20                         ; <i1> [#uses=1]
  br i1 %8, label %bb3, label %bb4

bb3:                                              ; preds = %bb2
  %9 = load %struct._IO_FILE** @stderr, align 8   ; <%struct._IO_FILE*> [#uses=1]
  %10 = bitcast %struct._IO_FILE* %9 to i8*       ; <i8*> [#uses=1]
  %11 = tail call i32 @fputc(i32 10, i8* %10) nounwind ; <i32> [#uses=0]
  br label %bb4

bb4:                                              ; preds = %bb3, %bb2
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 4000      ; <i1> [#uses=1]
  br i1 %exitcond, label %bb6, label %bb2

bb6:                                              ; preds = %bb4
  %12 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %13 = bitcast %struct._IO_FILE* %12 to i8*      ; <i8*> [#uses=1]
  %14 = tail call i32 @fputc(i32 10, i8* %13) nounwind ; <i32> [#uses=0]
  ret void

return:                                           ; preds = %bb, %entry
  ret void
}

declare i32 @fprintf(%struct._IO_FILE* noalias nocapture, i8* noalias nocapture, ...) nounwind

declare i32 @fputc(i32, i8* nocapture) nounwind

define void @scop_func() nounwind {
bb.nph10:
  %0 = load double* @alpha, align 8               ; <double> [#uses=1]
  %1 = load double* @beta, align 8                ; <double> [#uses=1]
  br label %bb.nph

bb.nph:                                           ; preds = %bb3, %bb.nph10
  %indvar15 = phi i64 [ 0, %bb.nph10 ], [ %indvar.next16, %bb3 ] ; <i64> [#uses=5]
  %scevgep20 = getelementptr [4000 x double]* @y, i64 0, i64 %indvar15 ; <double*> [#uses=2]
  %scevgep21 = getelementptr [4000 x double]* @tmp, i64 0, i64 %indvar15 ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep21, align 8
  store double 0.000000e+00, double* %scevgep20, align 8
  br label %bb1

bb1:                                              ; preds = %bb1, %bb.nph
  %indvar = phi i64 [ 0, %bb.nph ], [ %indvar.next, %bb1 ] ; <i64> [#uses=4]
  %.tmp2.0 = phi double [ 0.000000e+00, %bb.nph ], [ %8, %bb1 ] ; <double> [#uses=1]
  %.tmp.0 = phi double [ 0.000000e+00, %bb.nph ], [ %5, %bb1 ] ; <double> [#uses=1]
  %scevgep17 = getelementptr [4000 x [4000 x double]]* @B, i64 0, i64 %indvar15, i64 %indvar ; <double*> [#uses=1]
  %scevgep14 = getelementptr [4000 x double]* @x, i64 0, i64 %indvar ; <double*> [#uses=1]
  %tmp = mul i64 %indvar15, 4000                  ; <i64> [#uses=1]
  %scevgep.sum = add i64 %indvar, %tmp            ; <i64> [#uses=1]
  %scevgep13 = getelementptr [4000 x [4000 x double]]* @A, i64 0, i64 0, i64 %scevgep.sum ; <double*> [#uses=1]
  %2 = load double* %scevgep13, align 8           ; <double> [#uses=1]
  %3 = load double* %scevgep14, align 8           ; <double> [#uses=2]
  %4 = fmul double %2, %3                         ; <double> [#uses=1]
  %5 = fadd double %4, %.tmp.0                    ; <double> [#uses=3]
  %6 = load double* %scevgep17, align 8           ; <double> [#uses=1]
  %7 = fmul double %6, %3                         ; <double> [#uses=1]
  %8 = fadd double %7, %.tmp2.0                   ; <double> [#uses=2]
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 4000      ; <i1> [#uses=1]
  br i1 %exitcond, label %bb3, label %bb1

bb3:                                              ; preds = %bb1
  store double %5, double* %scevgep21
  %9 = fmul double %5, %0                         ; <double> [#uses=1]
  %10 = fmul double %8, %1                        ; <double> [#uses=1]
  %11 = fadd double %9, %10                       ; <double> [#uses=1]
  store double %11, double* %scevgep20, align 8
  %indvar.next16 = add i64 %indvar15, 1           ; <i64> [#uses=2]
  %exitcond18 = icmp eq i64 %indvar.next16, 4000  ; <i1> [#uses=1]
  br i1 %exitcond18, label %return, label %bb.nph

return:                                           ; preds = %bb3
  ret void
}

define i32 @main(i32 %argc, i8** nocapture %argv) nounwind {
entry:
  store double 4.353200e+04, double* @alpha, align 8
  store double 1.231300e+04, double* @beta, align 8
  br label %bb.nph.us.i

bb3.us.i:                                         ; preds = %bb1.us.i
  %indvar.next8.i = add i64 %indvar7.i, 1         ; <i64> [#uses=2]
  %exitcond15 = icmp eq i64 %indvar.next8.i, 4000 ; <i1> [#uses=1]
  br i1 %exitcond15, label %bb.nph.i, label %bb.nph.us.i

bb1.us.i:                                         ; preds = %bb.nph.us.i, %bb1.us.i
  %indvar.i = phi i64 [ 0, %bb.nph.us.i ], [ %indvar.next.i, %bb1.us.i ] ; <i64> [#uses=3]
  %scevgep.i = getelementptr [4000 x [4000 x double]]* @A, i64 0, i64 %indvar7.i, i64 %indvar.i ; <double*> [#uses=1]
  %j.01.us.i = trunc i64 %indvar.i to i32         ; <i32> [#uses=1]
  %0 = sitofp i32 %j.01.us.i to double            ; <double> [#uses=1]
  %1 = fmul double %3, %0                         ; <double> [#uses=1]
  %2 = fdiv double %1, 4.000000e+03               ; <double> [#uses=1]
  store double %2, double* %scevgep.i, align 8
  %indvar.next.i = add i64 %indvar.i, 1           ; <i64> [#uses=2]
  %exitcond14 = icmp eq i64 %indvar.next.i, 4000  ; <i1> [#uses=1]
  br i1 %exitcond14, label %bb3.us.i, label %bb1.us.i

bb.nph.us.i:                                      ; preds = %bb3.us.i, %entry
  %indvar7.i = phi i64 [ %indvar.next8.i, %bb3.us.i ], [ 0, %entry ] ; <i64> [#uses=4]
  %scevgep11.i = getelementptr [4000 x double]* @x, i64 0, i64 %indvar7.i ; <double*> [#uses=1]
  %i.02.us.i = trunc i64 %indvar7.i to i32        ; <i32> [#uses=1]
  %3 = sitofp i32 %i.02.us.i to double            ; <double> [#uses=2]
  %4 = fdiv double %3, 4.000000e+03               ; <double> [#uses=1]
  store double %4, double* %scevgep11.i, align 8
  br label %bb1.us.i

bb.nph.i:                                         ; preds = %bb3.i8, %bb3.us.i
  %indvar15.i = phi i64 [ %indvar.next16.i, %bb3.i8 ], [ 0, %bb3.us.i ] ; <i64> [#uses=5]
  %scevgep21.i = getelementptr [4000 x double]* @tmp, i64 0, i64 %indvar15.i ; <double*> [#uses=2]
  %scevgep20.i = getelementptr [4000 x double]* @y, i64 0, i64 %indvar15.i ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep21.i, align 8
  store double 0.000000e+00, double* %scevgep20.i, align 8
  br label %bb1.i

bb1.i:                                            ; preds = %bb1.i, %bb.nph.i
  %indvar.i5 = phi i64 [ 0, %bb.nph.i ], [ %indvar.next.i6, %bb1.i ] ; <i64> [#uses=4]
  %.tmp2.0.i = phi double [ 0.000000e+00, %bb.nph.i ], [ %11, %bb1.i ] ; <double> [#uses=1]
  %.tmp.0.i = phi double [ 0.000000e+00, %bb.nph.i ], [ %8, %bb1.i ] ; <double> [#uses=1]
  %scevgep17.i = getelementptr [4000 x [4000 x double]]* @B, i64 0, i64 %indvar15.i, i64 %indvar.i5 ; <double*> [#uses=1]
  %scevgep13.i = getelementptr [4000 x [4000 x double]]* @A, i64 0, i64 %indvar15.i, i64 %indvar.i5 ; <double*> [#uses=1]
  %scevgep14.i = getelementptr [4000 x double]* @x, i64 0, i64 %indvar.i5 ; <double*> [#uses=1]
  %5 = load double* %scevgep13.i, align 8         ; <double> [#uses=1]
  %6 = load double* %scevgep14.i, align 8         ; <double> [#uses=2]
  %7 = fmul double %5, %6                         ; <double> [#uses=1]
  %8 = fadd double %7, %.tmp.0.i                  ; <double> [#uses=3]
  %9 = load double* %scevgep17.i, align 8         ; <double> [#uses=1]
  %10 = fmul double %9, %6                        ; <double> [#uses=1]
  %11 = fadd double %10, %.tmp2.0.i               ; <double> [#uses=2]
  %indvar.next.i6 = add i64 %indvar.i5, 1         ; <i64> [#uses=2]
  %exitcond12 = icmp eq i64 %indvar.next.i6, 4000 ; <i1> [#uses=1]
  br i1 %exitcond12, label %bb3.i8, label %bb1.i

bb3.i8:                                           ; preds = %bb1.i
  store double %8, double* %scevgep21.i
  %12 = fmul double %8, 4.353200e+04              ; <double> [#uses=1]
  %13 = fmul double %11, 1.231300e+04             ; <double> [#uses=1]
  %14 = fadd double %12, %13                      ; <double> [#uses=1]
  store double %14, double* %scevgep20.i, align 8
  %indvar.next16.i = add i64 %indvar15.i, 1       ; <i64> [#uses=2]
  %exitcond13 = icmp eq i64 %indvar.next16.i, 4000 ; <i1> [#uses=1]
  br i1 %exitcond13, label %scop_func.exit, label %bb.nph.i

scop_func.exit:                                   ; preds = %bb3.i8
  %15 = icmp sgt i32 %argc, 42                    ; <i1> [#uses=1]
  br i1 %15, label %bb.i, label %print_array.exit

bb.i:                                             ; preds = %scop_func.exit
  %16 = load i8** %argv, align 1                  ; <i8*> [#uses=1]
  %17 = load i8* %16, align 1                     ; <i8> [#uses=1]
  %18 = icmp eq i8 %17, 0                         ; <i1> [#uses=1]
  br i1 %18, label %bb2.i, label %print_array.exit

bb2.i:                                            ; preds = %bb4.i, %bb.i
  %indvar.i1 = phi i64 [ %indvar.next.i3, %bb4.i ], [ 0, %bb.i ] ; <i64> [#uses=3]
  %i.01.i = trunc i64 %indvar.i1 to i32           ; <i32> [#uses=1]
  %scevgep.i2 = getelementptr [4000 x double]* @y, i64 0, i64 %indvar.i1 ; <double*> [#uses=1]
  %19 = load double* %scevgep.i2, align 8         ; <double> [#uses=1]
  %20 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %21 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %20, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %19) nounwind ; <i32> [#uses=0]
  %22 = srem i32 %i.01.i, 80                      ; <i32> [#uses=1]
  %23 = icmp eq i32 %22, 20                       ; <i1> [#uses=1]
  br i1 %23, label %bb3.i, label %bb4.i

bb3.i:                                            ; preds = %bb2.i
  %24 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %25 = bitcast %struct._IO_FILE* %24 to i8*      ; <i8*> [#uses=1]
  %26 = tail call i32 @fputc(i32 10, i8* %25) nounwind ; <i32> [#uses=0]
  br label %bb4.i

bb4.i:                                            ; preds = %bb3.i, %bb2.i
  %indvar.next.i3 = add i64 %indvar.i1, 1         ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next.i3, 4000   ; <i1> [#uses=1]
  br i1 %exitcond, label %bb6.i, label %bb2.i

bb6.i:                                            ; preds = %bb4.i
  %27 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %28 = bitcast %struct._IO_FILE* %27 to i8*      ; <i8*> [#uses=1]
  %29 = tail call i32 @fputc(i32 10, i8* %28) nounwind ; <i32> [#uses=0]
  ret i32 0

print_array.exit:                                 ; preds = %bb.i, %scop_func.exit
  ret i32 0
}
