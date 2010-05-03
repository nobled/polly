; RUN: opt  -indvars -polly-scop-detect -print-top-scop-only -polly-print-scop -analyze  %s | FileCheck %s
; XFAIL: *
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }

@A = common global [128 x [128 x [128 x double]]] zeroinitializer, align 32 ; <[128 x [128 x [128 x double]]]*> [#uses=8]
@C4 = common global [128 x [128 x double]] zeroinitializer, align 32 ; <[128 x [128 x double]]*> [#uses=4]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
@sum = common global [128 x [128 x [128 x double]]] zeroinitializer, align 32 ; <[128 x [128 x [128 x double]]]*> [#uses=4]

define void @init_array() nounwind inlinehint {
bb.nph22.bb.nph22.split_crit_edge:
  br label %bb.nph13.bb.nph13.split_crit_edge

bb.nph8:                                          ; preds = %bb.nph13.bb.nph13.split_crit_edge, %bb4
  %indvar32 = phi i64 [ 0, %bb.nph13.bb.nph13.split_crit_edge ], [ %indvar.next33, %bb4 ] ; <i64> [#uses=3]
  %j.09 = trunc i64 %indvar32 to i32              ; <i32> [#uses=1]
  %0 = sitofp i32 %j.09 to double                 ; <double> [#uses=1]
  %1 = fmul double %5, %0                         ; <double> [#uses=1]
  br label %bb2

bb2:                                              ; preds = %bb2, %bb.nph8
  %indvar27 = phi i64 [ 0, %bb.nph8 ], [ %indvar.next28, %bb2 ] ; <i64> [#uses=3]
  %scevgep34 = getelementptr [128 x [128 x [128 x double]]]* @A, i64 0, i64 %indvar30, i64 %indvar32, i64 %indvar27 ; <double*> [#uses=1]
  %k.07 = trunc i64 %indvar27 to i32              ; <i32> [#uses=1]
  %2 = sitofp i32 %k.07 to double                 ; <double> [#uses=1]
  %3 = fadd double %1, %2                         ; <double> [#uses=1]
  %4 = fdiv double %3, 1.280000e+02               ; <double> [#uses=1]
  store double %4, double* %scevgep34, align 8
  %indvar.next28 = add i64 %indvar27, 1           ; <i64> [#uses=2]
  %exitcond29 = icmp eq i64 %indvar.next28, 128   ; <i1> [#uses=1]
  br i1 %exitcond29, label %bb4, label %bb2

bb4:                                              ; preds = %bb2
  %indvar.next33 = add i64 %indvar32, 1           ; <i64> [#uses=2]
  %exitcond35 = icmp eq i64 %indvar.next33, 128   ; <i1> [#uses=1]
  br i1 %exitcond35, label %bb6, label %bb.nph8

bb.nph13.bb.nph13.split_crit_edge:                ; preds = %bb6, %bb.nph22.bb.nph22.split_crit_edge
  %indvar30 = phi i64 [ 0, %bb.nph22.bb.nph22.split_crit_edge ], [ %indvar.next31, %bb6 ] ; <i64> [#uses=3]
  %i.014 = trunc i64 %indvar30 to i32             ; <i32> [#uses=1]
  %5 = sitofp i32 %i.014 to double                ; <double> [#uses=1]
  br label %bb.nph8

bb6:                                              ; preds = %bb4
  %indvar.next31 = add i64 %indvar30, 1           ; <i64> [#uses=2]
  %exitcond37 = icmp eq i64 %indvar.next31, 128   ; <i1> [#uses=1]
  br i1 %exitcond37, label %bb.nph, label %bb.nph13.bb.nph13.split_crit_edge

bb.nph:                                           ; preds = %bb12, %bb6
  %indvar23 = phi i64 [ %indvar.next24, %bb12 ], [ 0, %bb6 ] ; <i64> [#uses=3]
  %i.12 = trunc i64 %indvar23 to i32              ; <i32> [#uses=1]
  %6 = sitofp i32 %i.12 to double                 ; <double> [#uses=1]
  br label %bb10

bb10:                                             ; preds = %bb10, %bb.nph
  %indvar = phi i64 [ 0, %bb.nph ], [ %indvar.next, %bb10 ] ; <i64> [#uses=3]
  %scevgep = getelementptr [128 x [128 x double]]* @C4, i64 0, i64 %indvar23, i64 %indvar ; <double*> [#uses=1]
  %j.11 = trunc i64 %indvar to i32                ; <i32> [#uses=1]
  %7 = sitofp i32 %j.11 to double                 ; <double> [#uses=1]
  %8 = fmul double %6, %7                         ; <double> [#uses=1]
  %9 = fdiv double %8, 1.280000e+02               ; <double> [#uses=1]
  store double %9, double* %scevgep, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 128       ; <i1> [#uses=1]
  br i1 %exitcond, label %bb12, label %bb10

bb12:                                             ; preds = %bb10
  %indvar.next24 = add i64 %indvar23, 1           ; <i64> [#uses=2]
  %exitcond25 = icmp eq i64 %indvar.next24, 128   ; <i1> [#uses=1]
  br i1 %exitcond25, label %return, label %bb.nph

return:                                           ; preds = %bb12
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
  br i1 %3, label %bb9.preheader, label %return

bb4:                                              ; preds = %bb7.preheader, %bb6
  %indvar = phi i64 [ %indvar.next, %bb6 ], [ 0, %bb7.preheader ] ; <i64> [#uses=3]
  %scevgep = getelementptr [128 x [128 x [128 x double]]]* @A, i64 0, i64 %indvar16, i64 %indvar18, i64 %indvar ; <double*> [#uses=1]
  %tmp35 = add i64 %tmp34, %indvar                ; <i64> [#uses=1]
  %tmp23 = trunc i64 %tmp35 to i32                ; <i32> [#uses=1]
  %4 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %5 = load %struct._IO_FILE** @stderr, align 8   ; <%struct._IO_FILE*> [#uses=1]
  %6 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %5, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %4) nounwind ; <i32> [#uses=0]
  %7 = srem i32 %tmp23, 80                        ; <i32> [#uses=1]
  %8 = icmp eq i32 %7, 20                         ; <i1> [#uses=1]
  br i1 %8, label %bb5, label %bb6

bb5:                                              ; preds = %bb4
  %9 = load %struct._IO_FILE** @stderr, align 8   ; <%struct._IO_FILE*> [#uses=1]
  %10 = bitcast %struct._IO_FILE* %9 to i8*       ; <i8*> [#uses=1]
  %11 = tail call i32 @fputc(i32 10, i8* %10) nounwind ; <i32> [#uses=0]
  br label %bb6

bb6:                                              ; preds = %bb5, %bb4
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 128       ; <i1> [#uses=1]
  br i1 %exitcond, label %bb8, label %bb4

bb8:                                              ; preds = %bb6
  %indvar.next19 = add i64 %indvar18, 1           ; <i64> [#uses=2]
  %exitcond24 = icmp eq i64 %indvar.next19, 128   ; <i1> [#uses=1]
  br i1 %exitcond24, label %bb10, label %bb7.preheader

bb7.preheader:                                    ; preds = %bb9.preheader, %bb8
  %indvar18 = phi i64 [ %indvar.next19, %bb8 ], [ 0, %bb9.preheader ] ; <i64> [#uses=3]
  %tmp = shl i64 %indvar18, 7                     ; <i64> [#uses=1]
  %tmp34 = add i64 %tmp33, %tmp                   ; <i64> [#uses=1]
  br label %bb4

bb10:                                             ; preds = %bb8
  %indvar.next17 = add i64 %indvar16, 1           ; <i64> [#uses=2]
  %exitcond31 = icmp eq i64 %indvar.next17, 128   ; <i1> [#uses=1]
  br i1 %exitcond31, label %bb12, label %bb9.preheader

bb9.preheader:                                    ; preds = %bb10, %bb
  %indvar16 = phi i64 [ %indvar.next17, %bb10 ], [ 0, %bb ] ; <i64> [#uses=3]
  %tmp33 = shl i64 %indvar16, 7                   ; <i64> [#uses=1]
  br label %bb7.preheader

bb12:                                             ; preds = %bb10
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
bb.nph45.bb.nph45.split_crit_edge:
  br label %bb.nph25

bb.nph6.split.us:                                 ; preds = %bb.nph25, %bb10
  %q.09 = phi i32 [ 0, %bb.nph25 ], [ %5, %bb10 ] ; <i32> [#uses=3]
  %tmp48 = sext i32 %q.09 to i64                  ; <i64> [#uses=2]
  br label %bb.nph.us

bb5.us:                                           ; preds = %bb3.us
  store double %3, double* %scevgep54
  %indvar.next47 = add i64 %indvar46, 1           ; <i64> [#uses=2]
  %exitcond50 = icmp eq i64 %indvar.next47, 128   ; <i1> [#uses=1]
  br i1 %exitcond50, label %bb.nph8, label %bb.nph.us

bb3.us:                                           ; preds = %bb.nph.us, %bb3.us
  %indvar = phi i64 [ 0, %bb.nph.us ], [ %indvar.next, %bb3.us ] ; <i64> [#uses=3]
  %.tmp.0.us = phi double [ 0.000000e+00, %bb.nph.us ], [ %3, %bb3.us ] ; <double> [#uses=1]
  %scevgep = getelementptr [128 x [128 x double]]* @C4, i64 0, i64 %indvar, i64 %indvar46 ; <double*> [#uses=1]
  %scevgep49 = getelementptr [128 x [128 x [128 x double]]]* @A, i64 0, i64 %tmp, i64 %tmp48, i64 %indvar ; <double*> [#uses=1]
  %0 = load double* %scevgep49, align 8           ; <double> [#uses=1]
  %1 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=1]
  %3 = fadd double %.tmp.0.us, %2                 ; <double> [#uses=2]
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 128       ; <i1> [#uses=1]
  br i1 %exitcond, label %bb5.us, label %bb3.us

bb.nph.us:                                        ; preds = %bb5.us, %bb.nph6.split.us
  %indvar46 = phi i64 [ %indvar.next47, %bb5.us ], [ 0, %bb.nph6.split.us ] ; <i64> [#uses=3]
  %scevgep54 = getelementptr [128 x [128 x [128 x double]]]* @sum, i64 0, i64 %tmp, i64 %tmp48, i64 %indvar46 ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep54, align 8
  br label %bb3.us

bb.nph8:                                          ; preds = %bb5.us
  %tmp63 = sext i32 %q.09 to i64                  ; <i64> [#uses=2]
  br label %bb8

bb8:                                              ; preds = %bb8, %bb.nph8
  %indvar59 = phi i64 [ 0, %bb.nph8 ], [ %indvar.next60, %bb8 ] ; <i64> [#uses=3]
  %scevgep64 = getelementptr [128 x [128 x [128 x double]]]* @A, i64 0, i64 %tmp, i64 %tmp63, i64 %indvar59 ; <double*> [#uses=1]
  %scevgep65 = getelementptr [128 x [128 x [128 x double]]]* @sum, i64 0, i64 %tmp, i64 %tmp63, i64 %indvar59 ; <double*> [#uses=1]
  %4 = load double* %scevgep65, align 8           ; <double> [#uses=1]
  store double %4, double* %scevgep64, align 8
  %indvar.next60 = add i64 %indvar59, 1           ; <i64> [#uses=2]
  %exitcond61 = icmp eq i64 %indvar.next60, 128   ; <i1> [#uses=1]
  br i1 %exitcond61, label %bb10, label %bb8

bb10:                                             ; preds = %bb8
  %5 = add nsw i32 %q.09, 1                       ; <i32> [#uses=2]
  %6 = icmp slt i32 %5, 128                       ; <i1> [#uses=1]
  br i1 %6, label %bb.nph6.split.us, label %bb12

bb.nph25:                                         ; preds = %bb12, %bb.nph45.bb.nph45.split_crit_edge
  %r.026 = phi i32 [ 0, %bb.nph45.bb.nph45.split_crit_edge ], [ %7, %bb12 ] ; <i32> [#uses=2]
  %tmp = sext i32 %r.026 to i64                   ; <i64> [#uses=4]
  br label %bb.nph6.split.us

bb12:                                             ; preds = %bb10
  %7 = add nsw i32 %r.026, 1                      ; <i32> [#uses=2]
  %8 = icmp slt i32 %7, 128                       ; <i1> [#uses=1]
  br i1 %8, label %bb.nph25, label %return

return:                                           ; preds = %bb12
  ret void
}

define i32 @main(i32 %argc, i8** nocapture %argv) nounwind {
entry:
  br label %bb.nph13.bb.nph13.split_crit_edge.i

bb.nph8.i:                                        ; preds = %bb.nph13.bb.nph13.split_crit_edge.i, %bb4.i
  %indvar32.i = phi i64 [ 0, %bb.nph13.bb.nph13.split_crit_edge.i ], [ %indvar.next33.i, %bb4.i ] ; <i64> [#uses=3]
  %j.09.i = trunc i64 %indvar32.i to i32          ; <i32> [#uses=1]
  %0 = sitofp i32 %j.09.i to double               ; <double> [#uses=1]
  %1 = fmul double %5, %0                         ; <double> [#uses=1]
  br label %bb2.i

bb2.i:                                            ; preds = %bb2.i, %bb.nph8.i
  %indvar27.i = phi i64 [ 0, %bb.nph8.i ], [ %indvar.next28.i, %bb2.i ] ; <i64> [#uses=3]
  %scevgep34.i = getelementptr [128 x [128 x [128 x double]]]* @A, i64 0, i64 %indvar30.i, i64 %indvar32.i, i64 %indvar27.i ; <double*> [#uses=1]
  %k.07.i = trunc i64 %indvar27.i to i32          ; <i32> [#uses=1]
  %2 = sitofp i32 %k.07.i to double               ; <double> [#uses=1]
  %3 = fadd double %1, %2                         ; <double> [#uses=1]
  %4 = fdiv double %3, 1.280000e+02               ; <double> [#uses=1]
  store double %4, double* %scevgep34.i, align 8
  %indvar.next28.i = add i64 %indvar27.i, 1       ; <i64> [#uses=2]
  %exitcond45 = icmp eq i64 %indvar.next28.i, 128 ; <i1> [#uses=1]
  br i1 %exitcond45, label %bb4.i, label %bb2.i

bb4.i:                                            ; preds = %bb2.i
  %indvar.next33.i = add i64 %indvar32.i, 1       ; <i64> [#uses=2]
  %exitcond47 = icmp eq i64 %indvar.next33.i, 128 ; <i1> [#uses=1]
  br i1 %exitcond47, label %bb6.i, label %bb.nph8.i

bb.nph13.bb.nph13.split_crit_edge.i:              ; preds = %bb6.i, %entry
  %indvar30.i = phi i64 [ 0, %entry ], [ %indvar.next31.i, %bb6.i ] ; <i64> [#uses=3]
  %i.014.i = trunc i64 %indvar30.i to i32         ; <i32> [#uses=1]
  %5 = sitofp i32 %i.014.i to double              ; <double> [#uses=1]
  br label %bb.nph8.i

bb6.i:                                            ; preds = %bb4.i
  %indvar.next31.i = add i64 %indvar30.i, 1       ; <i64> [#uses=2]
  %exitcond49 = icmp eq i64 %indvar.next31.i, 128 ; <i1> [#uses=1]
  br i1 %exitcond49, label %bb.nph.i, label %bb.nph13.bb.nph13.split_crit_edge.i

bb.nph.i:                                         ; preds = %bb12.i, %bb6.i
  %indvar23.i = phi i64 [ %indvar.next24.i, %bb12.i ], [ 0, %bb6.i ] ; <i64> [#uses=3]
  %i.12.i = trunc i64 %indvar23.i to i32          ; <i32> [#uses=1]
  %6 = sitofp i32 %i.12.i to double               ; <double> [#uses=1]
  br label %bb10.i

bb10.i:                                           ; preds = %bb10.i, %bb.nph.i
  %indvar.i = phi i64 [ 0, %bb.nph.i ], [ %indvar.next.i, %bb10.i ] ; <i64> [#uses=3]
  %scevgep.i = getelementptr [128 x [128 x double]]* @C4, i64 0, i64 %indvar23.i, i64 %indvar.i ; <double*> [#uses=1]
  %j.11.i = trunc i64 %indvar.i to i32            ; <i32> [#uses=1]
  %7 = sitofp i32 %j.11.i to double               ; <double> [#uses=1]
  %8 = fmul double %6, %7                         ; <double> [#uses=1]
  %9 = fdiv double %8, 1.280000e+02               ; <double> [#uses=1]
  store double %9, double* %scevgep.i, align 8
  %indvar.next.i = add i64 %indvar.i, 1           ; <i64> [#uses=2]
  %exitcond41 = icmp eq i64 %indvar.next.i, 128   ; <i1> [#uses=1]
  br i1 %exitcond41, label %bb12.i, label %bb10.i

bb12.i:                                           ; preds = %bb10.i
  %indvar.next24.i = add i64 %indvar23.i, 1       ; <i64> [#uses=2]
  %exitcond43 = icmp eq i64 %indvar.next24.i, 128 ; <i1> [#uses=1]
  br i1 %exitcond43, label %bb.nph25.i, label %bb.nph.i

bb.nph6.split.us.i:                               ; preds = %bb.nph25.i, %bb10.i15
  %indvar35 = phi i64 [ 0, %bb.nph25.i ], [ %indvar.next36, %bb10.i15 ] ; <i64> [#uses=5]
  br label %bb.nph.us.i

bb5.us.i:                                         ; preds = %bb3.us.i
  store double %13, double* %scevgep54.i
  %indvar.next47.i = add i64 %indvar46.i, 1       ; <i64> [#uses=2]
  %exitcond37 = icmp eq i64 %indvar.next47.i, 128 ; <i1> [#uses=1]
  br i1 %exitcond37, label %bb8.i14, label %bb.nph.us.i

bb3.us.i:                                         ; preds = %bb.nph.us.i, %bb3.us.i
  %indvar.i9 = phi i64 [ 0, %bb.nph.us.i ], [ %indvar.next.i11, %bb3.us.i ] ; <i64> [#uses=3]
  %.tmp.0.us.i = phi double [ 0.000000e+00, %bb.nph.us.i ], [ %13, %bb3.us.i ] ; <double> [#uses=1]
  %scevgep49.i = getelementptr [128 x [128 x [128 x double]]]* @A, i64 0, i64 %indvar, i64 %indvar35, i64 %indvar.i9 ; <double*> [#uses=1]
  %scevgep.i10 = getelementptr [128 x [128 x double]]* @C4, i64 0, i64 %indvar.i9, i64 %indvar46.i ; <double*> [#uses=1]
  %10 = load double* %scevgep49.i, align 8        ; <double> [#uses=1]
  %11 = load double* %scevgep.i10, align 8        ; <double> [#uses=1]
  %12 = fmul double %10, %11                      ; <double> [#uses=1]
  %13 = fadd double %.tmp.0.us.i, %12             ; <double> [#uses=2]
  %indvar.next.i11 = add i64 %indvar.i9, 1        ; <i64> [#uses=2]
  %exitcond34 = icmp eq i64 %indvar.next.i11, 128 ; <i1> [#uses=1]
  br i1 %exitcond34, label %bb5.us.i, label %bb3.us.i

bb.nph.us.i:                                      ; preds = %bb5.us.i, %bb.nph6.split.us.i
  %indvar46.i = phi i64 [ %indvar.next47.i, %bb5.us.i ], [ 0, %bb.nph6.split.us.i ] ; <i64> [#uses=3]
  %scevgep54.i = getelementptr [128 x [128 x [128 x double]]]* @sum, i64 0, i64 %indvar, i64 %indvar35, i64 %indvar46.i ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep54.i, align 8
  br label %bb3.us.i

bb8.i14:                                          ; preds = %bb8.i14, %bb5.us.i
  %indvar59.i = phi i64 [ %indvar.next60.i, %bb8.i14 ], [ 0, %bb5.us.i ] ; <i64> [#uses=3]
  %scevgep65.i = getelementptr [128 x [128 x [128 x double]]]* @sum, i64 0, i64 %indvar, i64 %indvar35, i64 %indvar59.i ; <double*> [#uses=1]
  %scevgep64.i = getelementptr [128 x [128 x [128 x double]]]* @A, i64 0, i64 %indvar, i64 %indvar35, i64 %indvar59.i ; <double*> [#uses=1]
  %14 = load double* %scevgep65.i, align 8        ; <double> [#uses=1]
  store double %14, double* %scevgep64.i, align 8
  %indvar.next60.i = add i64 %indvar59.i, 1       ; <i64> [#uses=2]
  %exitcond38 = icmp eq i64 %indvar.next60.i, 128 ; <i1> [#uses=1]
  br i1 %exitcond38, label %bb10.i15, label %bb8.i14

bb10.i15:                                         ; preds = %bb8.i14
  %indvar.next36 = add i64 %indvar35, 1           ; <i64> [#uses=2]
  %exitcond39 = icmp eq i64 %indvar.next36, 128   ; <i1> [#uses=1]
  br i1 %exitcond39, label %bb12.i17, label %bb.nph6.split.us.i

bb.nph25.i:                                       ; preds = %bb12.i17, %bb12.i
  %indvar = phi i64 [ %indvar.next, %bb12.i17 ], [ 0, %bb12.i ] ; <i64> [#uses=5]
  br label %bb.nph6.split.us.i

bb12.i17:                                         ; preds = %bb10.i15
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond40 = icmp eq i64 %indvar.next, 128     ; <i1> [#uses=1]
  br i1 %exitcond40, label %scop_func.exit, label %bb.nph25.i

scop_func.exit:                                   ; preds = %bb12.i17
  %15 = icmp sgt i32 %argc, 42                    ; <i1> [#uses=1]
  br i1 %15, label %bb.i, label %print_array.exit

bb.i:                                             ; preds = %scop_func.exit
  %16 = load i8** %argv, align 1                  ; <i8*> [#uses=1]
  %17 = load i8* %16, align 1                     ; <i8> [#uses=1]
  %18 = icmp eq i8 %17, 0                         ; <i1> [#uses=1]
  br i1 %18, label %bb9.preheader.i, label %print_array.exit

bb4.i3:                                           ; preds = %bb7.preheader.i, %bb6.i6
  %indvar.i1 = phi i64 [ %indvar.next.i4, %bb6.i6 ], [ 0, %bb7.preheader.i ] ; <i64> [#uses=3]
  %tmp32 = add i64 %tmp31, %indvar.i1             ; <i64> [#uses=1]
  %tmp23.i = trunc i64 %tmp32 to i32              ; <i32> [#uses=1]
  %scevgep.i2 = getelementptr [128 x [128 x [128 x double]]]* @A, i64 0, i64 %indvar16.i, i64 %indvar18.i, i64 %indvar.i1 ; <double*> [#uses=1]
  %19 = load double* %scevgep.i2, align 8         ; <double> [#uses=1]
  %20 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %21 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %20, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %19) nounwind ; <i32> [#uses=0]
  %22 = srem i32 %tmp23.i, 80                     ; <i32> [#uses=1]
  %23 = icmp eq i32 %22, 20                       ; <i1> [#uses=1]
  br i1 %23, label %bb5.i, label %bb6.i6

bb5.i:                                            ; preds = %bb4.i3
  %24 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %25 = bitcast %struct._IO_FILE* %24 to i8*      ; <i8*> [#uses=1]
  %26 = tail call i32 @fputc(i32 10, i8* %25) nounwind ; <i32> [#uses=0]
  br label %bb6.i6

bb6.i6:                                           ; preds = %bb5.i, %bb4.i3
  %indvar.next.i4 = add i64 %indvar.i1, 1         ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next.i4, 128    ; <i1> [#uses=1]
  br i1 %exitcond, label %bb8.i, label %bb4.i3

bb8.i:                                            ; preds = %bb6.i6
  %indvar.next19.i = add i64 %indvar18.i, 1       ; <i64> [#uses=2]
  %exitcond23 = icmp eq i64 %indvar.next19.i, 128 ; <i1> [#uses=1]
  br i1 %exitcond23, label %bb10.i7, label %bb7.preheader.i

bb7.preheader.i:                                  ; preds = %bb9.preheader.i, %bb8.i
  %indvar18.i = phi i64 [ %indvar.next19.i, %bb8.i ], [ 0, %bb9.preheader.i ] ; <i64> [#uses=3]
  %tmp = shl i64 %indvar18.i, 7                   ; <i64> [#uses=1]
  %tmp31 = add i64 %tmp30, %tmp                   ; <i64> [#uses=1]
  br label %bb4.i3

bb10.i7:                                          ; preds = %bb8.i
  %indvar.next17.i = add i64 %indvar16.i, 1       ; <i64> [#uses=2]
  %exitcond29 = icmp eq i64 %indvar.next17.i, 128 ; <i1> [#uses=1]
  br i1 %exitcond29, label %bb12.i8, label %bb9.preheader.i

bb9.preheader.i:                                  ; preds = %bb10.i7, %bb.i
  %indvar16.i = phi i64 [ %indvar.next17.i, %bb10.i7 ], [ 0, %bb.i ] ; <i64> [#uses=3]
  %tmp30 = shl i64 %indvar16.i, 7                 ; <i64> [#uses=1]
  br label %bb7.preheader.i

bb12.i8:                                          ; preds = %bb10.i7
  %27 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %28 = bitcast %struct._IO_FILE* %27 to i8*      ; <i8*> [#uses=1]
  %29 = tail call i32 @fputc(i32 10, i8* %28) nounwind ; <i32> [#uses=0]
  ret i32 0

print_array.exit:                                 ; preds = %bb.i, %scop_func.exit
  ret i32 0
}
