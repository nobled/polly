; RUN: opt -indvars -polly-scop-detect  -analyze %s | FileCheck %s
; XFAIL: *
; ModuleID = '<stdin>'
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
bb.nph43.bb.nph43.split_crit_edge:
  store double 3.241200e+04, double* @alpha1, align 8
  store double 2.123000e+03, double* @beta1, align 8
  store double 1.324120e+05, double* @alpha2, align 8
  store double 9.212300e+04, double* @beta2, align 8
  br label %bb.nph38

bb.nph38:                                         ; preds = %bb3, %bb.nph43.bb.nph43.split_crit_edge
  %indvar75 = phi i64 [ 0, %bb.nph43.bb.nph43.split_crit_edge ], [ %indvar.next76, %bb3 ] ; <i64> [#uses=3]
  %storemerge39 = trunc i64 %indvar75 to i32      ; <i32> [#uses=1]
  %0 = sitofp i32 %storemerge39 to double         ; <double> [#uses=1]
  br label %bb1

bb1:                                              ; preds = %bb1, %bb.nph38
  %indvar72 = phi i64 [ 0, %bb.nph38 ], [ %indvar.next73, %bb1 ] ; <i64> [#uses=3]
  %scevgep77 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %indvar75, i64 %indvar72 ; <double*> [#uses=1]
  %storemerge937 = trunc i64 %indvar72 to i32     ; <i32> [#uses=1]
  %1 = sitofp i32 %storemerge937 to double        ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=1]
  %3 = fdiv double %2, 5.120000e+02               ; <double> [#uses=1]
  store double %3, double* %scevgep77, align 8
  %indvar.next73 = add i64 %indvar72, 1           ; <i64> [#uses=2]
  %exitcond74 = icmp eq i64 %indvar.next73, 512   ; <i1> [#uses=1]
  br i1 %exitcond74, label %bb3, label %bb1

bb3:                                              ; preds = %bb1
  %indvar.next76 = add i64 %indvar75, 1           ; <i64> [#uses=2]
  %exitcond78 = icmp eq i64 %indvar.next76, 512   ; <i1> [#uses=1]
  br i1 %exitcond78, label %bb.nph31, label %bb.nph38

bb.nph31:                                         ; preds = %bb9, %bb3
  %indvar67 = phi i64 [ %indvar.next68, %bb9 ], [ 0, %bb3 ] ; <i64> [#uses=3]
  %storemerge132 = trunc i64 %indvar67 to i32     ; <i32> [#uses=1]
  %4 = sitofp i32 %storemerge132 to double        ; <double> [#uses=1]
  br label %bb7

bb7:                                              ; preds = %bb7, %bb.nph31
  %indvar64 = phi i64 [ 0, %bb.nph31 ], [ %indvar.next65, %bb7 ] ; <i64> [#uses=3]
  %scevgep69 = getelementptr [512 x [512 x double]]* @B, i64 0, i64 %indvar67, i64 %indvar64 ; <double*> [#uses=1]
  %storemerge830 = trunc i64 %indvar64 to i32     ; <i32> [#uses=1]
  %5 = sitofp i32 %storemerge830 to double        ; <double> [#uses=1]
  %6 = fmul double %4, %5                         ; <double> [#uses=1]
  %7 = fadd double %6, 1.000000e+00               ; <double> [#uses=1]
  %8 = fdiv double %7, 5.120000e+02               ; <double> [#uses=1]
  store double %8, double* %scevgep69, align 8
  %indvar.next65 = add i64 %indvar64, 1           ; <i64> [#uses=2]
  %exitcond66 = icmp eq i64 %indvar.next65, 512   ; <i1> [#uses=1]
  br i1 %exitcond66, label %bb9, label %bb7

bb9:                                              ; preds = %bb7
  %indvar.next68 = add i64 %indvar67, 1           ; <i64> [#uses=2]
  %exitcond70 = icmp eq i64 %indvar.next68, 512   ; <i1> [#uses=1]
  br i1 %exitcond70, label %bb.nph24, label %bb.nph31

bb.nph24:                                         ; preds = %bb15, %bb9
  %indvar59 = phi i64 [ %indvar.next60, %bb15 ], [ 0, %bb9 ] ; <i64> [#uses=3]
  %storemerge225 = trunc i64 %indvar59 to i32     ; <i32> [#uses=1]
  %9 = sitofp i32 %storemerge225 to double        ; <double> [#uses=1]
  br label %bb13

bb13:                                             ; preds = %bb13, %bb.nph24
  %indvar56 = phi i64 [ 0, %bb.nph24 ], [ %indvar.next57, %bb13 ] ; <i64> [#uses=3]
  %scevgep61 = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %indvar59, i64 %indvar56 ; <double*> [#uses=1]
  %storemerge723 = trunc i64 %indvar56 to i32     ; <i32> [#uses=1]
  %10 = sitofp i32 %storemerge723 to double       ; <double> [#uses=1]
  %11 = fmul double %9, %10                       ; <double> [#uses=1]
  %12 = fadd double %11, 2.000000e+00             ; <double> [#uses=1]
  %13 = fdiv double %12, 5.120000e+02             ; <double> [#uses=1]
  store double %13, double* %scevgep61, align 8
  %indvar.next57 = add i64 %indvar56, 1           ; <i64> [#uses=2]
  %exitcond58 = icmp eq i64 %indvar.next57, 512   ; <i1> [#uses=1]
  br i1 %exitcond58, label %bb15, label %bb13

bb15:                                             ; preds = %bb13
  %indvar.next60 = add i64 %indvar59, 1           ; <i64> [#uses=2]
  %exitcond62 = icmp eq i64 %indvar.next60, 512   ; <i1> [#uses=1]
  br i1 %exitcond62, label %bb.nph17, label %bb.nph24

bb.nph17:                                         ; preds = %bb21, %bb15
  %indvar51 = phi i64 [ %indvar.next52, %bb21 ], [ 0, %bb15 ] ; <i64> [#uses=3]
  %storemerge318 = trunc i64 %indvar51 to i32     ; <i32> [#uses=1]
  %14 = sitofp i32 %storemerge318 to double       ; <double> [#uses=1]
  br label %bb19

bb19:                                             ; preds = %bb19, %bb.nph17
  %indvar48 = phi i64 [ 0, %bb.nph17 ], [ %indvar.next49, %bb19 ] ; <i64> [#uses=3]
  %scevgep53 = getelementptr [512 x [512 x double]]* @D, i64 0, i64 %indvar51, i64 %indvar48 ; <double*> [#uses=1]
  %storemerge616 = trunc i64 %indvar48 to i32     ; <i32> [#uses=1]
  %15 = sitofp i32 %storemerge616 to double       ; <double> [#uses=1]
  %16 = fmul double %14, %15                      ; <double> [#uses=1]
  %17 = fadd double %16, 2.000000e+00             ; <double> [#uses=1]
  %18 = fdiv double %17, 5.120000e+02             ; <double> [#uses=1]
  store double %18, double* %scevgep53, align 8
  %indvar.next49 = add i64 %indvar48, 1           ; <i64> [#uses=2]
  %exitcond50 = icmp eq i64 %indvar.next49, 512   ; <i1> [#uses=1]
  br i1 %exitcond50, label %bb21, label %bb19

bb21:                                             ; preds = %bb19
  %indvar.next52 = add i64 %indvar51, 1           ; <i64> [#uses=2]
  %exitcond54 = icmp eq i64 %indvar.next52, 512   ; <i1> [#uses=1]
  br i1 %exitcond54, label %bb.nph, label %bb.nph17

bb.nph:                                           ; preds = %bb27, %bb21
  %indvar44 = phi i64 [ %indvar.next45, %bb27 ], [ 0, %bb21 ] ; <i64> [#uses=3]
  %storemerge411 = trunc i64 %indvar44 to i32     ; <i32> [#uses=1]
  %19 = sitofp i32 %storemerge411 to double       ; <double> [#uses=1]
  br label %bb25

bb25:                                             ; preds = %bb25, %bb.nph
  %indvar = phi i64 [ 0, %bb.nph ], [ %indvar.next, %bb25 ] ; <i64> [#uses=3]
  %scevgep = getelementptr [512 x [512 x double]]* @E, i64 0, i64 %indvar44, i64 %indvar ; <double*> [#uses=1]
  %storemerge510 = trunc i64 %indvar to i32       ; <i32> [#uses=1]
  %20 = sitofp i32 %storemerge510 to double       ; <double> [#uses=1]
  %21 = fmul double %19, %20                      ; <double> [#uses=1]
  %22 = fadd double %21, 2.000000e+00             ; <double> [#uses=1]
  %23 = fdiv double %22, 5.120000e+02             ; <double> [#uses=1]
  store double %23, double* %scevgep, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 512       ; <i1> [#uses=1]
  br i1 %exitcond, label %bb27, label %bb25

bb27:                                             ; preds = %bb25
  %indvar.next45 = add i64 %indvar44, 1           ; <i64> [#uses=2]
  %exitcond46 = icmp eq i64 %indvar.next45, 512   ; <i1> [#uses=1]
  br i1 %exitcond46, label %return, label %bb.nph

return:                                           ; preds = %bb27
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
  %scevgep = getelementptr [512 x [512 x double]]* @E, i64 0, i64 %indvar8, i64 %indvar ; <double*> [#uses=1]
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
bb.nph50.bb.nph50.split_crit_edge:
  br label %bb5.preheader

bb4.us:                                           ; preds = %bb2.us
  store double %4, double* %scevgep61
  %0 = add nsw i64 %storemerge431.us, 1           ; <i64> [#uses=2]
  %exitcond59 = icmp eq i64 %0, 512               ; <i1> [#uses=1]
  br i1 %exitcond59, label %bb6, label %bb.nph27.us

bb2.us:                                           ; preds = %bb.nph27.us, %bb2.us
  %.tmp.029.us = phi double [ 0.000000e+00, %bb.nph27.us ], [ %4, %bb2.us ] ; <double> [#uses=1]
  %storemerge526.us = phi i64 [ 0, %bb.nph27.us ], [ %5, %bb2.us ] ; <i64> [#uses=3]
  %scevgep57 = getelementptr [512 x [512 x double]]* @B, i64 0, i64 %storemerge526.us, i64 %storemerge431.us ; <double*> [#uses=1]
  %scevgep58 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %storemerge37, i64 %storemerge526.us ; <double*> [#uses=1]
  %1 = load double* %scevgep58, align 8           ; <double> [#uses=1]
  %2 = load double* %scevgep57, align 8           ; <double> [#uses=1]
  %3 = fmul double %1, %2                         ; <double> [#uses=1]
  %4 = fadd double %.tmp.029.us, %3               ; <double> [#uses=2]
  %5 = add nsw i64 %storemerge526.us, 1           ; <i64> [#uses=2]
  %exitcond56 = icmp eq i64 %5, 512               ; <i1> [#uses=1]
  br i1 %exitcond56, label %bb4.us, label %bb2.us

bb.nph27.us:                                      ; preds = %bb5.preheader, %bb4.us
  %storemerge431.us = phi i64 [ %0, %bb4.us ], [ 0, %bb5.preheader ] ; <i64> [#uses=3]
  %scevgep61 = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %storemerge37, i64 %storemerge431.us ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep61, align 8
  br label %bb2.us

bb6:                                              ; preds = %bb4.us
  %6 = add nsw i64 %storemerge37, 1               ; <i64> [#uses=2]
  %7 = icmp slt i64 %6, 512                       ; <i1> [#uses=1]
  br i1 %7, label %bb5.preheader, label %bb14.preheader

bb5.preheader:                                    ; preds = %bb6, %bb.nph50.bb.nph50.split_crit_edge
  %storemerge37 = phi i64 [ 0, %bb.nph50.bb.nph50.split_crit_edge ], [ %6, %bb6 ] ; <i64> [#uses=3]
  br label %bb.nph27.us

bb13.us:                                          ; preds = %bb11.us
  store double %12, double* %scevgep54
  %8 = add nsw i64 %storemerge27.us, 1            ; <i64> [#uses=2]
  %exitcond52 = icmp eq i64 %8, 512               ; <i1> [#uses=1]
  br i1 %exitcond52, label %bb15, label %bb.nph.us

bb11.us:                                          ; preds = %bb.nph.us, %bb11.us
  %.tmp.0.us = phi double [ 0.000000e+00, %bb.nph.us ], [ %12, %bb11.us ] ; <double> [#uses=1]
  %storemerge36.us = phi i64 [ 0, %bb.nph.us ], [ %13, %bb11.us ] ; <i64> [#uses=3]
  %scevgep = getelementptr [512 x [512 x double]]* @D, i64 0, i64 %storemerge36.us, i64 %storemerge27.us ; <double*> [#uses=1]
  %scevgep51 = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %storemerge112, i64 %storemerge36.us ; <double*> [#uses=1]
  %9 = load double* %scevgep51, align 8           ; <double> [#uses=1]
  %10 = load double* %scevgep, align 8            ; <double> [#uses=1]
  %11 = fmul double %9, %10                       ; <double> [#uses=1]
  %12 = fadd double %.tmp.0.us, %11               ; <double> [#uses=2]
  %13 = add nsw i64 %storemerge36.us, 1           ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %13, 512                ; <i1> [#uses=1]
  br i1 %exitcond, label %bb13.us, label %bb11.us

bb.nph.us:                                        ; preds = %bb14.preheader, %bb13.us
  %storemerge27.us = phi i64 [ %8, %bb13.us ], [ 0, %bb14.preheader ] ; <i64> [#uses=3]
  %scevgep54 = getelementptr [512 x [512 x double]]* @E, i64 0, i64 %storemerge112, i64 %storemerge27.us ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep54, align 8
  br label %bb11.us

bb15:                                             ; preds = %bb13.us
  %14 = add nsw i64 %storemerge112, 1             ; <i64> [#uses=2]
  %15 = icmp slt i64 %14, 512                     ; <i1> [#uses=1]
  br i1 %15, label %bb14.preheader, label %return

bb14.preheader:                                   ; preds = %bb15, %bb6
  %storemerge112 = phi i64 [ %14, %bb15 ], [ 0, %bb6 ] ; <i64> [#uses=3]
  br label %bb.nph.us

return:                                           ; preds = %bb15
  ret void
}

define i32 @main(i32 %argc, i8** %argv) nounwind {
entry:
  tail call void @init_array() nounwind
  br label %bb5.preheader.i

bb4.us.i:                                         ; preds = %bb2.us.i
  store double %4, double* %scevgep61.i
  %0 = add nsw i64 %storemerge431.us.i, 1         ; <i64> [#uses=2]
  %exitcond17 = icmp eq i64 %0, 512               ; <i1> [#uses=1]
  br i1 %exitcond17, label %bb6.i, label %bb.nph27.us.i

bb2.us.i:                                         ; preds = %bb.nph27.us.i, %bb2.us.i
  %.tmp.029.us.i = phi double [ 0.000000e+00, %bb.nph27.us.i ], [ %4, %bb2.us.i ] ; <double> [#uses=1]
  %storemerge526.us.i = phi i64 [ 0, %bb.nph27.us.i ], [ %5, %bb2.us.i ] ; <i64> [#uses=3]
  %scevgep58.i = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %storemerge37.i, i64 %storemerge526.us.i ; <double*> [#uses=1]
  %scevgep57.i = getelementptr [512 x [512 x double]]* @B, i64 0, i64 %storemerge526.us.i, i64 %storemerge431.us.i ; <double*> [#uses=1]
  %1 = load double* %scevgep58.i, align 8         ; <double> [#uses=1]
  %2 = load double* %scevgep57.i, align 8         ; <double> [#uses=1]
  %3 = fmul double %1, %2                         ; <double> [#uses=1]
  %4 = fadd double %.tmp.029.us.i, %3             ; <double> [#uses=2]
  %5 = add nsw i64 %storemerge526.us.i, 1         ; <i64> [#uses=2]
  %exitcond16 = icmp eq i64 %5, 512               ; <i1> [#uses=1]
  br i1 %exitcond16, label %bb4.us.i, label %bb2.us.i

bb.nph27.us.i:                                    ; preds = %bb5.preheader.i, %bb4.us.i
  %storemerge431.us.i = phi i64 [ %0, %bb4.us.i ], [ 0, %bb5.preheader.i ] ; <i64> [#uses=3]
  %scevgep61.i = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %storemerge37.i, i64 %storemerge431.us.i ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep61.i, align 8
  br label %bb2.us.i

bb6.i:                                            ; preds = %bb4.us.i
  %6 = add nsw i64 %storemerge37.i, 1             ; <i64> [#uses=2]
  %exitcond18 = icmp eq i64 %6, 512               ; <i1> [#uses=1]
  br i1 %exitcond18, label %bb14.preheader.i, label %bb5.preheader.i

bb5.preheader.i:                                  ; preds = %bb6.i, %entry
  %storemerge37.i = phi i64 [ 0, %entry ], [ %6, %bb6.i ] ; <i64> [#uses=3]
  br label %bb.nph27.us.i

bb13.us.i:                                        ; preds = %bb11.us.i
  store double %11, double* %scevgep54.i
  %7 = add nsw i64 %storemerge27.us.i, 1          ; <i64> [#uses=2]
  %exitcond13 = icmp eq i64 %7, 512               ; <i1> [#uses=1]
  br i1 %exitcond13, label %bb15.i, label %bb.nph.us.i

bb11.us.i:                                        ; preds = %bb.nph.us.i, %bb11.us.i
  %.tmp.0.us.i = phi double [ 0.000000e+00, %bb.nph.us.i ], [ %11, %bb11.us.i ] ; <double> [#uses=1]
  %storemerge36.us.i = phi i64 [ 0, %bb.nph.us.i ], [ %12, %bb11.us.i ] ; <i64> [#uses=3]
  %scevgep51.i = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %storemerge112.i, i64 %storemerge36.us.i ; <double*> [#uses=1]
  %scevgep.i = getelementptr [512 x [512 x double]]* @D, i64 0, i64 %storemerge36.us.i, i64 %storemerge27.us.i ; <double*> [#uses=1]
  %8 = load double* %scevgep51.i, align 8         ; <double> [#uses=1]
  %9 = load double* %scevgep.i, align 8           ; <double> [#uses=1]
  %10 = fmul double %8, %9                        ; <double> [#uses=1]
  %11 = fadd double %.tmp.0.us.i, %10             ; <double> [#uses=2]
  %12 = add nsw i64 %storemerge36.us.i, 1         ; <i64> [#uses=2]
  %exitcond12 = icmp eq i64 %12, 512              ; <i1> [#uses=1]
  br i1 %exitcond12, label %bb13.us.i, label %bb11.us.i

bb.nph.us.i:                                      ; preds = %bb14.preheader.i, %bb13.us.i
  %storemerge27.us.i = phi i64 [ %7, %bb13.us.i ], [ 0, %bb14.preheader.i ] ; <i64> [#uses=3]
  %scevgep54.i = getelementptr [512 x [512 x double]]* @E, i64 0, i64 %storemerge112.i, i64 %storemerge27.us.i ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep54.i, align 8
  br label %bb11.us.i

bb15.i:                                           ; preds = %bb13.us.i
  %13 = add nsw i64 %storemerge112.i, 1           ; <i64> [#uses=2]
  %exitcond14 = icmp eq i64 %13, 512              ; <i1> [#uses=1]
  br i1 %exitcond14, label %scop_func.exit, label %bb14.preheader.i

bb14.preheader.i:                                 ; preds = %bb15.i, %bb6.i
  %storemerge112.i = phi i64 [ %13, %bb15.i ], [ 0, %bb6.i ] ; <i64> [#uses=3]
  br label %bb.nph.us.i

scop_func.exit:                                   ; preds = %bb15.i
  %14 = icmp sgt i32 %argc, 42                    ; <i1> [#uses=1]
  br i1 %14, label %bb.i, label %print_array.exit

bb.i:                                             ; preds = %scop_func.exit
  %15 = load i8** %argv, align 1                  ; <i8*> [#uses=1]
  %16 = load i8* %15, align 1                     ; <i8> [#uses=1]
  %17 = icmp eq i8 %16, 0                         ; <i1> [#uses=1]
  br i1 %17, label %bb.nph.us.i4, label %print_array.exit

bb7.us.i:                                         ; preds = %bb5.us.i
  %18 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %19 = bitcast %struct._IO_FILE* %18 to i8*      ; <i8*> [#uses=1]
  %20 = tail call i32 @fputc(i32 10, i8* %19) nounwind ; <i32> [#uses=0]
  %indvar.next9.i = add i64 %indvar8.i, 1         ; <i64> [#uses=2]
  %exitcond8 = icmp eq i64 %indvar.next9.i, 512   ; <i1> [#uses=1]
  br i1 %exitcond8, label %print_array.exit, label %bb.nph.us.i4

bb5.us.i:                                         ; preds = %bb4.us.i3, %bb3.us.i
  %indvar.next.i = add i64 %indvar.i, 1           ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next.i, 512     ; <i1> [#uses=1]
  br i1 %exitcond, label %bb7.us.i, label %bb3.us.i

bb3.us.i:                                         ; preds = %bb.nph.us.i4, %bb5.us.i
  %indvar.i = phi i64 [ 0, %bb.nph.us.i4 ], [ %indvar.next.i, %bb5.us.i ] ; <i64> [#uses=3]
  %scevgep.i2 = getelementptr [512 x [512 x double]]* @E, i64 0, i64 %indvar8.i, i64 %indvar.i ; <double*> [#uses=1]
  %tmp10 = add i64 %tmp9, %indvar.i               ; <i64> [#uses=1]
  %tmp11.i = trunc i64 %tmp10 to i32              ; <i32> [#uses=1]
  %21 = load double* %scevgep.i2, align 8         ; <double> [#uses=1]
  %22 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %23 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %22, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %21) nounwind ; <i32> [#uses=0]
  %24 = srem i32 %tmp11.i, 80                     ; <i32> [#uses=1]
  %25 = icmp eq i32 %24, 20                       ; <i1> [#uses=1]
  br i1 %25, label %bb4.us.i3, label %bb5.us.i

bb4.us.i3:                                        ; preds = %bb3.us.i
  %26 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %27 = bitcast %struct._IO_FILE* %26 to i8*      ; <i8*> [#uses=1]
  %28 = tail call i32 @fputc(i32 10, i8* %27) nounwind ; <i32> [#uses=0]
  br label %bb5.us.i

bb.nph.us.i4:                                     ; preds = %bb7.us.i, %bb.i
  %indvar8.i = phi i64 [ %indvar.next9.i, %bb7.us.i ], [ 0, %bb.i ] ; <i64> [#uses=3]
  %tmp9 = shl i64 %indvar8.i, 9                   ; <i64> [#uses=1]
  br label %bb3.us.i

print_array.exit:                                 ; preds = %bb7.us.i, %bb.i, %scop_func.exit
  ret i32 0
}
