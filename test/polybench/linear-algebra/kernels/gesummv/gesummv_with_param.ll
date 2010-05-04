; RUN: opt -indvars -polly-scop-detect  -analyze %s | FileCheck %s
; XFAIL: *
; ModuleID = '<stdin>'
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
bb.nph7.split.us:
  store double 4.353200e+04, double* @alpha, align 8
  store double 1.231300e+04, double* @beta, align 8
  br label %bb.nph.us

bb3.us:                                           ; preds = %bb1.us
  %indvar.next9 = add i64 %indvar8, 1             ; <i64> [#uses=2]
  %exitcond10 = icmp eq i64 %indvar.next9, 4000   ; <i1> [#uses=1]
  br i1 %exitcond10, label %return, label %bb.nph.us

bb1.us:                                           ; preds = %bb.nph.us, %bb1.us
  %indvar = phi i64 [ 0, %bb.nph.us ], [ %indvar.next, %bb1.us ] ; <i64> [#uses=3]
  %scevgep = getelementptr [4000 x [4000 x double]]* @A, i64 0, i64 %indvar8, i64 %indvar ; <double*> [#uses=1]
  %storemerge12.us = trunc i64 %indvar to i32     ; <i32> [#uses=1]
  %0 = sitofp i32 %storemerge12.us to double      ; <double> [#uses=1]
  %1 = fmul double %3, %0                         ; <double> [#uses=1]
  %2 = fdiv double %1, 4.000000e+03               ; <double> [#uses=1]
  store double %2, double* %scevgep, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 4000      ; <i1> [#uses=1]
  br i1 %exitcond, label %bb3.us, label %bb1.us

bb.nph.us:                                        ; preds = %bb3.us, %bb.nph7.split.us
  %indvar8 = phi i64 [ %indvar.next9, %bb3.us ], [ 0, %bb.nph7.split.us ] ; <i64> [#uses=4]
  %storemerge3.us = trunc i64 %indvar8 to i32     ; <i32> [#uses=1]
  %scevgep12 = getelementptr [4000 x double]* @x, i64 0, i64 %indvar8 ; <double*> [#uses=1]
  %3 = sitofp i32 %storemerge3.us to double       ; <double> [#uses=2]
  %4 = fdiv double %3, 4.000000e+03               ; <double> [#uses=1]
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
  %indvar = phi i64 [ %indvar.next, %bb4 ], [ 0, %bb ] ; <i64> [#uses=3]
  %storemerge1 = trunc i64 %indvar to i32         ; <i32> [#uses=1]
  %scevgep = getelementptr [4000 x double]* @y, i64 0, i64 %indvar ; <double*> [#uses=1]
  %4 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %5 = load %struct._IO_FILE** @stderr, align 8   ; <%struct._IO_FILE*> [#uses=1]
  %6 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %5, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %4) nounwind ; <i32> [#uses=0]
  %7 = srem i32 %storemerge1, 80                  ; <i32> [#uses=1]
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

define void @scop_func(i64 %n) nounwind {
entry:
  %0 = icmp sgt i64 %n, 0                         ; <i1> [#uses=1]
  br i1 %0, label %bb.nph10.split.us, label %return

bb.nph10.split.us:                                ; preds = %entry
  %1 = load double* @alpha, align 8               ; <double> [#uses=1]
  %2 = load double* @beta, align 8                ; <double> [#uses=1]
  br label %bb.nph.us

bb3.us:                                           ; preds = %bb1.us
  store double %10, double* %scevgep17
  %3 = fmul double %10, %1                        ; <double> [#uses=1]
  %4 = fmul double %13, %2                        ; <double> [#uses=1]
  %5 = fadd double %3, %4                         ; <double> [#uses=1]
  store double %5, double* %scevgep18, align 8
  %6 = add nsw i64 %storemerge6.us, 1             ; <i64> [#uses=2]
  %exitcond14 = icmp eq i64 %6, %n                ; <i1> [#uses=1]
  br i1 %exitcond14, label %return, label %bb.nph.us

bb1.us:                                           ; preds = %bb.nph.us, %bb1.us
  %.tmp3.0.us = phi double [ 0.000000e+00, %bb.nph.us ], [ %13, %bb1.us ] ; <double> [#uses=1]
  %.tmp.0.us = phi double [ 0.000000e+00, %bb.nph.us ], [ %10, %bb1.us ] ; <double> [#uses=1]
  %storemerge12.us = phi i64 [ 0, %bb.nph.us ], [ %14, %bb1.us ] ; <i64> [#uses=4]
  %scevgep13 = getelementptr [4000 x [4000 x double]]* @A, i64 0, i64 %storemerge6.us, i64 %storemerge12.us ; <double*> [#uses=1]
  %scevgep = getelementptr [4000 x [4000 x double]]* @B, i64 0, i64 %storemerge6.us, i64 %storemerge12.us ; <double*> [#uses=1]
  %scevgep12 = getelementptr [4000 x double]* @x, i64 0, i64 %storemerge12.us ; <double*> [#uses=1]
  %7 = load double* %scevgep13, align 8           ; <double> [#uses=1]
  %8 = load double* %scevgep12, align 8           ; <double> [#uses=2]
  %9 = fmul double %7, %8                         ; <double> [#uses=1]
  %10 = fadd double %9, %.tmp.0.us                ; <double> [#uses=3]
  %11 = load double* %scevgep, align 8            ; <double> [#uses=1]
  %12 = fmul double %11, %8                       ; <double> [#uses=1]
  %13 = fadd double %12, %.tmp3.0.us              ; <double> [#uses=2]
  %14 = add nsw i64 %storemerge12.us, 1           ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %14, %n                 ; <i1> [#uses=1]
  br i1 %exitcond, label %bb3.us, label %bb1.us

bb.nph.us:                                        ; preds = %bb3.us, %bb.nph10.split.us
  %storemerge6.us = phi i64 [ 0, %bb.nph10.split.us ], [ %6, %bb3.us ] ; <i64> [#uses=5]
  %scevgep17 = getelementptr [4000 x double]* @tmp, i64 0, i64 %storemerge6.us ; <double*> [#uses=2]
  %scevgep18 = getelementptr [4000 x double]* @y, i64 0, i64 %storemerge6.us ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep17, align 8
  store double 0.000000e+00, double* %scevgep18, align 8
  br label %bb1.us

return:                                           ; preds = %bb3.us, %entry
  ret void
}

define i32 @main(i32 %argc, i8** %argv) nounwind {
entry:
  store double 4.353200e+04, double* @alpha, align 8
  store double 1.231300e+04, double* @beta, align 8
  br label %bb.nph.us.i

bb3.us.i:                                         ; preds = %bb1.us.i
  %indvar.next9.i = add i64 %indvar8.i, 1         ; <i64> [#uses=2]
  %exitcond18 = icmp eq i64 %indvar.next9.i, 4000 ; <i1> [#uses=1]
  br i1 %exitcond18, label %bb.nph.us.i11, label %bb.nph.us.i

bb1.us.i:                                         ; preds = %bb.nph.us.i, %bb1.us.i
  %indvar.i = phi i64 [ 0, %bb.nph.us.i ], [ %indvar.next.i, %bb1.us.i ] ; <i64> [#uses=3]
  %scevgep.i = getelementptr [4000 x [4000 x double]]* @A, i64 0, i64 %indvar8.i, i64 %indvar.i ; <double*> [#uses=1]
  %storemerge12.us.i = trunc i64 %indvar.i to i32 ; <i32> [#uses=1]
  %0 = sitofp i32 %storemerge12.us.i to double    ; <double> [#uses=1]
  %1 = fmul double %3, %0                         ; <double> [#uses=1]
  %2 = fdiv double %1, 4.000000e+03               ; <double> [#uses=1]
  store double %2, double* %scevgep.i, align 8
  %indvar.next.i = add i64 %indvar.i, 1           ; <i64> [#uses=2]
  %exitcond17 = icmp eq i64 %indvar.next.i, 4000  ; <i1> [#uses=1]
  br i1 %exitcond17, label %bb3.us.i, label %bb1.us.i

bb.nph.us.i:                                      ; preds = %bb3.us.i, %entry
  %indvar8.i = phi i64 [ %indvar.next9.i, %bb3.us.i ], [ 0, %entry ] ; <i64> [#uses=4]
  %scevgep12.i = getelementptr [4000 x double]* @x, i64 0, i64 %indvar8.i ; <double*> [#uses=1]
  %storemerge3.us.i = trunc i64 %indvar8.i to i32 ; <i32> [#uses=1]
  %3 = sitofp i32 %storemerge3.us.i to double     ; <double> [#uses=2]
  %4 = fdiv double %3, 4.000000e+03               ; <double> [#uses=1]
  store double %4, double* %scevgep12.i, align 8
  br label %bb1.us.i

bb3.us.i5:                                        ; preds = %bb1.us.i10
  store double %12, double* %scevgep17.i
  %5 = fmul double %12, 4.353200e+04              ; <double> [#uses=1]
  %6 = fmul double %15, 1.231300e+04              ; <double> [#uses=1]
  %7 = fadd double %5, %6                         ; <double> [#uses=1]
  store double %7, double* %scevgep18.i, align 8
  %8 = add nsw i64 %storemerge6.us.i, 1           ; <i64> [#uses=2]
  %exitcond16 = icmp eq i64 %8, 4000              ; <i1> [#uses=1]
  br i1 %exitcond16, label %scop_func.exit, label %bb.nph.us.i11

bb1.us.i10:                                       ; preds = %bb.nph.us.i11, %bb1.us.i10
  %.tmp3.0.us.i = phi double [ 0.000000e+00, %bb.nph.us.i11 ], [ %15, %bb1.us.i10 ] ; <double> [#uses=1]
  %.tmp.0.us.i = phi double [ 0.000000e+00, %bb.nph.us.i11 ], [ %12, %bb1.us.i10 ] ; <double> [#uses=1]
  %storemerge12.us.i6 = phi i64 [ 0, %bb.nph.us.i11 ], [ %16, %bb1.us.i10 ] ; <i64> [#uses=4]
  %scevgep13.i = getelementptr [4000 x [4000 x double]]* @A, i64 0, i64 %storemerge6.us.i, i64 %storemerge12.us.i6 ; <double*> [#uses=1]
  %scevgep.i7 = getelementptr [4000 x [4000 x double]]* @B, i64 0, i64 %storemerge6.us.i, i64 %storemerge12.us.i6 ; <double*> [#uses=1]
  %scevgep12.i8 = getelementptr [4000 x double]* @x, i64 0, i64 %storemerge12.us.i6 ; <double*> [#uses=1]
  %9 = load double* %scevgep13.i, align 8         ; <double> [#uses=1]
  %10 = load double* %scevgep12.i8, align 8       ; <double> [#uses=2]
  %11 = fmul double %9, %10                       ; <double> [#uses=1]
  %12 = fadd double %11, %.tmp.0.us.i             ; <double> [#uses=3]
  %13 = load double* %scevgep.i7, align 8         ; <double> [#uses=1]
  %14 = fmul double %13, %10                      ; <double> [#uses=1]
  %15 = fadd double %14, %.tmp3.0.us.i            ; <double> [#uses=2]
  %16 = add nsw i64 %storemerge12.us.i6, 1        ; <i64> [#uses=2]
  %exitcond15 = icmp eq i64 %16, 4000             ; <i1> [#uses=1]
  br i1 %exitcond15, label %bb3.us.i5, label %bb1.us.i10

bb.nph.us.i11:                                    ; preds = %bb3.us.i5, %bb3.us.i
  %storemerge6.us.i = phi i64 [ %8, %bb3.us.i5 ], [ 0, %bb3.us.i ] ; <i64> [#uses=5]
  %scevgep18.i = getelementptr [4000 x double]* @y, i64 0, i64 %storemerge6.us.i ; <double*> [#uses=2]
  %scevgep17.i = getelementptr [4000 x double]* @tmp, i64 0, i64 %storemerge6.us.i ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep17.i, align 8
  store double 0.000000e+00, double* %scevgep18.i, align 8
  br label %bb1.us.i10

scop_func.exit:                                   ; preds = %bb3.us.i5
  %17 = icmp sgt i32 %argc, 42                    ; <i1> [#uses=1]
  br i1 %17, label %bb.i, label %print_array.exit

bb.i:                                             ; preds = %scop_func.exit
  %18 = load i8** %argv, align 1                  ; <i8*> [#uses=1]
  %19 = load i8* %18, align 1                     ; <i8> [#uses=1]
  %20 = icmp eq i8 %19, 0                         ; <i1> [#uses=1]
  br i1 %20, label %bb2.i, label %print_array.exit

bb2.i:                                            ; preds = %bb4.i, %bb.i
  %indvar.i1 = phi i64 [ %indvar.next.i3, %bb4.i ], [ 0, %bb.i ] ; <i64> [#uses=3]
  %scevgep.i2 = getelementptr [4000 x double]* @y, i64 0, i64 %indvar.i1 ; <double*> [#uses=1]
  %storemerge1.i = trunc i64 %indvar.i1 to i32    ; <i32> [#uses=1]
  %21 = load double* %scevgep.i2, align 8         ; <double> [#uses=1]
  %22 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %23 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %22, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %21) nounwind ; <i32> [#uses=0]
  %24 = srem i32 %storemerge1.i, 80               ; <i32> [#uses=1]
  %25 = icmp eq i32 %24, 20                       ; <i1> [#uses=1]
  br i1 %25, label %bb3.i, label %bb4.i

bb3.i:                                            ; preds = %bb2.i
  %26 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %27 = bitcast %struct._IO_FILE* %26 to i8*      ; <i8*> [#uses=1]
  %28 = tail call i32 @fputc(i32 10, i8* %27) nounwind ; <i32> [#uses=0]
  br label %bb4.i

bb4.i:                                            ; preds = %bb3.i, %bb2.i
  %indvar.next.i3 = add i64 %indvar.i1, 1         ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next.i3, 4000   ; <i1> [#uses=1]
  br i1 %exitcond, label %bb6.i, label %bb2.i

bb6.i:                                            ; preds = %bb4.i
  %29 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %30 = bitcast %struct._IO_FILE* %29 to i8*      ; <i8*> [#uses=1]
  %31 = tail call i32 @fputc(i32 10, i8* %30) nounwind ; <i32> [#uses=0]
  ret i32 0

print_array.exit:                                 ; preds = %bb.i, %scop_func.exit
  ret i32 0
}
