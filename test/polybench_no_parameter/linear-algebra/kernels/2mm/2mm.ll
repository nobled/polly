; RUN: opt  -indvars -polly-scop-detect -print-top-scop-only -polly-print-scop -analyze  %s | FileCheck %s
; XFAIL: *
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }

@alpha1 = common global double 0.000000e+00       ; <double*> [#uses=1]
@beta1 = common global double 0.000000e+00        ; <double*> [#uses=1]
@alpha2 = common global double 0.000000e+00       ; <double*> [#uses=1]
@beta2 = common global double 0.000000e+00        ; <double*> [#uses=1]
@A = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=3]
@B = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=3]
@C = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=5]
@D = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=3]
@E = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=5]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]

define void @init_array() nounwind inlinehint {
bb.nph34.bb.nph34.split_crit_edge:
  store double 3.241200e+04, double* @alpha1, align 8
  store double 2.123000e+03, double* @beta1, align 8
  store double 1.324120e+05, double* @alpha2, align 8
  store double 9.212300e+04, double* @beta2, align 8
  br label %bb.nph29

bb.nph29:                                         ; preds = %bb3, %bb.nph34.bb.nph34.split_crit_edge
  %indvar66 = phi i64 [ 0, %bb.nph34.bb.nph34.split_crit_edge ], [ %indvar.next67, %bb3 ] ; <i64> [#uses=3]
  %i.030 = trunc i64 %indvar66 to i32             ; <i32> [#uses=1]
  %0 = sitofp i32 %i.030 to double                ; <double> [#uses=1]
  br label %bb1

bb1:                                              ; preds = %bb1, %bb.nph29
  %indvar63 = phi i64 [ 0, %bb.nph29 ], [ %indvar.next64, %bb1 ] ; <i64> [#uses=3]
  %scevgep68 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %indvar66, i64 %indvar63 ; <double*> [#uses=1]
  %j.028 = trunc i64 %indvar63 to i32             ; <i32> [#uses=1]
  %1 = sitofp i32 %j.028 to double                ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=1]
  %3 = fdiv double %2, 5.120000e+02               ; <double> [#uses=1]
  store double %3, double* %scevgep68, align 8
  %indvar.next64 = add i64 %indvar63, 1           ; <i64> [#uses=2]
  %exitcond65 = icmp eq i64 %indvar.next64, 512   ; <i1> [#uses=1]
  br i1 %exitcond65, label %bb3, label %bb1

bb3:                                              ; preds = %bb1
  %indvar.next67 = add i64 %indvar66, 1           ; <i64> [#uses=2]
  %exitcond69 = icmp eq i64 %indvar.next67, 512   ; <i1> [#uses=1]
  br i1 %exitcond69, label %bb.nph22, label %bb.nph29

bb.nph22:                                         ; preds = %bb9, %bb3
  %indvar58 = phi i64 [ %indvar.next59, %bb9 ], [ 0, %bb3 ] ; <i64> [#uses=3]
  %i.123 = trunc i64 %indvar58 to i32             ; <i32> [#uses=1]
  %4 = sitofp i32 %i.123 to double                ; <double> [#uses=1]
  br label %bb7

bb7:                                              ; preds = %bb7, %bb.nph22
  %indvar55 = phi i64 [ 0, %bb.nph22 ], [ %indvar.next56, %bb7 ] ; <i64> [#uses=3]
  %scevgep60 = getelementptr [512 x [512 x double]]* @B, i64 0, i64 %indvar58, i64 %indvar55 ; <double*> [#uses=1]
  %j.121 = trunc i64 %indvar55 to i32             ; <i32> [#uses=1]
  %5 = sitofp i32 %j.121 to double                ; <double> [#uses=1]
  %6 = fmul double %4, %5                         ; <double> [#uses=1]
  %7 = fadd double %6, 1.000000e+00               ; <double> [#uses=1]
  %8 = fdiv double %7, 5.120000e+02               ; <double> [#uses=1]
  store double %8, double* %scevgep60, align 8
  %indvar.next56 = add i64 %indvar55, 1           ; <i64> [#uses=2]
  %exitcond57 = icmp eq i64 %indvar.next56, 512   ; <i1> [#uses=1]
  br i1 %exitcond57, label %bb9, label %bb7

bb9:                                              ; preds = %bb7
  %indvar.next59 = add i64 %indvar58, 1           ; <i64> [#uses=2]
  %exitcond61 = icmp eq i64 %indvar.next59, 512   ; <i1> [#uses=1]
  br i1 %exitcond61, label %bb.nph15, label %bb.nph22

bb.nph15:                                         ; preds = %bb15, %bb9
  %indvar50 = phi i64 [ %indvar.next51, %bb15 ], [ 0, %bb9 ] ; <i64> [#uses=3]
  %i.216 = trunc i64 %indvar50 to i32             ; <i32> [#uses=1]
  %9 = sitofp i32 %i.216 to double                ; <double> [#uses=1]
  br label %bb13

bb13:                                             ; preds = %bb13, %bb.nph15
  %indvar47 = phi i64 [ 0, %bb.nph15 ], [ %indvar.next48, %bb13 ] ; <i64> [#uses=3]
  %scevgep52 = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %indvar50, i64 %indvar47 ; <double*> [#uses=1]
  %j.214 = trunc i64 %indvar47 to i32             ; <i32> [#uses=1]
  %10 = sitofp i32 %j.214 to double               ; <double> [#uses=1]
  %11 = fmul double %9, %10                       ; <double> [#uses=1]
  %12 = fadd double %11, 2.000000e+00             ; <double> [#uses=1]
  %13 = fdiv double %12, 5.120000e+02             ; <double> [#uses=1]
  store double %13, double* %scevgep52, align 8
  %indvar.next48 = add i64 %indvar47, 1           ; <i64> [#uses=2]
  %exitcond49 = icmp eq i64 %indvar.next48, 512   ; <i1> [#uses=1]
  br i1 %exitcond49, label %bb15, label %bb13

bb15:                                             ; preds = %bb13
  %indvar.next51 = add i64 %indvar50, 1           ; <i64> [#uses=2]
  %exitcond53 = icmp eq i64 %indvar.next51, 512   ; <i1> [#uses=1]
  br i1 %exitcond53, label %bb.nph8, label %bb.nph15

bb.nph8:                                          ; preds = %bb21, %bb15
  %indvar42 = phi i64 [ %indvar.next43, %bb21 ], [ 0, %bb15 ] ; <i64> [#uses=3]
  %i.39 = trunc i64 %indvar42 to i32              ; <i32> [#uses=1]
  %14 = sitofp i32 %i.39 to double                ; <double> [#uses=1]
  br label %bb19

bb19:                                             ; preds = %bb19, %bb.nph8
  %indvar39 = phi i64 [ 0, %bb.nph8 ], [ %indvar.next40, %bb19 ] ; <i64> [#uses=3]
  %scevgep44 = getelementptr [512 x [512 x double]]* @D, i64 0, i64 %indvar42, i64 %indvar39 ; <double*> [#uses=1]
  %j.37 = trunc i64 %indvar39 to i32              ; <i32> [#uses=1]
  %15 = sitofp i32 %j.37 to double                ; <double> [#uses=1]
  %16 = fmul double %14, %15                      ; <double> [#uses=1]
  %17 = fadd double %16, 2.000000e+00             ; <double> [#uses=1]
  %18 = fdiv double %17, 5.120000e+02             ; <double> [#uses=1]
  store double %18, double* %scevgep44, align 8
  %indvar.next40 = add i64 %indvar39, 1           ; <i64> [#uses=2]
  %exitcond41 = icmp eq i64 %indvar.next40, 512   ; <i1> [#uses=1]
  br i1 %exitcond41, label %bb21, label %bb19

bb21:                                             ; preds = %bb19
  %indvar.next43 = add i64 %indvar42, 1           ; <i64> [#uses=2]
  %exitcond45 = icmp eq i64 %indvar.next43, 512   ; <i1> [#uses=1]
  br i1 %exitcond45, label %bb.nph, label %bb.nph8

bb.nph:                                           ; preds = %bb27, %bb21
  %indvar35 = phi i64 [ %indvar.next36, %bb27 ], [ 0, %bb21 ] ; <i64> [#uses=3]
  %i.42 = trunc i64 %indvar35 to i32              ; <i32> [#uses=1]
  %19 = sitofp i32 %i.42 to double                ; <double> [#uses=1]
  br label %bb25

bb25:                                             ; preds = %bb25, %bb.nph
  %indvar = phi i64 [ 0, %bb.nph ], [ %indvar.next, %bb25 ] ; <i64> [#uses=3]
  %scevgep = getelementptr [512 x [512 x double]]* @E, i64 0, i64 %indvar35, i64 %indvar ; <double*> [#uses=1]
  %j.41 = trunc i64 %indvar to i32                ; <i32> [#uses=1]
  %20 = sitofp i32 %j.41 to double                ; <double> [#uses=1]
  %21 = fmul double %19, %20                      ; <double> [#uses=1]
  %22 = fadd double %21, 2.000000e+00             ; <double> [#uses=1]
  %23 = fdiv double %22, 5.120000e+02             ; <double> [#uses=1]
  store double %23, double* %scevgep, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 512       ; <i1> [#uses=1]
  br i1 %exitcond, label %bb27, label %bb25

bb27:                                             ; preds = %bb25
  %indvar.next36 = add i64 %indvar35, 1           ; <i64> [#uses=2]
  %exitcond37 = icmp eq i64 %indvar.next36, 512   ; <i1> [#uses=1]
  br i1 %exitcond37, label %return, label %bb.nph

return:                                           ; preds = %bb27
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
  br i1 %3, label %bb.nph.us, label %return

bb7.us:                                           ; preds = %bb5.us
  %4 = load %struct._IO_FILE** @stderr, align 8   ; <%struct._IO_FILE*> [#uses=1]
  %5 = bitcast %struct._IO_FILE* %4 to i8*        ; <i8*> [#uses=1]
  %6 = tail call i32 @fputc(i32 10, i8* %5) nounwind ; <i32> [#uses=0]
  %indvar.next8 = add i64 %indvar7, 1             ; <i64> [#uses=2]
  %exitcond11 = icmp eq i64 %indvar.next8, 512    ; <i1> [#uses=1]
  br i1 %exitcond11, label %return, label %bb.nph.us

bb5.us:                                           ; preds = %bb4.us, %bb3.us
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 512       ; <i1> [#uses=1]
  br i1 %exitcond, label %bb7.us, label %bb3.us

bb3.us:                                           ; preds = %bb.nph.us, %bb5.us
  %indvar = phi i64 [ 0, %bb.nph.us ], [ %indvar.next, %bb5.us ] ; <i64> [#uses=3]
  %scevgep = getelementptr [512 x [512 x double]]* @E, i64 0, i64 %indvar7, i64 %indvar ; <double*> [#uses=1]
  %tmp14 = add i64 %tmp13, %indvar                ; <i64> [#uses=1]
  %tmp10 = trunc i64 %tmp14 to i32                ; <i32> [#uses=1]
  %7 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %8 = load %struct._IO_FILE** @stderr, align 8   ; <%struct._IO_FILE*> [#uses=1]
  %9 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %8, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %7) nounwind ; <i32> [#uses=0]
  %10 = srem i32 %tmp10, 80                       ; <i32> [#uses=1]
  %11 = icmp eq i32 %10, 20                       ; <i1> [#uses=1]
  br i1 %11, label %bb4.us, label %bb5.us

bb4.us:                                           ; preds = %bb3.us
  %12 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %13 = bitcast %struct._IO_FILE* %12 to i8*      ; <i8*> [#uses=1]
  %14 = tail call i32 @fputc(i32 10, i8* %13) nounwind ; <i32> [#uses=0]
  br label %bb5.us

bb.nph.us:                                        ; preds = %bb7.us, %bb
  %indvar7 = phi i64 [ %indvar.next8, %bb7.us ], [ 0, %bb ] ; <i64> [#uses=3]
  %tmp13 = shl i64 %indvar7, 9                    ; <i64> [#uses=1]
  br label %bb3.us

return:                                           ; preds = %bb7.us, %bb, %entry
  ret void
}

declare i32 @fprintf(%struct._IO_FILE* noalias nocapture, i8* noalias nocapture, ...) nounwind

declare i32 @fputc(i32, i8* nocapture) nounwind

define void @scop_func() nounwind {
bb.nph44.bb.nph44.split_crit_edge:
  br label %bb.nph30.split.us

bb.nph30.split.us:                                ; preds = %bb6, %bb.nph44.bb.nph44.split_crit_edge
  %i.031 = phi i32 [ 0, %bb.nph44.bb.nph44.split_crit_edge ], [ %4, %bb6 ] ; <i32> [#uses=2]
  %tmp63 = sext i32 %i.031 to i64                 ; <i64> [#uses=2]
  br label %bb.nph22.us

bb4.us:                                           ; preds = %bb2.us
  store double %3, double* %scevgep68
  %indvar.next61 = add i64 %indvar60, 1           ; <i64> [#uses=2]
  %exitcond65 = icmp eq i64 %indvar.next61, 512   ; <i1> [#uses=1]
  br i1 %exitcond65, label %bb6, label %bb.nph22.us

bb2.us:                                           ; preds = %bb.nph22.us, %bb2.us
  %indvar57 = phi i64 [ 0, %bb.nph22.us ], [ %indvar.next58, %bb2.us ] ; <i64> [#uses=3]
  %.tmp.024.us = phi double [ 0.000000e+00, %bb.nph22.us ], [ %3, %bb2.us ] ; <double> [#uses=1]
  %scevgep62 = getelementptr [512 x [512 x double]]* @B, i64 0, i64 %indvar57, i64 %indvar60 ; <double*> [#uses=1]
  %scevgep64 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %tmp63, i64 %indvar57 ; <double*> [#uses=1]
  %0 = load double* %scevgep64, align 8           ; <double> [#uses=1]
  %1 = load double* %scevgep62, align 8           ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=1]
  %3 = fadd double %.tmp.024.us, %2               ; <double> [#uses=2]
  %indvar.next58 = add i64 %indvar57, 1           ; <i64> [#uses=2]
  %exitcond59 = icmp eq i64 %indvar.next58, 512   ; <i1> [#uses=1]
  br i1 %exitcond59, label %bb4.us, label %bb2.us

bb.nph22.us:                                      ; preds = %bb4.us, %bb.nph30.split.us
  %indvar60 = phi i64 [ %indvar.next61, %bb4.us ], [ 0, %bb.nph30.split.us ] ; <i64> [#uses=3]
  %scevgep68 = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %tmp63, i64 %indvar60 ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep68, align 8
  br label %bb2.us

bb6:                                              ; preds = %bb4.us
  %4 = add nsw i32 %i.031, 1                      ; <i32> [#uses=2]
  %5 = icmp slt i32 %4, 512                       ; <i1> [#uses=1]
  br i1 %5, label %bb.nph30.split.us, label %bb.nph6.split.us

bb.nph6.split.us:                                 ; preds = %bb15, %bb6
  %i.17 = phi i32 [ %10, %bb15 ], [ 0, %bb6 ]     ; <i32> [#uses=2]
  %tmp = sext i32 %i.17 to i64                    ; <i64> [#uses=2]
  br label %bb.nph.us

bb13.us:                                          ; preds = %bb11.us
  store double %9, double* %scevgep51
  %indvar.next46 = add i64 %indvar45, 1           ; <i64> [#uses=2]
  %exitcond48 = icmp eq i64 %indvar.next46, 512   ; <i1> [#uses=1]
  br i1 %exitcond48, label %bb15, label %bb.nph.us

bb11.us:                                          ; preds = %bb.nph.us, %bb11.us
  %indvar = phi i64 [ 0, %bb.nph.us ], [ %indvar.next, %bb11.us ] ; <i64> [#uses=3]
  %.tmp.0.us = phi double [ 0.000000e+00, %bb.nph.us ], [ %9, %bb11.us ] ; <double> [#uses=1]
  %scevgep = getelementptr [512 x [512 x double]]* @D, i64 0, i64 %indvar, i64 %indvar45 ; <double*> [#uses=1]
  %scevgep47 = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %tmp, i64 %indvar ; <double*> [#uses=1]
  %6 = load double* %scevgep47, align 8           ; <double> [#uses=1]
  %7 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %8 = fmul double %6, %7                         ; <double> [#uses=1]
  %9 = fadd double %.tmp.0.us, %8                 ; <double> [#uses=2]
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 512       ; <i1> [#uses=1]
  br i1 %exitcond, label %bb13.us, label %bb11.us

bb.nph.us:                                        ; preds = %bb13.us, %bb.nph6.split.us
  %indvar45 = phi i64 [ %indvar.next46, %bb13.us ], [ 0, %bb.nph6.split.us ] ; <i64> [#uses=3]
  %scevgep51 = getelementptr [512 x [512 x double]]* @E, i64 0, i64 %tmp, i64 %indvar45 ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep51, align 8
  br label %bb11.us

bb15:                                             ; preds = %bb13.us
  %10 = add nsw i32 %i.17, 1                      ; <i32> [#uses=2]
  %11 = icmp slt i32 %10, 512                     ; <i1> [#uses=1]
  br i1 %11, label %bb.nph6.split.us, label %return

return:                                           ; preds = %bb15
  ret void
}
; CHECK:

define i32 @main(i32 %argc, i8** nocapture %argv) nounwind {
entry:
  tail call void @init_array() nounwind
  br label %bb.nph30.split.us.i

bb.nph30.split.us.i:                              ; preds = %bb6.i, %entry
  %indvar19 = phi i64 [ %indvar.next20, %bb6.i ], [ 0, %entry ] ; <i64> [#uses=3]
  br label %bb.nph22.us.i

bb4.us.i:                                         ; preds = %bb2.us.i
  store double %3, double* %scevgep68.i
  %indvar.next61.i = add i64 %indvar60.i, 1       ; <i64> [#uses=2]
  %exitcond21 = icmp eq i64 %indvar.next61.i, 512 ; <i1> [#uses=1]
  br i1 %exitcond21, label %bb6.i, label %bb.nph22.us.i

bb2.us.i:                                         ; preds = %bb.nph22.us.i, %bb2.us.i
  %indvar57.i = phi i64 [ 0, %bb.nph22.us.i ], [ %indvar.next58.i, %bb2.us.i ] ; <i64> [#uses=3]
  %.tmp.024.us.i = phi double [ 0.000000e+00, %bb.nph22.us.i ], [ %3, %bb2.us.i ] ; <double> [#uses=1]
  %scevgep64.i = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %indvar19, i64 %indvar57.i ; <double*> [#uses=1]
  %scevgep62.i = getelementptr [512 x [512 x double]]* @B, i64 0, i64 %indvar57.i, i64 %indvar60.i ; <double*> [#uses=1]
  %0 = load double* %scevgep64.i, align 8         ; <double> [#uses=1]
  %1 = load double* %scevgep62.i, align 8         ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=1]
  %3 = fadd double %.tmp.024.us.i, %2             ; <double> [#uses=2]
  %indvar.next58.i = add i64 %indvar57.i, 1       ; <i64> [#uses=2]
  %exitcond18 = icmp eq i64 %indvar.next58.i, 512 ; <i1> [#uses=1]
  br i1 %exitcond18, label %bb4.us.i, label %bb2.us.i

bb.nph22.us.i:                                    ; preds = %bb4.us.i, %bb.nph30.split.us.i
  %indvar60.i = phi i64 [ %indvar.next61.i, %bb4.us.i ], [ 0, %bb.nph30.split.us.i ] ; <i64> [#uses=3]
  %scevgep68.i = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %indvar19, i64 %indvar60.i ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep68.i, align 8
  br label %bb2.us.i

bb6.i:                                            ; preds = %bb4.us.i
  %indvar.next20 = add i64 %indvar19, 1           ; <i64> [#uses=2]
  %exitcond22 = icmp eq i64 %indvar.next20, 512   ; <i1> [#uses=1]
  br i1 %exitcond22, label %bb.nph6.split.us.i, label %bb.nph30.split.us.i

bb.nph6.split.us.i:                               ; preds = %bb15.i, %bb6.i
  %indvar = phi i64 [ %indvar.next, %bb15.i ], [ 0, %bb6.i ] ; <i64> [#uses=3]
  br label %bb.nph.us.i

bb13.us.i:                                        ; preds = %bb11.us.i
  store double %7, double* %scevgep51.i
  %indvar.next46.i = add i64 %indvar45.i, 1       ; <i64> [#uses=2]
  %exitcond15 = icmp eq i64 %indvar.next46.i, 512 ; <i1> [#uses=1]
  br i1 %exitcond15, label %bb15.i, label %bb.nph.us.i

bb11.us.i:                                        ; preds = %bb.nph.us.i, %bb11.us.i
  %indvar.i = phi i64 [ 0, %bb.nph.us.i ], [ %indvar.next.i, %bb11.us.i ] ; <i64> [#uses=3]
  %.tmp.0.us.i = phi double [ 0.000000e+00, %bb.nph.us.i ], [ %7, %bb11.us.i ] ; <double> [#uses=1]
  %scevgep47.i = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %indvar, i64 %indvar.i ; <double*> [#uses=1]
  %scevgep.i = getelementptr [512 x [512 x double]]* @D, i64 0, i64 %indvar.i, i64 %indvar45.i ; <double*> [#uses=1]
  %4 = load double* %scevgep47.i, align 8         ; <double> [#uses=1]
  %5 = load double* %scevgep.i, align 8           ; <double> [#uses=1]
  %6 = fmul double %4, %5                         ; <double> [#uses=1]
  %7 = fadd double %.tmp.0.us.i, %6               ; <double> [#uses=2]
  %indvar.next.i = add i64 %indvar.i, 1           ; <i64> [#uses=2]
  %exitcond14 = icmp eq i64 %indvar.next.i, 512   ; <i1> [#uses=1]
  br i1 %exitcond14, label %bb13.us.i, label %bb11.us.i

bb.nph.us.i:                                      ; preds = %bb13.us.i, %bb.nph6.split.us.i
  %indvar45.i = phi i64 [ %indvar.next46.i, %bb13.us.i ], [ 0, %bb.nph6.split.us.i ] ; <i64> [#uses=3]
  %scevgep51.i = getelementptr [512 x [512 x double]]* @E, i64 0, i64 %indvar, i64 %indvar45.i ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep51.i, align 8
  br label %bb11.us.i

bb15.i:                                           ; preds = %bb13.us.i
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond16 = icmp eq i64 %indvar.next, 512     ; <i1> [#uses=1]
  br i1 %exitcond16, label %scop_func.exit, label %bb.nph6.split.us.i

scop_func.exit:                                   ; preds = %bb15.i
  %8 = icmp sgt i32 %argc, 42                     ; <i1> [#uses=1]
  br i1 %8, label %bb.i, label %print_array.exit

bb.i:                                             ; preds = %scop_func.exit
  %9 = load i8** %argv, align 1                   ; <i8*> [#uses=1]
  %10 = load i8* %9, align 1                      ; <i8> [#uses=1]
  %11 = icmp eq i8 %10, 0                         ; <i1> [#uses=1]
  br i1 %11, label %bb.nph.us.i6, label %print_array.exit

bb7.us.i:                                         ; preds = %bb5.us.i
  %12 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %13 = bitcast %struct._IO_FILE* %12 to i8*      ; <i8*> [#uses=1]
  %14 = tail call i32 @fputc(i32 10, i8* %13) nounwind ; <i32> [#uses=0]
  %indvar.next8.i = add i64 %indvar7.i, 1         ; <i64> [#uses=2]
  %exitcond10 = icmp eq i64 %indvar.next8.i, 512  ; <i1> [#uses=1]
  br i1 %exitcond10, label %print_array.exit, label %bb.nph.us.i6

bb5.us.i:                                         ; preds = %bb4.us.i5, %bb3.us.i
  %indvar.next.i1 = add i64 %indvar.i3, 1         ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next.i1, 512    ; <i1> [#uses=1]
  br i1 %exitcond, label %bb7.us.i, label %bb3.us.i

bb3.us.i:                                         ; preds = %bb.nph.us.i6, %bb5.us.i
  %indvar.i3 = phi i64 [ 0, %bb.nph.us.i6 ], [ %indvar.next.i1, %bb5.us.i ] ; <i64> [#uses=3]
  %scevgep.i4 = getelementptr [512 x [512 x double]]* @E, i64 0, i64 %indvar7.i, i64 %indvar.i3 ; <double*> [#uses=1]
  %tmp12 = add i64 %tmp11, %indvar.i3             ; <i64> [#uses=1]
  %tmp10.i = trunc i64 %tmp12 to i32              ; <i32> [#uses=1]
  %15 = load double* %scevgep.i4, align 8         ; <double> [#uses=1]
  %16 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %17 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %16, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %15) nounwind ; <i32> [#uses=0]
  %18 = srem i32 %tmp10.i, 80                     ; <i32> [#uses=1]
  %19 = icmp eq i32 %18, 20                       ; <i1> [#uses=1]
  br i1 %19, label %bb4.us.i5, label %bb5.us.i

bb4.us.i5:                                        ; preds = %bb3.us.i
  %20 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %21 = bitcast %struct._IO_FILE* %20 to i8*      ; <i8*> [#uses=1]
  %22 = tail call i32 @fputc(i32 10, i8* %21) nounwind ; <i32> [#uses=0]
  br label %bb5.us.i

bb.nph.us.i6:                                     ; preds = %bb7.us.i, %bb.i
  %indvar7.i = phi i64 [ %indvar.next8.i, %bb7.us.i ], [ 0, %bb.i ] ; <i64> [#uses=3]
  %tmp11 = shl i64 %indvar7.i, 9                  ; <i64> [#uses=1]
  br label %bb3.us.i

print_array.exit:                                 ; preds = %bb7.us.i, %bb.i, %scop_func.exit
  ret i32 0
}
