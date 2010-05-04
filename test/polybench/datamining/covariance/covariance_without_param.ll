; RUN: opt -indvars -polly-scop-detect  -analyze %s | FileCheck %s
; XFAIL: *
; ModuleID = '<stdin>'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }

@float_n = global double 0x41B32863F6028F5C       ; <double*> [#uses=1]
@data = common global [501 x [501 x double]] zeroinitializer, align 32 ; <[501 x [501 x double]]*> [#uses=6]
@symmat = common global [501 x [501 x double]] zeroinitializer, align 32 ; <[501 x [501 x double]]*> [#uses=4]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
@mean = common global [501 x double] zeroinitializer, align 32 ; <[501 x double]*> [#uses=2]

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

define void @scop_func() nounwind {
bb.nph44.bb.nph44.split_crit_edge:
  %0 = load double* @float_n, align 8             ; <double> [#uses=1]
  br label %bb.nph36

bb.nph36:                                         ; preds = %bb3, %bb.nph44.bb.nph44.split_crit_edge
  %indvar77 = phi i64 [ 0, %bb.nph44.bb.nph44.split_crit_edge ], [ %tmp83, %bb3 ] ; <i64> [#uses=1]
  %tmp83 = add i64 %indvar77, 1                   ; <i64> [#uses=4]
  %scevgep85 = getelementptr [501 x double]* @mean, i64 0, i64 %tmp83 ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep85, align 8
  br label %bb1

bb1:                                              ; preds = %bb1, %bb.nph36
  %indvar73 = phi i64 [ 0, %bb.nph36 ], [ %tmp82, %bb1 ] ; <i64> [#uses=1]
  %1 = phi double [ 0.000000e+00, %bb.nph36 ], [ %3, %bb1 ] ; <double> [#uses=1]
  %tmp82 = add i64 %indvar73, 1                   ; <i64> [#uses=3]
  %scevgep80 = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %tmp82, i64 %tmp83 ; <double*> [#uses=1]
  %2 = load double* %scevgep80, align 8           ; <double> [#uses=1]
  %3 = fadd double %1, %2                         ; <double> [#uses=2]
  %exitcond75 = icmp eq i64 %tmp82, 500           ; <i1> [#uses=1]
  br i1 %exitcond75, label %bb3, label %bb1

bb3:                                              ; preds = %bb1
  %4 = fdiv double %3, %0                         ; <double> [#uses=1]
  store double %4, double* %scevgep85, align 8
  %exitcond81 = icmp eq i64 %tmp83, 500           ; <i1> [#uses=1]
  br i1 %exitcond81, label %bb8.preheader, label %bb.nph36

bb7:                                              ; preds = %bb8.preheader, %bb7
  %indvar59 = phi i64 [ %tmp70, %bb7 ], [ 0, %bb8.preheader ] ; <i64> [#uses=1]
  %tmp70 = add i64 %indvar59, 1                   ; <i64> [#uses=4]
  %scevgep66 = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %tmp69, i64 %tmp70 ; <double*> [#uses=2]
  %scevgep67 = getelementptr [501 x double]* @mean, i64 0, i64 %tmp70 ; <double*> [#uses=1]
  %5 = load double* %scevgep66, align 8           ; <double> [#uses=1]
  %6 = load double* %scevgep67, align 8           ; <double> [#uses=1]
  %7 = fsub double %5, %6                         ; <double> [#uses=1]
  store double %7, double* %scevgep66, align 8
  %exitcond61 = icmp eq i64 %tmp70, 500           ; <i1> [#uses=1]
  br i1 %exitcond61, label %bb9, label %bb7

bb9:                                              ; preds = %bb7
  %exitcond68 = icmp eq i64 %tmp69, 500           ; <i1> [#uses=1]
  br i1 %exitcond68, label %bb17.preheader, label %bb8.preheader

bb8.preheader:                                    ; preds = %bb9, %bb3
  %indvar62 = phi i64 [ %tmp69, %bb9 ], [ 0, %bb3 ] ; <i64> [#uses=1]
  %tmp69 = add i64 %indvar62, 1                   ; <i64> [#uses=3]
  br label %bb7

bb.nph13.bb.nph13.split_crit_edge:                ; preds = %bb17.preheader
  %tmp53 = add i64 %storemerge214, 1              ; <i64> [#uses=1]
  %tmp55 = mul i64 %storemerge214, 502            ; <i64> [#uses=2]
  br label %bb.nph

bb.nph:                                           ; preds = %bb16, %bb.nph13.bb.nph13.split_crit_edge
  %indvar46 = phi i64 [ 0, %bb.nph13.bb.nph13.split_crit_edge ], [ %indvar.next47, %bb16 ] ; <i64> [#uses=5]
  %tmp51 = add i64 %storemerge214, %indvar46      ; <i64> [#uses=1]
  %tmp54 = add i64 %tmp53, %indvar46              ; <i64> [#uses=1]
  %scevgep56 = getelementptr [501 x [501 x double]]* @symmat, i64 0, i64 %indvar46, i64 %tmp55 ; <double*> [#uses=1]
  %tmp57 = add i64 %tmp55, %indvar46              ; <i64> [#uses=1]
  %scevgep58 = getelementptr [501 x [501 x double]]* @symmat, i64 0, i64 0, i64 %tmp57 ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep58, align 8
  br label %bb14

bb14:                                             ; preds = %bb14, %bb.nph
  %indvar = phi i64 [ 0, %bb.nph ], [ %tmp50, %bb14 ] ; <i64> [#uses=1]
  %8 = phi double [ 0.000000e+00, %bb.nph ], [ %12, %bb14 ] ; <double> [#uses=1]
  %tmp50 = add i64 %indvar, 1                     ; <i64> [#uses=4]
  %scevgep = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %tmp50, i64 %tmp51 ; <double*> [#uses=1]
  %scevgep49 = getelementptr [501 x [501 x double]]* @data, i64 0, i64 %tmp50, i64 %storemerge214 ; <double*> [#uses=1]
  %9 = load double* %scevgep49, align 8           ; <double> [#uses=1]
  %10 = load double* %scevgep, align 8            ; <double> [#uses=1]
  %11 = fmul double %9, %10                       ; <double> [#uses=1]
  %12 = fadd double %8, %11                       ; <double> [#uses=3]
  %exitcond = icmp eq i64 %tmp50, 500             ; <i1> [#uses=1]
  br i1 %exitcond, label %bb16, label %bb14

bb16:                                             ; preds = %bb14
  store double %12, double* %scevgep58
  store double %12, double* %scevgep56, align 8
  %13 = icmp sgt i64 %tmp54, 500                  ; <i1> [#uses=1]
  %indvar.next47 = add i64 %indvar46, 1           ; <i64> [#uses=1]
  br i1 %13, label %bb18, label %bb.nph

bb18:                                             ; preds = %bb17.preheader, %bb16
  %14 = add nsw i64 %storemerge214, 1             ; <i64> [#uses=2]
  %15 = icmp sgt i64 %14, 500                     ; <i1> [#uses=1]
  br i1 %15, label %return, label %bb17.preheader

bb17.preheader:                                   ; preds = %bb18, %bb9
  %storemerge214 = phi i64 [ %14, %bb18 ], [ 1, %bb9 ] ; <i64> [#uses=6]
  %16 = icmp sgt i64 %storemerge214, 500          ; <i1> [#uses=1]
  br i1 %16, label %bb18, label %bb.nph13.bb.nph13.split_crit_edge

return:                                           ; preds = %bb18
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
  tail call void @scop_func() nounwind
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
