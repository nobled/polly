; RUN: opt  -indvars -polly-scop-detect -print-top-scop-only -polly-print-scop -analyze  %s | FileCheck %s
; XFAIL: *
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }

@float_n = global double 0x41B32863F6028F5C       ; <double*> [#uses=3]
@eps = global double 5.000000e-03                 ; <double*> [#uses=1]
@data = common global [501 x [501 x double]] zeroinitializer, align 32 ; <[501 x [501 x double]]*> [#uses=7]
@symmat = common global [501 x [501 x double]] zeroinitializer, align 32 ; <[501 x [501 x double]]*> [#uses=6]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
@mean = common global [501 x double] zeroinitializer, align 32 ; <[501 x double]*> [#uses=3]
@stddev = common global [501 x double] zeroinitializer, align 32 ; <[501 x double]*> [#uses=2]

define void @init_array() nounwind inlinehint {
bb.nph6.bb.nph6.split_crit_edge:
  br label %bb.nph

bb.nph:                                           ; preds = %bb3, %bb.nph6.bb.nph6.split_crit_edge
  %indvar7 = phi i64 [ 0, %bb.nph6.bb.nph6.split_crit_edge ], [ %indvar.next8, %bb3 ] ; <i64> [#uses=3]
  %i.02 = trunc i64 %indvar7 to i32               ; <i32> [#uses=1]
  %0 = sitofp i32 %i.02 to double                 ; <double> [#uses=1]
  br label %bb1

bb1:                                              ; preds = %bb1, %bb.nph
  %indvar = phi i64 [ 0, %bb.nph ], [ %indvar.next, %bb1 ] ; <i64> [#uses=3]
  %scevgep = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %indvar7, i64 %indvar ; <double*> [#uses=1]
  %j.01 = trunc i64 %indvar to i32                ; <i32> [#uses=1]
  %1 = sitofp i32 %j.01 to double                 ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=1]
  %3 = fdiv double %2, 5.000000e+02               ; <double> [#uses=1]
  store double %3, double* %scevgep, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 501       ; <i1> [#uses=1]
  br i1 %exitcond, label %bb3, label %bb1

bb3:                                              ; preds = %bb1
  %indvar.next8 = add i64 %indvar7, 1             ; <i64> [#uses=2]
  %exitcond9 = icmp eq i64 %indvar.next8, 501     ; <i1> [#uses=1]
  br i1 %exitcond9, label %return, label %bb.nph

return:                                           ; preds = %bb3
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
  br i1 %3, label %bb6.preheader, label %return

bb3:                                              ; preds = %bb6.preheader, %bb5
  %indvar = phi i64 [ %indvar.next, %bb5 ], [ 0, %bb6.preheader ] ; <i64> [#uses=3]
  %tmp13 = add i64 %tmp12, %indvar                ; <i64> [#uses=1]
  %tmp10 = trunc i64 %tmp13 to i32                ; <i32> [#uses=1]
  %scevgep = getelementptr [501 x [501 x double]]* @symmat, i64 0, i64 %indvar7, i64 %indvar ; <double*> [#uses=1]
  %4 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %5 = load %struct._IO_FILE** @stderr, align 8   ; <%struct._IO_FILE*> [#uses=1]
  %6 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %5, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %4) nounwind ; <i32> [#uses=0]
  %7 = srem i32 %tmp10, 80                        ; <i32> [#uses=1]
  %8 = icmp eq i32 %7, 20                         ; <i1> [#uses=1]
  br i1 %8, label %bb4, label %bb5

bb4:                                              ; preds = %bb3
  %9 = load %struct._IO_FILE** @stderr, align 8   ; <%struct._IO_FILE*> [#uses=1]
  %10 = bitcast %struct._IO_FILE* %9 to i8*       ; <i8*> [#uses=1]
  %11 = tail call i32 @fputc(i32 10, i8* %10) nounwind ; <i32> [#uses=0]
  br label %bb5

bb5:                                              ; preds = %bb4, %bb3
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 501       ; <i1> [#uses=1]
  br i1 %exitcond, label %bb7, label %bb3

bb7:                                              ; preds = %bb5
  %indvar.next8 = add i64 %indvar7, 1             ; <i64> [#uses=2]
  %exitcond11 = icmp eq i64 %indvar.next8, 501    ; <i1> [#uses=1]
  br i1 %exitcond11, label %bb9, label %bb6.preheader

bb6.preheader:                                    ; preds = %bb7, %bb
  %indvar7 = phi i64 [ %indvar.next8, %bb7 ], [ 0, %bb ] ; <i64> [#uses=3]
  %tmp12 = mul i64 %indvar7, 500                  ; <i64> [#uses=1]
  br label %bb3

bb9:                                              ; preds = %bb7
  %12 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %13 = bitcast %struct._IO_FILE* %12 to i8*      ; <i8*> [#uses=1]
  %14 = tail call i32 @fputc(i32 10, i8* %13) nounwind ; <i32> [#uses=0]
  ret void

return:                                           ; preds = %bb, %entry
  ret void
}

declare i32 @fprintf(%struct._IO_FILE* noalias nocapture, i8* noalias nocapture, ...) nounwind

declare i32 @fputc(i32, i8* nocapture) nounwind

define void @scop_func() nounwind {
bb.nph29.bb.nph29.split_crit_edge:
  br label %bb2.preheader

bb1:                                              ; preds = %bb2.preheader, %bb1
  %indvar52 = phi i64 [ %tmp64, %bb1 ], [ 0, %bb2.preheader ] ; <i64> [#uses=1]
  %tmp64 = add i64 %indvar52, 1                   ; <i64> [#uses=5]
  %scevgep59 = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %tmp63, i64 %tmp64 ; <double*> [#uses=3]
  %scevgep60 = getelementptr [501 x double]* @mean, i64 0, i64 %tmp64 ; <double*> [#uses=1]
  %scevgep61 = getelementptr [501 x double]* @stddev, i64 0, i64 %tmp64 ; <double*> [#uses=1]
  %0 = load double* %scevgep59, align 8           ; <double> [#uses=1]
  %1 = load double* %scevgep60, align 8           ; <double> [#uses=1]
  %2 = fsub double %0, %1                         ; <double> [#uses=2]
  store double %2, double* %scevgep59, align 8
  %3 = load double* @float_n, align 8             ; <double> [#uses=1]
  %4 = tail call double @sqrt(double %3) nounwind readonly ; <double> [#uses=1]
  %5 = load double* %scevgep61, align 8           ; <double> [#uses=1]
  %6 = fmul double %4, %5                         ; <double> [#uses=1]
  %7 = fdiv double %2, %6                         ; <double> [#uses=1]
  store double %7, double* %scevgep59, align 8
  %exitcond54 = icmp eq i64 %tmp64, 500           ; <i1> [#uses=1]
  br i1 %exitcond54, label %bb3, label %bb1

bb3:                                              ; preds = %bb1
  %exitcond62 = icmp eq i64 %tmp63, 500           ; <i1> [#uses=1]
  br i1 %exitcond62, label %bb6, label %bb2.preheader

bb2.preheader:                                    ; preds = %bb3, %bb.nph29.bb.nph29.split_crit_edge
  %indvar55 = phi i64 [ 0, %bb.nph29.bb.nph29.split_crit_edge ], [ %tmp63, %bb3 ] ; <i64> [#uses=1]
  %tmp63 = add i64 %indvar55, 1                   ; <i64> [#uses=3]
  br label %bb1

bb6:                                              ; preds = %bb12, %bb3
  %j1.09 = phi i32 [ %j2.03, %bb12 ], [ 1, %bb3 ] ; <i32> [#uses=3]
  %8 = sext i32 %j1.09 to i64                     ; <i64> [#uses=5]
  %9 = getelementptr inbounds [501 x [501 x double]]* @symmat, i64 0, i64 %8, i64 %8 ; <double*> [#uses=1]
  store double 1.000000e+00, double* %9, align 8
  %j2.03 = add i32 %j1.09, 1                      ; <i32> [#uses=4]
  %10 = icmp slt i32 %j2.03, 501                  ; <i1> [#uses=1]
  br i1 %10, label %bb.nph8.split.us, label %bb12

bb.nph8.split.us:                                 ; preds = %bb6
  %tmp37 = sub i32 499, %j1.09                    ; <i32> [#uses=1]
  %tmp38 = zext i32 %tmp37 to i64                 ; <i64> [#uses=1]
  %tmp39 = add i64 %tmp38, 1                      ; <i64> [#uses=1]
  %tmp42 = sext i32 %j2.03 to i64                 ; <i64> [#uses=1]
  br label %bb.nph.us

bb10.us:                                          ; preds = %bb8.us
  store double %15, double* %scevgep47
  store double %15, double* %scevgep46, align 8
  %indvar.next31 = add i64 %indvar30, 1           ; <i64> [#uses=2]
  %exitcond40 = icmp eq i64 %indvar.next31, %tmp39 ; <i1> [#uses=1]
  br i1 %exitcond40, label %bb12, label %bb.nph.us

bb8.us:                                           ; preds = %bb.nph.us, %bb8.us
  %indvar = phi i64 [ 0, %bb.nph.us ], [ %tmp41, %bb8.us ] ; <i64> [#uses=1]
  %11 = phi double [ 0.000000e+00, %bb.nph.us ], [ %15, %bb8.us ] ; <double> [#uses=1]
  %tmp41 = add i64 %indvar, 1                     ; <i64> [#uses=4]
  %scevgep = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %tmp41, i64 %tmp43 ; <double*> [#uses=1]
  %scevgep36 = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %tmp41, i64 %8 ; <double*> [#uses=1]
  %12 = load double* %scevgep36, align 8          ; <double> [#uses=1]
  %13 = load double* %scevgep, align 8            ; <double> [#uses=1]
  %14 = fmul double %12, %13                      ; <double> [#uses=1]
  %15 = fadd double %11, %14                      ; <double> [#uses=3]
  %exitcond = icmp eq i64 %tmp41, 500             ; <i1> [#uses=1]
  br i1 %exitcond, label %bb10.us, label %bb8.us

bb.nph.us:                                        ; preds = %bb10.us, %bb.nph8.split.us
  %indvar30 = phi i64 [ %indvar.next31, %bb10.us ], [ 0, %bb.nph8.split.us ] ; <i64> [#uses=2]
  %tmp43 = add i64 %tmp42, %indvar30              ; <i64> [#uses=3]
  %scevgep46 = getelementptr [501 x [501 x double]]* @symmat, i64 0, i64 %tmp43, i64 %8 ; <double*> [#uses=1]
  %scevgep47 = getelementptr [501 x [501 x double]]* @symmat, i64 0, i64 %8, i64 %tmp43 ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep47, align 8
  br label %bb8.us

bb12:                                             ; preds = %bb10.us, %bb6
  %16 = icmp slt i32 %j2.03, 500                  ; <i1> [#uses=1]
  br i1 %16, label %bb6, label %return

return:                                           ; preds = %bb12
  ret void
}

declare double @sqrt(double) nounwind readonly

define i32 @main(i32 %argc, i8** nocapture %argv) nounwind {
entry:
  br label %bb.nph.i

bb.nph.i:                                         ; preds = %bb3.i, %entry
  %indvar7.i = phi i64 [ 0, %entry ], [ %indvar.next8.i, %bb3.i ] ; <i64> [#uses=3]
  %i.02.i = trunc i64 %indvar7.i to i32           ; <i32> [#uses=1]
  %0 = sitofp i32 %i.02.i to double               ; <double> [#uses=1]
  br label %bb1.i

bb1.i:                                            ; preds = %bb1.i, %bb.nph.i
  %indvar.i = phi i64 [ 0, %bb.nph.i ], [ %indvar.next.i, %bb1.i ] ; <i64> [#uses=3]
  %scevgep.i = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %indvar7.i, i64 %indvar.i ; <double*> [#uses=1]
  %j.01.i = trunc i64 %indvar.i to i32            ; <i32> [#uses=1]
  %1 = sitofp i32 %j.01.i to double               ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=1]
  %3 = fdiv double %2, 5.000000e+02               ; <double> [#uses=1]
  store double %3, double* %scevgep.i, align 8
  %indvar.next.i = add i64 %indvar.i, 1           ; <i64> [#uses=2]
  %exitcond62 = icmp eq i64 %indvar.next.i, 501   ; <i1> [#uses=1]
  br i1 %exitcond62, label %bb3.i, label %bb1.i

bb3.i:                                            ; preds = %bb1.i
  %indvar.next8.i = add i64 %indvar7.i, 1         ; <i64> [#uses=2]
  %exitcond64 = icmp eq i64 %indvar.next8.i, 501  ; <i1> [#uses=1]
  br i1 %exitcond64, label %bb.nph25.split.us, label %bb.nph.i

bb.nph25.split.us:                                ; preds = %bb3.i
  %4 = load double* @float_n, align 8             ; <double> [#uses=1]
  br label %bb.nph17.us

bb3.us:                                           ; preds = %bb1.us
  %5 = fdiv double %8, %4                         ; <double> [#uses=1]
  store double %5, double* %scevgep34, align 8
  %exitcond30 = icmp eq i64 %tmp32, 500           ; <i1> [#uses=1]
  br i1 %exitcond30, label %bb.nph, label %bb.nph17.us

bb1.us:                                           ; preds = %bb.nph17.us, %bb1.us
  %indvar = phi i64 [ 0, %bb.nph17.us ], [ %tmp31, %bb1.us ] ; <i64> [#uses=1]
  %6 = phi double [ 0.000000e+00, %bb.nph17.us ], [ %8, %bb1.us ] ; <double> [#uses=1]
  %tmp31 = add i64 %indvar, 1                     ; <i64> [#uses=3]
  %scevgep = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %tmp31, i64 %tmp32 ; <double*> [#uses=1]
  %7 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %8 = fadd double %6, %7                         ; <double> [#uses=2]
  %exitcond = icmp eq i64 %tmp31, 500             ; <i1> [#uses=1]
  br i1 %exitcond, label %bb3.us, label %bb1.us

bb.nph17.us:                                      ; preds = %bb3.us, %bb.nph25.split.us
  %indvar27 = phi i64 [ %tmp32, %bb3.us ], [ 0, %bb.nph25.split.us ] ; <i64> [#uses=1]
  %tmp32 = add i64 %indvar27, 1                   ; <i64> [#uses=4]
  %scevgep34 = getelementptr [501 x double]* @mean, i64 0, i64 %tmp32 ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep34, align 8
  br label %bb1.us

bb.nph:                                           ; preds = %bb9, %bb3.us
  %indvar48 = phi i64 [ %tmp54, %bb9 ], [ 0, %bb3.us ] ; <i64> [#uses=1]
  %tmp54 = add i64 %indvar48, 1                   ; <i64> [#uses=5]
  %scevgep56 = getelementptr [501 x double]* @stddev, i64 0, i64 %tmp54 ; <double*> [#uses=4]
  store double 0.000000e+00, double* %scevgep56, align 8
  %scevgep57 = getelementptr [501 x double]* @mean, i64 0, i64 %tmp54 ; <double*> [#uses=1]
  %9 = load double* %scevgep57, align 8           ; <double> [#uses=1]
  br label %bb7

bb7:                                              ; preds = %bb7, %bb.nph
  %indvar45 = phi i64 [ 0, %bb.nph ], [ %tmp53, %bb7 ] ; <i64> [#uses=1]
  %10 = phi double [ 0.000000e+00, %bb.nph ], [ %14, %bb7 ] ; <double> [#uses=1]
  %tmp53 = add i64 %indvar45, 1                   ; <i64> [#uses=3]
  %scevgep51 = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %tmp53, i64 %tmp54 ; <double*> [#uses=1]
  %11 = load double* %scevgep51, align 8          ; <double> [#uses=1]
  %12 = fsub double %11, %9                       ; <double> [#uses=2]
  %13 = fmul double %12, %12                      ; <double> [#uses=1]
  %14 = fadd double %10, %13                      ; <double> [#uses=2]
  %exitcond47 = icmp eq i64 %tmp53, 500           ; <i1> [#uses=1]
  br i1 %exitcond47, label %bb9, label %bb7

bb9:                                              ; preds = %bb7
  %15 = load double* @float_n, align 8            ; <double> [#uses=1]
  %16 = fdiv double %14, %15                      ; <double> [#uses=2]
  store double %16, double* %scevgep56, align 8
  %17 = tail call double @sqrt(double %16) nounwind readonly ; <double> [#uses=3]
  store double %17, double* %scevgep56, align 8
  %18 = load double* @eps, align 8                ; <double> [#uses=1]
  %19 = fcmp ugt double %17, %18                  ; <i1> [#uses=1]
  %iftmp.57.0 = select i1 %19, double %17, double 1.000000e+00 ; <double> [#uses=1]
  store double %iftmp.57.0, double* %scevgep56, align 8
  %exitcond52 = icmp eq i64 %tmp54, 500           ; <i1> [#uses=1]
  br i1 %exitcond52, label %bb14, label %bb.nph

bb14:                                             ; preds = %bb9
  tail call void @scop_func() nounwind
  store double 1.000000e+00, double* getelementptr inbounds ([501 x [501 x double]]* @symmat, i64 0, i64 500, i64 500), align 32
  %20 = icmp sgt i32 %argc, 42                    ; <i1> [#uses=1]
  br i1 %20, label %bb.i, label %print_array.exit

bb.i:                                             ; preds = %bb14
  %21 = load i8** %argv, align 1                  ; <i8*> [#uses=1]
  %22 = load i8* %21, align 1                     ; <i8> [#uses=1]
  %23 = icmp eq i8 %22, 0                         ; <i1> [#uses=1]
  br i1 %23, label %bb6.preheader.i, label %print_array.exit

bb3.i3:                                           ; preds = %bb6.preheader.i, %bb5.i
  %indvar.i1 = phi i64 [ %indvar.next.i4, %bb5.i ], [ 0, %bb6.preheader.i ] ; <i64> [#uses=3]
  %scevgep.i2 = getelementptr [501 x [501 x double]]* @symmat, i64 0, i64 %indvar7.i7, i64 %indvar.i1 ; <double*> [#uses=1]
  %tmp42 = add i64 %tmp41, %indvar.i1             ; <i64> [#uses=1]
  %tmp10.i = trunc i64 %tmp42 to i32              ; <i32> [#uses=1]
  %24 = load double* %scevgep.i2, align 8         ; <double> [#uses=1]
  %25 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %26 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %25, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %24) nounwind ; <i32> [#uses=0]
  %27 = srem i32 %tmp10.i, 80                     ; <i32> [#uses=1]
  %28 = icmp eq i32 %27, 20                       ; <i1> [#uses=1]
  br i1 %28, label %bb4.i, label %bb5.i

bb4.i:                                            ; preds = %bb3.i3
  %29 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %30 = bitcast %struct._IO_FILE* %29 to i8*      ; <i8*> [#uses=1]
  %31 = tail call i32 @fputc(i32 10, i8* %30) nounwind ; <i32> [#uses=0]
  br label %bb5.i

bb5.i:                                            ; preds = %bb4.i, %bb3.i3
  %indvar.next.i4 = add i64 %indvar.i1, 1         ; <i64> [#uses=2]
  %exitcond35 = icmp eq i64 %indvar.next.i4, 501  ; <i1> [#uses=1]
  br i1 %exitcond35, label %bb7.i, label %bb3.i3

bb7.i:                                            ; preds = %bb5.i
  %indvar.next8.i6 = add i64 %indvar7.i7, 1       ; <i64> [#uses=2]
  %exitcond39 = icmp eq i64 %indvar.next8.i6, 501 ; <i1> [#uses=1]
  br i1 %exitcond39, label %bb9.i, label %bb6.preheader.i

bb6.preheader.i:                                  ; preds = %bb7.i, %bb.i
  %indvar7.i7 = phi i64 [ %indvar.next8.i6, %bb7.i ], [ 0, %bb.i ] ; <i64> [#uses=3]
  %tmp41 = mul i64 %indvar7.i7, 500               ; <i64> [#uses=1]
  br label %bb3.i3

bb9.i:                                            ; preds = %bb7.i
  %32 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %33 = bitcast %struct._IO_FILE* %32 to i8*      ; <i8*> [#uses=1]
  %34 = tail call i32 @fputc(i32 10, i8* %33) nounwind ; <i32> [#uses=0]
  ret i32 0

print_array.exit:                                 ; preds = %bb.i, %bb14
  ret i32 0
}
