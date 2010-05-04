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
@A = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=2]
@B = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=2]
@C = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=4]
@D = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=2]
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

define void @scop_func(i64 %ni, i64 %nj, i64 %nk, i64 %nl) nounwind {
entry:
  %0 = icmp sgt i64 %ni, 0                        ; <i1> [#uses=2]
  br i1 %0, label %bb.nph50, label %return

bb.nph35:                                         ; preds = %bb.nph50, %bb6
  %storemerge37 = phi i64 [ %7, %bb6 ], [ 0, %bb.nph50 ] ; <i64> [#uses=4]
  br i1 %10, label %bb.nph27.us, label %bb4

bb4.us:                                           ; preds = %bb2.us
  store double %5, double* %scevgep64
  %1 = add nsw i64 %storemerge431.us, 1           ; <i64> [#uses=2]
  %exitcond62 = icmp eq i64 %1, %nj               ; <i1> [#uses=1]
  br i1 %exitcond62, label %bb6, label %bb.nph27.us

bb2.us:                                           ; preds = %bb.nph27.us, %bb2.us
  %.tmp.029.us = phi double [ 0.000000e+00, %bb.nph27.us ], [ %5, %bb2.us ] ; <double> [#uses=1]
  %storemerge526.us = phi i64 [ 0, %bb.nph27.us ], [ %6, %bb2.us ] ; <i64> [#uses=3]
  %scevgep60 = getelementptr [512 x [512 x double]]* @B, i64 0, i64 %storemerge526.us, i64 %storemerge431.us ; <double*> [#uses=1]
  %scevgep61 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %storemerge37, i64 %storemerge526.us ; <double*> [#uses=1]
  %2 = load double* %scevgep61, align 8           ; <double> [#uses=1]
  %3 = load double* %scevgep60, align 8           ; <double> [#uses=1]
  %4 = fmul double %2, %3                         ; <double> [#uses=1]
  %5 = fadd double %.tmp.029.us, %4               ; <double> [#uses=2]
  %6 = add nsw i64 %storemerge526.us, 1           ; <i64> [#uses=2]
  %exitcond59 = icmp eq i64 %6, %nk               ; <i1> [#uses=1]
  br i1 %exitcond59, label %bb4.us, label %bb2.us

bb.nph27.us:                                      ; preds = %bb4.us, %bb.nph35
  %storemerge431.us = phi i64 [ %1, %bb4.us ], [ 0, %bb.nph35 ] ; <i64> [#uses=3]
  %scevgep64 = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %storemerge37, i64 %storemerge431.us ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep64, align 8
  br label %bb2.us

bb4:                                              ; preds = %bb4, %bb.nph35
  %indvar67 = phi i64 [ %indvar.next68, %bb4 ], [ 0, %bb.nph35 ] ; <i64> [#uses=2]
  %storemerge44 = phi i64 [ %storemerge44, %bb4 ], [ %storemerge37, %bb.nph35 ] ; <i64> [#uses=3]
  %tmp71 = shl i64 %storemerge44, 9               ; <i64> [#uses=1]
  %scevgep70.sum = add i64 %indvar67, %tmp71      ; <i64> [#uses=1]
  %scevgep72 = getelementptr [512 x [512 x double]]* @C, i64 0, i64 0, i64 %scevgep70.sum ; <double*> [#uses=1]
  store double 0.000000e+00, double* %scevgep72, align 8
  %indvar.next68 = add i64 %indvar67, 1           ; <i64> [#uses=2]
  %exitcond69 = icmp eq i64 %indvar.next68, %nj   ; <i1> [#uses=1]
  br i1 %exitcond69, label %bb6, label %bb4

bb6:                                              ; preds = %bb4, %bb4.us
  %storemerge49 = phi i64 [ %storemerge37, %bb4.us ], [ %storemerge44, %bb4 ] ; <i64> [#uses=1]
  %7 = add nsw i64 %storemerge49, 1               ; <i64> [#uses=2]
  %8 = icmp slt i64 %7, %ni                       ; <i1> [#uses=1]
  br i1 %8, label %bb.nph35, label %bb16.preheader

bb.nph50:                                         ; preds = %entry
  %9 = icmp sgt i64 %nj, 0                        ; <i1> [#uses=1]
  %10 = icmp sgt i64 %nk, 0                       ; <i1> [#uses=1]
  br i1 %9, label %bb.nph35, label %bb16.preheader

bb16.preheader:                                   ; preds = %bb.nph50, %bb6
  br i1 %0, label %bb.nph25, label %return

bb.nph11:                                         ; preds = %bb.nph25, %bb15
  %storemerge112 = phi i64 [ %17, %bb15 ], [ 0, %bb.nph25 ] ; <i64> [#uses=4]
  br i1 %20, label %bb.nph.us, label %bb13

bb13.us:                                          ; preds = %bb11.us
  store double %15, double* %scevgep54
  %11 = add nsw i64 %storemerge27.us, 1           ; <i64> [#uses=2]
  %exitcond52 = icmp eq i64 %11, %nl              ; <i1> [#uses=1]
  br i1 %exitcond52, label %bb15, label %bb.nph.us

bb11.us:                                          ; preds = %bb.nph.us, %bb11.us
  %.tmp.0.us = phi double [ 0.000000e+00, %bb.nph.us ], [ %15, %bb11.us ] ; <double> [#uses=1]
  %storemerge36.us = phi i64 [ 0, %bb.nph.us ], [ %16, %bb11.us ] ; <i64> [#uses=3]
  %scevgep = getelementptr [512 x [512 x double]]* @D, i64 0, i64 %storemerge36.us, i64 %storemerge27.us ; <double*> [#uses=1]
  %scevgep51 = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %storemerge112, i64 %storemerge36.us ; <double*> [#uses=1]
  %12 = load double* %scevgep51, align 8          ; <double> [#uses=1]
  %13 = load double* %scevgep, align 8            ; <double> [#uses=1]
  %14 = fmul double %12, %13                      ; <double> [#uses=1]
  %15 = fadd double %.tmp.0.us, %14               ; <double> [#uses=2]
  %16 = add nsw i64 %storemerge36.us, 1           ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %16, %nj                ; <i1> [#uses=1]
  br i1 %exitcond, label %bb13.us, label %bb11.us

bb.nph.us:                                        ; preds = %bb13.us, %bb.nph11
  %storemerge27.us = phi i64 [ %11, %bb13.us ], [ 0, %bb.nph11 ] ; <i64> [#uses=3]
  %scevgep54 = getelementptr [512 x [512 x double]]* @E, i64 0, i64 %storemerge112, i64 %storemerge27.us ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep54, align 8
  br label %bb11.us

bb13:                                             ; preds = %bb13, %bb.nph11
  %indvar = phi i64 [ %indvar.next, %bb13 ], [ 0, %bb.nph11 ] ; <i64> [#uses=2]
  %storemerge119 = phi i64 [ %storemerge119, %bb13 ], [ %storemerge112, %bb.nph11 ] ; <i64> [#uses=3]
  %tmp = shl i64 %storemerge119, 9                ; <i64> [#uses=1]
  %scevgep56.sum = add i64 %indvar, %tmp          ; <i64> [#uses=1]
  %scevgep57 = getelementptr [512 x [512 x double]]* @E, i64 0, i64 0, i64 %scevgep56.sum ; <double*> [#uses=1]
  store double 0.000000e+00, double* %scevgep57, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond55 = icmp eq i64 %indvar.next, %nl     ; <i1> [#uses=1]
  br i1 %exitcond55, label %bb15, label %bb13

bb15:                                             ; preds = %bb13, %bb13.us
  %storemerge124 = phi i64 [ %storemerge112, %bb13.us ], [ %storemerge119, %bb13 ] ; <i64> [#uses=1]
  %17 = add nsw i64 %storemerge124, 1             ; <i64> [#uses=2]
  %18 = icmp slt i64 %17, %ni                     ; <i1> [#uses=1]
  br i1 %18, label %bb.nph11, label %return

bb.nph25:                                         ; preds = %bb16.preheader
  %19 = icmp sgt i64 %nl, 0                       ; <i1> [#uses=1]
  %20 = icmp sgt i64 %nj, 0                       ; <i1> [#uses=1]
  br i1 %19, label %bb.nph11, label %return

return:                                           ; preds = %bb.nph25, %bb15, %bb16.preheader, %entry
  ret void
}

define i32 @main(i32 %argc, i8** %argv) nounwind {
entry:
  tail call void @init_array() nounwind
  tail call void @scop_func(i64 512, i64 512, i64 512, i64 512) nounwind
  %0 = icmp sgt i32 %argc, 42                     ; <i1> [#uses=1]
  br i1 %0, label %bb.i, label %print_array.exit

bb.i:                                             ; preds = %entry
  %1 = load i8** %argv, align 1                   ; <i8*> [#uses=1]
  %2 = load i8* %1, align 1                       ; <i8> [#uses=1]
  %3 = icmp eq i8 %2, 0                           ; <i1> [#uses=1]
  br i1 %3, label %bb.nph.us.i, label %print_array.exit

bb7.us.i:                                         ; preds = %bb5.us.i
  %4 = load %struct._IO_FILE** @stderr, align 8   ; <%struct._IO_FILE*> [#uses=1]
  %5 = bitcast %struct._IO_FILE* %4 to i8*        ; <i8*> [#uses=1]
  %6 = tail call i32 @fputc(i32 10, i8* %5) nounwind ; <i32> [#uses=0]
  %indvar.next9.i = add i64 %indvar8.i, 1         ; <i64> [#uses=2]
  %exitcond3 = icmp eq i64 %indvar.next9.i, 512   ; <i1> [#uses=1]
  br i1 %exitcond3, label %print_array.exit, label %bb.nph.us.i

bb5.us.i:                                         ; preds = %bb4.us.i, %bb3.us.i
  %indvar.next.i = add i64 %indvar.i, 1           ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next.i, 512     ; <i1> [#uses=1]
  br i1 %exitcond, label %bb7.us.i, label %bb3.us.i

bb3.us.i:                                         ; preds = %bb.nph.us.i, %bb5.us.i
  %indvar.i = phi i64 [ 0, %bb.nph.us.i ], [ %indvar.next.i, %bb5.us.i ] ; <i64> [#uses=3]
  %scevgep.i = getelementptr [512 x [512 x double]]* @E, i64 0, i64 %indvar8.i, i64 %indvar.i ; <double*> [#uses=1]
  %tmp5 = add i64 %tmp4, %indvar.i                ; <i64> [#uses=1]
  %tmp11.i = trunc i64 %tmp5 to i32               ; <i32> [#uses=1]
  %7 = load double* %scevgep.i, align 8           ; <double> [#uses=1]
  %8 = load %struct._IO_FILE** @stderr, align 8   ; <%struct._IO_FILE*> [#uses=1]
  %9 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %8, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %7) nounwind ; <i32> [#uses=0]
  %10 = srem i32 %tmp11.i, 80                     ; <i32> [#uses=1]
  %11 = icmp eq i32 %10, 20                       ; <i1> [#uses=1]
  br i1 %11, label %bb4.us.i, label %bb5.us.i

bb4.us.i:                                         ; preds = %bb3.us.i
  %12 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %13 = bitcast %struct._IO_FILE* %12 to i8*      ; <i8*> [#uses=1]
  %14 = tail call i32 @fputc(i32 10, i8* %13) nounwind ; <i32> [#uses=0]
  br label %bb5.us.i

bb.nph.us.i:                                      ; preds = %bb7.us.i, %bb.i
  %indvar8.i = phi i64 [ %indvar.next9.i, %bb7.us.i ], [ 0, %bb.i ] ; <i64> [#uses=3]
  %tmp4 = shl i64 %indvar8.i, 9                   ; <i64> [#uses=1]
  br label %bb3.us.i

print_array.exit:                                 ; preds = %bb7.us.i, %bb.i, %entry
  ret i32 0
}
