; RUN: opt  -indvars -polly-scop-detect -print-top-scop-only -polly-print-scop -analyze  %s | FileCheck %s
; XFAIL: *
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }

@alpha = common global double 0.000000e+00        ; <double*> [#uses=3]
@beta = common global double 0.000000e+00         ; <double*> [#uses=3]
@A = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=4]
@B = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=4]
@C = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=6]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]

define void @init_array() nounwind inlinehint {
bb.nph20.bb.nph20.split_crit_edge:
  store double 3.241200e+04, double* @alpha, align 8
  store double 2.123000e+03, double* @beta, align 8
  br label %bb.nph15

bb.nph15:                                         ; preds = %bb3, %bb.nph20.bb.nph20.split_crit_edge
  %indvar36 = phi i64 [ 0, %bb.nph20.bb.nph20.split_crit_edge ], [ %indvar.next37, %bb3 ] ; <i64> [#uses=3]
  %i.016 = trunc i64 %indvar36 to i32             ; <i32> [#uses=1]
  %0 = sitofp i32 %i.016 to double                ; <double> [#uses=1]
  br label %bb1

bb1:                                              ; preds = %bb1, %bb.nph15
  %indvar33 = phi i64 [ 0, %bb.nph15 ], [ %indvar.next34, %bb1 ] ; <i64> [#uses=3]
  %scevgep38 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %indvar36, i64 %indvar33 ; <double*> [#uses=1]
  %j.014 = trunc i64 %indvar33 to i32             ; <i32> [#uses=1]
  %1 = sitofp i32 %j.014 to double                ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=1]
  %3 = fdiv double %2, 5.120000e+02               ; <double> [#uses=1]
  store double %3, double* %scevgep38, align 8
  %indvar.next34 = add i64 %indvar33, 1           ; <i64> [#uses=2]
  %exitcond35 = icmp eq i64 %indvar.next34, 512   ; <i1> [#uses=1]
  br i1 %exitcond35, label %bb3, label %bb1

bb3:                                              ; preds = %bb1
  %indvar.next37 = add i64 %indvar36, 1           ; <i64> [#uses=2]
  %exitcond39 = icmp eq i64 %indvar.next37, 512   ; <i1> [#uses=1]
  br i1 %exitcond39, label %bb.nph8, label %bb.nph15

bb.nph8:                                          ; preds = %bb9, %bb3
  %indvar28 = phi i64 [ %indvar.next29, %bb9 ], [ 0, %bb3 ] ; <i64> [#uses=3]
  %i.19 = trunc i64 %indvar28 to i32              ; <i32> [#uses=1]
  %4 = sitofp i32 %i.19 to double                 ; <double> [#uses=1]
  br label %bb7

bb7:                                              ; preds = %bb7, %bb.nph8
  %indvar25 = phi i64 [ 0, %bb.nph8 ], [ %indvar.next26, %bb7 ] ; <i64> [#uses=3]
  %scevgep30 = getelementptr [512 x [512 x double]]* @B, i64 0, i64 %indvar28, i64 %indvar25 ; <double*> [#uses=1]
  %j.17 = trunc i64 %indvar25 to i32              ; <i32> [#uses=1]
  %5 = sitofp i32 %j.17 to double                 ; <double> [#uses=1]
  %6 = fmul double %4, %5                         ; <double> [#uses=1]
  %7 = fadd double %6, 1.000000e+00               ; <double> [#uses=1]
  %8 = fdiv double %7, 5.120000e+02               ; <double> [#uses=1]
  store double %8, double* %scevgep30, align 8
  %indvar.next26 = add i64 %indvar25, 1           ; <i64> [#uses=2]
  %exitcond27 = icmp eq i64 %indvar.next26, 512   ; <i1> [#uses=1]
  br i1 %exitcond27, label %bb9, label %bb7

bb9:                                              ; preds = %bb7
  %indvar.next29 = add i64 %indvar28, 1           ; <i64> [#uses=2]
  %exitcond31 = icmp eq i64 %indvar.next29, 512   ; <i1> [#uses=1]
  br i1 %exitcond31, label %bb.nph, label %bb.nph8

bb.nph:                                           ; preds = %bb15, %bb9
  %indvar21 = phi i64 [ %indvar.next22, %bb15 ], [ 0, %bb9 ] ; <i64> [#uses=3]
  %i.22 = trunc i64 %indvar21 to i32              ; <i32> [#uses=1]
  %9 = sitofp i32 %i.22 to double                 ; <double> [#uses=1]
  br label %bb13

bb13:                                             ; preds = %bb13, %bb.nph
  %indvar = phi i64 [ 0, %bb.nph ], [ %indvar.next, %bb13 ] ; <i64> [#uses=3]
  %scevgep = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %indvar21, i64 %indvar ; <double*> [#uses=1]
  %j.21 = trunc i64 %indvar to i32                ; <i32> [#uses=1]
  %10 = sitofp i32 %j.21 to double                ; <double> [#uses=1]
  %11 = fmul double %9, %10                       ; <double> [#uses=1]
  %12 = fadd double %11, 2.000000e+00             ; <double> [#uses=1]
  %13 = fdiv double %12, 5.120000e+02             ; <double> [#uses=1]
  store double %13, double* %scevgep, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 512       ; <i1> [#uses=1]
  br i1 %exitcond, label %bb15, label %bb13

bb15:                                             ; preds = %bb13
  %indvar.next22 = add i64 %indvar21, 1           ; <i64> [#uses=2]
  %exitcond23 = icmp eq i64 %indvar.next22, 512   ; <i1> [#uses=1]
  br i1 %exitcond23, label %return, label %bb.nph

return:                                           ; preds = %bb15
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
  %scevgep = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %indvar7, i64 %indvar ; <double*> [#uses=1]
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
bb.nph20.bb.nph20.split_crit_edge:
  %0 = load double* @beta, align 8                ; <double> [#uses=1]
  %1 = load double* @alpha, align 8               ; <double> [#uses=1]
  br label %bb.nph6.split.us

bb.nph6.split.us:                                 ; preds = %bb6, %bb.nph20.bb.nph20.split_crit_edge
  %i.07 = phi i32 [ 0, %bb.nph20.bb.nph20.split_crit_edge ], [ %9, %bb6 ] ; <i32> [#uses=2]
  %tmp = sext i32 %i.07 to i64                    ; <i64> [#uses=2]
  br label %bb.nph.us

bb4.us:                                           ; preds = %bb2.us
  store double %6, double* %scevgep27
  %indvar.next22 = add i64 %indvar21, 1           ; <i64> [#uses=2]
  %exitcond24 = icmp eq i64 %indvar.next22, 512   ; <i1> [#uses=1]
  br i1 %exitcond24, label %bb6, label %bb.nph.us

bb2.us:                                           ; preds = %bb.nph.us, %bb2.us
  %indvar = phi i64 [ 0, %bb.nph.us ], [ %indvar.next, %bb2.us ] ; <i64> [#uses=3]
  %.tmp.0.us = phi double [ %8, %bb.nph.us ], [ %6, %bb2.us ] ; <double> [#uses=1]
  %scevgep = getelementptr [512 x [512 x double]]* @B, i64 0, i64 %indvar, i64 %indvar21 ; <double*> [#uses=1]
  %scevgep23 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %tmp, i64 %indvar ; <double*> [#uses=1]
  %2 = load double* %scevgep23, align 8           ; <double> [#uses=1]
  %3 = fmul double %2, %1                         ; <double> [#uses=1]
  %4 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %5 = fmul double %3, %4                         ; <double> [#uses=1]
  %6 = fadd double %.tmp.0.us, %5                 ; <double> [#uses=2]
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 512       ; <i1> [#uses=1]
  br i1 %exitcond, label %bb4.us, label %bb2.us

bb.nph.us:                                        ; preds = %bb4.us, %bb.nph6.split.us
  %indvar21 = phi i64 [ %indvar.next22, %bb4.us ], [ 0, %bb.nph6.split.us ] ; <i64> [#uses=3]
  %scevgep27 = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %tmp, i64 %indvar21 ; <double*> [#uses=3]
  %7 = load double* %scevgep27, align 8           ; <double> [#uses=1]
  %8 = fmul double %7, %0                         ; <double> [#uses=2]
  store double %8, double* %scevgep27, align 8
  br label %bb2.us

bb6:                                              ; preds = %bb4.us
  %9 = add nsw i32 %i.07, 1                       ; <i32> [#uses=2]
  %10 = icmp slt i32 %9, 512                      ; <i1> [#uses=1]
  br i1 %10, label %bb.nph6.split.us, label %return

return:                                           ; preds = %bb6
  ret void
}

define i32 @main(i32 %argc, i8** nocapture %argv) nounwind {
entry:
  store double 3.241200e+04, double* @alpha, align 8
  store double 2.123000e+03, double* @beta, align 8
  br label %bb.nph15.i

bb.nph15.i:                                       ; preds = %bb3.i, %entry
  %indvar36.i = phi i64 [ 0, %entry ], [ %indvar.next37.i, %bb3.i ] ; <i64> [#uses=3]
  %i.016.i = trunc i64 %indvar36.i to i32         ; <i32> [#uses=1]
  %0 = sitofp i32 %i.016.i to double              ; <double> [#uses=1]
  br label %bb1.i

bb1.i:                                            ; preds = %bb1.i, %bb.nph15.i
  %indvar33.i = phi i64 [ 0, %bb.nph15.i ], [ %indvar.next34.i, %bb1.i ] ; <i64> [#uses=3]
  %scevgep38.i = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %indvar36.i, i64 %indvar33.i ; <double*> [#uses=1]
  %j.014.i = trunc i64 %indvar33.i to i32         ; <i32> [#uses=1]
  %1 = sitofp i32 %j.014.i to double              ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=1]
  %3 = fdiv double %2, 5.120000e+02               ; <double> [#uses=1]
  store double %3, double* %scevgep38.i, align 8
  %indvar.next34.i = add i64 %indvar33.i, 1       ; <i64> [#uses=2]
  %exitcond26 = icmp eq i64 %indvar.next34.i, 512 ; <i1> [#uses=1]
  br i1 %exitcond26, label %bb3.i, label %bb1.i

bb3.i:                                            ; preds = %bb1.i
  %indvar.next37.i = add i64 %indvar36.i, 1       ; <i64> [#uses=2]
  %exitcond27 = icmp eq i64 %indvar.next37.i, 512 ; <i1> [#uses=1]
  br i1 %exitcond27, label %bb.nph8.i, label %bb.nph15.i

bb.nph8.i:                                        ; preds = %bb9.i, %bb3.i
  %indvar28.i = phi i64 [ %indvar.next29.i, %bb9.i ], [ 0, %bb3.i ] ; <i64> [#uses=3]
  %i.19.i = trunc i64 %indvar28.i to i32          ; <i32> [#uses=1]
  %4 = sitofp i32 %i.19.i to double               ; <double> [#uses=1]
  br label %bb7.i

bb7.i:                                            ; preds = %bb7.i, %bb.nph8.i
  %indvar25.i = phi i64 [ 0, %bb.nph8.i ], [ %indvar.next26.i, %bb7.i ] ; <i64> [#uses=3]
  %scevgep30.i = getelementptr [512 x [512 x double]]* @B, i64 0, i64 %indvar28.i, i64 %indvar25.i ; <double*> [#uses=1]
  %j.17.i = trunc i64 %indvar25.i to i32          ; <i32> [#uses=1]
  %5 = sitofp i32 %j.17.i to double               ; <double> [#uses=1]
  %6 = fmul double %4, %5                         ; <double> [#uses=1]
  %7 = fadd double %6, 1.000000e+00               ; <double> [#uses=1]
  %8 = fdiv double %7, 5.120000e+02               ; <double> [#uses=1]
  store double %8, double* %scevgep30.i, align 8
  %indvar.next26.i = add i64 %indvar25.i, 1       ; <i64> [#uses=2]
  %exitcond24 = icmp eq i64 %indvar.next26.i, 512 ; <i1> [#uses=1]
  br i1 %exitcond24, label %bb9.i, label %bb7.i

bb9.i:                                            ; preds = %bb7.i
  %indvar.next29.i = add i64 %indvar28.i, 1       ; <i64> [#uses=2]
  %exitcond25 = icmp eq i64 %indvar.next29.i, 512 ; <i1> [#uses=1]
  br i1 %exitcond25, label %bb.nph.i, label %bb.nph8.i

bb.nph.i:                                         ; preds = %bb15.i, %bb9.i
  %indvar21.i = phi i64 [ %indvar.next22.i, %bb15.i ], [ 0, %bb9.i ] ; <i64> [#uses=3]
  %i.22.i = trunc i64 %indvar21.i to i32          ; <i32> [#uses=1]
  %9 = sitofp i32 %i.22.i to double               ; <double> [#uses=1]
  br label %bb13.i

bb13.i:                                           ; preds = %bb13.i, %bb.nph.i
  %indvar.i = phi i64 [ 0, %bb.nph.i ], [ %indvar.next.i, %bb13.i ] ; <i64> [#uses=3]
  %scevgep.i = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %indvar21.i, i64 %indvar.i ; <double*> [#uses=1]
  %j.21.i = trunc i64 %indvar.i to i32            ; <i32> [#uses=1]
  %10 = sitofp i32 %j.21.i to double              ; <double> [#uses=1]
  %11 = fmul double %9, %10                       ; <double> [#uses=1]
  %12 = fadd double %11, 2.000000e+00             ; <double> [#uses=1]
  %13 = fdiv double %12, 5.120000e+02             ; <double> [#uses=1]
  store double %13, double* %scevgep.i, align 8
  %indvar.next.i = add i64 %indvar.i, 1           ; <i64> [#uses=2]
  %exitcond22 = icmp eq i64 %indvar.next.i, 512   ; <i1> [#uses=1]
  br i1 %exitcond22, label %bb15.i, label %bb13.i

bb15.i:                                           ; preds = %bb13.i
  %indvar.next22.i = add i64 %indvar21.i, 1       ; <i64> [#uses=2]
  %exitcond23 = icmp eq i64 %indvar.next22.i, 512 ; <i1> [#uses=1]
  br i1 %exitcond23, label %bb.nph6.split.us.i, label %bb.nph.i

bb.nph6.split.us.i:                               ; preds = %bb6.i, %bb15.i
  %indvar = phi i64 [ %indvar.next, %bb6.i ], [ 0, %bb15.i ] ; <i64> [#uses=3]
  br label %bb.nph.us.i12

bb4.us.i6:                                        ; preds = %bb2.us.i
  store double %18, double* %scevgep27.i
  %indvar.next22.i5 = add i64 %indvar21.i11, 1    ; <i64> [#uses=2]
  %exitcond20 = icmp eq i64 %indvar.next22.i5, 512 ; <i1> [#uses=1]
  br i1 %exitcond20, label %bb6.i, label %bb.nph.us.i12

bb2.us.i:                                         ; preds = %bb.nph.us.i12, %bb2.us.i
  %indvar.i7 = phi i64 [ 0, %bb.nph.us.i12 ], [ %indvar.next.i9, %bb2.us.i ] ; <i64> [#uses=3]
  %.tmp.0.us.i = phi double [ %20, %bb.nph.us.i12 ], [ %18, %bb2.us.i ] ; <double> [#uses=1]
  %scevgep23.i = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %indvar, i64 %indvar.i7 ; <double*> [#uses=1]
  %scevgep.i8 = getelementptr [512 x [512 x double]]* @B, i64 0, i64 %indvar.i7, i64 %indvar21.i11 ; <double*> [#uses=1]
  %14 = load double* %scevgep23.i, align 8        ; <double> [#uses=1]
  %15 = fmul double %14, 3.241200e+04             ; <double> [#uses=1]
  %16 = load double* %scevgep.i8, align 8         ; <double> [#uses=1]
  %17 = fmul double %15, %16                      ; <double> [#uses=1]
  %18 = fadd double %.tmp.0.us.i, %17             ; <double> [#uses=2]
  %indvar.next.i9 = add i64 %indvar.i7, 1         ; <i64> [#uses=2]
  %exitcond19 = icmp eq i64 %indvar.next.i9, 512  ; <i1> [#uses=1]
  br i1 %exitcond19, label %bb4.us.i6, label %bb2.us.i

bb.nph.us.i12:                                    ; preds = %bb4.us.i6, %bb.nph6.split.us.i
  %indvar21.i11 = phi i64 [ %indvar.next22.i5, %bb4.us.i6 ], [ 0, %bb.nph6.split.us.i ] ; <i64> [#uses=3]
  %scevgep27.i = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %indvar, i64 %indvar21.i11 ; <double*> [#uses=3]
  %19 = load double* %scevgep27.i, align 8        ; <double> [#uses=1]
  %20 = fmul double %19, 2.123000e+03             ; <double> [#uses=2]
  store double %20, double* %scevgep27.i, align 8
  br label %bb2.us.i

bb6.i:                                            ; preds = %bb4.us.i6
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond21 = icmp eq i64 %indvar.next, 512     ; <i1> [#uses=1]
  br i1 %exitcond21, label %scop_func.exit, label %bb.nph6.split.us.i

scop_func.exit:                                   ; preds = %bb6.i
  %21 = icmp sgt i32 %argc, 42                    ; <i1> [#uses=1]
  br i1 %21, label %bb.i, label %print_array.exit

bb.i:                                             ; preds = %scop_func.exit
  %22 = load i8** %argv, align 1                  ; <i8*> [#uses=1]
  %23 = load i8* %22, align 1                     ; <i8> [#uses=1]
  %24 = icmp eq i8 %23, 0                         ; <i1> [#uses=1]
  br i1 %24, label %bb.nph.us.i, label %print_array.exit

bb7.us.i:                                         ; preds = %bb5.us.i
  %25 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %26 = bitcast %struct._IO_FILE* %25 to i8*      ; <i8*> [#uses=1]
  %27 = tail call i32 @fputc(i32 10, i8* %26) nounwind ; <i32> [#uses=0]
  %indvar.next8.i = add i64 %indvar7.i, 1         ; <i64> [#uses=2]
  %exitcond15 = icmp eq i64 %indvar.next8.i, 512  ; <i1> [#uses=1]
  br i1 %exitcond15, label %print_array.exit, label %bb.nph.us.i

bb5.us.i:                                         ; preds = %bb4.us.i, %bb3.us.i
  %indvar.next.i1 = add i64 %indvar.i3, 1         ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next.i1, 512    ; <i1> [#uses=1]
  br i1 %exitcond, label %bb7.us.i, label %bb3.us.i

bb3.us.i:                                         ; preds = %bb.nph.us.i, %bb5.us.i
  %indvar.i3 = phi i64 [ 0, %bb.nph.us.i ], [ %indvar.next.i1, %bb5.us.i ] ; <i64> [#uses=3]
  %scevgep.i4 = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %indvar7.i, i64 %indvar.i3 ; <double*> [#uses=1]
  %tmp17 = add i64 %tmp16, %indvar.i3             ; <i64> [#uses=1]
  %tmp10.i = trunc i64 %tmp17 to i32              ; <i32> [#uses=1]
  %28 = load double* %scevgep.i4, align 8         ; <double> [#uses=1]
  %29 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %30 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %29, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %28) nounwind ; <i32> [#uses=0]
  %31 = srem i32 %tmp10.i, 80                     ; <i32> [#uses=1]
  %32 = icmp eq i32 %31, 20                       ; <i1> [#uses=1]
  br i1 %32, label %bb4.us.i, label %bb5.us.i

bb4.us.i:                                         ; preds = %bb3.us.i
  %33 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %34 = bitcast %struct._IO_FILE* %33 to i8*      ; <i8*> [#uses=1]
  %35 = tail call i32 @fputc(i32 10, i8* %34) nounwind ; <i32> [#uses=0]
  br label %bb5.us.i

bb.nph.us.i:                                      ; preds = %bb7.us.i, %bb.i
  %indvar7.i = phi i64 [ %indvar.next8.i, %bb7.us.i ], [ 0, %bb.i ] ; <i64> [#uses=3]
  %tmp16 = shl i64 %indvar7.i, 9                  ; <i64> [#uses=1]
  br label %bb3.us.i

print_array.exit:                                 ; preds = %bb7.us.i, %bb.i, %scop_func.exit
  ret i32 0
}
