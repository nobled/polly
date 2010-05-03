; RUN: opt  -indvars -polly-scop-detect -print-top-scop-only -polly-print-scop -analyze  %s | FileCheck %s
; XFAIL: *
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }

@A = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=2]
@B = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=2]
@C = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=2]
@D = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=2]
@E = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=3]
@F = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=3]
@G = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=4]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]

define void @init_array() nounwind inlinehint {
bb.nph48.bb.nph48.split_crit_edge:
  br label %bb.nph43

bb.nph43:                                         ; preds = %bb3, %bb.nph48.bb.nph48.split_crit_edge
  %indvar96 = phi i64 [ 0, %bb.nph48.bb.nph48.split_crit_edge ], [ %indvar.next97, %bb3 ] ; <i64> [#uses=3]
  %i.044 = trunc i64 %indvar96 to i32             ; <i32> [#uses=1]
  %0 = sitofp i32 %i.044 to double                ; <double> [#uses=1]
  br label %bb1

bb1:                                              ; preds = %bb1, %bb.nph43
  %indvar93 = phi i64 [ 0, %bb.nph43 ], [ %indvar.next94, %bb1 ] ; <i64> [#uses=3]
  %scevgep98 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %indvar96, i64 %indvar93 ; <double*> [#uses=1]
  %j.042 = trunc i64 %indvar93 to i32             ; <i32> [#uses=1]
  %1 = sitofp i32 %j.042 to double                ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=1]
  %3 = fdiv double %2, 5.120000e+02               ; <double> [#uses=1]
  store double %3, double* %scevgep98, align 8
  %indvar.next94 = add i64 %indvar93, 1           ; <i64> [#uses=2]
  %exitcond95 = icmp eq i64 %indvar.next94, 512   ; <i1> [#uses=1]
  br i1 %exitcond95, label %bb3, label %bb1

bb3:                                              ; preds = %bb1
  %indvar.next97 = add i64 %indvar96, 1           ; <i64> [#uses=2]
  %exitcond99 = icmp eq i64 %indvar.next97, 512   ; <i1> [#uses=1]
  br i1 %exitcond99, label %bb.nph36, label %bb.nph43

bb.nph36:                                         ; preds = %bb9, %bb3
  %indvar88 = phi i64 [ %indvar.next89, %bb9 ], [ 0, %bb3 ] ; <i64> [#uses=3]
  %i.137 = trunc i64 %indvar88 to i32             ; <i32> [#uses=1]
  %4 = sitofp i32 %i.137 to double                ; <double> [#uses=1]
  br label %bb7

bb7:                                              ; preds = %bb7, %bb.nph36
  %indvar85 = phi i64 [ 0, %bb.nph36 ], [ %indvar.next86, %bb7 ] ; <i64> [#uses=3]
  %scevgep90 = getelementptr [512 x [512 x double]]* @B, i64 0, i64 %indvar88, i64 %indvar85 ; <double*> [#uses=1]
  %j.135 = trunc i64 %indvar85 to i32             ; <i32> [#uses=1]
  %5 = sitofp i32 %j.135 to double                ; <double> [#uses=1]
  %6 = fmul double %4, %5                         ; <double> [#uses=1]
  %7 = fadd double %6, 1.000000e+00               ; <double> [#uses=1]
  %8 = fdiv double %7, 5.120000e+02               ; <double> [#uses=1]
  store double %8, double* %scevgep90, align 8
  %indvar.next86 = add i64 %indvar85, 1           ; <i64> [#uses=2]
  %exitcond87 = icmp eq i64 %indvar.next86, 512   ; <i1> [#uses=1]
  br i1 %exitcond87, label %bb9, label %bb7

bb9:                                              ; preds = %bb7
  %indvar.next89 = add i64 %indvar88, 1           ; <i64> [#uses=2]
  %exitcond91 = icmp eq i64 %indvar.next89, 512   ; <i1> [#uses=1]
  br i1 %exitcond91, label %bb.nph29, label %bb.nph36

bb.nph29:                                         ; preds = %bb15, %bb9
  %indvar80 = phi i64 [ %indvar.next81, %bb15 ], [ 0, %bb9 ] ; <i64> [#uses=3]
  %i.230 = trunc i64 %indvar80 to i32             ; <i32> [#uses=1]
  %9 = sitofp i32 %i.230 to double                ; <double> [#uses=1]
  br label %bb13

bb13:                                             ; preds = %bb13, %bb.nph29
  %indvar77 = phi i64 [ 0, %bb.nph29 ], [ %indvar.next78, %bb13 ] ; <i64> [#uses=3]
  %scevgep82 = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %indvar80, i64 %indvar77 ; <double*> [#uses=1]
  %j.228 = trunc i64 %indvar77 to i32             ; <i32> [#uses=1]
  %10 = sitofp i32 %j.228 to double               ; <double> [#uses=1]
  %11 = fmul double %9, %10                       ; <double> [#uses=1]
  %12 = fadd double %11, 2.000000e+00             ; <double> [#uses=1]
  %13 = fdiv double %12, 5.120000e+02             ; <double> [#uses=1]
  store double %13, double* %scevgep82, align 8
  %indvar.next78 = add i64 %indvar77, 1           ; <i64> [#uses=2]
  %exitcond79 = icmp eq i64 %indvar.next78, 512   ; <i1> [#uses=1]
  br i1 %exitcond79, label %bb15, label %bb13

bb15:                                             ; preds = %bb13
  %indvar.next81 = add i64 %indvar80, 1           ; <i64> [#uses=2]
  %exitcond83 = icmp eq i64 %indvar.next81, 512   ; <i1> [#uses=1]
  br i1 %exitcond83, label %bb.nph22, label %bb.nph29

bb.nph22:                                         ; preds = %bb21, %bb15
  %indvar72 = phi i64 [ %indvar.next73, %bb21 ], [ 0, %bb15 ] ; <i64> [#uses=3]
  %i.323 = trunc i64 %indvar72 to i32             ; <i32> [#uses=1]
  %14 = sitofp i32 %i.323 to double               ; <double> [#uses=1]
  br label %bb19

bb19:                                             ; preds = %bb19, %bb.nph22
  %indvar69 = phi i64 [ 0, %bb.nph22 ], [ %indvar.next70, %bb19 ] ; <i64> [#uses=3]
  %scevgep74 = getelementptr [512 x [512 x double]]* @D, i64 0, i64 %indvar72, i64 %indvar69 ; <double*> [#uses=1]
  %j.321 = trunc i64 %indvar69 to i32             ; <i32> [#uses=1]
  %15 = sitofp i32 %j.321 to double               ; <double> [#uses=1]
  %16 = fmul double %14, %15                      ; <double> [#uses=1]
  %17 = fadd double %16, 2.000000e+00             ; <double> [#uses=1]
  %18 = fdiv double %17, 5.120000e+02             ; <double> [#uses=1]
  store double %18, double* %scevgep74, align 8
  %indvar.next70 = add i64 %indvar69, 1           ; <i64> [#uses=2]
  %exitcond71 = icmp eq i64 %indvar.next70, 512   ; <i1> [#uses=1]
  br i1 %exitcond71, label %bb21, label %bb19

bb21:                                             ; preds = %bb19
  %indvar.next73 = add i64 %indvar72, 1           ; <i64> [#uses=2]
  %exitcond75 = icmp eq i64 %indvar.next73, 512   ; <i1> [#uses=1]
  br i1 %exitcond75, label %bb.nph15, label %bb.nph22

bb.nph15:                                         ; preds = %bb27, %bb21
  %indvar64 = phi i64 [ %indvar.next65, %bb27 ], [ 0, %bb21 ] ; <i64> [#uses=3]
  %i.416 = trunc i64 %indvar64 to i32             ; <i32> [#uses=1]
  %19 = sitofp i32 %i.416 to double               ; <double> [#uses=1]
  br label %bb25

bb25:                                             ; preds = %bb25, %bb.nph15
  %indvar61 = phi i64 [ 0, %bb.nph15 ], [ %indvar.next62, %bb25 ] ; <i64> [#uses=3]
  %scevgep66 = getelementptr [512 x [512 x double]]* @E, i64 0, i64 %indvar64, i64 %indvar61 ; <double*> [#uses=1]
  %j.414 = trunc i64 %indvar61 to i32             ; <i32> [#uses=1]
  %20 = sitofp i32 %j.414 to double               ; <double> [#uses=1]
  %21 = fmul double %19, %20                      ; <double> [#uses=1]
  %22 = fadd double %21, 2.000000e+00             ; <double> [#uses=1]
  %23 = fdiv double %22, 5.120000e+02             ; <double> [#uses=1]
  store double %23, double* %scevgep66, align 8
  %indvar.next62 = add i64 %indvar61, 1           ; <i64> [#uses=2]
  %exitcond63 = icmp eq i64 %indvar.next62, 512   ; <i1> [#uses=1]
  br i1 %exitcond63, label %bb27, label %bb25

bb27:                                             ; preds = %bb25
  %indvar.next65 = add i64 %indvar64, 1           ; <i64> [#uses=2]
  %exitcond67 = icmp eq i64 %indvar.next65, 512   ; <i1> [#uses=1]
  br i1 %exitcond67, label %bb.nph8, label %bb.nph15

bb.nph8:                                          ; preds = %bb33, %bb27
  %indvar56 = phi i64 [ %indvar.next57, %bb33 ], [ 0, %bb27 ] ; <i64> [#uses=3]
  %i.59 = trunc i64 %indvar56 to i32              ; <i32> [#uses=1]
  %24 = sitofp i32 %i.59 to double                ; <double> [#uses=1]
  br label %bb31

bb31:                                             ; preds = %bb31, %bb.nph8
  %indvar53 = phi i64 [ 0, %bb.nph8 ], [ %indvar.next54, %bb31 ] ; <i64> [#uses=3]
  %scevgep58 = getelementptr [512 x [512 x double]]* @F, i64 0, i64 %indvar56, i64 %indvar53 ; <double*> [#uses=1]
  %j.57 = trunc i64 %indvar53 to i32              ; <i32> [#uses=1]
  %25 = sitofp i32 %j.57 to double                ; <double> [#uses=1]
  %26 = fmul double %24, %25                      ; <double> [#uses=1]
  %27 = fadd double %26, 2.000000e+00             ; <double> [#uses=1]
  %28 = fdiv double %27, 5.120000e+02             ; <double> [#uses=1]
  store double %28, double* %scevgep58, align 8
  %indvar.next54 = add i64 %indvar53, 1           ; <i64> [#uses=2]
  %exitcond55 = icmp eq i64 %indvar.next54, 512   ; <i1> [#uses=1]
  br i1 %exitcond55, label %bb33, label %bb31

bb33:                                             ; preds = %bb31
  %indvar.next57 = add i64 %indvar56, 1           ; <i64> [#uses=2]
  %exitcond59 = icmp eq i64 %indvar.next57, 512   ; <i1> [#uses=1]
  br i1 %exitcond59, label %bb.nph, label %bb.nph8

bb.nph:                                           ; preds = %bb39, %bb33
  %indvar49 = phi i64 [ %indvar.next50, %bb39 ], [ 0, %bb33 ] ; <i64> [#uses=3]
  %i.62 = trunc i64 %indvar49 to i32              ; <i32> [#uses=1]
  %29 = sitofp i32 %i.62 to double                ; <double> [#uses=1]
  br label %bb37

bb37:                                             ; preds = %bb37, %bb.nph
  %indvar = phi i64 [ 0, %bb.nph ], [ %indvar.next, %bb37 ] ; <i64> [#uses=3]
  %scevgep = getelementptr [512 x [512 x double]]* @G, i64 0, i64 %indvar49, i64 %indvar ; <double*> [#uses=1]
  %j.61 = trunc i64 %indvar to i32                ; <i32> [#uses=1]
  %30 = sitofp i32 %j.61 to double                ; <double> [#uses=1]
  %31 = fmul double %29, %30                      ; <double> [#uses=1]
  %32 = fadd double %31, 2.000000e+00             ; <double> [#uses=1]
  %33 = fdiv double %32, 5.120000e+02             ; <double> [#uses=1]
  store double %33, double* %scevgep, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 512       ; <i1> [#uses=1]
  br i1 %exitcond, label %bb39, label %bb37

bb39:                                             ; preds = %bb37
  %indvar.next50 = add i64 %indvar49, 1           ; <i64> [#uses=2]
  %exitcond51 = icmp eq i64 %indvar.next50, 512   ; <i1> [#uses=1]
  br i1 %exitcond51, label %return, label %bb.nph

return:                                           ; preds = %bb39
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
  %scevgep = getelementptr [512 x [512 x double]]* @G, i64 0, i64 %indvar7, i64 %indvar ; <double*> [#uses=1]
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
bb.nph68.bb.nph68.split_crit_edge:
  br label %bb.nph54.split.us

bb.nph54.split.us:                                ; preds = %bb6, %bb.nph68.bb.nph68.split_crit_edge
  %i.055 = phi i32 [ 0, %bb.nph68.bb.nph68.split_crit_edge ], [ %4, %bb6 ] ; <i32> [#uses=2]
  %tmp104 = sext i32 %i.055 to i64                ; <i64> [#uses=2]
  br label %bb.nph46.us

bb4.us:                                           ; preds = %bb2.us
  store double %3, double* %scevgep109
  %indvar.next102 = add i64 %indvar101, 1         ; <i64> [#uses=2]
  %exitcond106 = icmp eq i64 %indvar.next102, 512 ; <i1> [#uses=1]
  br i1 %exitcond106, label %bb6, label %bb.nph46.us

bb2.us:                                           ; preds = %bb.nph46.us, %bb2.us
  %indvar98 = phi i64 [ 0, %bb.nph46.us ], [ %indvar.next99, %bb2.us ] ; <i64> [#uses=3]
  %.tmp.048.us = phi double [ 0.000000e+00, %bb.nph46.us ], [ %3, %bb2.us ] ; <double> [#uses=1]
  %scevgep103 = getelementptr [512 x [512 x double]]* @B, i64 0, i64 %indvar98, i64 %indvar101 ; <double*> [#uses=1]
  %scevgep105 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %tmp104, i64 %indvar98 ; <double*> [#uses=1]
  %0 = load double* %scevgep105, align 8          ; <double> [#uses=1]
  %1 = load double* %scevgep103, align 8          ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=1]
  %3 = fadd double %.tmp.048.us, %2               ; <double> [#uses=2]
  %indvar.next99 = add i64 %indvar98, 1           ; <i64> [#uses=2]
  %exitcond100 = icmp eq i64 %indvar.next99, 512  ; <i1> [#uses=1]
  br i1 %exitcond100, label %bb4.us, label %bb2.us

bb.nph46.us:                                      ; preds = %bb4.us, %bb.nph54.split.us
  %indvar101 = phi i64 [ %indvar.next102, %bb4.us ], [ 0, %bb.nph54.split.us ] ; <i64> [#uses=3]
  %scevgep109 = getelementptr [512 x [512 x double]]* @E, i64 0, i64 %tmp104, i64 %indvar101 ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep109, align 8
  br label %bb2.us

bb6:                                              ; preds = %bb4.us
  %4 = add nsw i32 %i.055, 1                      ; <i32> [#uses=2]
  %5 = icmp slt i32 %4, 512                       ; <i1> [#uses=1]
  br i1 %5, label %bb.nph54.split.us, label %bb.nph30.split.us

bb.nph30.split.us:                                ; preds = %bb15, %bb6
  %i.131 = phi i32 [ %10, %bb15 ], [ 0, %bb6 ]    ; <i32> [#uses=2]
  %tmp87 = sext i32 %i.131 to i64                 ; <i64> [#uses=2]
  br label %bb.nph22.us

bb13.us:                                          ; preds = %bb11.us
  store double %9, double* %scevgep92
  %indvar.next85 = add i64 %indvar84, 1           ; <i64> [#uses=2]
  %exitcond89 = icmp eq i64 %indvar.next85, 512   ; <i1> [#uses=1]
  br i1 %exitcond89, label %bb15, label %bb.nph22.us

bb11.us:                                          ; preds = %bb.nph22.us, %bb11.us
  %indvar81 = phi i64 [ 0, %bb.nph22.us ], [ %indvar.next82, %bb11.us ] ; <i64> [#uses=3]
  %.tmp.024.us = phi double [ 0.000000e+00, %bb.nph22.us ], [ %9, %bb11.us ] ; <double> [#uses=1]
  %scevgep86 = getelementptr [512 x [512 x double]]* @D, i64 0, i64 %indvar81, i64 %indvar84 ; <double*> [#uses=1]
  %scevgep88 = getelementptr [512 x [512 x double]]* @C, i64 0, i64 %tmp87, i64 %indvar81 ; <double*> [#uses=1]
  %6 = load double* %scevgep88, align 8           ; <double> [#uses=1]
  %7 = load double* %scevgep86, align 8           ; <double> [#uses=1]
  %8 = fmul double %6, %7                         ; <double> [#uses=1]
  %9 = fadd double %.tmp.024.us, %8               ; <double> [#uses=2]
  %indvar.next82 = add i64 %indvar81, 1           ; <i64> [#uses=2]
  %exitcond83 = icmp eq i64 %indvar.next82, 512   ; <i1> [#uses=1]
  br i1 %exitcond83, label %bb13.us, label %bb11.us

bb.nph22.us:                                      ; preds = %bb13.us, %bb.nph30.split.us
  %indvar84 = phi i64 [ %indvar.next85, %bb13.us ], [ 0, %bb.nph30.split.us ] ; <i64> [#uses=3]
  %scevgep92 = getelementptr [512 x [512 x double]]* @F, i64 0, i64 %tmp87, i64 %indvar84 ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep92, align 8
  br label %bb11.us

bb15:                                             ; preds = %bb13.us
  %10 = add nsw i32 %i.131, 1                     ; <i32> [#uses=2]
  %11 = icmp slt i32 %10, 512                     ; <i1> [#uses=1]
  br i1 %11, label %bb.nph30.split.us, label %bb.nph6.split.us

bb.nph6.split.us:                                 ; preds = %bb24, %bb15
  %i.27 = phi i32 [ %16, %bb24 ], [ 0, %bb15 ]    ; <i32> [#uses=2]
  %tmp = sext i32 %i.27 to i64                    ; <i64> [#uses=2]
  br label %bb.nph.us

bb22.us:                                          ; preds = %bb20.us
  store double %15, double* %scevgep75
  %indvar.next70 = add i64 %indvar69, 1           ; <i64> [#uses=2]
  %exitcond72 = icmp eq i64 %indvar.next70, 512   ; <i1> [#uses=1]
  br i1 %exitcond72, label %bb24, label %bb.nph.us

bb20.us:                                          ; preds = %bb.nph.us, %bb20.us
  %indvar = phi i64 [ 0, %bb.nph.us ], [ %indvar.next, %bb20.us ] ; <i64> [#uses=3]
  %.tmp.0.us = phi double [ 0.000000e+00, %bb.nph.us ], [ %15, %bb20.us ] ; <double> [#uses=1]
  %scevgep = getelementptr [512 x [512 x double]]* @F, i64 0, i64 %indvar, i64 %indvar69 ; <double*> [#uses=1]
  %scevgep71 = getelementptr [512 x [512 x double]]* @E, i64 0, i64 %tmp, i64 %indvar ; <double*> [#uses=1]
  %12 = load double* %scevgep71, align 8          ; <double> [#uses=1]
  %13 = load double* %scevgep, align 8            ; <double> [#uses=1]
  %14 = fmul double %12, %13                      ; <double> [#uses=1]
  %15 = fadd double %.tmp.0.us, %14               ; <double> [#uses=2]
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 512       ; <i1> [#uses=1]
  br i1 %exitcond, label %bb22.us, label %bb20.us

bb.nph.us:                                        ; preds = %bb22.us, %bb.nph6.split.us
  %indvar69 = phi i64 [ %indvar.next70, %bb22.us ], [ 0, %bb.nph6.split.us ] ; <i64> [#uses=3]
  %scevgep75 = getelementptr [512 x [512 x double]]* @G, i64 0, i64 %tmp, i64 %indvar69 ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep75, align 8
  br label %bb20.us

bb24:                                             ; preds = %bb22.us
  %16 = add nsw i32 %i.27, 1                      ; <i32> [#uses=2]
  %17 = icmp slt i32 %16, 512                     ; <i1> [#uses=1]
  br i1 %17, label %bb.nph6.split.us, label %return

return:                                           ; preds = %bb24
  ret void
}

define i32 @main(i32 %argc, i8** nocapture %argv) nounwind {
entry:
  tail call void @init_array() nounwind
  tail call void @scop_func() nounwind
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
  %indvar.next8.i = add i64 %indvar7.i, 1         ; <i64> [#uses=2]
  %exitcond3 = icmp eq i64 %indvar.next8.i, 512   ; <i1> [#uses=1]
  br i1 %exitcond3, label %print_array.exit, label %bb.nph.us.i

bb5.us.i:                                         ; preds = %bb4.us.i, %bb3.us.i
  %indvar.next.i = add i64 %indvar.i, 1           ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next.i, 512     ; <i1> [#uses=1]
  br i1 %exitcond, label %bb7.us.i, label %bb3.us.i

bb3.us.i:                                         ; preds = %bb.nph.us.i, %bb5.us.i
  %indvar.i = phi i64 [ 0, %bb.nph.us.i ], [ %indvar.next.i, %bb5.us.i ] ; <i64> [#uses=3]
  %scevgep.i = getelementptr [512 x [512 x double]]* @G, i64 0, i64 %indvar7.i, i64 %indvar.i ; <double*> [#uses=1]
  %tmp5 = add i64 %tmp4, %indvar.i                ; <i64> [#uses=1]
  %tmp10.i = trunc i64 %tmp5 to i32               ; <i32> [#uses=1]
  %7 = load double* %scevgep.i, align 8           ; <double> [#uses=1]
  %8 = load %struct._IO_FILE** @stderr, align 8   ; <%struct._IO_FILE*> [#uses=1]
  %9 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %8, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %7) nounwind ; <i32> [#uses=0]
  %10 = srem i32 %tmp10.i, 80                     ; <i32> [#uses=1]
  %11 = icmp eq i32 %10, 20                       ; <i1> [#uses=1]
  br i1 %11, label %bb4.us.i, label %bb5.us.i

bb4.us.i:                                         ; preds = %bb3.us.i
  %12 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %13 = bitcast %struct._IO_FILE* %12 to i8*      ; <i8*> [#uses=1]
  %14 = tail call i32 @fputc(i32 10, i8* %13) nounwind ; <i32> [#uses=0]
  br label %bb5.us.i

bb.nph.us.i:                                      ; preds = %bb7.us.i, %bb.i
  %indvar7.i = phi i64 [ %indvar.next8.i, %bb7.us.i ], [ 0, %bb.i ] ; <i64> [#uses=3]
  %tmp4 = shl i64 %indvar7.i, 9                   ; <i64> [#uses=1]
  br label %bb3.us.i

print_array.exit:                                 ; preds = %bb7.us.i, %bb.i, %entry
  ret i32 0
}
