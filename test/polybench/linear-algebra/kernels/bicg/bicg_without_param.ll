; RUN: opt -indvars -polly-scop-detect  -analyze %s | FileCheck %s
; XFAIL: *
; ModuleID = '<stdin>'
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
@q = common global [8000 x double] zeroinitializer, align 32 ; <[8000 x double]*> [#uses=4]

define void @init_array() nounwind inlinehint {
bb.nph7.split.us:
  br label %bb.nph.us

bb3.us:                                           ; preds = %bb1.us
  %indvar.next9 = add i64 %indvar8, 1             ; <i64> [#uses=2]
  %exitcond10 = icmp eq i64 %indvar.next9, 8000   ; <i1> [#uses=1]
  br i1 %exitcond10, label %return, label %bb.nph.us

bb1.us:                                           ; preds = %bb.nph.us, %bb1.us
  %indvar = phi i64 [ 0, %bb.nph.us ], [ %indvar.next, %bb1.us ] ; <i64> [#uses=3]
  %scevgep = getelementptr [8000 x [8000 x double]]* @A, i64 0, i64 %indvar8, i64 %indvar ; <double*> [#uses=1]
  %storemerge12.us = trunc i64 %indvar to i32     ; <i32> [#uses=1]
  %0 = sitofp i32 %storemerge12.us to double      ; <double> [#uses=1]
  %1 = fmul double %3, %0                         ; <double> [#uses=1]
  %2 = fdiv double %1, 8.000000e+03               ; <double> [#uses=1]
  store double %2, double* %scevgep, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 8000      ; <i1> [#uses=1]
  br i1 %exitcond, label %bb3.us, label %bb1.us

bb.nph.us:                                        ; preds = %bb3.us, %bb.nph7.split.us
  %indvar8 = phi i64 [ %indvar.next9, %bb3.us ], [ 0, %bb.nph7.split.us ] ; <i64> [#uses=5]
  %storemerge3.us = trunc i64 %indvar8 to i32     ; <i32> [#uses=1]
  %scevgep12 = getelementptr [8000 x double]* @p, i64 0, i64 %indvar8 ; <double*> [#uses=1]
  %scevgep13 = getelementptr [8000 x double]* @r, i64 0, i64 %indvar8 ; <double*> [#uses=1]
  %3 = sitofp i32 %storemerge3.us to double       ; <double> [#uses=2]
  %4 = fmul double %3, 0x400921FB54442D18         ; <double> [#uses=2]
  store double %4, double* %scevgep13, align 8
  store double %4, double* %scevgep12, align 8
  br label %bb1.us

return:                                           ; preds = %bb3.us
  ret void
}

define void @print_array(i32 %argc, i8** %argv) nounwind inlinehint {
entry:
  %0 = icmp sgt i32 %argc, 42                     ; <i1> [#uses=1]
  br i1 %0, label %bb, label %return

bb:                                               ; preds = %entry
  %1 = load i8** %argv, align 1                   ; <i8*> [#uses=1]
  %2 = load i8* %1, align 1                       ; <i8> [#uses=1]
  %3 = icmp eq i8 %2, 0                           ; <i1> [#uses=1]
  br i1 %3, label %bb2, label %return

bb2:                                              ; preds = %bb4, %bb
  %indvar = phi i64 [ %indvar.next, %bb4 ], [ 0, %bb ] ; <i64> [#uses=4]
  %storemerge1 = trunc i64 %indvar to i32         ; <i32> [#uses=1]
  %scevgep = getelementptr [8000 x double]* @q, i64 0, i64 %indvar ; <double*> [#uses=1]
  %scevgep2 = getelementptr [8000 x double]* @s, i64 0, i64 %indvar ; <double*> [#uses=1]
  %4 = load double* %scevgep2, align 8            ; <double> [#uses=1]
  %5 = load %struct._IO_FILE** @stderr, align 8   ; <%struct._IO_FILE*> [#uses=1]
  %6 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %5, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %4) nounwind ; <i32> [#uses=0]
  %7 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %8 = load %struct._IO_FILE** @stderr, align 8   ; <%struct._IO_FILE*> [#uses=1]
  %9 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %8, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %7) nounwind ; <i32> [#uses=0]
  %10 = srem i32 %storemerge1, 80                 ; <i32> [#uses=1]
  %11 = icmp eq i32 %10, 20                       ; <i1> [#uses=1]
  br i1 %11, label %bb3, label %bb4

bb3:                                              ; preds = %bb2
  %12 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %13 = bitcast %struct._IO_FILE* %12 to i8*      ; <i8*> [#uses=1]
  %14 = tail call i32 @fputc(i32 10, i8* %13) nounwind ; <i32> [#uses=0]
  br label %bb4

bb4:                                              ; preds = %bb3, %bb2
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 8000      ; <i1> [#uses=1]
  br i1 %exitcond, label %bb6, label %bb2

bb6:                                              ; preds = %bb4
  %15 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %16 = bitcast %struct._IO_FILE* %15 to i8*      ; <i8*> [#uses=1]
  %17 = tail call i32 @fputc(i32 10, i8* %16) nounwind ; <i32> [#uses=0]
  ret void

return:                                           ; preds = %bb, %entry
  ret void
}

declare i32 @fprintf(%struct._IO_FILE* noalias nocapture, i8* noalias nocapture, ...) nounwind

declare i32 @fputc(i32, i8* nocapture) nounwind

define void @scop_func() nounwind {
bb.nph10:
  br label %bb

bb:                                               ; preds = %bb, %bb.nph10
  %storemerge9 = phi i64 [ 0, %bb.nph10 ], [ %0, %bb ] ; <i64> [#uses=2]
  %scevgep18 = getelementptr [8000 x double]* @s, i64 0, i64 %storemerge9 ; <double*> [#uses=1]
  store double 0.000000e+00, double* %scevgep18, align 8
  %0 = add nsw i64 %storemerge9, 1                ; <i64> [#uses=2]
  %exitcond17 = icmp eq i64 %0, 8000              ; <i1> [#uses=1]
  br i1 %exitcond17, label %bb.nph.us, label %bb

bb6.us:                                           ; preds = %bb4.us
  store double %8, double* %scevgep15
  %1 = add nsw i64 %storemerge14.us, 1            ; <i64> [#uses=2]
  %exitcond13 = icmp eq i64 %1, 8000              ; <i1> [#uses=1]
  br i1 %exitcond13, label %return, label %bb.nph.us

bb4.us:                                           ; preds = %bb.nph.us, %bb4.us
  %.tmp.0.us = phi double [ 0.000000e+00, %bb.nph.us ], [ %8, %bb4.us ] ; <double> [#uses=1]
  %storemerge23.us = phi i64 [ 0, %bb.nph.us ], [ %9, %bb4.us ] ; <i64> [#uses=4]
  %scevgep11 = getelementptr [8000 x [8000 x double]]* @A, i64 0, i64 %storemerge14.us, i64 %storemerge23.us ; <double*> [#uses=1]
  %scevgep = getelementptr [8000 x double]* @p, i64 0, i64 %storemerge23.us ; <double*> [#uses=1]
  %scevgep12 = getelementptr [8000 x double]* @s, i64 0, i64 %storemerge23.us ; <double*> [#uses=2]
  %2 = load double* %scevgep12, align 8           ; <double> [#uses=1]
  %3 = load double* %scevgep11, align 8           ; <double> [#uses=2]
  %4 = fmul double %10, %3                        ; <double> [#uses=1]
  %5 = fadd double %2, %4                         ; <double> [#uses=1]
  store double %5, double* %scevgep12, align 8
  %6 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %7 = fmul double %3, %6                         ; <double> [#uses=1]
  %8 = fadd double %.tmp.0.us, %7                 ; <double> [#uses=2]
  %9 = add nsw i64 %storemerge23.us, 1            ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %9, 8000                ; <i1> [#uses=1]
  br i1 %exitcond, label %bb6.us, label %bb4.us

bb.nph.us:                                        ; preds = %bb6.us, %bb
  %storemerge14.us = phi i64 [ %1, %bb6.us ], [ 0, %bb ] ; <i64> [#uses=4]
  %scevgep15 = getelementptr [8000 x double]* @q, i64 0, i64 %storemerge14.us ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep15, align 8
  %scevgep16 = getelementptr [8000 x double]* @r, i64 0, i64 %storemerge14.us ; <double*> [#uses=1]
  %10 = load double* %scevgep16, align 8          ; <double> [#uses=1]
  br label %bb4.us

return:                                           ; preds = %bb6.us
  ret void
}

define i32 @main(i32 %argc, i8** %argv) nounwind {
entry:
  br label %bb.nph.us.i

bb3.us.i:                                         ; preds = %bb1.us.i
  %indvar.next9.i = add i64 %indvar8.i, 1         ; <i64> [#uses=2]
  %exitcond15 = icmp eq i64 %indvar.next9.i, 8000 ; <i1> [#uses=1]
  br i1 %exitcond15, label %bb.i5, label %bb.nph.us.i

bb1.us.i:                                         ; preds = %bb.nph.us.i, %bb1.us.i
  %indvar.i = phi i64 [ 0, %bb.nph.us.i ], [ %indvar.next.i, %bb1.us.i ] ; <i64> [#uses=3]
  %scevgep.i = getelementptr [8000 x [8000 x double]]* @A, i64 0, i64 %indvar8.i, i64 %indvar.i ; <double*> [#uses=1]
  %storemerge12.us.i = trunc i64 %indvar.i to i32 ; <i32> [#uses=1]
  %0 = sitofp i32 %storemerge12.us.i to double    ; <double> [#uses=1]
  %1 = fmul double %3, %0                         ; <double> [#uses=1]
  %2 = fdiv double %1, 8.000000e+03               ; <double> [#uses=1]
  store double %2, double* %scevgep.i, align 8
  %indvar.next.i = add i64 %indvar.i, 1           ; <i64> [#uses=2]
  %exitcond14 = icmp eq i64 %indvar.next.i, 8000  ; <i1> [#uses=1]
  br i1 %exitcond14, label %bb3.us.i, label %bb1.us.i

bb.nph.us.i:                                      ; preds = %bb3.us.i, %entry
  %indvar8.i = phi i64 [ %indvar.next9.i, %bb3.us.i ], [ 0, %entry ] ; <i64> [#uses=5]
  %scevgep13.i = getelementptr [8000 x double]* @r, i64 0, i64 %indvar8.i ; <double*> [#uses=1]
  %scevgep12.i = getelementptr [8000 x double]* @p, i64 0, i64 %indvar8.i ; <double*> [#uses=1]
  %storemerge3.us.i = trunc i64 %indvar8.i to i32 ; <i32> [#uses=1]
  %3 = sitofp i32 %storemerge3.us.i to double     ; <double> [#uses=2]
  %4 = fmul double %3, 0x400921FB54442D18         ; <double> [#uses=2]
  store double %4, double* %scevgep13.i, align 8
  store double %4, double* %scevgep12.i, align 8
  br label %bb1.us.i

bb.i5:                                            ; preds = %bb.i5, %bb3.us.i
  %storemerge9.i = phi i64 [ %5, %bb.i5 ], [ 0, %bb3.us.i ] ; <i64> [#uses=2]
  %scevgep18.i = getelementptr [8000 x double]* @s, i64 0, i64 %storemerge9.i ; <double*> [#uses=1]
  store double 0.000000e+00, double* %scevgep18.i, align 8
  %5 = add nsw i64 %storemerge9.i, 1              ; <i64> [#uses=2]
  %exitcond13 = icmp eq i64 %5, 8000              ; <i1> [#uses=1]
  br i1 %exitcond13, label %bb.nph.us.i9, label %bb.i5

bb6.us.i:                                         ; preds = %bb4.us.i
  store double %13, double* %scevgep15.i
  %6 = add nsw i64 %storemerge14.us.i, 1          ; <i64> [#uses=2]
  %exitcond12 = icmp eq i64 %6, 8000              ; <i1> [#uses=1]
  br i1 %exitcond12, label %scop_func.exit, label %bb.nph.us.i9

bb4.us.i:                                         ; preds = %bb.nph.us.i9, %bb4.us.i
  %.tmp.0.us.i = phi double [ 0.000000e+00, %bb.nph.us.i9 ], [ %13, %bb4.us.i ] ; <double> [#uses=1]
  %storemerge23.us.i = phi i64 [ 0, %bb.nph.us.i9 ], [ %14, %bb4.us.i ] ; <i64> [#uses=4]
  %scevgep11.i = getelementptr [8000 x [8000 x double]]* @A, i64 0, i64 %storemerge14.us.i, i64 %storemerge23.us.i ; <double*> [#uses=1]
  %scevgep12.i7 = getelementptr [8000 x double]* @s, i64 0, i64 %storemerge23.us.i ; <double*> [#uses=2]
  %scevgep.i6 = getelementptr [8000 x double]* @p, i64 0, i64 %storemerge23.us.i ; <double*> [#uses=1]
  %7 = load double* %scevgep12.i7, align 8        ; <double> [#uses=1]
  %8 = load double* %scevgep11.i, align 8         ; <double> [#uses=2]
  %9 = fmul double %15, %8                        ; <double> [#uses=1]
  %10 = fadd double %7, %9                        ; <double> [#uses=1]
  store double %10, double* %scevgep12.i7, align 8
  %11 = load double* %scevgep.i6, align 8         ; <double> [#uses=1]
  %12 = fmul double %8, %11                       ; <double> [#uses=1]
  %13 = fadd double %.tmp.0.us.i, %12             ; <double> [#uses=2]
  %14 = add nsw i64 %storemerge23.us.i, 1         ; <i64> [#uses=2]
  %exitcond11 = icmp eq i64 %14, 8000             ; <i1> [#uses=1]
  br i1 %exitcond11, label %bb6.us.i, label %bb4.us.i

bb.nph.us.i9:                                     ; preds = %bb6.us.i, %bb.i5
  %storemerge14.us.i = phi i64 [ %6, %bb6.us.i ], [ 0, %bb.i5 ] ; <i64> [#uses=4]
  %scevgep16.i = getelementptr [8000 x double]* @r, i64 0, i64 %storemerge14.us.i ; <double*> [#uses=1]
  %scevgep15.i = getelementptr [8000 x double]* @q, i64 0, i64 %storemerge14.us.i ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep15.i, align 8
  %15 = load double* %scevgep16.i, align 8        ; <double> [#uses=1]
  br label %bb4.us.i

scop_func.exit:                                   ; preds = %bb6.us.i
  %16 = icmp sgt i32 %argc, 42                    ; <i1> [#uses=1]
  br i1 %16, label %bb.i, label %print_array.exit

bb.i:                                             ; preds = %scop_func.exit
  %17 = load i8** %argv, align 1                  ; <i8*> [#uses=1]
  %18 = load i8* %17, align 1                     ; <i8> [#uses=1]
  %19 = icmp eq i8 %18, 0                         ; <i1> [#uses=1]
  br i1 %19, label %bb2.i, label %print_array.exit

bb2.i:                                            ; preds = %bb4.i, %bb.i
  %indvar.i1 = phi i64 [ %indvar.next.i3, %bb4.i ], [ 0, %bb.i ] ; <i64> [#uses=4]
  %scevgep2.i = getelementptr [8000 x double]* @s, i64 0, i64 %indvar.i1 ; <double*> [#uses=1]
  %scevgep.i2 = getelementptr [8000 x double]* @q, i64 0, i64 %indvar.i1 ; <double*> [#uses=1]
  %storemerge1.i = trunc i64 %indvar.i1 to i32    ; <i32> [#uses=1]
  %20 = load double* %scevgep2.i, align 8         ; <double> [#uses=1]
  %21 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %22 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %21, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %20) nounwind ; <i32> [#uses=0]
  %23 = load double* %scevgep.i2, align 8         ; <double> [#uses=1]
  %24 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %25 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %24, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %23) nounwind ; <i32> [#uses=0]
  %26 = srem i32 %storemerge1.i, 80               ; <i32> [#uses=1]
  %27 = icmp eq i32 %26, 20                       ; <i1> [#uses=1]
  br i1 %27, label %bb3.i, label %bb4.i

bb3.i:                                            ; preds = %bb2.i
  %28 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %29 = bitcast %struct._IO_FILE* %28 to i8*      ; <i8*> [#uses=1]
  %30 = tail call i32 @fputc(i32 10, i8* %29) nounwind ; <i32> [#uses=0]
  br label %bb4.i

bb4.i:                                            ; preds = %bb3.i, %bb2.i
  %indvar.next.i3 = add i64 %indvar.i1, 1         ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next.i3, 8000   ; <i1> [#uses=1]
  br i1 %exitcond, label %bb6.i, label %bb2.i

bb6.i:                                            ; preds = %bb4.i
  %31 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %32 = bitcast %struct._IO_FILE* %31 to i8*      ; <i8*> [#uses=1]
  %33 = tail call i32 @fputc(i32 10, i8* %32) nounwind ; <i32> [#uses=0]
  ret i32 0

print_array.exit:                                 ; preds = %bb.i, %scop_func.exit
  ret i32 0
}
