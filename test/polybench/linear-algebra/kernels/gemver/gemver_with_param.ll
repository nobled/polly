; RUN: opt -indvars -polly-scop-detect  -analyze %s | FileCheck %s
; XFAIL: *
; ModuleID = '<stdin>'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }

@alpha = common global double 0.000000e+00        ; <double*> [#uses=3]
@beta = common global double 0.000000e+00         ; <double*> [#uses=3]
@u1 = common global [4000 x double] zeroinitializer, align 32 ; <[4000 x double]*> [#uses=3]
@u2 = common global [4000 x double] zeroinitializer, align 32 ; <[4000 x double]*> [#uses=3]
@v1 = common global [4000 x double] zeroinitializer, align 32 ; <[4000 x double]*> [#uses=3]
@v2 = common global [4000 x double] zeroinitializer, align 32 ; <[4000 x double]*> [#uses=3]
@y = common global [4000 x double] zeroinitializer, align 32 ; <[4000 x double]*> [#uses=3]
@z = common global [4000 x double] zeroinitializer, align 32 ; <[4000 x double]*> [#uses=3]
@x = common global [4000 x double] zeroinitializer, align 32 ; <[4000 x double]*> [#uses=5]
@w = common global [4000 x double] zeroinitializer, align 32 ; <[4000 x double]*> [#uses=5]
@A = common global [4000 x [4000 x double]] zeroinitializer, align 32 ; <[4000 x [4000 x double]]*> [#uses=5]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
@B = common global [4000 x [4000 x double]] zeroinitializer, align 32 ; <[4000 x [4000 x double]]*> [#uses=0]

define void @init_array() nounwind inlinehint {
bb.nph7:
  store double 4.353200e+04, double* @alpha, align 8
  store double 1.231300e+04, double* @beta, align 8
  br label %bb.nph

bb.nph:                                           ; preds = %bb3, %bb.nph7
  %indvar8 = phi i64 [ 0, %bb.nph7 ], [ %tmp, %bb3 ] ; <i64> [#uses=11]
  %storemerge3 = trunc i64 %indvar8 to i32        ; <i32> [#uses=1]
  %scevgep12 = getelementptr [4000 x double]* @w, i64 0, i64 %indvar8 ; <double*> [#uses=1]
  %scevgep13 = getelementptr [4000 x double]* @x, i64 0, i64 %indvar8 ; <double*> [#uses=1]
  %scevgep14 = getelementptr [4000 x double]* @z, i64 0, i64 %indvar8 ; <double*> [#uses=1]
  %scevgep15 = getelementptr [4000 x double]* @y, i64 0, i64 %indvar8 ; <double*> [#uses=1]
  %scevgep16 = getelementptr [4000 x double]* @v2, i64 0, i64 %indvar8 ; <double*> [#uses=1]
  %scevgep17 = getelementptr [4000 x double]* @v1, i64 0, i64 %indvar8 ; <double*> [#uses=1]
  %scevgep18 = getelementptr [4000 x double]* @u2, i64 0, i64 %indvar8 ; <double*> [#uses=1]
  %scevgep19 = getelementptr [4000 x double]* @u1, i64 0, i64 %indvar8 ; <double*> [#uses=1]
  %tmp = add i64 %indvar8, 1                      ; <i64> [#uses=3]
  %tmp20 = trunc i64 %tmp to i32                  ; <i32> [#uses=1]
  %0 = sitofp i32 %storemerge3 to double          ; <double> [#uses=2]
  store double %0, double* %scevgep19, align 8
  %1 = sdiv i32 %tmp20, 4000                      ; <i32> [#uses=1]
  %2 = sitofp i32 %1 to double                    ; <double> [#uses=5]
  %3 = fdiv double %2, 2.000000e+00               ; <double> [#uses=1]
  store double %3, double* %scevgep18, align 8
  %4 = fdiv double %2, 4.000000e+00               ; <double> [#uses=1]
  store double %4, double* %scevgep17, align 8
  %5 = fdiv double %2, 6.000000e+00               ; <double> [#uses=1]
  store double %5, double* %scevgep16, align 8
  %6 = fdiv double %2, 8.000000e+00               ; <double> [#uses=1]
  store double %6, double* %scevgep15, align 8
  %7 = fdiv double %2, 9.000000e+00               ; <double> [#uses=1]
  store double %7, double* %scevgep14, align 8
  store double 0.000000e+00, double* %scevgep13, align 8
  store double 0.000000e+00, double* %scevgep12, align 8
  br label %bb1

bb1:                                              ; preds = %bb1, %bb.nph
  %indvar = phi i64 [ 0, %bb.nph ], [ %indvar.next, %bb1 ] ; <i64> [#uses=3]
  %scevgep = getelementptr [4000 x [4000 x double]]* @A, i64 0, i64 %indvar8, i64 %indvar ; <double*> [#uses=1]
  %storemerge12 = trunc i64 %indvar to i32        ; <i32> [#uses=1]
  %8 = sitofp i32 %storemerge12 to double         ; <double> [#uses=1]
  %9 = fmul double %0, %8                         ; <double> [#uses=1]
  %10 = fdiv double %9, 4.000000e+03              ; <double> [#uses=1]
  store double %10, double* %scevgep, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 4000      ; <i1> [#uses=1]
  br i1 %exitcond, label %bb3, label %bb1

bb3:                                              ; preds = %bb1
  %exitcond10 = icmp eq i64 %tmp, 4000            ; <i1> [#uses=1]
  br i1 %exitcond10, label %return, label %bb.nph

return:                                           ; preds = %bb3
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
  %scevgep = getelementptr [4000 x double]* @w, i64 0, i64 %indvar ; <double*> [#uses=1]
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
bb.nph31.bb.nph31.split_crit_edge:
  br label %bb.nph26

bb.nph26:                                         ; preds = %bb3, %bb.nph31.bb.nph31.split_crit_edge
  %storemerge27 = phi i64 [ 0, %bb.nph31.bb.nph31.split_crit_edge ], [ %10, %bb3 ] ; <i64> [#uses=4]
  %scevgep53 = getelementptr [4000 x double]* @u2, i64 0, i64 %storemerge27 ; <double*> [#uses=1]
  %scevgep52 = getelementptr [4000 x double]* @u1, i64 0, i64 %storemerge27 ; <double*> [#uses=1]
  %0 = load double* %scevgep52, align 8           ; <double> [#uses=1]
  %1 = load double* %scevgep53, align 8           ; <double> [#uses=1]
  br label %bb1

bb1:                                              ; preds = %bb1, %bb.nph26
  %storemerge625 = phi i64 [ 0, %bb.nph26 ], [ %9, %bb1 ] ; <i64> [#uses=4]
  %scevgep47 = getelementptr [4000 x [4000 x double]]* @A, i64 0, i64 %storemerge27, i64 %storemerge625 ; <double*> [#uses=2]
  %scevgep48 = getelementptr [4000 x double]* @v1, i64 0, i64 %storemerge625 ; <double*> [#uses=1]
  %scevgep49 = getelementptr [4000 x double]* @v2, i64 0, i64 %storemerge625 ; <double*> [#uses=1]
  %2 = load double* %scevgep47, align 8           ; <double> [#uses=1]
  %3 = load double* %scevgep48, align 8           ; <double> [#uses=1]
  %4 = fmul double %0, %3                         ; <double> [#uses=1]
  %5 = fadd double %2, %4                         ; <double> [#uses=1]
  %6 = load double* %scevgep49, align 8           ; <double> [#uses=1]
  %7 = fmul double %1, %6                         ; <double> [#uses=1]
  %8 = fadd double %5, %7                         ; <double> [#uses=1]
  store double %8, double* %scevgep47, align 8
  %9 = add nsw i64 %storemerge625, 1              ; <i64> [#uses=2]
  %exitcond46 = icmp eq i64 %9, 4000              ; <i1> [#uses=1]
  br i1 %exitcond46, label %bb3, label %bb1

bb3:                                              ; preds = %bb1
  %10 = add nsw i64 %storemerge27, 1              ; <i64> [#uses=2]
  %exitcond50 = icmp eq i64 %10, 4000             ; <i1> [#uses=1]
  br i1 %exitcond50, label %bb.nph24.bb.nph24.split_crit_edge, label %bb.nph26

bb.nph16:                                         ; preds = %bb.nph24.bb.nph24.split_crit_edge, %bb9
  %storemerge120 = phi i64 [ 0, %bb.nph24.bb.nph24.split_crit_edge ], [ %17, %bb9 ] ; <i64> [#uses=3]
  %scevgep45 = getelementptr [4000 x double]* @x, i64 0, i64 %storemerge120 ; <double*> [#uses=2]
  %.promoted17 = load double* %scevgep45          ; <double> [#uses=1]
  br label %bb7

bb7:                                              ; preds = %bb7, %bb.nph16
  %.tmp.018 = phi double [ %.promoted17, %bb.nph16 ], [ %15, %bb7 ] ; <double> [#uses=1]
  %storemerge515 = phi i64 [ 0, %bb.nph16 ], [ %16, %bb7 ] ; <i64> [#uses=3]
  %scevgep42 = getelementptr [4000 x [4000 x double]]* @A, i64 0, i64 %storemerge515, i64 %storemerge120 ; <double*> [#uses=1]
  %scevgep41 = getelementptr [4000 x double]* @y, i64 0, i64 %storemerge515 ; <double*> [#uses=1]
  %11 = load double* %scevgep42, align 8          ; <double> [#uses=1]
  %12 = fmul double %11, %18                      ; <double> [#uses=1]
  %13 = load double* %scevgep41, align 8          ; <double> [#uses=1]
  %14 = fmul double %12, %13                      ; <double> [#uses=1]
  %15 = fadd double %.tmp.018, %14                ; <double> [#uses=2]
  %16 = add nsw i64 %storemerge515, 1             ; <i64> [#uses=2]
  %exitcond40 = icmp eq i64 %16, 4000             ; <i1> [#uses=1]
  br i1 %exitcond40, label %bb9, label %bb7

bb9:                                              ; preds = %bb7
  store double %15, double* %scevgep45
  %17 = add nsw i64 %storemerge120, 1             ; <i64> [#uses=2]
  %exitcond43 = icmp eq i64 %17, 4000             ; <i1> [#uses=1]
  br i1 %exitcond43, label %bb12, label %bb.nph16

bb.nph24.bb.nph24.split_crit_edge:                ; preds = %bb3
  %18 = load double* @beta, align 8               ; <double> [#uses=1]
  br label %bb.nph16

bb12:                                             ; preds = %bb12, %bb9
  %storemerge213 = phi i64 [ %22, %bb12 ], [ 0, %bb9 ] ; <i64> [#uses=3]
  %scevgep37 = getelementptr [4000 x double]* @z, i64 0, i64 %storemerge213 ; <double*> [#uses=1]
  %scevgep38 = getelementptr [4000 x double]* @x, i64 0, i64 %storemerge213 ; <double*> [#uses=2]
  %19 = load double* %scevgep38, align 8          ; <double> [#uses=1]
  %20 = load double* %scevgep37, align 8          ; <double> [#uses=1]
  %21 = fadd double %19, %20                      ; <double> [#uses=1]
  store double %21, double* %scevgep38, align 8
  %22 = add nsw i64 %storemerge213, 1             ; <i64> [#uses=2]
  %exitcond36 = icmp eq i64 %22, 4000             ; <i1> [#uses=1]
  br i1 %exitcond36, label %bb.nph12.bb.nph12.split_crit_edge, label %bb12

bb.nph:                                           ; preds = %bb.nph12.bb.nph12.split_crit_edge, %bb18
  %storemerge38 = phi i64 [ 0, %bb.nph12.bb.nph12.split_crit_edge ], [ %29, %bb18 ] ; <i64> [#uses=3]
  %scevgep35 = getelementptr [4000 x double]* @w, i64 0, i64 %storemerge38 ; <double*> [#uses=2]
  %.promoted = load double* %scevgep35            ; <double> [#uses=1]
  br label %bb16

bb16:                                             ; preds = %bb16, %bb.nph
  %.tmp.0 = phi double [ %.promoted, %bb.nph ], [ %27, %bb16 ] ; <double> [#uses=1]
  %storemerge47 = phi i64 [ 0, %bb.nph ], [ %28, %bb16 ] ; <i64> [#uses=3]
  %scevgep32 = getelementptr [4000 x [4000 x double]]* @A, i64 0, i64 %storemerge38, i64 %storemerge47 ; <double*> [#uses=1]
  %scevgep = getelementptr [4000 x double]* @x, i64 0, i64 %storemerge47 ; <double*> [#uses=1]
  %23 = load double* %scevgep32, align 8          ; <double> [#uses=1]
  %24 = fmul double %23, %30                      ; <double> [#uses=1]
  %25 = load double* %scevgep, align 8            ; <double> [#uses=1]
  %26 = fmul double %24, %25                      ; <double> [#uses=1]
  %27 = fadd double %.tmp.0, %26                  ; <double> [#uses=2]
  %28 = add nsw i64 %storemerge47, 1              ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %28, 4000               ; <i1> [#uses=1]
  br i1 %exitcond, label %bb18, label %bb16

bb18:                                             ; preds = %bb16
  store double %27, double* %scevgep35
  %29 = add nsw i64 %storemerge38, 1              ; <i64> [#uses=2]
  %exitcond33 = icmp eq i64 %29, 4000             ; <i1> [#uses=1]
  br i1 %exitcond33, label %return, label %bb.nph

bb.nph12.bb.nph12.split_crit_edge:                ; preds = %bb12
  %30 = load double* @alpha, align 8              ; <double> [#uses=1]
  br label %bb.nph

return:                                           ; preds = %bb18
  ret void
}

define i32 @main(i32 %argc, i8** %argv) nounwind {
entry:
  store double 4.353200e+04, double* @alpha, align 8
  store double 1.231300e+04, double* @beta, align 8
  br label %bb.nph.i

bb.nph.i:                                         ; preds = %bb3.i, %entry
  %indvar8.i = phi i64 [ 0, %entry ], [ %tmp, %bb3.i ] ; <i64> [#uses=11]
  %tmp = add i64 %indvar8.i, 1                    ; <i64> [#uses=3]
  %tmp20.i = trunc i64 %tmp to i32                ; <i32> [#uses=1]
  %scevgep19.i = getelementptr [4000 x double]* @u1, i64 0, i64 %indvar8.i ; <double*> [#uses=1]
  %scevgep18.i = getelementptr [4000 x double]* @u2, i64 0, i64 %indvar8.i ; <double*> [#uses=1]
  %scevgep17.i = getelementptr [4000 x double]* @v1, i64 0, i64 %indvar8.i ; <double*> [#uses=1]
  %scevgep16.i = getelementptr [4000 x double]* @v2, i64 0, i64 %indvar8.i ; <double*> [#uses=1]
  %scevgep15.i = getelementptr [4000 x double]* @y, i64 0, i64 %indvar8.i ; <double*> [#uses=1]
  %scevgep14.i = getelementptr [4000 x double]* @z, i64 0, i64 %indvar8.i ; <double*> [#uses=1]
  %scevgep13.i = getelementptr [4000 x double]* @x, i64 0, i64 %indvar8.i ; <double*> [#uses=1]
  %scevgep12.i = getelementptr [4000 x double]* @w, i64 0, i64 %indvar8.i ; <double*> [#uses=1]
  %storemerge3.i = trunc i64 %indvar8.i to i32    ; <i32> [#uses=1]
  %0 = sitofp i32 %storemerge3.i to double        ; <double> [#uses=2]
  store double %0, double* %scevgep19.i, align 8
  %1 = sdiv i32 %tmp20.i, 4000                    ; <i32> [#uses=1]
  %2 = sitofp i32 %1 to double                    ; <double> [#uses=5]
  %3 = fdiv double %2, 2.000000e+00               ; <double> [#uses=1]
  store double %3, double* %scevgep18.i, align 8
  %4 = fdiv double %2, 4.000000e+00               ; <double> [#uses=1]
  store double %4, double* %scevgep17.i, align 8
  %5 = fdiv double %2, 6.000000e+00               ; <double> [#uses=1]
  store double %5, double* %scevgep16.i, align 8
  %6 = fdiv double %2, 8.000000e+00               ; <double> [#uses=1]
  store double %6, double* %scevgep15.i, align 8
  %7 = fdiv double %2, 9.000000e+00               ; <double> [#uses=1]
  store double %7, double* %scevgep14.i, align 8
  store double 0.000000e+00, double* %scevgep13.i, align 8
  store double 0.000000e+00, double* %scevgep12.i, align 8
  br label %bb1.i

bb1.i:                                            ; preds = %bb1.i, %bb.nph.i
  %indvar.i = phi i64 [ 0, %bb.nph.i ], [ %indvar.next.i, %bb1.i ] ; <i64> [#uses=3]
  %scevgep.i = getelementptr [4000 x [4000 x double]]* @A, i64 0, i64 %indvar8.i, i64 %indvar.i ; <double*> [#uses=1]
  %storemerge12.i = trunc i64 %indvar.i to i32    ; <i32> [#uses=1]
  %8 = sitofp i32 %storemerge12.i to double       ; <double> [#uses=1]
  %9 = fmul double %0, %8                         ; <double> [#uses=1]
  %10 = fdiv double %9, 4.000000e+03              ; <double> [#uses=1]
  store double %10, double* %scevgep.i, align 8
  %indvar.next.i = add i64 %indvar.i, 1           ; <i64> [#uses=2]
  %exitcond6 = icmp eq i64 %indvar.next.i, 4000   ; <i1> [#uses=1]
  br i1 %exitcond6, label %bb3.i, label %bb1.i

bb3.i:                                            ; preds = %bb1.i
  %exitcond7 = icmp eq i64 %tmp, 4000             ; <i1> [#uses=1]
  br i1 %exitcond7, label %init_array.exit, label %bb.nph.i

init_array.exit:                                  ; preds = %bb3.i
  tail call void @scop_func(i64 4000) nounwind
  %11 = icmp sgt i32 %argc, 42                    ; <i1> [#uses=1]
  br i1 %11, label %bb.i, label %print_array.exit

bb.i:                                             ; preds = %init_array.exit
  %12 = load i8** %argv, align 1                  ; <i8*> [#uses=1]
  %13 = load i8* %12, align 1                     ; <i8> [#uses=1]
  %14 = icmp eq i8 %13, 0                         ; <i1> [#uses=1]
  br i1 %14, label %bb2.i, label %print_array.exit

bb2.i:                                            ; preds = %bb4.i, %bb.i
  %indvar.i1 = phi i64 [ %indvar.next.i4, %bb4.i ], [ 0, %bb.i ] ; <i64> [#uses=3]
  %scevgep.i2 = getelementptr [4000 x double]* @w, i64 0, i64 %indvar.i1 ; <double*> [#uses=1]
  %storemerge1.i = trunc i64 %indvar.i1 to i32    ; <i32> [#uses=1]
  %15 = load double* %scevgep.i2, align 8         ; <double> [#uses=1]
  %16 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %17 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %16, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %15) nounwind ; <i32> [#uses=0]
  %18 = srem i32 %storemerge1.i, 80               ; <i32> [#uses=1]
  %19 = icmp eq i32 %18, 20                       ; <i1> [#uses=1]
  br i1 %19, label %bb3.i3, label %bb4.i

bb3.i3:                                           ; preds = %bb2.i
  %20 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %21 = bitcast %struct._IO_FILE* %20 to i8*      ; <i8*> [#uses=1]
  %22 = tail call i32 @fputc(i32 10, i8* %21) nounwind ; <i32> [#uses=0]
  br label %bb4.i

bb4.i:                                            ; preds = %bb3.i3, %bb2.i
  %indvar.next.i4 = add i64 %indvar.i1, 1         ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next.i4, 4000   ; <i1> [#uses=1]
  br i1 %exitcond, label %bb6.i, label %bb2.i

bb6.i:                                            ; preds = %bb4.i
  %23 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %24 = bitcast %struct._IO_FILE* %23 to i8*      ; <i8*> [#uses=1]
  %25 = tail call i32 @fputc(i32 10, i8* %24) nounwind ; <i32> [#uses=0]
  ret i32 0

print_array.exit:                                 ; preds = %bb.i, %init_array.exit
  ret i32 0
}
