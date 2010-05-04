; RUN: opt -indvars -polly-scop-detect  -analyze %s | FileCheck %s
; XFAIL: *
; ModuleID = '<stdin>'
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
bb.nph25.bb.nph25.split_crit_edge:
  store double 3.241200e+04, double* @alpha, align 8
  store double 2.123000e+03, double* @beta, align 8
  br label %bb.nph20

bb.nph20:                                         ; preds = %bb3, %bb.nph25.bb.nph25.split_crit_edge
  %indvar41 = phi i64 [ 0, %bb.nph25.bb.nph25.split_crit_edge ], [ %indvar.next42, %bb3 ] ; <i64> [#uses=3]
  %storemerge21 = trunc i64 %indvar41 to i32      ; <i32> [#uses=1]
  %0 = sitofp i32 %storemerge21 to double         ; <double> [#uses=1]
  br label %bb1

bb1:                                              ; preds = %bb1, %bb.nph20
  %indvar38 = phi i64 [ 0, %bb.nph20 ], [ %indvar.next39, %bb1 ] ; <i64> [#uses=3]
  %scevgep43 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %indvar41, i64 %indvar38 ; <double*> [#uses=1]
  %storemerge519 = trunc i64 %indvar38 to i32     ; <i32> [#uses=1]
  %1 = sitofp i32 %storemerge519 to double        ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=1]
  %3 = fdiv double %2, 5.120000e+02               ; <double> [#uses=1]
  store double %3, double* %scevgep43, align 8
  %indvar.next39 = add i64 %indvar38, 1           ; <i64> [#uses=2]
  %exitcond40 = icmp eq i64 %indvar.next39, 512   ; <i1> [#uses=1]
  br i1 %exitcond40, label %bb3, label %bb1

bb3:                                              ; preds = %bb1
  %indvar.next42 = add i64 %indvar41, 1           ; <i64> [#uses=2]
  %exitcond44 = icmp eq i64 %indvar.next42, 512   ; <i1> [#uses=1]
  br i1 %exitcond44, label %bb.nph13, label %bb.nph20

bb.nph13:                                         ; preds = %bb9, %bb3
  %indvar33 = phi i64 [ %indvar.next34, %bb9 ], [ 0, %bb3 ] ; <i64> [#uses=3]
  %storemerge114 = trunc i64 %indvar33 to i32     ; <i32> [#uses=1]
  %4 = sitofp i32 %storemerge114 to double        ; <double> [#uses=1]
  br label %bb7

bb7:                                              ; preds = %bb7, %bb.nph13
  %indvar30 = phi i64 [ 0, %bb.nph13 ], [ %indvar.next31, %bb7 ] ; <i64> [#uses=3]
  %scevgep35 = getelementptr [512 x [512 x double]]* @B, i64 0, i64 %indvar33, i64 %indvar30 ; <double*> [#uses=1]
  %storemerge412 = trunc i64 %indvar30 to i32     ; <i32> [#uses=1]
  %5 = sitofp i32 %storemerge412 to double        ; <double> [#uses=1]
  %6 = fmul double %4, %5                         ; <double> [#uses=1]
  %7 = fadd double %6, 1.000000e+00               ; <double> [#uses=1]
  %8 = fdiv double %7, 5.120000e+02               ; <double> [#uses=1]
  store double %8, double* %scevgep35, align 8
  %indvar.next31 = add i64 %indvar30, 1           ; <i64> [#uses=2]
  %exitcond32 = icmp eq i64 %indvar.next31, 512   ; <i1> [#uses=1]
  br i1 %exitcond32, label %bb9, label %bb7

bb9:                                              ; preds = %bb7
  %indvar.next34 = add i64 %indvar33, 1           ; <i64> [#uses=2]
  %exitcond36 = icmp eq i64 %indvar.next34, 512   ; <i1> [#uses=1]
  br i1 %exitcond36, label %bb.nph, label %bb.nph13

bb.nph:                                           ; preds = %bb15, %bb9
  %indvar26 = phi i64 [ %indvar.next27, %bb15 ], [ 0, %bb9 ] ; <i64> [#uses=3]
  %storemerge27 = trunc i64 %indvar26 to i32      ; <i32> [#uses=1]
  %9 = sitofp i32 %storemerge27 to double         ; <double> [#uses=1]
  br label %bb13

bb13:                                             ; preds = %bb13, %bb.nph
  %indvar = phi i64 [ 0, %bb.nph ], [ %indvar.next, %bb13 ] ; <i64> [#uses=3]
  %scevgep = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %indvar26, i64 %indvar ; <double*> [#uses=1]
  %storemerge36 = trunc i64 %indvar to i32        ; <i32> [#uses=1]
  %10 = sitofp i32 %storemerge36 to double        ; <double> [#uses=1]
  %11 = fmul double %9, %10                       ; <double> [#uses=1]
  %12 = fadd double %11, 2.000000e+00             ; <double> [#uses=1]
  %13 = fdiv double %12, 5.120000e+02             ; <double> [#uses=1]
  store double %13, double* %scevgep, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 512       ; <i1> [#uses=1]
  br i1 %exitcond, label %bb15, label %bb13

bb15:                                             ; preds = %bb13
  %indvar.next27 = add i64 %indvar26, 1           ; <i64> [#uses=2]
  %exitcond28 = icmp eq i64 %indvar.next27, 512   ; <i1> [#uses=1]
  br i1 %exitcond28, label %return, label %bb.nph

return:                                           ; preds = %bb15
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
  br i1 %3, label %bb.nph.us, label %return

bb7.us:                                           ; preds = %bb5.us
  %4 = load %struct._IO_FILE** @stderr, align 8   ; <%struct._IO_FILE*> [#uses=1]
  %5 = bitcast %struct._IO_FILE* %4 to i8*        ; <i8*> [#uses=1]
  %6 = tail call i32 @fputc(i32 10, i8* %5) nounwind ; <i32> [#uses=0]
  %indvar.next9 = add i64 %indvar8, 1             ; <i64> [#uses=2]
  %exitcond12 = icmp eq i64 %indvar.next9, 512    ; <i1> [#uses=1]
  br i1 %exitcond12, label %return, label %bb.nph.us

bb5.us:                                           ; preds = %bb4.us, %bb3.us
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 512       ; <i1> [#uses=1]
  br i1 %exitcond, label %bb7.us, label %bb3.us

bb3.us:                                           ; preds = %bb.nph.us, %bb5.us
  %indvar = phi i64 [ 0, %bb.nph.us ], [ %indvar.next, %bb5.us ] ; <i64> [#uses=3]
  %scevgep = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %indvar8, i64 %indvar ; <double*> [#uses=1]
  %tmp15 = add i64 %tmp14, %indvar                ; <i64> [#uses=1]
  %tmp11 = trunc i64 %tmp15 to i32                ; <i32> [#uses=1]
  %7 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %8 = load %struct._IO_FILE** @stderr, align 8   ; <%struct._IO_FILE*> [#uses=1]
  %9 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %8, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %7) nounwind ; <i32> [#uses=0]
  %10 = srem i32 %tmp11, 80                       ; <i32> [#uses=1]
  %11 = icmp eq i32 %10, 20                       ; <i1> [#uses=1]
  br i1 %11, label %bb4.us, label %bb5.us

bb4.us:                                           ; preds = %bb3.us
  %12 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %13 = bitcast %struct._IO_FILE* %12 to i8*      ; <i8*> [#uses=1]
  %14 = tail call i32 @fputc(i32 10, i8* %13) nounwind ; <i32> [#uses=0]
  br label %bb5.us

bb.nph.us:                                        ; preds = %bb7.us, %bb
  %indvar8 = phi i64 [ %indvar.next9, %bb7.us ], [ 0, %bb ] ; <i64> [#uses=3]
  %tmp14 = shl i64 %indvar8, 9                    ; <i64> [#uses=1]
  br label %bb3.us

return:                                           ; preds = %bb7.us, %bb, %entry
  ret void
}

declare i32 @fprintf(%struct._IO_FILE* noalias nocapture, i8* noalias nocapture, ...) nounwind

declare i32 @fputc(i32, i8* nocapture) nounwind

define void @scop_func() nounwind {
bb.nph26.bb.nph26.split_crit_edge:
  %0 = load double* @beta, align 8                ; <double> [#uses=1]
  %1 = load double* @alpha, align 8               ; <double> [#uses=1]
  br label %bb5.preheader

bb4.us:                                           ; preds = %bb2.us
  store double %7, double* %scevgep30
  %2 = add nsw i64 %storemerge14.us, 1            ; <i64> [#uses=2]
  %exitcond28 = icmp eq i64 %2, 512               ; <i1> [#uses=1]
  br i1 %exitcond28, label %bb6, label %bb.nph.us

bb2.us:                                           ; preds = %bb.nph.us, %bb2.us
  %.tmp.0.us = phi double [ %10, %bb.nph.us ], [ %7, %bb2.us ] ; <double> [#uses=1]
  %storemerge23.us = phi i64 [ 0, %bb.nph.us ], [ %8, %bb2.us ] ; <i64> [#uses=3]
  %scevgep = getelementptr [512 x [512 x double]]* @B, i64 0, i64 %storemerge23.us, i64 %storemerge14.us ; <double*> [#uses=1]
  %scevgep27 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %storemerge9, i64 %storemerge23.us ; <double*> [#uses=1]
  %3 = load double* %scevgep27, align 8           ; <double> [#uses=1]
  %4 = fmul double %3, %1                         ; <double> [#uses=1]
  %5 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %6 = fmul double %4, %5                         ; <double> [#uses=1]
  %7 = fadd double %.tmp.0.us, %6                 ; <double> [#uses=2]
  %8 = add nsw i64 %storemerge23.us, 1            ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %8, 512                 ; <i1> [#uses=1]
  br i1 %exitcond, label %bb4.us, label %bb2.us

bb.nph.us:                                        ; preds = %bb5.preheader, %bb4.us
  %storemerge14.us = phi i64 [ %2, %bb4.us ], [ 0, %bb5.preheader ] ; <i64> [#uses=3]
  %scevgep30 = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %storemerge9, i64 %storemerge14.us ; <double*> [#uses=3]
  %9 = load double* %scevgep30, align 8           ; <double> [#uses=1]
  %10 = fmul double %9, %0                        ; <double> [#uses=2]
  store double %10, double* %scevgep30, align 8
  br label %bb2.us

bb6:                                              ; preds = %bb4.us
  %11 = add nsw i64 %storemerge9, 1               ; <i64> [#uses=2]
  %12 = icmp slt i64 %11, 512                     ; <i1> [#uses=1]
  br i1 %12, label %bb5.preheader, label %return

bb5.preheader:                                    ; preds = %bb6, %bb.nph26.bb.nph26.split_crit_edge
  %storemerge9 = phi i64 [ 0, %bb.nph26.bb.nph26.split_crit_edge ], [ %11, %bb6 ] ; <i64> [#uses=3]
  br label %bb.nph.us

return:                                           ; preds = %bb6
  ret void
}

define i32 @main(i32 %argc, i8** %argv) nounwind {
entry:
  store double 3.241200e+04, double* @alpha, align 8
  store double 2.123000e+03, double* @beta, align 8
  br label %bb.nph20.i

bb.nph20.i:                                       ; preds = %bb3.i, %entry
  %indvar41.i = phi i64 [ 0, %entry ], [ %indvar.next42.i, %bb3.i ] ; <i64> [#uses=3]
  %storemerge21.i = trunc i64 %indvar41.i to i32  ; <i32> [#uses=1]
  %0 = sitofp i32 %storemerge21.i to double       ; <double> [#uses=1]
  br label %bb1.i

bb1.i:                                            ; preds = %bb1.i, %bb.nph20.i
  %indvar38.i = phi i64 [ 0, %bb.nph20.i ], [ %indvar.next39.i, %bb1.i ] ; <i64> [#uses=3]
  %scevgep43.i = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %indvar41.i, i64 %indvar38.i ; <double*> [#uses=1]
  %storemerge519.i = trunc i64 %indvar38.i to i32 ; <i32> [#uses=1]
  %1 = sitofp i32 %storemerge519.i to double      ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=1]
  %3 = fdiv double %2, 5.120000e+02               ; <double> [#uses=1]
  store double %3, double* %scevgep43.i, align 8
  %indvar.next39.i = add i64 %indvar38.i, 1       ; <i64> [#uses=2]
  %exitcond23 = icmp eq i64 %indvar.next39.i, 512 ; <i1> [#uses=1]
  br i1 %exitcond23, label %bb3.i, label %bb1.i

bb3.i:                                            ; preds = %bb1.i
  %indvar.next42.i = add i64 %indvar41.i, 1       ; <i64> [#uses=2]
  %exitcond24 = icmp eq i64 %indvar.next42.i, 512 ; <i1> [#uses=1]
  br i1 %exitcond24, label %bb.nph13.i, label %bb.nph20.i

bb.nph13.i:                                       ; preds = %bb9.i, %bb3.i
  %indvar33.i = phi i64 [ %indvar.next34.i, %bb9.i ], [ 0, %bb3.i ] ; <i64> [#uses=3]
  %storemerge114.i = trunc i64 %indvar33.i to i32 ; <i32> [#uses=1]
  %4 = sitofp i32 %storemerge114.i to double      ; <double> [#uses=1]
  br label %bb7.i

bb7.i:                                            ; preds = %bb7.i, %bb.nph13.i
  %indvar30.i = phi i64 [ 0, %bb.nph13.i ], [ %indvar.next31.i, %bb7.i ] ; <i64> [#uses=3]
  %scevgep35.i = getelementptr [512 x [512 x double]]* @B, i64 0, i64 %indvar33.i, i64 %indvar30.i ; <double*> [#uses=1]
  %storemerge412.i = trunc i64 %indvar30.i to i32 ; <i32> [#uses=1]
  %5 = sitofp i32 %storemerge412.i to double      ; <double> [#uses=1]
  %6 = fmul double %4, %5                         ; <double> [#uses=1]
  %7 = fadd double %6, 1.000000e+00               ; <double> [#uses=1]
  %8 = fdiv double %7, 5.120000e+02               ; <double> [#uses=1]
  store double %8, double* %scevgep35.i, align 8
  %indvar.next31.i = add i64 %indvar30.i, 1       ; <i64> [#uses=2]
  %exitcond21 = icmp eq i64 %indvar.next31.i, 512 ; <i1> [#uses=1]
  br i1 %exitcond21, label %bb9.i, label %bb7.i

bb9.i:                                            ; preds = %bb7.i
  %indvar.next34.i = add i64 %indvar33.i, 1       ; <i64> [#uses=2]
  %exitcond22 = icmp eq i64 %indvar.next34.i, 512 ; <i1> [#uses=1]
  br i1 %exitcond22, label %bb.nph.i, label %bb.nph13.i

bb.nph.i:                                         ; preds = %bb15.i, %bb9.i
  %indvar26.i = phi i64 [ %indvar.next27.i, %bb15.i ], [ 0, %bb9.i ] ; <i64> [#uses=3]
  %storemerge27.i = trunc i64 %indvar26.i to i32  ; <i32> [#uses=1]
  %9 = sitofp i32 %storemerge27.i to double       ; <double> [#uses=1]
  br label %bb13.i

bb13.i:                                           ; preds = %bb13.i, %bb.nph.i
  %indvar.i = phi i64 [ 0, %bb.nph.i ], [ %indvar.next.i, %bb13.i ] ; <i64> [#uses=3]
  %scevgep.i = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %indvar26.i, i64 %indvar.i ; <double*> [#uses=1]
  %storemerge36.i = trunc i64 %indvar.i to i32    ; <i32> [#uses=1]
  %10 = sitofp i32 %storemerge36.i to double      ; <double> [#uses=1]
  %11 = fmul double %9, %10                       ; <double> [#uses=1]
  %12 = fadd double %11, 2.000000e+00             ; <double> [#uses=1]
  %13 = fdiv double %12, 5.120000e+02             ; <double> [#uses=1]
  store double %13, double* %scevgep.i, align 8
  %indvar.next.i = add i64 %indvar.i, 1           ; <i64> [#uses=2]
  %exitcond19 = icmp eq i64 %indvar.next.i, 512   ; <i1> [#uses=1]
  br i1 %exitcond19, label %bb15.i, label %bb13.i

bb15.i:                                           ; preds = %bb13.i
  %indvar.next27.i = add i64 %indvar26.i, 1       ; <i64> [#uses=2]
  %exitcond20 = icmp eq i64 %indvar.next27.i, 512 ; <i1> [#uses=1]
  br i1 %exitcond20, label %bb5.preheader.i, label %bb.nph.i

bb4.us.i6:                                        ; preds = %bb2.us.i
  store double %19, double* %scevgep30.i
  %14 = add nsw i64 %storemerge14.us.i, 1         ; <i64> [#uses=2]
  %exitcond17 = icmp eq i64 %14, 512              ; <i1> [#uses=1]
  br i1 %exitcond17, label %bb6.i, label %bb.nph.us.i9

bb2.us.i:                                         ; preds = %bb.nph.us.i9, %bb2.us.i
  %.tmp.0.us.i = phi double [ %22, %bb.nph.us.i9 ], [ %19, %bb2.us.i ] ; <double> [#uses=1]
  %storemerge23.us.i = phi i64 [ 0, %bb.nph.us.i9 ], [ %20, %bb2.us.i ] ; <i64> [#uses=3]
  %scevgep27.i = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %storemerge9.i, i64 %storemerge23.us.i ; <double*> [#uses=1]
  %scevgep.i7 = getelementptr [512 x [512 x double]]* @B, i64 0, i64 %storemerge23.us.i, i64 %storemerge14.us.i ; <double*> [#uses=1]
  %15 = load double* %scevgep27.i, align 8        ; <double> [#uses=1]
  %16 = fmul double %15, 3.241200e+04             ; <double> [#uses=1]
  %17 = load double* %scevgep.i7, align 8         ; <double> [#uses=1]
  %18 = fmul double %16, %17                      ; <double> [#uses=1]
  %19 = fadd double %.tmp.0.us.i, %18             ; <double> [#uses=2]
  %20 = add nsw i64 %storemerge23.us.i, 1         ; <i64> [#uses=2]
  %exitcond16 = icmp eq i64 %20, 512              ; <i1> [#uses=1]
  br i1 %exitcond16, label %bb4.us.i6, label %bb2.us.i

bb.nph.us.i9:                                     ; preds = %bb5.preheader.i, %bb4.us.i6
  %storemerge14.us.i = phi i64 [ %14, %bb4.us.i6 ], [ 0, %bb5.preheader.i ] ; <i64> [#uses=3]
  %scevgep30.i = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %storemerge9.i, i64 %storemerge14.us.i ; <double*> [#uses=3]
  %21 = load double* %scevgep30.i, align 8        ; <double> [#uses=1]
  %22 = fmul double %21, 2.123000e+03             ; <double> [#uses=2]
  store double %22, double* %scevgep30.i, align 8
  br label %bb2.us.i

bb6.i:                                            ; preds = %bb4.us.i6
  %23 = add nsw i64 %storemerge9.i, 1             ; <i64> [#uses=2]
  %exitcond18 = icmp eq i64 %23, 512              ; <i1> [#uses=1]
  br i1 %exitcond18, label %scop_func.exit, label %bb5.preheader.i

bb5.preheader.i:                                  ; preds = %bb6.i, %bb15.i
  %storemerge9.i = phi i64 [ %23, %bb6.i ], [ 0, %bb15.i ] ; <i64> [#uses=3]
  br label %bb.nph.us.i9

scop_func.exit:                                   ; preds = %bb6.i
  %24 = icmp sgt i32 %argc, 42                    ; <i1> [#uses=1]
  br i1 %24, label %bb.i, label %print_array.exit

bb.i:                                             ; preds = %scop_func.exit
  %25 = load i8** %argv, align 1                  ; <i8*> [#uses=1]
  %26 = load i8* %25, align 1                     ; <i8> [#uses=1]
  %27 = icmp eq i8 %26, 0                         ; <i1> [#uses=1]
  br i1 %27, label %bb.nph.us.i, label %print_array.exit

bb7.us.i:                                         ; preds = %bb5.us.i
  %28 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %29 = bitcast %struct._IO_FILE* %28 to i8*      ; <i8*> [#uses=1]
  %30 = tail call i32 @fputc(i32 10, i8* %29) nounwind ; <i32> [#uses=0]
  %indvar.next9.i = add i64 %indvar8.i, 1         ; <i64> [#uses=2]
  %exitcond12 = icmp eq i64 %indvar.next9.i, 512  ; <i1> [#uses=1]
  br i1 %exitcond12, label %print_array.exit, label %bb.nph.us.i

bb5.us.i:                                         ; preds = %bb4.us.i, %bb3.us.i
  %indvar.next.i1 = add i64 %indvar.i3, 1         ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next.i1, 512    ; <i1> [#uses=1]
  br i1 %exitcond, label %bb7.us.i, label %bb3.us.i

bb3.us.i:                                         ; preds = %bb.nph.us.i, %bb5.us.i
  %indvar.i3 = phi i64 [ 0, %bb.nph.us.i ], [ %indvar.next.i1, %bb5.us.i ] ; <i64> [#uses=3]
  %scevgep.i4 = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %indvar8.i, i64 %indvar.i3 ; <double*> [#uses=1]
  %tmp14 = add i64 %tmp13, %indvar.i3             ; <i64> [#uses=1]
  %tmp11.i = trunc i64 %tmp14 to i32              ; <i32> [#uses=1]
  %31 = load double* %scevgep.i4, align 8         ; <double> [#uses=1]
  %32 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %33 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %32, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %31) nounwind ; <i32> [#uses=0]
  %34 = srem i32 %tmp11.i, 80                     ; <i32> [#uses=1]
  %35 = icmp eq i32 %34, 20                       ; <i1> [#uses=1]
  br i1 %35, label %bb4.us.i, label %bb5.us.i

bb4.us.i:                                         ; preds = %bb3.us.i
  %36 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %37 = bitcast %struct._IO_FILE* %36 to i8*      ; <i8*> [#uses=1]
  %38 = tail call i32 @fputc(i32 10, i8* %37) nounwind ; <i32> [#uses=0]
  br label %bb5.us.i

bb.nph.us.i:                                      ; preds = %bb7.us.i, %bb.i
  %indvar8.i = phi i64 [ %indvar.next9.i, %bb7.us.i ], [ 0, %bb.i ] ; <i64> [#uses=3]
  %tmp13 = shl i64 %indvar8.i, 9                  ; <i64> [#uses=1]
  br label %bb3.us.i

print_array.exit:                                 ; preds = %bb7.us.i, %bb.i, %scop_func.exit
  ret i32 0
}
