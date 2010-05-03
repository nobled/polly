; RUN: opt  -indvars -polly-scop-detect -print-top-scop-only -polly-print-scop -analyze  %s | FileCheck %s
; XFAIL: *
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }

@A = common global [1024 x [1024 x double]] zeroinitializer, align 32 ; <[1024 x [1024 x double]]*> [#uses=24]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]

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
  %scevgep = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar7, i64 %indvar ; <double*> [#uses=1]
  %j.01 = trunc i64 %indvar to i32                ; <i32> [#uses=1]
  %1 = sitofp i32 %j.01 to double                 ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=1]
  %3 = fadd double %2, 1.000000e+01               ; <double> [#uses=1]
  %4 = fdiv double %3, 1.024000e+03               ; <double> [#uses=1]
  store double %4, double* %scevgep, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 1024      ; <i1> [#uses=1]
  br i1 %exitcond, label %bb3, label %bb1

bb3:                                              ; preds = %bb1
  %indvar.next8 = add i64 %indvar7, 1             ; <i64> [#uses=2]
  %exitcond9 = icmp eq i64 %indvar.next8, 1024    ; <i1> [#uses=1]
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
  %scevgep = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar7, i64 %indvar ; <double*> [#uses=1]
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
  %exitcond = icmp eq i64 %indvar.next, 1024      ; <i1> [#uses=1]
  br i1 %exitcond, label %bb7, label %bb3

bb7:                                              ; preds = %bb5
  %indvar.next8 = add i64 %indvar7, 1             ; <i64> [#uses=2]
  %exitcond11 = icmp eq i64 %indvar.next8, 1024   ; <i1> [#uses=1]
  br i1 %exitcond11, label %bb9, label %bb6.preheader

bb6.preheader:                                    ; preds = %bb7, %bb
  %indvar7 = phi i64 [ %indvar.next8, %bb7 ], [ 0, %bb ] ; <i64> [#uses=3]
  %tmp12 = shl i64 %indvar7, 10                   ; <i64> [#uses=1]
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
bb.nph15.bb.nph15.split_crit_edge:
  br label %bb5.preheader

bb.nph:                                           ; preds = %bb5.preheader, %bb4
  %indvar16 = phi i64 [ %tmp39, %bb4 ], [ 0, %bb5.preheader ] ; <i64> [#uses=5]
  %tmp33 = add i64 %indvar16, 2                   ; <i64> [#uses=3]
  %tmp39 = add i64 %indvar16, 1                   ; <i64> [#uses=6]
  %scevgep19.phi.trans.insert = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar16, i64 1 ; <double*> [#uses=1]
  %.pre = load double* %scevgep19.phi.trans.insert, align 8 ; <double> [#uses=1]
  %scevgep.phi.trans.insert = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp39, i64 1 ; <double*> [#uses=1]
  %.pre44 = load double* %scevgep.phi.trans.insert, align 8 ; <double> [#uses=1]
  %scevgep21.phi.trans.insert = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp33, i64 1 ; <double*> [#uses=1]
  %.pre45 = load double* %scevgep21.phi.trans.insert, align 8 ; <double> [#uses=1]
  %scevgep25.phi.trans.insert = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp39, i64 0 ; <double*> [#uses=1]
  %.pre46 = load double* %scevgep25.phi.trans.insert, align 32 ; <double> [#uses=1]
  br label %bb2

bb2:                                              ; preds = %bb2, %bb.nph
  %0 = phi double [ %.pre46, %bb.nph ], [ %17, %bb2 ] ; <double> [#uses=1]
  %1 = phi double [ %.pre45, %bb.nph ], [ %15, %bb2 ] ; <double> [#uses=1]
  %2 = phi double [ %.pre44, %bb.nph ], [ %10, %bb2 ] ; <double> [#uses=1]
  %3 = phi double [ %.pre, %bb.nph ], [ %6, %bb2 ] ; <double> [#uses=1]
  %indvar = phi i64 [ 0, %bb.nph ], [ %tmp36, %bb2 ] ; <i64> [#uses=4]
  %tmp30 = add i64 %indvar, 2                     ; <i64> [#uses=3]
  %scevgep24 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar16, i64 %tmp30 ; <double*> [#uses=1]
  %scevgep22 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar16, i64 %indvar ; <double*> [#uses=1]
  %scevgep28 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp33, i64 %tmp30 ; <double*> [#uses=1]
  %scevgep27 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp33, i64 %indvar ; <double*> [#uses=1]
  %tmp36 = add i64 %indvar, 1                     ; <i64> [#uses=3]
  %scevgep26 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp39, i64 %tmp30 ; <double*> [#uses=1]
  %scevgep = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp39, i64 %tmp36 ; <double*> [#uses=1]
  %4 = load double* %scevgep22, align 8           ; <double> [#uses=1]
  %5 = fadd double %4, %3                         ; <double> [#uses=1]
  %6 = load double* %scevgep24, align 8           ; <double> [#uses=2]
  %7 = fadd double %5, %6                         ; <double> [#uses=1]
  %8 = fadd double %7, %0                         ; <double> [#uses=1]
  %9 = fadd double %8, %2                         ; <double> [#uses=1]
  %10 = load double* %scevgep26, align 8          ; <double> [#uses=2]
  %11 = fadd double %9, %10                       ; <double> [#uses=1]
  %12 = load double* %scevgep27, align 8          ; <double> [#uses=1]
  %13 = fadd double %11, %12                      ; <double> [#uses=1]
  %14 = fadd double %13, %1                       ; <double> [#uses=1]
  %15 = load double* %scevgep28, align 8          ; <double> [#uses=2]
  %16 = fadd double %14, %15                      ; <double> [#uses=1]
  %17 = fdiv double %16, 9.000000e+00             ; <double> [#uses=2]
  store double %17, double* %scevgep, align 8
  %exitcond = icmp eq i64 %tmp36, 1022            ; <i1> [#uses=1]
  br i1 %exitcond, label %bb4, label %bb2

bb4:                                              ; preds = %bb2
  %exitcond29 = icmp eq i64 %tmp39, 1022          ; <i1> [#uses=1]
  br i1 %exitcond29, label %bb6, label %bb.nph

bb6:                                              ; preds = %bb4
  %18 = add nsw i32 %t.07, 1                      ; <i32> [#uses=2]
  %exitcond43 = icmp eq i32 %18, 20               ; <i1> [#uses=1]
  br i1 %exitcond43, label %return, label %bb5.preheader

bb5.preheader:                                    ; preds = %bb6, %bb.nph15.bb.nph15.split_crit_edge
  %t.07 = phi i32 [ 0, %bb.nph15.bb.nph15.split_crit_edge ], [ %18, %bb6 ] ; <i32> [#uses=1]
  br label %bb.nph

return:                                           ; preds = %bb6
  ret void
}

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
  %scevgep.i = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar7.i, i64 %indvar.i ; <double*> [#uses=1]
  %j.01.i = trunc i64 %indvar.i to i32            ; <i32> [#uses=1]
  %1 = sitofp i32 %j.01.i to double               ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=1]
  %3 = fadd double %2, 1.000000e+01               ; <double> [#uses=1]
  %4 = fdiv double %3, 1.024000e+03               ; <double> [#uses=1]
  store double %4, double* %scevgep.i, align 8
  %indvar.next.i = add i64 %indvar.i, 1           ; <i64> [#uses=2]
  %exitcond30 = icmp eq i64 %indvar.next.i, 1024  ; <i1> [#uses=1]
  br i1 %exitcond30, label %bb3.i, label %bb1.i

bb3.i:                                            ; preds = %bb1.i
  %indvar.next8.i = add i64 %indvar7.i, 1         ; <i64> [#uses=2]
  %exitcond31 = icmp eq i64 %indvar.next8.i, 1024 ; <i1> [#uses=1]
  br i1 %exitcond31, label %bb5.preheader.i, label %bb.nph.i

bb.nph.i8:                                        ; preds = %bb5.preheader.i, %bb4.i12
  %indvar16.i = phi i64 [ %tmp27, %bb4.i12 ], [ 0, %bb5.preheader.i ] ; <i64> [#uses=5]
  %tmp26 = add i64 %indvar16.i, 2                 ; <i64> [#uses=3]
  %tmp27 = add i64 %indvar16.i, 1                 ; <i64> [#uses=6]
  %scevgep25.phi.trans.insert.i = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp27, i64 0 ; <double*> [#uses=1]
  %scevgep.phi.trans.insert.i = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp27, i64 1 ; <double*> [#uses=1]
  %scevgep21.phi.trans.insert.i = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp26, i64 1 ; <double*> [#uses=1]
  %scevgep19.phi.trans.insert.i = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar16.i, i64 1 ; <double*> [#uses=1]
  %.pre.i = load double* %scevgep19.phi.trans.insert.i, align 8 ; <double> [#uses=1]
  %.pre44.i = load double* %scevgep.phi.trans.insert.i, align 8 ; <double> [#uses=1]
  %.pre45.i = load double* %scevgep21.phi.trans.insert.i, align 8 ; <double> [#uses=1]
  %.pre46.i = load double* %scevgep25.phi.trans.insert.i, align 32 ; <double> [#uses=1]
  br label %bb2.i

bb2.i:                                            ; preds = %bb2.i, %bb.nph.i8
  %5 = phi double [ %.pre46.i, %bb.nph.i8 ], [ %22, %bb2.i ] ; <double> [#uses=1]
  %6 = phi double [ %.pre45.i, %bb.nph.i8 ], [ %20, %bb2.i ] ; <double> [#uses=1]
  %7 = phi double [ %.pre44.i, %bb.nph.i8 ], [ %15, %bb2.i ] ; <double> [#uses=1]
  %8 = phi double [ %.pre.i, %bb.nph.i8 ], [ %11, %bb2.i ] ; <double> [#uses=1]
  %indvar.i9 = phi i64 [ 0, %bb.nph.i8 ], [ %tmp28, %bb2.i ] ; <i64> [#uses=4]
  %scevgep22.i = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar16.i, i64 %indvar.i9 ; <double*> [#uses=1]
  %tmp25 = add i64 %indvar.i9, 2                  ; <i64> [#uses=3]
  %scevgep24.i = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar16.i, i64 %tmp25 ; <double*> [#uses=1]
  %scevgep27.i = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp26, i64 %indvar.i9 ; <double*> [#uses=1]
  %scevgep28.i = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp26, i64 %tmp25 ; <double*> [#uses=1]
  %scevgep26.i = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp27, i64 %tmp25 ; <double*> [#uses=1]
  %tmp28 = add i64 %indvar.i9, 1                  ; <i64> [#uses=3]
  %scevgep.i10 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp27, i64 %tmp28 ; <double*> [#uses=1]
  %9 = load double* %scevgep22.i, align 8         ; <double> [#uses=1]
  %10 = fadd double %9, %8                        ; <double> [#uses=1]
  %11 = load double* %scevgep24.i, align 8        ; <double> [#uses=2]
  %12 = fadd double %10, %11                      ; <double> [#uses=1]
  %13 = fadd double %12, %5                       ; <double> [#uses=1]
  %14 = fadd double %13, %7                       ; <double> [#uses=1]
  %15 = load double* %scevgep26.i, align 8        ; <double> [#uses=2]
  %16 = fadd double %14, %15                      ; <double> [#uses=1]
  %17 = load double* %scevgep27.i, align 8        ; <double> [#uses=1]
  %18 = fadd double %16, %17                      ; <double> [#uses=1]
  %19 = fadd double %18, %6                       ; <double> [#uses=1]
  %20 = load double* %scevgep28.i, align 8        ; <double> [#uses=2]
  %21 = fadd double %19, %20                      ; <double> [#uses=1]
  %22 = fdiv double %21, 9.000000e+00             ; <double> [#uses=2]
  store double %22, double* %scevgep.i10, align 8
  %exitcond20 = icmp eq i64 %tmp28, 1022          ; <i1> [#uses=1]
  br i1 %exitcond20, label %bb4.i12, label %bb2.i

bb4.i12:                                          ; preds = %bb2.i
  %exitcond24 = icmp eq i64 %tmp27, 1022          ; <i1> [#uses=1]
  br i1 %exitcond24, label %bb6.i, label %bb.nph.i8

bb6.i:                                            ; preds = %bb4.i12
  %23 = add nsw i32 %t.07.i, 1                    ; <i32> [#uses=2]
  %exitcond29 = icmp eq i32 %23, 20               ; <i1> [#uses=1]
  br i1 %exitcond29, label %scop_func.exit, label %bb5.preheader.i

bb5.preheader.i:                                  ; preds = %bb6.i, %bb3.i
  %t.07.i = phi i32 [ %23, %bb6.i ], [ 0, %bb3.i ] ; <i32> [#uses=1]
  br label %bb.nph.i8

scop_func.exit:                                   ; preds = %bb6.i
  %24 = icmp sgt i32 %argc, 42                    ; <i1> [#uses=1]
  br i1 %24, label %bb.i, label %print_array.exit

bb.i:                                             ; preds = %scop_func.exit
  %25 = load i8** %argv, align 1                  ; <i8*> [#uses=1]
  %26 = load i8* %25, align 1                     ; <i8> [#uses=1]
  %27 = icmp eq i8 %26, 0                         ; <i1> [#uses=1]
  br i1 %27, label %bb6.preheader.i, label %print_array.exit

bb3.i3:                                           ; preds = %bb6.preheader.i, %bb5.i
  %indvar.i1 = phi i64 [ %indvar.next.i4, %bb5.i ], [ 0, %bb6.preheader.i ] ; <i64> [#uses=3]
  %scevgep.i2 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar7.i7, i64 %indvar.i1 ; <double*> [#uses=1]
  %tmp18 = add i64 %tmp17, %indvar.i1             ; <i64> [#uses=1]
  %tmp10.i = trunc i64 %tmp18 to i32              ; <i32> [#uses=1]
  %28 = load double* %scevgep.i2, align 8         ; <double> [#uses=1]
  %29 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %30 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %29, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %28) nounwind ; <i32> [#uses=0]
  %31 = srem i32 %tmp10.i, 80                     ; <i32> [#uses=1]
  %32 = icmp eq i32 %31, 20                       ; <i1> [#uses=1]
  br i1 %32, label %bb4.i, label %bb5.i

bb4.i:                                            ; preds = %bb3.i3
  %33 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %34 = bitcast %struct._IO_FILE* %33 to i8*      ; <i8*> [#uses=1]
  %35 = tail call i32 @fputc(i32 10, i8* %34) nounwind ; <i32> [#uses=0]
  br label %bb5.i

bb5.i:                                            ; preds = %bb4.i, %bb3.i3
  %indvar.next.i4 = add i64 %indvar.i1, 1         ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next.i4, 1024   ; <i1> [#uses=1]
  br i1 %exitcond, label %bb7.i, label %bb3.i3

bb7.i:                                            ; preds = %bb5.i
  %indvar.next8.i6 = add i64 %indvar7.i7, 1       ; <i64> [#uses=2]
  %exitcond16 = icmp eq i64 %indvar.next8.i6, 1024 ; <i1> [#uses=1]
  br i1 %exitcond16, label %bb9.i, label %bb6.preheader.i

bb6.preheader.i:                                  ; preds = %bb7.i, %bb.i
  %indvar7.i7 = phi i64 [ %indvar.next8.i6, %bb7.i ], [ 0, %bb.i ] ; <i64> [#uses=3]
  %tmp17 = shl i64 %indvar7.i7, 10                ; <i64> [#uses=1]
  br label %bb3.i3

bb9.i:                                            ; preds = %bb7.i
  %36 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %37 = bitcast %struct._IO_FILE* %36 to i8*      ; <i8*> [#uses=1]
  %38 = tail call i32 @fputc(i32 10, i8* %37) nounwind ; <i32> [#uses=0]
  ret i32 0

print_array.exit:                                 ; preds = %bb.i, %scop_func.exit
  ret i32 0
}
