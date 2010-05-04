; RUN: opt -indvars -polly-scop-detect  -analyze %s | FileCheck %s
; XFAIL: *
; ModuleID = '<stdin>'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }

@float_n = global double 0x41B32863F6028F5C       ; <double*> [#uses=1]
@data = common global [501 x [501 x double]] zeroinitializer, align 32 ; <[501 x [501 x double]]*> [#uses=6]
@symmat = common global [501 x [501 x double]] zeroinitializer, align 32 ; <[501 x [501 x double]]*> [#uses=6]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
@mean = common global [501 x double] zeroinitializer, align 32 ; <[501 x double]*> [#uses=3]

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
  %0 = icmp slt i64 %m, 1                         ; <i1> [#uses=3]
  br i1 %0, label %bb10.preheader, label %bb.nph44

bb.nph44:                                         ; preds = %entry
  %1 = icmp slt i64 %n, 1                         ; <i1> [#uses=1]
  %2 = load double* @float_n, align 8             ; <double> [#uses=2]
  br i1 %1, label %bb3.us, label %bb.nph36

bb3.us:                                           ; preds = %bb3.us, %bb.nph44
  %indvar = phi i64 [ %tmp, %bb3.us ], [ 0, %bb.nph44 ] ; <i64> [#uses=2]
  %tmp = add i64 %indvar, 1                       ; <i64> [#uses=2]
  %scevgep = getelementptr [501 x double]* @mean, i64 0, i64 %tmp ; <double*> [#uses=1]
  %tmp45 = add i64 %indvar, 2                     ; <i64> [#uses=1]
  %3 = fdiv double 0.000000e+00, %2               ; <double> [#uses=1]
  store double %3, double* %scevgep, align 8
  %4 = icmp sgt i64 %tmp45, %m                    ; <i1> [#uses=1]
  br i1 %4, label %bb10.preheader, label %bb3.us

bb.nph36:                                         ; preds = %bb3, %bb.nph44
  %indvar94 = phi i64 [ %tmp100, %bb3 ], [ 0, %bb.nph44 ] ; <i64> [#uses=2]
  %tmp100 = add i64 %indvar94, 1                  ; <i64> [#uses=3]
  %tmp102 = add i64 %indvar94, 2                  ; <i64> [#uses=1]
  %scevgep103 = getelementptr [501 x double]* @mean, i64 0, i64 %tmp100 ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep103, align 8
  br label %bb1

bb1:                                              ; preds = %bb1, %bb.nph36
  %indvar91 = phi i64 [ 0, %bb.nph36 ], [ %tmp99, %bb1 ] ; <i64> [#uses=2]
  %5 = phi double [ 0.000000e+00, %bb.nph36 ], [ %7, %bb1 ] ; <double> [#uses=1]
  %tmp99 = add i64 %indvar91, 1                   ; <i64> [#uses=2]
  %scevgep97 = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %tmp99, i64 %tmp100 ; <double*> [#uses=1]
  %6 = load double* %scevgep97, align 8           ; <double> [#uses=1]
  %7 = fadd double %5, %6                         ; <double> [#uses=2]
  %tmp98 = add i64 %indvar91, 2                   ; <i64> [#uses=1]
  %8 = icmp sgt i64 %tmp98, %n                    ; <i1> [#uses=1]
  br i1 %8, label %bb3, label %bb1

bb3:                                              ; preds = %bb1
  %9 = fdiv double %7, %2                         ; <double> [#uses=1]
  store double %9, double* %scevgep103, align 8
  %10 = icmp sgt i64 %tmp102, %m                  ; <i1> [#uses=1]
  br i1 %10, label %bb10.preheader, label %bb.nph36

bb10.preheader:                                   ; preds = %bb3, %bb3.us, %entry
  %11 = icmp slt i64 %n, 1                        ; <i1> [#uses=2]
  br i1 %11, label %bb19.preheader, label %bb.nph33

bb7:                                              ; preds = %bb8.preheader, %bb7
  %indvar77 = phi i64 [ %tmp87, %bb7 ], [ 0, %bb8.preheader ] ; <i64> [#uses=2]
  %tmp87 = add i64 %indvar77, 1                   ; <i64> [#uses=3]
  %scevgep83 = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %tmp86, i64 %tmp87 ; <double*> [#uses=2]
  %scevgep84 = getelementptr [501 x double]* @mean, i64 0, i64 %tmp87 ; <double*> [#uses=1]
  %12 = load double* %scevgep83, align 8          ; <double> [#uses=1]
  %13 = load double* %scevgep84, align 8          ; <double> [#uses=1]
  %14 = fsub double %12, %13                      ; <double> [#uses=1]
  store double %14, double* %scevgep83, align 8
  %tmp85 = add i64 %indvar77, 2                   ; <i64> [#uses=1]
  %15 = icmp sgt i64 %tmp85, %m                   ; <i1> [#uses=1]
  br i1 %15, label %bb9, label %bb7

bb9:                                              ; preds = %bb7
  %16 = icmp sgt i64 %tmp89, %n                   ; <i1> [#uses=1]
  br i1 %16, label %bb19.preheader, label %bb8.preheader

bb.nph33:                                         ; preds = %bb10.preheader
  br i1 %0, label %return, label %bb8.preheader

bb8.preheader:                                    ; preds = %bb.nph33, %bb9
  %indvar79 = phi i64 [ %tmp86, %bb9 ], [ 0, %bb.nph33 ] ; <i64> [#uses=2]
  %tmp86 = add i64 %indvar79, 1                   ; <i64> [#uses=2]
  %tmp89 = add i64 %indvar79, 2                   ; <i64> [#uses=1]
  br label %bb7

bb19.preheader:                                   ; preds = %bb9, %bb10.preheader
  br i1 %0, label %return, label %bb17.preheader

bb.nph13:                                         ; preds = %bb17.preheader
  %tmp53 = add i64 %storemerge214, 1              ; <i64> [#uses=2]
  br i1 %11, label %bb16.us, label %bb.nph13.bb.nph13.split_crit_edge

bb.nph13.bb.nph13.split_crit_edge:                ; preds = %bb.nph13
  %tmp73 = mul i64 %storemerge214, 502            ; <i64> [#uses=2]
  br label %bb.nph

bb16.us:                                          ; preds = %bb16.us, %bb.nph13
  %indvar48 = phi i64 [ %indvar.next49, %bb16.us ], [ 0, %bb.nph13 ] ; <i64> [#uses=3]
  %storemerge218 = phi i64 [ %storemerge218, %bb16.us ], [ %storemerge214, %bb.nph13 ] ; <i64> [#uses=4]
  %tmp50 = add i64 %storemerge214, %indvar48      ; <i64> [#uses=2]
  %tmp54 = add i64 %tmp53, %indvar48              ; <i64> [#uses=1]
  %tmp56 = mul i64 %storemerge218, 501            ; <i64> [#uses=1]
  %scevgep55.sum = add i64 %tmp50, %tmp56         ; <i64> [#uses=1]
  %scevgep57 = getelementptr [501 x [501 x double]]* @symmat, i64 0, i64 0, i64 %scevgep55.sum ; <double*> [#uses=1]
  store double 0.000000e+00, double* %scevgep57, align 8
  %scevgep52 = getelementptr [501 x [501 x double]]* @symmat, i64 0, i64 %tmp50, i64 %storemerge218 ; <double*> [#uses=1]
  store double 0.000000e+00, double* %scevgep52, align 8
  %17 = icmp sgt i64 %tmp54, %m                   ; <i1> [#uses=1]
  %indvar.next49 = add i64 %indvar48, 1           ; <i64> [#uses=1]
  br i1 %17, label %bb18, label %bb16.us

bb.nph:                                           ; preds = %bb16, %bb.nph13.bb.nph13.split_crit_edge
  %indvar62 = phi i64 [ 0, %bb.nph13.bb.nph13.split_crit_edge ], [ %indvar.next63, %bb16 ] ; <i64> [#uses=5]
  %tmp69 = add i64 %storemerge214, %indvar62      ; <i64> [#uses=1]
  %tmp72 = add i64 %tmp53, %indvar62              ; <i64> [#uses=1]
  %scevgep74 = getelementptr [501 x [501 x double]]* @symmat, i64 0, i64 %indvar62, i64 %tmp73 ; <double*> [#uses=1]
  %tmp75 = add i64 %tmp73, %indvar62              ; <i64> [#uses=1]
  %scevgep76 = getelementptr [501 x [501 x double]]* @symmat, i64 0, i64 0, i64 %tmp75 ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep76, align 8
  br label %bb14

bb14:                                             ; preds = %bb14, %bb.nph
  %indvar59 = phi i64 [ 0, %bb.nph ], [ %tmp68, %bb14 ] ; <i64> [#uses=2]
  %18 = phi double [ 0.000000e+00, %bb.nph ], [ %22, %bb14 ] ; <double> [#uses=1]
  %tmp68 = add i64 %indvar59, 1                   ; <i64> [#uses=3]
  %scevgep65 = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %tmp68, i64 %tmp69 ; <double*> [#uses=1]
  %scevgep66 = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %tmp68, i64 %storemerge214 ; <double*> [#uses=1]
  %19 = load double* %scevgep66, align 8          ; <double> [#uses=1]
  %20 = load double* %scevgep65, align 8          ; <double> [#uses=1]
  %21 = fmul double %19, %20                      ; <double> [#uses=1]
  %22 = fadd double %18, %21                      ; <double> [#uses=3]
  %tmp67 = add i64 %indvar59, 2                   ; <i64> [#uses=1]
  %23 = icmp sgt i64 %tmp67, %n                   ; <i1> [#uses=1]
  br i1 %23, label %bb16, label %bb14

bb16:                                             ; preds = %bb14
  store double %22, double* %scevgep76
  store double %22, double* %scevgep74, align 8
  %24 = icmp sgt i64 %tmp72, %m                   ; <i1> [#uses=1]
  %indvar.next63 = add i64 %indvar62, 1           ; <i64> [#uses=1]
  br i1 %24, label %bb18, label %bb.nph

bb18:                                             ; preds = %bb17.preheader, %bb16, %bb16.us
  %storemerge225 = phi i64 [ %storemerge214, %bb17.preheader ], [ %storemerge218, %bb16.us ], [ %storemerge214, %bb16 ] ; <i64> [#uses=1]
  %25 = add nsw i64 %storemerge225, 1             ; <i64> [#uses=2]
  %26 = icmp sgt i64 %25, %m                      ; <i1> [#uses=1]
  br i1 %26, label %return, label %bb17.preheader

bb17.preheader:                                   ; preds = %bb18, %bb19.preheader
  %storemerge214 = phi i64 [ %25, %bb18 ], [ 1, %bb19.preheader ] ; <i64> [#uses=9]
  %27 = icmp sgt i64 %storemerge214, %m           ; <i1> [#uses=1]
  br i1 %27, label %bb18, label %bb.nph13

return:                                           ; preds = %bb18, %bb19.preheader, %bb.nph33
  ret void
}

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
  %exitcond14 = icmp eq i64 %indvar.next.i, 501   ; <i1> [#uses=1]
  br i1 %exitcond14, label %bb3.i, label %bb1.i

bb3.i:                                            ; preds = %bb1.i
  %indvar.next9.i = add i64 %indvar8.i, 1         ; <i64> [#uses=2]
  %exitcond15 = icmp eq i64 %indvar.next9.i, 501  ; <i1> [#uses=1]
  br i1 %exitcond15, label %init_array.exit, label %bb.nph.i

init_array.exit:                                  ; preds = %bb3.i
  tail call void @scop_func(i64 500, i64 500) nounwind
  %4 = icmp sgt i32 %argc, 42                     ; <i1> [#uses=1]
  br i1 %4, label %bb.i, label %print_array.exit

bb.i:                                             ; preds = %init_array.exit
  %5 = load i8** %argv, align 1                   ; <i8*> [#uses=1]
  %6 = load i8* %5, align 1                       ; <i8> [#uses=1]
  %7 = icmp eq i8 %6, 0                           ; <i1> [#uses=1]
  br i1 %7, label %bb6.preheader.i, label %print_array.exit

bb3.i3:                                           ; preds = %bb6.preheader.i, %bb5.i
  %indvar.i1 = phi i64 [ %indvar.next.i4, %bb5.i ], [ 0, %bb6.preheader.i ] ; <i64> [#uses=3]
  %scevgep.i2 = getelementptr [501 x [501 x double]]* @symmat, i64 0, i64 %indvar8.i7, i64 %indvar.i1 ; <double*> [#uses=1]
  %tmp12 = add i64 %tmp11, %indvar.i1             ; <i64> [#uses=1]
  %tmp11.i = trunc i64 %tmp12 to i32              ; <i32> [#uses=1]
  %8 = load double* %scevgep.i2, align 8          ; <double> [#uses=1]
  %9 = load %struct._IO_FILE** @stderr, align 8   ; <%struct._IO_FILE*> [#uses=1]
  %10 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %9, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %8) nounwind ; <i32> [#uses=0]
  %11 = srem i32 %tmp11.i, 80                     ; <i32> [#uses=1]
  %12 = icmp eq i32 %11, 20                       ; <i1> [#uses=1]
  br i1 %12, label %bb4.i, label %bb5.i

bb4.i:                                            ; preds = %bb3.i3
  %13 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %14 = bitcast %struct._IO_FILE* %13 to i8*      ; <i8*> [#uses=1]
  %15 = tail call i32 @fputc(i32 10, i8* %14) nounwind ; <i32> [#uses=0]
  br label %bb5.i

bb5.i:                                            ; preds = %bb4.i, %bb3.i3
  %indvar.next.i4 = add i64 %indvar.i1, 1         ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next.i4, 501    ; <i1> [#uses=1]
  br i1 %exitcond, label %bb7.i, label %bb3.i3

bb7.i:                                            ; preds = %bb5.i
  %indvar.next9.i6 = add i64 %indvar8.i7, 1       ; <i64> [#uses=2]
  %exitcond10 = icmp eq i64 %indvar.next9.i6, 501 ; <i1> [#uses=1]
  br i1 %exitcond10, label %bb9.i, label %bb6.preheader.i

bb6.preheader.i:                                  ; preds = %bb7.i, %bb.i
  %indvar8.i7 = phi i64 [ %indvar.next9.i6, %bb7.i ], [ 0, %bb.i ] ; <i64> [#uses=3]
  %tmp11 = mul i64 %indvar8.i7, 500               ; <i64> [#uses=1]
  br label %bb3.i3

bb9.i:                                            ; preds = %bb7.i
  %16 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %17 = bitcast %struct._IO_FILE* %16 to i8*      ; <i8*> [#uses=1]
  %18 = tail call i32 @fputc(i32 10, i8* %17) nounwind ; <i32> [#uses=0]
  ret i32 0

print_array.exit:                                 ; preds = %bb.i, %init_array.exit
  ret i32 0
}
