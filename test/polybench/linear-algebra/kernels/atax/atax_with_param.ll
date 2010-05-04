; RUN: opt -indvars -polly-scop-detect  -analyze %s | FileCheck %s
; XFAIL: *
; ModuleID = '<stdin>'
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
  %indvar8 = phi i64 [ %indvar.next9, %bb3.us ], [ 0, %bb.nph7.split.us ] ; <i64> [#uses=4]
  %storemerge3.us = trunc i64 %indvar8 to i32     ; <i32> [#uses=1]
  %scevgep12 = getelementptr [8000 x double]* @x, i64 0, i64 %indvar8 ; <double*> [#uses=1]
  %3 = sitofp i32 %storemerge3.us to double       ; <double> [#uses=2]
  %4 = fmul double %3, 0x400921FB54442D18         ; <double> [#uses=1]
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
  %scevgep = getelementptr [8000 x double]* @y, i64 0, i64 %indvar ; <double*> [#uses=1]
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
  %exitcond = icmp eq i64 %indvar.next, 8000      ; <i1> [#uses=1]
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
  %indvar8.i = phi i64 [ %indvar.next9.i, %bb3.us.i ], [ 0, %entry ] ; <i64> [#uses=4]
  %scevgep12.i = getelementptr [8000 x double]* @x, i64 0, i64 %indvar8.i ; <double*> [#uses=1]
  %storemerge3.us.i = trunc i64 %indvar8.i to i32 ; <i32> [#uses=1]
  %3 = sitofp i32 %storemerge3.us.i to double     ; <double> [#uses=2]
  %4 = fmul double %3, 0x400921FB54442D18         ; <double> [#uses=1]
  store double %4, double* %scevgep12.i, align 8
  br label %bb1.us.i

bb.i5:                                            ; preds = %bb.i5, %bb3.us.i
  %storemerge15.i = phi i64 [ %5, %bb.i5 ], [ 0, %bb3.us.i ] ; <i64> [#uses=2]
  %scevgep26.i = getelementptr [8000 x double]* @y, i64 0, i64 %storemerge15.i ; <double*> [#uses=1]
  store double 0.000000e+00, double* %scevgep26.i, align 8
  %5 = add nsw i64 %storemerge15.i, 1             ; <i64> [#uses=2]
  %exitcond13 = icmp eq i64 %5, 8000              ; <i1> [#uses=1]
  br i1 %exitcond13, label %bb.nph.i, label %bb.i5

bb.nph.i:                                         ; preds = %bb9.i, %bb.i5
  %storemerge17.i = phi i64 [ %16, %bb9.i ], [ 0, %bb.i5 ] ; <i64> [#uses=4]
  %scevgep24.i = getelementptr [8000 x double]* @tmp, i64 0, i64 %storemerge17.i ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep24.i, align 8
  br label %bb4.i8

bb4.i8:                                           ; preds = %bb4.i8, %bb.nph.i
  %.tmp.0.i = phi double [ 0.000000e+00, %bb.nph.i ], [ %9, %bb4.i8 ] ; <double> [#uses=1]
  %storemerge24.i = phi i64 [ 0, %bb.nph.i ], [ %10, %bb4.i8 ] ; <i64> [#uses=3]
  %scevgep17.i = getelementptr [8000 x [8000 x double]]* @A, i64 0, i64 %storemerge17.i, i64 %storemerge24.i ; <double*> [#uses=1]
  %scevgep.i6 = getelementptr [8000 x double]* @x, i64 0, i64 %storemerge24.i ; <double*> [#uses=1]
  %6 = load double* %scevgep17.i, align 8         ; <double> [#uses=1]
  %7 = load double* %scevgep.i6, align 8          ; <double> [#uses=1]
  %8 = fmul double %6, %7                         ; <double> [#uses=1]
  %9 = fadd double %.tmp.0.i, %8                  ; <double> [#uses=3]
  %10 = add nsw i64 %storemerge24.i, 1            ; <i64> [#uses=2]
  %exitcond10 = icmp eq i64 %10, 8000             ; <i1> [#uses=1]
  br i1 %exitcond10, label %bb8.loopexit.i, label %bb4.i8

bb7.i:                                            ; preds = %bb8.loopexit.i, %bb7.i
  %storemerge35.i = phi i64 [ %15, %bb7.i ], [ 0, %bb8.loopexit.i ] ; <i64> [#uses=3]
  %scevgep19.i = getelementptr [8000 x [8000 x double]]* @A, i64 0, i64 %storemerge17.i, i64 %storemerge35.i ; <double*> [#uses=1]
  %scevgep20.i = getelementptr [8000 x double]* @y, i64 0, i64 %storemerge35.i ; <double*> [#uses=2]
  %11 = load double* %scevgep20.i, align 8        ; <double> [#uses=1]
  %12 = load double* %scevgep19.i, align 8        ; <double> [#uses=1]
  %13 = fmul double %12, %9                       ; <double> [#uses=1]
  %14 = fadd double %11, %13                      ; <double> [#uses=1]
  store double %14, double* %scevgep20.i, align 8
  %15 = add nsw i64 %storemerge35.i, 1            ; <i64> [#uses=2]
  %exitcond11 = icmp eq i64 %15, 8000             ; <i1> [#uses=1]
  br i1 %exitcond11, label %bb9.i, label %bb7.i

bb8.loopexit.i:                                   ; preds = %bb4.i8
  store double %9, double* %scevgep24.i
  br label %bb7.i

bb9.i:                                            ; preds = %bb7.i
  %16 = add nsw i64 %storemerge17.i, 1            ; <i64> [#uses=2]
  %exitcond12 = icmp eq i64 %16, 8000             ; <i1> [#uses=1]
  br i1 %exitcond12, label %scop_func.exit, label %bb.nph.i

scop_func.exit:                                   ; preds = %bb9.i
  %17 = icmp sgt i32 %argc, 42                    ; <i1> [#uses=1]
  br i1 %17, label %bb.i, label %print_array.exit

bb.i:                                             ; preds = %scop_func.exit
  %18 = load i8** %argv, align 1                  ; <i8*> [#uses=1]
  %19 = load i8* %18, align 1                     ; <i8> [#uses=1]
  %20 = icmp eq i8 %19, 0                         ; <i1> [#uses=1]
  br i1 %20, label %bb2.i, label %print_array.exit

bb2.i:                                            ; preds = %bb4.i, %bb.i
  %indvar.i1 = phi i64 [ %indvar.next.i3, %bb4.i ], [ 0, %bb.i ] ; <i64> [#uses=3]
  %scevgep.i2 = getelementptr [8000 x double]* @y, i64 0, i64 %indvar.i1 ; <double*> [#uses=1]
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
  %exitcond = icmp eq i64 %indvar.next.i3, 8000   ; <i1> [#uses=1]
  br i1 %exitcond, label %bb6.i, label %bb2.i

bb6.i:                                            ; preds = %bb4.i
  %29 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %30 = bitcast %struct._IO_FILE* %29 to i8*      ; <i8*> [#uses=1]
  %31 = tail call i32 @fputc(i32 10, i8* %30) nounwind ; <i32> [#uses=0]
  ret i32 0

print_array.exit:                                 ; preds = %bb.i, %scop_func.exit
  ret i32 0
}
