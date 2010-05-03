; RUN: opt  -indvars -polly-scop-detect -print-top-scop-only -polly-print-scop -analyze  %s | FileCheck %s
; XFAIL: *
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }

@A = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=8]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
@nrm = common global double 0.000000e+00          ; <double*> [#uses=2]
@R = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=2]
@Q = common global [512 x [512 x double]] zeroinitializer, align 32 ; <[512 x [512 x double]]*> [#uses=3]

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
  %scevgep = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %indvar7, i64 %indvar ; <double*> [#uses=1]
  %j.01 = trunc i64 %indvar to i32                ; <i32> [#uses=1]
  %1 = sitofp i32 %j.01 to double                 ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=1]
  %3 = fdiv double %2, 5.120000e+02               ; <double> [#uses=1]
  store double %3, double* %scevgep, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 512       ; <i1> [#uses=1]
  br i1 %exitcond, label %bb3, label %bb1

bb3:                                              ; preds = %bb1
  %indvar.next8 = add i64 %indvar7, 1             ; <i64> [#uses=2]
  %exitcond9 = icmp eq i64 %indvar.next8, 512     ; <i1> [#uses=1]
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
  %scevgep = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %indvar7, i64 %indvar ; <double*> [#uses=1]
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
  %exitcond = icmp eq i64 %indvar.next, 512       ; <i1> [#uses=1]
  br i1 %exitcond, label %bb7, label %bb3

bb7:                                              ; preds = %bb5
  %indvar.next8 = add i64 %indvar7, 1             ; <i64> [#uses=2]
  %exitcond11 = icmp eq i64 %indvar.next8, 512    ; <i1> [#uses=1]
  br i1 %exitcond11, label %bb9, label %bb6.preheader

bb6.preheader:                                    ; preds = %bb7, %bb
  %indvar7 = phi i64 [ %indvar.next8, %bb7 ], [ 0, %bb ] ; <i64> [#uses=3]
  %tmp12 = shl i64 %indvar7, 9                    ; <i64> [#uses=1]
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
bb.nph35:
  br label %bb.nph

bb.nph:                                           ; preds = %bb15, %bb.nph35
  %indvar36 = phi i64 [ 0, %bb.nph35 ], [ %tmp77, %bb15 ] ; <i64> [#uses=8]
  %tmp = mul i64 %indvar36, 513                   ; <i64> [#uses=2]
  %tmp74 = add i64 %tmp, 1                        ; <i64> [#uses=1]
  %tmp77 = add i64 %indvar36, 1                   ; <i64> [#uses=4]
  %tmp82 = sub i64 510, %indvar36                 ; <i64> [#uses=1]
  %j.010 = trunc i64 %tmp77 to i32                ; <i32> [#uses=1]
  %scevgep90 = getelementptr [512 x [512 x double]]* @R, i64 0, i64 0, i64 %tmp ; <double*> [#uses=1]
  %tmp62 = and i64 %tmp82, 4294967295             ; <i64> [#uses=1]
  %tmp63 = add i64 %tmp62, 1                      ; <i64> [#uses=1]
  store double 0.000000e+00, double* @nrm, align 8
  br label %bb1

bb1:                                              ; preds = %bb1, %bb.nph
  %indvar = phi i64 [ 0, %bb.nph ], [ %indvar.next, %bb1 ] ; <i64> [#uses=2]
  %nrm.tmp.0 = phi double [ 0.000000e+00, %bb.nph ], [ %2, %bb1 ] ; <double> [#uses=1]
  %scevgep = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %indvar, i64 %indvar36 ; <double*> [#uses=1]
  %0 = load double* %scevgep, align 8             ; <double> [#uses=2]
  %1 = fmul double %0, %0                         ; <double> [#uses=1]
  %2 = fadd double %1, %nrm.tmp.0                 ; <double> [#uses=3]
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 512       ; <i1> [#uses=1]
  br i1 %exitcond, label %bb.nph4, label %bb1

bb.nph4:                                          ; preds = %bb1
  store double %2, double* @nrm
  %3 = tail call double @sqrt(double %2) nounwind readonly ; <double> [#uses=2]
  store double %3, double* %scevgep90, align 8
  br label %bb4

bb4:                                              ; preds = %bb4, %bb.nph4
  %indvar38 = phi i64 [ 0, %bb.nph4 ], [ %indvar.next39, %bb4 ] ; <i64> [#uses=3]
  %scevgep42 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %indvar38, i64 %indvar36 ; <double*> [#uses=1]
  %scevgep41 = getelementptr [512 x [512 x double]]* @Q, i64 0, i64 %indvar38, i64 %indvar36 ; <double*> [#uses=1]
  %4 = load double* %scevgep42, align 8           ; <double> [#uses=1]
  %5 = fdiv double %4, %3                         ; <double> [#uses=1]
  store double %5, double* %scevgep41, align 8
  %indvar.next39 = add i64 %indvar38, 1           ; <i64> [#uses=2]
  %exitcond40 = icmp eq i64 %indvar.next39, 512   ; <i1> [#uses=1]
  br i1 %exitcond40, label %bb14.loopexit, label %bb4

bb.nph6:                                          ; preds = %bb14.loopexit1, %bb14.loopexit
  %indvar48 = phi i64 [ %indvar.next49, %bb14.loopexit1 ], [ 0, %bb14.loopexit ] ; <i64> [#uses=3]
  %tmp75 = add i64 %tmp74, %indvar48              ; <i64> [#uses=1]
  %scevgep72 = getelementptr [512 x [512 x double]]* @R, i64 0, i64 0, i64 %tmp75 ; <double*> [#uses=2]
  %tmp78 = add i64 %tmp77, %indvar48              ; <i64> [#uses=2]
  store double 0.000000e+00, double* %scevgep72, align 8
  br label %bb8

bb8:                                              ; preds = %bb8, %bb.nph6
  %indvar44 = phi i64 [ 0, %bb.nph6 ], [ %indvar.next45, %bb8 ] ; <i64> [#uses=3]
  %.tmp.0 = phi double [ 0.000000e+00, %bb.nph6 ], [ %9, %bb8 ] ; <double> [#uses=1]
  %scevgep51 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %indvar44, i64 %tmp78 ; <double*> [#uses=1]
  %scevgep47 = getelementptr [512 x [512 x double]]* @Q, i64 0, i64 %indvar44, i64 %indvar36 ; <double*> [#uses=1]
  %6 = load double* %scevgep47, align 8           ; <double> [#uses=1]
  %7 = load double* %scevgep51, align 8           ; <double> [#uses=1]
  %8 = fmul double %6, %7                         ; <double> [#uses=1]
  %9 = fadd double %.tmp.0, %8                    ; <double> [#uses=3]
  %indvar.next45 = add i64 %indvar44, 1           ; <i64> [#uses=2]
  %exitcond46 = icmp eq i64 %indvar.next45, 512   ; <i1> [#uses=1]
  br i1 %exitcond46, label %bb12.loopexit, label %bb8

bb11:                                             ; preds = %bb12.loopexit, %bb11
  %indvar52 = phi i64 [ %indvar.next53, %bb11 ], [ 0, %bb12.loopexit ] ; <i64> [#uses=3]
  %scevgep57 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %indvar52, i64 %tmp78 ; <double*> [#uses=2]
  %scevgep58 = getelementptr [512 x [512 x double]]* @Q, i64 0, i64 %indvar52, i64 %indvar36 ; <double*> [#uses=1]
  %10 = load double* %scevgep57, align 8          ; <double> [#uses=1]
  %11 = load double* %scevgep58, align 8          ; <double> [#uses=1]
  %12 = fmul double %11, %9                       ; <double> [#uses=1]
  %13 = fsub double %10, %12                      ; <double> [#uses=1]
  store double %13, double* %scevgep57, align 8
  %indvar.next53 = add i64 %indvar52, 1           ; <i64> [#uses=2]
  %exitcond54 = icmp eq i64 %indvar.next53, 512   ; <i1> [#uses=1]
  br i1 %exitcond54, label %bb14.loopexit1, label %bb11

bb12.loopexit:                                    ; preds = %bb8
  store double %9, double* %scevgep72
  br label %bb11

bb14.loopexit:                                    ; preds = %bb4
  %14 = icmp slt i32 %j.010, 512                  ; <i1> [#uses=1]
  br i1 %14, label %bb.nph6, label %bb15

bb14.loopexit1:                                   ; preds = %bb11
  %indvar.next49 = add i64 %indvar48, 1           ; <i64> [#uses=2]
  %exitcond64 = icmp eq i64 %indvar.next49, %tmp63 ; <i1> [#uses=1]
  br i1 %exitcond64, label %bb15, label %bb.nph6

bb15:                                             ; preds = %bb14.loopexit1, %bb14.loopexit
  %exitcond73 = icmp eq i64 %tmp77, 512           ; <i1> [#uses=1]
  br i1 %exitcond73, label %return, label %bb.nph

return:                                           ; preds = %bb15
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
  %scevgep.i = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %indvar7.i, i64 %indvar.i ; <double*> [#uses=1]
  %j.01.i = trunc i64 %indvar.i to i32            ; <i32> [#uses=1]
  %1 = sitofp i32 %j.01.i to double               ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=1]
  %3 = fdiv double %2, 5.120000e+02               ; <double> [#uses=1]
  store double %3, double* %scevgep.i, align 8
  %indvar.next.i = add i64 %indvar.i, 1           ; <i64> [#uses=2]
  %exitcond14 = icmp eq i64 %indvar.next.i, 512   ; <i1> [#uses=1]
  br i1 %exitcond14, label %bb3.i, label %bb1.i

bb3.i:                                            ; preds = %bb1.i
  %indvar.next8.i = add i64 %indvar7.i, 1         ; <i64> [#uses=2]
  %exitcond15 = icmp eq i64 %indvar.next8.i, 512  ; <i1> [#uses=1]
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
  %scevgep.i2 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %indvar7.i7, i64 %indvar.i1 ; <double*> [#uses=1]
  %tmp12 = add i64 %tmp11, %indvar.i1             ; <i64> [#uses=1]
  %tmp10.i = trunc i64 %tmp12 to i32              ; <i32> [#uses=1]
  %8 = load double* %scevgep.i2, align 8          ; <double> [#uses=1]
  %9 = load %struct._IO_FILE** @stderr, align 8   ; <%struct._IO_FILE*> [#uses=1]
  %10 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %9, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %8) nounwind ; <i32> [#uses=0]
  %11 = srem i32 %tmp10.i, 80                     ; <i32> [#uses=1]
  %12 = icmp eq i32 %11, 20                       ; <i1> [#uses=1]
  br i1 %12, label %bb4.i, label %bb5.i

bb4.i:                                            ; preds = %bb3.i3
  %13 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %14 = bitcast %struct._IO_FILE* %13 to i8*      ; <i8*> [#uses=1]
  %15 = tail call i32 @fputc(i32 10, i8* %14) nounwind ; <i32> [#uses=0]
  br label %bb5.i

bb5.i:                                            ; preds = %bb4.i, %bb3.i3
  %indvar.next.i4 = add i64 %indvar.i1, 1         ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next.i4, 512    ; <i1> [#uses=1]
  br i1 %exitcond, label %bb7.i, label %bb3.i3

bb7.i:                                            ; preds = %bb5.i
  %indvar.next8.i6 = add i64 %indvar7.i7, 1       ; <i64> [#uses=2]
  %exitcond10 = icmp eq i64 %indvar.next8.i6, 512 ; <i1> [#uses=1]
  br i1 %exitcond10, label %bb9.i, label %bb6.preheader.i

bb6.preheader.i:                                  ; preds = %bb7.i, %bb.i
  %indvar7.i7 = phi i64 [ %indvar.next8.i6, %bb7.i ], [ 0, %bb.i ] ; <i64> [#uses=3]
  %tmp11 = shl i64 %indvar7.i7, 9                 ; <i64> [#uses=1]
  br label %bb3.i3

bb9.i:                                            ; preds = %bb7.i
  %16 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %17 = bitcast %struct._IO_FILE* %16 to i8*      ; <i8*> [#uses=1]
  %18 = tail call i32 @fputc(i32 10, i8* %17) nounwind ; <i32> [#uses=0]
  ret i32 0

print_array.exit:                                 ; preds = %bb.i, %init_array.exit
  ret i32 0
}
