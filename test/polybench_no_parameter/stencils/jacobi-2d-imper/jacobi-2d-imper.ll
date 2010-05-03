; RUN: opt  -indvars -polly-scop-detect -print-top-scop-only -polly-print-scop -analyze  %s | FileCheck %s
; XFAIL: *
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }

@A = common global [1024 x [1024 x double]] zeroinitializer, align 32 ; <[1024 x [1024 x double]]*> [#uses=16]
@B = common global [1024 x [1024 x double]] zeroinitializer, align 32 ; <[1024 x [1024 x double]]*> [#uses=6]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]

define void @init_array() nounwind inlinehint {
bb.nph6.bb.nph6.split_crit_edge:
  br label %bb.nph

bb.nph:                                           ; preds = %bb3, %bb.nph6.bb.nph6.split_crit_edge
  %indvar7 = phi i64 [ 0, %bb.nph6.bb.nph6.split_crit_edge ], [ %indvar.next8, %bb3 ] ; <i64> [#uses=4]
  %i.02 = trunc i64 %indvar7 to i32               ; <i32> [#uses=1]
  %0 = sitofp i32 %i.02 to double                 ; <double> [#uses=1]
  br label %bb1

bb1:                                              ; preds = %bb1, %bb.nph
  %indvar = phi i64 [ 0, %bb.nph ], [ %indvar.next, %bb1 ] ; <i64> [#uses=4]
  %scevgep9 = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %indvar7, i64 %indvar ; <double*> [#uses=1]
  %scevgep = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar7, i64 %indvar ; <double*> [#uses=1]
  %j.01 = trunc i64 %indvar to i32                ; <i32> [#uses=1]
  %1 = sitofp i32 %j.01 to double                 ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=2]
  %3 = fadd double %2, 1.000000e+01               ; <double> [#uses=1]
  %4 = fdiv double %3, 1.024000e+03               ; <double> [#uses=1]
  store double %4, double* %scevgep, align 8
  %5 = fadd double %2, 1.100000e+01               ; <double> [#uses=1]
  %6 = fdiv double %5, 1.024000e+03               ; <double> [#uses=1]
  store double %6, double* %scevgep9, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 1024      ; <i1> [#uses=1]
  br i1 %exitcond, label %bb3, label %bb1

bb3:                                              ; preds = %bb1
  %indvar.next8 = add i64 %indvar7, 1             ; <i64> [#uses=2]
  %exitcond10 = icmp eq i64 %indvar.next8, 1024   ; <i1> [#uses=1]
  br i1 %exitcond10, label %return, label %bb.nph

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
bb.nph29:
  br label %bb5.preheader

bb.nph:                                           ; preds = %bb5.preheader, %bb4
  %indvar30 = phi i64 [ %tmp43, %bb4 ], [ 0, %bb5.preheader ] ; <i64> [#uses=3]
  %tmp43 = add i64 %indvar30, 1                   ; <i64> [#uses=3]
  %tmp46 = add i64 %indvar30, 3                   ; <i64> [#uses=1]
  %tmp48 = add i64 %indvar30, 2                   ; <i64> [#uses=4]
  %scevgep33.phi.trans.insert = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp48, i64 2 ; <double*> [#uses=1]
  %.pre = load double* %scevgep33.phi.trans.insert, align 16 ; <double> [#uses=1]
  br label %bb2

bb2:                                              ; preds = %bb2, %bb.nph
  %0 = phi double [ %.pre, %bb.nph ], [ %3, %bb2 ] ; <double> [#uses=1]
  %indvar = phi i64 [ 0, %bb.nph ], [ %tmp51, %bb2 ] ; <i64> [#uses=3]
  %tmp44 = add i64 %indvar, 2                     ; <i64> [#uses=3]
  %scevgep37 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp43, i64 %tmp44 ; <double*> [#uses=1]
  %scevgep35 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp46, i64 %tmp44 ; <double*> [#uses=1]
  %tmp49 = add i64 %indvar, 3                     ; <i64> [#uses=1]
  %scevgep41 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp48, i64 %tmp49 ; <double*> [#uses=1]
  %tmp51 = add i64 %indvar, 1                     ; <i64> [#uses=3]
  %scevgep39 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp48, i64 %tmp51 ; <double*> [#uses=1]
  %scevgep = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %tmp48, i64 %tmp44 ; <double*> [#uses=1]
  %1 = load double* %scevgep39, align 8           ; <double> [#uses=1]
  %2 = fadd double %0, %1                         ; <double> [#uses=1]
  %3 = load double* %scevgep41, align 8           ; <double> [#uses=2]
  %4 = fadd double %2, %3                         ; <double> [#uses=1]
  %5 = load double* %scevgep35, align 8           ; <double> [#uses=1]
  %6 = fadd double %4, %5                         ; <double> [#uses=1]
  %7 = load double* %scevgep37, align 8           ; <double> [#uses=1]
  %8 = fadd double %6, %7                         ; <double> [#uses=1]
  %9 = fmul double %8, 2.000000e-01               ; <double> [#uses=1]
  store double %9, double* %scevgep, align 8
  %exitcond = icmp eq i64 %tmp51, 1021            ; <i1> [#uses=1]
  br i1 %exitcond, label %bb4, label %bb2

bb4:                                              ; preds = %bb2
  %exitcond42 = icmp eq i64 %tmp43, 1021          ; <i1> [#uses=1]
  br i1 %exitcond42, label %bb9.preheader, label %bb.nph

bb8:                                              ; preds = %bb9.preheader, %bb8
  %indvar55 = phi i64 [ %indvar.next56, %bb8 ], [ 0, %bb9.preheader ] ; <i64> [#uses=2]
  %tmp65 = add i64 %indvar55, 2                   ; <i64> [#uses=2]
  %scevgep62 = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %tmp64, i64 %tmp65 ; <double*> [#uses=1]
  %scevgep61 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp64, i64 %tmp65 ; <double*> [#uses=1]
  %10 = load double* %scevgep62, align 8          ; <double> [#uses=1]
  store double %10, double* %scevgep61, align 8
  %indvar.next56 = add i64 %indvar55, 1           ; <i64> [#uses=2]
  %exitcond57 = icmp eq i64 %indvar.next56, 1021  ; <i1> [#uses=1]
  br i1 %exitcond57, label %bb10, label %bb8

bb10:                                             ; preds = %bb8
  %indvar.next59 = add i64 %indvar58, 1           ; <i64> [#uses=2]
  %exitcond63 = icmp eq i64 %indvar.next59, 1021  ; <i1> [#uses=1]
  br i1 %exitcond63, label %bb12, label %bb9.preheader

bb9.preheader:                                    ; preds = %bb10, %bb4
  %indvar58 = phi i64 [ %indvar.next59, %bb10 ], [ 0, %bb4 ] ; <i64> [#uses=2]
  %tmp64 = add i64 %indvar58, 2                   ; <i64> [#uses=2]
  br label %bb8

bb12:                                             ; preds = %bb10
  %11 = add nsw i32 %t.014, 1                     ; <i32> [#uses=2]
  %exitcond68 = icmp eq i32 %11, 20               ; <i1> [#uses=1]
  br i1 %exitcond68, label %return, label %bb5.preheader

bb5.preheader:                                    ; preds = %bb12, %bb.nph29
  %t.014 = phi i32 [ 0, %bb.nph29 ], [ %11, %bb12 ] ; <i32> [#uses=1]
  br label %bb.nph

return:                                           ; preds = %bb12
  ret void
}

define i32 @main(i32 %argc, i8** nocapture %argv) nounwind {
entry:
  br label %bb.nph.i

bb.nph.i:                                         ; preds = %bb3.i, %entry
  %indvar7.i = phi i64 [ 0, %entry ], [ %indvar.next8.i, %bb3.i ] ; <i64> [#uses=4]
  %i.02.i = trunc i64 %indvar7.i to i32           ; <i32> [#uses=1]
  %0 = sitofp i32 %i.02.i to double               ; <double> [#uses=1]
  br label %bb1.i

bb1.i:                                            ; preds = %bb1.i, %bb.nph.i
  %indvar.i = phi i64 [ 0, %bb.nph.i ], [ %indvar.next.i, %bb1.i ] ; <i64> [#uses=4]
  %scevgep9.i = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %indvar7.i, i64 %indvar.i ; <double*> [#uses=1]
  %scevgep.i = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar7.i, i64 %indvar.i ; <double*> [#uses=1]
  %j.01.i = trunc i64 %indvar.i to i32            ; <i32> [#uses=1]
  %1 = sitofp i32 %j.01.i to double               ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=2]
  %3 = fadd double %2, 1.000000e+01               ; <double> [#uses=1]
  %4 = fdiv double %3, 1.024000e+03               ; <double> [#uses=1]
  store double %4, double* %scevgep.i, align 8
  %5 = fadd double %2, 1.100000e+01               ; <double> [#uses=1]
  %6 = fdiv double %5, 1.024000e+03               ; <double> [#uses=1]
  store double %6, double* %scevgep9.i, align 8
  %indvar.next.i = add i64 %indvar.i, 1           ; <i64> [#uses=2]
  %exitcond39 = icmp eq i64 %indvar.next.i, 1024  ; <i1> [#uses=1]
  br i1 %exitcond39, label %bb3.i, label %bb1.i

bb3.i:                                            ; preds = %bb1.i
  %indvar.next8.i = add i64 %indvar7.i, 1         ; <i64> [#uses=2]
  %exitcond40 = icmp eq i64 %indvar.next8.i, 1024 ; <i1> [#uses=1]
  br i1 %exitcond40, label %bb5.preheader.i, label %bb.nph.i

bb.nph.i8:                                        ; preds = %bb5.preheader.i, %bb4.i12
  %indvar30.i = phi i64 [ %tmp27, %bb4.i12 ], [ 0, %bb5.preheader.i ] ; <i64> [#uses=3]
  %tmp27 = add i64 %indvar30.i, 1                 ; <i64> [#uses=3]
  %tmp29 = add i64 %indvar30.i, 3                 ; <i64> [#uses=1]
  %tmp30 = add i64 %indvar30.i, 2                 ; <i64> [#uses=4]
  %scevgep33.phi.trans.insert.i = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp30, i64 2 ; <double*> [#uses=1]
  %.pre.i = load double* %scevgep33.phi.trans.insert.i, align 16 ; <double> [#uses=1]
  br label %bb2.i

bb2.i:                                            ; preds = %bb2.i, %bb.nph.i8
  %7 = phi double [ %.pre.i, %bb.nph.i8 ], [ %10, %bb2.i ] ; <double> [#uses=1]
  %indvar.i9 = phi i64 [ 0, %bb.nph.i8 ], [ %tmp32, %bb2.i ] ; <i64> [#uses=3]
  %tmp28 = add i64 %indvar.i9, 2                  ; <i64> [#uses=3]
  %scevgep37.i = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp27, i64 %tmp28 ; <double*> [#uses=1]
  %scevgep35.i = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp29, i64 %tmp28 ; <double*> [#uses=1]
  %scevgep.i10 = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %tmp30, i64 %tmp28 ; <double*> [#uses=1]
  %tmp31 = add i64 %indvar.i9, 3                  ; <i64> [#uses=1]
  %scevgep41.i = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp30, i64 %tmp31 ; <double*> [#uses=1]
  %tmp32 = add i64 %indvar.i9, 1                  ; <i64> [#uses=3]
  %scevgep39.i = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp30, i64 %tmp32 ; <double*> [#uses=1]
  %8 = load double* %scevgep39.i, align 8         ; <double> [#uses=1]
  %9 = fadd double %7, %8                         ; <double> [#uses=1]
  %10 = load double* %scevgep41.i, align 8        ; <double> [#uses=2]
  %11 = fadd double %9, %10                       ; <double> [#uses=1]
  %12 = load double* %scevgep35.i, align 8        ; <double> [#uses=1]
  %13 = fadd double %11, %12                      ; <double> [#uses=1]
  %14 = load double* %scevgep37.i, align 8        ; <double> [#uses=1]
  %15 = fadd double %13, %14                      ; <double> [#uses=1]
  %16 = fmul double %15, 2.000000e-01             ; <double> [#uses=1]
  store double %16, double* %scevgep.i10, align 8
  %exitcond20 = icmp eq i64 %tmp32, 1021          ; <i1> [#uses=1]
  br i1 %exitcond20, label %bb4.i12, label %bb2.i

bb4.i12:                                          ; preds = %bb2.i
  %exitcond26 = icmp eq i64 %tmp27, 1021          ; <i1> [#uses=1]
  br i1 %exitcond26, label %bb9.preheader.i, label %bb.nph.i8

bb8.i:                                            ; preds = %bb9.preheader.i, %bb8.i
  %indvar55.i = phi i64 [ %indvar.next56.i, %bb8.i ], [ 0, %bb9.preheader.i ] ; <i64> [#uses=2]
  %tmp37 = add i64 %indvar55.i, 2                 ; <i64> [#uses=2]
  %scevgep62.i = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %tmp36, i64 %tmp37 ; <double*> [#uses=1]
  %scevgep61.i = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp36, i64 %tmp37 ; <double*> [#uses=1]
  %17 = load double* %scevgep62.i, align 8        ; <double> [#uses=1]
  store double %17, double* %scevgep61.i, align 8
  %indvar.next56.i = add i64 %indvar55.i, 1       ; <i64> [#uses=2]
  %exitcond33 = icmp eq i64 %indvar.next56.i, 1021 ; <i1> [#uses=1]
  br i1 %exitcond33, label %bb10.i, label %bb8.i

bb10.i:                                           ; preds = %bb8.i
  %indvar.next59.i = add i64 %indvar58.i, 1       ; <i64> [#uses=2]
  %exitcond35 = icmp eq i64 %indvar.next59.i, 1021 ; <i1> [#uses=1]
  br i1 %exitcond35, label %bb12.i, label %bb9.preheader.i

bb9.preheader.i:                                  ; preds = %bb10.i, %bb4.i12
  %indvar58.i = phi i64 [ %indvar.next59.i, %bb10.i ], [ 0, %bb4.i12 ] ; <i64> [#uses=2]
  %tmp36 = add i64 %indvar58.i, 2                 ; <i64> [#uses=2]
  br label %bb8.i

bb12.i:                                           ; preds = %bb10.i
  %18 = add nsw i32 %t.014.i, 1                   ; <i32> [#uses=2]
  %exitcond38 = icmp eq i32 %18, 20               ; <i1> [#uses=1]
  br i1 %exitcond38, label %scop_func.exit, label %bb5.preheader.i

bb5.preheader.i:                                  ; preds = %bb12.i, %bb3.i
  %t.014.i = phi i32 [ %18, %bb12.i ], [ 0, %bb3.i ] ; <i32> [#uses=1]
  br label %bb.nph.i8

scop_func.exit:                                   ; preds = %bb12.i
  %19 = icmp sgt i32 %argc, 42                    ; <i1> [#uses=1]
  br i1 %19, label %bb.i, label %print_array.exit

bb.i:                                             ; preds = %scop_func.exit
  %20 = load i8** %argv, align 1                  ; <i8*> [#uses=1]
  %21 = load i8* %20, align 1                     ; <i8> [#uses=1]
  %22 = icmp eq i8 %21, 0                         ; <i1> [#uses=1]
  br i1 %22, label %bb6.preheader.i, label %print_array.exit

bb3.i3:                                           ; preds = %bb6.preheader.i, %bb5.i
  %indvar.i1 = phi i64 [ %indvar.next.i4, %bb5.i ], [ 0, %bb6.preheader.i ] ; <i64> [#uses=3]
  %scevgep.i2 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar7.i7, i64 %indvar.i1 ; <double*> [#uses=1]
  %tmp18 = add i64 %tmp17, %indvar.i1             ; <i64> [#uses=1]
  %tmp10.i = trunc i64 %tmp18 to i32              ; <i32> [#uses=1]
  %23 = load double* %scevgep.i2, align 8         ; <double> [#uses=1]
  %24 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %25 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %24, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %23) nounwind ; <i32> [#uses=0]
  %26 = srem i32 %tmp10.i, 80                     ; <i32> [#uses=1]
  %27 = icmp eq i32 %26, 20                       ; <i1> [#uses=1]
  br i1 %27, label %bb4.i, label %bb5.i

bb4.i:                                            ; preds = %bb3.i3
  %28 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %29 = bitcast %struct._IO_FILE* %28 to i8*      ; <i8*> [#uses=1]
  %30 = tail call i32 @fputc(i32 10, i8* %29) nounwind ; <i32> [#uses=0]
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
  %31 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %32 = bitcast %struct._IO_FILE* %31 to i8*      ; <i8*> [#uses=1]
  %33 = tail call i32 @fputc(i32 10, i8* %32) nounwind ; <i32> [#uses=0]
  ret i32 0

print_array.exit:                                 ; preds = %bb.i, %scop_func.exit
  ret i32 0
}
