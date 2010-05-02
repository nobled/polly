; RUN: opt -indvars -polly-scop-detect  -analyze %s | FileCheck %s
; XFAIL: *
; ModuleID = '<stdin>'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }

@float_n = global double 0x41B32863F6028F5C       ; <double*> [#uses=3]
@eps = global double 5.000000e-03                 ; <double*> [#uses=1]
@data = common global [501 x [501 x double]] zeroinitializer, align 32 ; <[501 x [501 x double]]*> [#uses=7]
@symmat = common global [501 x [501 x double]] zeroinitializer, align 32 ; <[501 x [501 x double]]*> [#uses=8]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
@mean = common global [501 x double] zeroinitializer, align 32 ; <[501 x double]*> [#uses=3]
@stddev = common global [501 x double] zeroinitializer, align 32 ; <[501 x double]*> [#uses=2]

define void @init_array() nounwind inlinehint {
bb.nph7.bb.nph7.split_crit_edge:
  br label %bb.nph

bb.nph:                                           ; preds = %bb3, %bb.nph7.bb.nph7.split_crit_edge
  %indvar8 = phi i64 [ 0, %bb.nph7.bb.nph7.split_crit_edge ], [ %indvar.next9, %bb3 ] ; <i64> [#uses=3]
  %storemerge3 = trunc i64 %indvar8 to i32        ; <i32> [#uses=1]
  %0 = sitofp i32 %storemerge3 to double          ; <double> [#uses=1]
  br label %bb1

bb1:                                              ; preds = %bb1, %bb.nph
  %indvar = phi i64 [ 0, %bb.nph ], [ %indvar.next, %bb1 ] ; <i64> [#uses=3]
  %scevgep = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %indvar8, i64 %indvar ; <double*> [#uses=1]
  %storemerge12 = trunc i64 %indvar to i32        ; <i32> [#uses=1]
  %1 = sitofp i32 %storemerge12 to double         ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=1]
  %3 = fdiv double %2, 5.000000e+02               ; <double> [#uses=1]
  store double %3, double* %scevgep, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 501       ; <i1> [#uses=1]
  br i1 %exitcond, label %bb3, label %bb1

bb3:                                              ; preds = %bb1
  %indvar.next9 = add i64 %indvar8, 1             ; <i64> [#uses=2]
  %exitcond10 = icmp eq i64 %indvar.next9, 501    ; <i1> [#uses=1]
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
  br i1 %3, label %bb6.preheader, label %return

bb3:                                              ; preds = %bb6.preheader, %bb5
  %indvar = phi i64 [ %indvar.next, %bb5 ], [ 0, %bb6.preheader ] ; <i64> [#uses=3]
  %tmp14 = add i64 %tmp13, %indvar                ; <i64> [#uses=1]
  %tmp11 = trunc i64 %tmp14 to i32                ; <i32> [#uses=1]
  %scevgep = getelementptr [501 x [501 x double]]* @symmat, i64 0, i64 %indvar8, i64 %indvar ; <double*> [#uses=1]
  %4 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %5 = load %struct._IO_FILE** @stderr, align 8   ; <%struct._IO_FILE*> [#uses=1]
  %6 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %5, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %4) nounwind ; <i32> [#uses=0]
  %7 = srem i32 %tmp11, 80                        ; <i32> [#uses=1]
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
  %indvar.next9 = add i64 %indvar8, 1             ; <i64> [#uses=2]
  %exitcond12 = icmp eq i64 %indvar.next9, 501    ; <i1> [#uses=1]
  br i1 %exitcond12, label %bb9, label %bb6.preheader

bb6.preheader:                                    ; preds = %bb7, %bb
  %indvar8 = phi i64 [ %indvar.next9, %bb7 ], [ 0, %bb ] ; <i64> [#uses=3]
  %tmp13 = mul i64 %indvar8, 500                  ; <i64> [#uses=1]
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

define void @scop_func(i64 %m, i64 %n) nounwind {
entry:
  %0 = icmp slt i64 %n, 1                         ; <i1> [#uses=2]
  %1 = icmp slt i64 %m, 1                         ; <i1> [#uses=1]
  %or.cond = or i1 %0, %1                         ; <i1> [#uses=1]
  br i1 %or.cond, label %bb13.preheader, label %bb2.preheader

bb1:                                              ; preds = %bb2.preheader, %bb1
  %indvar52 = phi i64 [ %tmp63, %bb1 ], [ 0, %bb2.preheader ] ; <i64> [#uses=2]
  %tmp63 = add i64 %indvar52, 1                   ; <i64> [#uses=4]
  %scevgep59 = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %tmp62, i64 %tmp63 ; <double*> [#uses=3]
  %scevgep55 = getelementptr [501 x double]* @stddev, i64 0, i64 %tmp63 ; <double*> [#uses=1]
  %scevgep60 = getelementptr [501 x double]* @mean, i64 0, i64 %tmp63 ; <double*> [#uses=1]
  %2 = load double* %scevgep59, align 8           ; <double> [#uses=1]
  %3 = load double* %scevgep60, align 8           ; <double> [#uses=1]
  %4 = fsub double %2, %3                         ; <double> [#uses=2]
  store double %4, double* %scevgep59, align 8
  %5 = load double* @float_n, align 8             ; <double> [#uses=1]
  %6 = tail call double @sqrt(double %5) nounwind readonly ; <double> [#uses=1]
  %7 = load double* %scevgep55, align 8           ; <double> [#uses=1]
  %8 = fmul double %6, %7                         ; <double> [#uses=1]
  %9 = fdiv double %4, %8                         ; <double> [#uses=1]
  store double %9, double* %scevgep59, align 8
  %tmp61 = add i64 %indvar52, 2                   ; <i64> [#uses=1]
  %10 = icmp sgt i64 %tmp61, %m                   ; <i1> [#uses=1]
  br i1 %10, label %bb3, label %bb1

bb3:                                              ; preds = %bb1
  %11 = icmp sgt i64 %tmp65, %n                   ; <i1> [#uses=1]
  br i1 %11, label %bb13.preheader, label %bb2.preheader

bb2.preheader:                                    ; preds = %bb3, %entry
  %indvar56 = phi i64 [ %tmp62, %bb3 ], [ 0, %entry ] ; <i64> [#uses=2]
  %tmp62 = add i64 %indvar56, 1                   ; <i64> [#uses=2]
  %tmp65 = add i64 %indvar56, 2                   ; <i64> [#uses=1]
  br label %bb1

bb13.preheader:                                   ; preds = %bb3, %entry
  %12 = add nsw i64 %m, -1                        ; <i64> [#uses=2]
  %13 = icmp slt i64 %12, 1                       ; <i1> [#uses=1]
  br i1 %13, label %return, label %bb6

bb6:                                              ; preds = %bb12, %bb13.preheader
  %storemerge113 = phi i32 [ %33, %bb12 ], [ 1, %bb13.preheader ] ; <i32> [#uses=6]
  %14 = sext i32 %storemerge113 to i64            ; <i64> [#uses=7]
  %15 = getelementptr inbounds [501 x [501 x double]]* @symmat, i64 0, i64 %14, i64 %14 ; <double*> [#uses=1]
  store double 1.000000e+00, double* %15, align 8
  %storemerge27 = add i32 %storemerge113, 1       ; <i32> [#uses=3]
  %16 = sext i32 %storemerge27 to i64             ; <i64> [#uses=1]
  %17 = icmp sgt i64 %16, %m                      ; <i1> [#uses=1]
  br i1 %17, label %bb12, label %bb.nph12

bb.nph12:                                         ; preds = %bb6
  %tmp = add i32 %storemerge113, 2                ; <i32> [#uses=2]
  br i1 %0, label %bb10.us, label %bb.nph

bb10.us:                                          ; preds = %bb10.us, %bb.nph12
  %indvar = phi i32 [ %indvar.next, %bb10.us ], [ 0, %bb.nph12 ] ; <i32> [#uses=3]
  %storemerge115 = phi i32 [ %storemerge115, %bb10.us ], [ %storemerge113, %bb.nph12 ] ; <i32> [#uses=2]
  %storemerge2.us = add i32 %tmp, %indvar         ; <i32> [#uses=1]
  %storemerge28.us = add i32 %storemerge27, %indvar ; <i32> [#uses=1]
  %18 = sext i32 %storemerge28.us to i64          ; <i64> [#uses=2]
  %19 = getelementptr inbounds [501 x [501 x double]]* @symmat, i64 0, i64 %14, i64 %18 ; <double*> [#uses=1]
  store double 0.000000e+00, double* %19, align 8
  %20 = getelementptr inbounds [501 x [501 x double]]* @symmat, i64 0, i64 %18, i64 %14 ; <double*> [#uses=1]
  store double 0.000000e+00, double* %20, align 8
  %21 = sext i32 %storemerge2.us to i64           ; <i64> [#uses=1]
  %22 = icmp sgt i64 %21, %m                      ; <i1> [#uses=1]
  %indvar.next = add i32 %indvar, 1               ; <i32> [#uses=1]
  br i1 %22, label %bb12, label %bb10.us

bb.nph:                                           ; preds = %bb10, %bb.nph12
  %indvar41 = phi i32 [ %indvar.next42, %bb10 ], [ 0, %bb.nph12 ] ; <i32> [#uses=3]
  %storemerge28 = add i32 %storemerge27, %indvar41 ; <i32> [#uses=1]
  %storemerge2 = add i32 %tmp, %indvar41          ; <i32> [#uses=1]
  %tmp45 = sext i32 %storemerge28 to i64          ; <i64> [#uses=3]
  %23 = getelementptr inbounds [501 x [501 x double]]* @symmat, i64 0, i64 %14, i64 %tmp45 ; <double*> [#uses=2]
  store double 0.000000e+00, double* %23, align 8
  br label %bb8

bb8:                                              ; preds = %bb8, %bb.nph
  %indvar38 = phi i64 [ 0, %bb.nph ], [ %tmp40, %bb8 ] ; <i64> [#uses=2]
  %24 = phi double [ 0.000000e+00, %bb.nph ], [ %28, %bb8 ] ; <double> [#uses=1]
  %tmp40 = add i64 %indvar38, 1                   ; <i64> [#uses=3]
  %scevgep = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %tmp40, i64 %tmp45 ; <double*> [#uses=1]
  %scevgep47 = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %tmp40, i64 %14 ; <double*> [#uses=1]
  %25 = load double* %scevgep47, align 8          ; <double> [#uses=1]
  %26 = load double* %scevgep, align 8            ; <double> [#uses=1]
  %27 = fmul double %25, %26                      ; <double> [#uses=1]
  %28 = fadd double %24, %27                      ; <double> [#uses=3]
  %tmp48 = add i64 %indvar38, 2                   ; <i64> [#uses=1]
  %29 = icmp sgt i64 %tmp48, %n                   ; <i1> [#uses=1]
  br i1 %29, label %bb10, label %bb8

bb10:                                             ; preds = %bb8
  store double %28, double* %23
  %30 = getelementptr inbounds [501 x [501 x double]]* @symmat, i64 0, i64 %tmp45, i64 %14 ; <double*> [#uses=1]
  store double %28, double* %30, align 8
  %31 = sext i32 %storemerge2 to i64              ; <i64> [#uses=1]
  %32 = icmp sgt i64 %31, %m                      ; <i1> [#uses=1]
  %indvar.next42 = add i32 %indvar41, 1           ; <i32> [#uses=1]
  br i1 %32, label %bb12, label %bb.nph

bb12:                                             ; preds = %bb10, %bb10.us, %bb6
  %storemerge125 = phi i32 [ %storemerge113, %bb6 ], [ %storemerge115, %bb10.us ], [ %storemerge113, %bb10 ] ; <i32> [#uses=1]
  %33 = add nsw i32 %storemerge125, 1             ; <i32> [#uses=2]
  %34 = sext i32 %33 to i64                       ; <i64> [#uses=1]
  %35 = icmp sgt i64 %34, %12                     ; <i1> [#uses=1]
  br i1 %35, label %return, label %bb6

return:                                           ; preds = %bb12, %bb13.preheader
  ret void
}

declare double @sqrt(double) nounwind readonly

define i32 @main(i32 %argc, i8** %argv) nounwind {
entry:
  br label %bb.nph.i

bb.nph.i:                                         ; preds = %bb3.i, %entry
  %indvar8.i = phi i64 [ 0, %entry ], [ %indvar.next9.i, %bb3.i ] ; <i64> [#uses=3]
  %storemerge3.i = trunc i64 %indvar8.i to i32    ; <i32> [#uses=1]
  %0 = sitofp i32 %storemerge3.i to double        ; <double> [#uses=1]
  br label %bb1.i

bb1.i:                                            ; preds = %bb1.i, %bb.nph.i
  %indvar.i = phi i64 [ 0, %bb.nph.i ], [ %indvar.next.i, %bb1.i ] ; <i64> [#uses=3]
  %scevgep.i = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %indvar8.i, i64 %indvar.i ; <double*> [#uses=1]
  %storemerge12.i = trunc i64 %indvar.i to i32    ; <i32> [#uses=1]
  %1 = sitofp i32 %storemerge12.i to double       ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=1]
  %3 = fdiv double %2, 5.000000e+02               ; <double> [#uses=1]
  store double %3, double* %scevgep.i, align 8
  %indvar.next.i = add i64 %indvar.i, 1           ; <i64> [#uses=2]
  %exitcond60 = icmp eq i64 %indvar.next.i, 501   ; <i1> [#uses=1]
  br i1 %exitcond60, label %bb3.i, label %bb1.i

bb3.i:                                            ; preds = %bb1.i
  %indvar.next9.i = add i64 %indvar8.i, 1         ; <i64> [#uses=2]
  %exitcond62 = icmp eq i64 %indvar.next9.i, 501  ; <i1> [#uses=1]
  br i1 %exitcond62, label %bb.nph29.bb.nph29.split_crit_edge, label %bb.nph.i

bb.nph29.bb.nph29.split_crit_edge:                ; preds = %bb3.i
  %4 = load double* @float_n, align 8             ; <double> [#uses=1]
  br label %bb.nph21

bb.nph21:                                         ; preds = %bb3, %bb.nph29.bb.nph29.split_crit_edge
  %indvar51 = phi i64 [ 0, %bb.nph29.bb.nph29.split_crit_edge ], [ %tmp57, %bb3 ] ; <i64> [#uses=1]
  %tmp57 = add i64 %indvar51, 1                   ; <i64> [#uses=4]
  %scevgep59 = getelementptr [501 x double]* @mean, i64 0, i64 %tmp57 ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep59, align 8
  br label %bb1

bb1:                                              ; preds = %bb1, %bb.nph21
  %indvar48 = phi i64 [ 0, %bb.nph21 ], [ %tmp56, %bb1 ] ; <i64> [#uses=1]
  %5 = phi double [ 0.000000e+00, %bb.nph21 ], [ %7, %bb1 ] ; <double> [#uses=1]
  %tmp56 = add i64 %indvar48, 1                   ; <i64> [#uses=3]
  %scevgep54 = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %tmp56, i64 %tmp57 ; <double*> [#uses=1]
  %6 = load double* %scevgep54, align 8           ; <double> [#uses=1]
  %7 = fadd double %5, %6                         ; <double> [#uses=2]
  %exitcond50 = icmp eq i64 %tmp56, 500           ; <i1> [#uses=1]
  br i1 %exitcond50, label %bb3, label %bb1

bb3:                                              ; preds = %bb1
  %8 = fdiv double %7, %4                         ; <double> [#uses=1]
  store double %8, double* %scevgep59, align 8
  %exitcond55 = icmp eq i64 %tmp57, 500           ; <i1> [#uses=1]
  br i1 %exitcond55, label %bb.nph, label %bb.nph21

bb.nph:                                           ; preds = %bb9, %bb3
  %indvar38 = phi i64 [ %tmp43, %bb9 ], [ 0, %bb3 ] ; <i64> [#uses=1]
  %tmp43 = add i64 %indvar38, 1                   ; <i64> [#uses=5]
  %scevgep45 = getelementptr [501 x double]* @stddev, i64 0, i64 %tmp43 ; <double*> [#uses=4]
  store double 0.000000e+00, double* %scevgep45, align 8
  %scevgep46 = getelementptr [501 x double]* @mean, i64 0, i64 %tmp43 ; <double*> [#uses=1]
  %9 = load double* %scevgep46, align 8           ; <double> [#uses=1]
  br label %bb7

bb7:                                              ; preds = %bb7, %bb.nph
  %indvar = phi i64 [ 0, %bb.nph ], [ %tmp42, %bb7 ] ; <i64> [#uses=1]
  %10 = phi double [ 0.000000e+00, %bb.nph ], [ %14, %bb7 ] ; <double> [#uses=1]
  %tmp42 = add i64 %indvar, 1                     ; <i64> [#uses=3]
  %scevgep = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %tmp42, i64 %tmp43 ; <double*> [#uses=1]
  %11 = load double* %scevgep, align 8            ; <double> [#uses=1]
  %12 = fsub double %11, %9                       ; <double> [#uses=2]
  %13 = fmul double %12, %12                      ; <double> [#uses=1]
  %14 = fadd double %10, %13                      ; <double> [#uses=2]
  %exitcond37 = icmp eq i64 %tmp42, 500           ; <i1> [#uses=1]
  br i1 %exitcond37, label %bb9, label %bb7

bb9:                                              ; preds = %bb7
  %15 = load double* @float_n, align 8            ; <double> [#uses=1]
  %16 = fdiv double %14, %15                      ; <double> [#uses=2]
  store double %16, double* %scevgep45, align 8
  %17 = tail call double @sqrt(double %16) nounwind readonly ; <double> [#uses=3]
  store double %17, double* %scevgep45, align 8
  %18 = load double* @eps, align 8                ; <double> [#uses=1]
  %19 = fcmp ugt double %17, %18                  ; <i1> [#uses=1]
  %storemerge3 = select i1 %19, double %17, double 1.000000e+00 ; <double> [#uses=1]
  store double %storemerge3, double* %scevgep45, align 8
  %exitcond41 = icmp eq i64 %tmp43, 500           ; <i1> [#uses=1]
  br i1 %exitcond41, label %bb14, label %bb.nph

bb14:                                             ; preds = %bb9
  tail call void @scop_func(i64 500, i64 500) nounwind
  store double 1.000000e+00, double* getelementptr inbounds ([501 x [501 x double]]* @symmat, i64 0, i64 500, i64 500), align 32
  %20 = icmp sgt i32 %argc, 42                    ; <i1> [#uses=1]
  br i1 %20, label %bb.i, label %print_array.exit

bb.i:                                             ; preds = %bb14
  %21 = load i8** %argv, align 1                  ; <i8*> [#uses=1]
  %22 = load i8* %21, align 1                     ; <i8> [#uses=1]
  %23 = icmp eq i8 %22, 0                         ; <i1> [#uses=1]
  br i1 %23, label %bb6.preheader.i, label %print_array.exit

bb3.i7:                                           ; preds = %bb6.preheader.i, %bb5.i
  %indvar.i5 = phi i64 [ %indvar.next.i8, %bb5.i ], [ 0, %bb6.preheader.i ] ; <i64> [#uses=3]
  %scevgep.i6 = getelementptr [501 x [501 x double]]* @symmat, i64 0, i64 %indvar8.i11, i64 %indvar.i5 ; <double*> [#uses=1]
  %tmp34 = add i64 %tmp33, %indvar.i5             ; <i64> [#uses=1]
  %tmp11.i = trunc i64 %tmp34 to i32              ; <i32> [#uses=1]
  %24 = load double* %scevgep.i6, align 8         ; <double> [#uses=1]
  %25 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %26 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %25, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %24) nounwind ; <i32> [#uses=0]
  %27 = srem i32 %tmp11.i, 80                     ; <i32> [#uses=1]
  %28 = icmp eq i32 %27, 20                       ; <i1> [#uses=1]
  br i1 %28, label %bb4.i, label %bb5.i

bb4.i:                                            ; preds = %bb3.i7
  %29 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %30 = bitcast %struct._IO_FILE* %29 to i8*      ; <i8*> [#uses=1]
  %31 = tail call i32 @fputc(i32 10, i8* %30) nounwind ; <i32> [#uses=0]
  br label %bb5.i

bb5.i:                                            ; preds = %bb4.i, %bb3.i7
  %indvar.next.i8 = add i64 %indvar.i5, 1         ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next.i8, 501    ; <i1> [#uses=1]
  br i1 %exitcond, label %bb7.i, label %bb3.i7

bb7.i:                                            ; preds = %bb5.i
  %indvar.next9.i10 = add i64 %indvar8.i11, 1     ; <i64> [#uses=2]
  %exitcond32 = icmp eq i64 %indvar.next9.i10, 501 ; <i1> [#uses=1]
  br i1 %exitcond32, label %bb9.i, label %bb6.preheader.i

bb6.preheader.i:                                  ; preds = %bb7.i, %bb.i
  %indvar8.i11 = phi i64 [ %indvar.next9.i10, %bb7.i ], [ 0, %bb.i ] ; <i64> [#uses=3]
  %tmp33 = mul i64 %indvar8.i11, 500              ; <i64> [#uses=1]
  br label %bb3.i7

bb9.i:                                            ; preds = %bb7.i
  %32 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %33 = bitcast %struct._IO_FILE* %32 to i8*      ; <i8*> [#uses=1]
  %34 = tail call i32 @fputc(i32 10, i8* %33) nounwind ; <i32> [#uses=0]
  ret i32 0

print_array.exit:                                 ; preds = %bb.i, %bb14
  ret i32 0
}