; RUN: opt  -indvars -polly-scop-detect -print-top-scop-only -polly-print-scop -analyze  %s | FileCheck %s
; XFAIL: *
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }

@A = common global [1024 x [1024 x double]] zeroinitializer, align 32 ; <[1024 x [1024 x double]]*> [#uses=14]
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
  %3 = fadd double %2, 1.000000e+00               ; <double> [#uses=1]
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
bb.nph16:
  br label %bb2.preheader

bb.nph:                                           ; preds = %bb2.preheader
  %0 = sext i32 %k.012 to i64                     ; <i64> [#uses=3]
  %1 = getelementptr inbounds [1024 x [1024 x double]]* @A, i64 0, i64 %0, i64 %0 ; <double*> [#uses=1]
  %tmp = sub i32 1022, %k.012                     ; <i32> [#uses=1]
  %tmp17 = zext i32 %tmp to i64                   ; <i64> [#uses=1]
  %tmp18 = add i64 %tmp17, 1                      ; <i64> [#uses=1]
  %tmp21 = sext i32 %j.02 to i64                  ; <i64> [#uses=1]
  br label %bb1

bb1:                                              ; preds = %bb1, %bb.nph
  %indvar = phi i64 [ 0, %bb.nph ], [ %indvar.next, %bb1 ] ; <i64> [#uses=2]
  %tmp22 = add i64 %tmp21, %indvar                ; <i64> [#uses=1]
  %scevgep = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %0, i64 %tmp22 ; <double*> [#uses=2]
  %2 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %3 = load double* %1, align 8                   ; <double> [#uses=1]
  %4 = fdiv double %2, %3                         ; <double> [#uses=1]
  store double %4, double* %scevgep, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, %tmp18    ; <i1> [#uses=1]
  br i1 %exitcond, label %bb8.loopexit, label %bb1

bb.nph5:                                          ; preds = %bb.nph11, %bb8.loopexit1
  %indvar29 = phi i64 [ 0, %bb.nph11 ], [ %indvar.next30, %bb8.loopexit1 ] ; <i64> [#uses=2]
  %tmp43 = add i64 %tmp42, %indvar29              ; <i64> [#uses=2]
  %scevgep48 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp43, i64 %tmp36 ; <double*> [#uses=1]
  %tmp25 = sub i32 1022, %k.012                   ; <i32> [#uses=1]
  %tmp26 = zext i32 %tmp25 to i64                 ; <i64> [#uses=1]
  %tmp27 = add i64 %tmp26, 1                      ; <i64> [#uses=1]
  br label %bb5

bb5:                                              ; preds = %bb5, %bb.nph5
  %indvar23 = phi i64 [ 0, %bb.nph5 ], [ %indvar.next24, %bb5 ] ; <i64> [#uses=2]
  %tmp45 = add i64 %tmp42, %indvar23              ; <i64> [#uses=2]
  %scevgep35 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp43, i64 %tmp45 ; <double*> [#uses=2]
  %scevgep37 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp36, i64 %tmp45 ; <double*> [#uses=1]
  %5 = load double* %scevgep35, align 8           ; <double> [#uses=1]
  %6 = load double* %scevgep48, align 8           ; <double> [#uses=1]
  %7 = load double* %scevgep37, align 8           ; <double> [#uses=1]
  %8 = fmul double %6, %7                         ; <double> [#uses=1]
  %9 = fsub double %5, %8                         ; <double> [#uses=1]
  store double %9, double* %scevgep35, align 8
  %indvar.next24 = add i64 %indvar23, 1           ; <i64> [#uses=2]
  %exitcond28 = icmp eq i64 %indvar.next24, %tmp27 ; <i1> [#uses=1]
  br i1 %exitcond28, label %bb8.loopexit1, label %bb5

bb8.loopexit:                                     ; preds = %bb1
  br i1 %10, label %bb.nph11, label %return

bb8.loopexit1:                                    ; preds = %bb5
  %indvar.next30 = add i64 %indvar29, 1           ; <i64> [#uses=2]
  %exitcond41 = icmp eq i64 %indvar.next30, %tmp40 ; <i1> [#uses=1]
  br i1 %exitcond41, label %bb10.loopexit, label %bb.nph5

bb.nph11:                                         ; preds = %bb8.loopexit
  %tmp36 = sext i32 %k.012 to i64                 ; <i64> [#uses=2]
  %tmp38 = sub i32 1022, %k.012                   ; <i32> [#uses=1]
  %tmp39 = zext i32 %tmp38 to i64                 ; <i64> [#uses=1]
  %tmp40 = add i64 %tmp39, 1                      ; <i64> [#uses=1]
  %tmp42 = sext i32 %j.02 to i64                  ; <i64> [#uses=2]
  br label %bb.nph5

bb10.loopexit:                                    ; preds = %bb8.loopexit1
  br i1 %10, label %bb2.preheader, label %return

bb2.preheader:                                    ; preds = %bb10.loopexit, %bb.nph16
  %k.012 = phi i32 [ 0, %bb.nph16 ], [ %j.02, %bb10.loopexit ] ; <i32> [#uses=6]
  %j.02 = add i32 %k.012, 1                       ; <i32> [#uses=4]
  %10 = icmp slt i32 %j.02, 1024                  ; <i1> [#uses=3]
  br i1 %10, label %bb.nph, label %return

return:                                           ; preds = %bb2.preheader, %bb10.loopexit, %bb8.loopexit
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
  %3 = fadd double %2, 1.000000e+00               ; <double> [#uses=1]
  %4 = fdiv double %3, 1.024000e+03               ; <double> [#uses=1]
  store double %4, double* %scevgep.i, align 8
  %indvar.next.i = add i64 %indvar.i, 1           ; <i64> [#uses=2]
  %exitcond63 = icmp eq i64 %indvar.next.i, 1024  ; <i1> [#uses=1]
  br i1 %exitcond63, label %bb3.i, label %bb1.i

bb3.i:                                            ; preds = %bb1.i
  %indvar.next8.i = add i64 %indvar7.i, 1         ; <i64> [#uses=2]
  %exitcond66 = icmp eq i64 %indvar.next8.i, 1024 ; <i1> [#uses=1]
  br i1 %exitcond66, label %bb2.preheader, label %bb.nph.i

bb.nph:                                           ; preds = %bb2.preheader
  %5 = sext i32 %k.019 to i64                     ; <i64> [#uses=3]
  %6 = getelementptr inbounds [1024 x [1024 x double]]* @A, i64 0, i64 %5, i64 %5 ; <double*> [#uses=1]
  %tmp = sub i32 1022, %k.019                     ; <i32> [#uses=1]
  %tmp30 = zext i32 %tmp to i64                   ; <i64> [#uses=1]
  %tmp31 = add i64 %tmp30, 1                      ; <i64> [#uses=1]
  %tmp35 = sext i32 %j.09 to i64                  ; <i64> [#uses=1]
  br label %bb1

bb1:                                              ; preds = %bb1, %bb.nph
  %indvar = phi i64 [ 0, %bb.nph ], [ %indvar.next, %bb1 ] ; <i64> [#uses=2]
  %tmp36 = add i64 %tmp35, %indvar                ; <i64> [#uses=1]
  %scevgep = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %5, i64 %tmp36 ; <double*> [#uses=2]
  %7 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %8 = load double* %6, align 8                   ; <double> [#uses=1]
  %9 = fdiv double %7, %8                         ; <double> [#uses=1]
  store double %9, double* %scevgep, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond32 = icmp eq i64 %indvar.next, %tmp31  ; <i1> [#uses=1]
  br i1 %exitcond32, label %bb8.loopexit, label %bb1

bb.nph12:                                         ; preds = %bb.nph18, %bb8.loopexit8
  %indvar43 = phi i64 [ 0, %bb.nph18 ], [ %indvar.next44, %bb8.loopexit8 ] ; <i64> [#uses=2]
  %tmp57 = add i64 %tmp56, %indvar43              ; <i64> [#uses=2]
  %scevgep62 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp57, i64 %tmp50 ; <double*> [#uses=1]
  %tmp39 = sub i32 1022, %k.019                   ; <i32> [#uses=1]
  %tmp40 = zext i32 %tmp39 to i64                 ; <i64> [#uses=1]
  %tmp41 = add i64 %tmp40, 1                      ; <i64> [#uses=1]
  br label %bb5

bb5:                                              ; preds = %bb5, %bb.nph12
  %indvar37 = phi i64 [ 0, %bb.nph12 ], [ %indvar.next38, %bb5 ] ; <i64> [#uses=2]
  %tmp59 = add i64 %tmp56, %indvar37              ; <i64> [#uses=2]
  %scevgep49 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp57, i64 %tmp59 ; <double*> [#uses=2]
  %scevgep51 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp50, i64 %tmp59 ; <double*> [#uses=1]
  %10 = load double* %scevgep49, align 8          ; <double> [#uses=1]
  %11 = load double* %scevgep62, align 8          ; <double> [#uses=1]
  %12 = load double* %scevgep51, align 8          ; <double> [#uses=1]
  %13 = fmul double %11, %12                      ; <double> [#uses=1]
  %14 = fsub double %10, %13                      ; <double> [#uses=1]
  store double %14, double* %scevgep49, align 8
  %indvar.next38 = add i64 %indvar37, 1           ; <i64> [#uses=2]
  %exitcond42 = icmp eq i64 %indvar.next38, %tmp41 ; <i1> [#uses=1]
  br i1 %exitcond42, label %bb8.loopexit8, label %bb5

bb8.loopexit:                                     ; preds = %bb1
  br i1 %15, label %bb.nph18, label %bb11

bb8.loopexit8:                                    ; preds = %bb5
  %indvar.next44 = add i64 %indvar43, 1           ; <i64> [#uses=2]
  %exitcond55 = icmp eq i64 %indvar.next44, %tmp54 ; <i1> [#uses=1]
  br i1 %exitcond55, label %bb10.loopexit, label %bb.nph12

bb.nph18:                                         ; preds = %bb8.loopexit
  %tmp50 = sext i32 %k.019 to i64                 ; <i64> [#uses=2]
  %tmp52 = sub i32 1022, %k.019                   ; <i32> [#uses=1]
  %tmp53 = zext i32 %tmp52 to i64                 ; <i64> [#uses=1]
  %tmp54 = add i64 %tmp53, 1                      ; <i64> [#uses=1]
  %tmp56 = sext i32 %j.09 to i64                  ; <i64> [#uses=2]
  br label %bb.nph12

bb10.loopexit:                                    ; preds = %bb8.loopexit8
  br i1 %15, label %bb2.preheader, label %bb11

bb2.preheader:                                    ; preds = %bb10.loopexit, %bb3.i
  %k.019 = phi i32 [ %j.09, %bb10.loopexit ], [ 0, %bb3.i ] ; <i32> [#uses=6]
  %j.09 = add i32 %k.019, 1                       ; <i32> [#uses=4]
  %15 = icmp slt i32 %j.09, 1024                  ; <i1> [#uses=3]
  br i1 %15, label %bb.nph, label %bb11

bb11:                                             ; preds = %bb2.preheader, %bb10.loopexit, %bb8.loopexit
  %16 = icmp sgt i32 %argc, 42                    ; <i1> [#uses=1]
  br i1 %16, label %bb.i, label %print_array.exit

bb.i:                                             ; preds = %bb11
  %17 = load i8** %argv, align 1                  ; <i8*> [#uses=1]
  %18 = load i8* %17, align 1                     ; <i8> [#uses=1]
  %19 = icmp eq i8 %18, 0                         ; <i1> [#uses=1]
  br i1 %19, label %bb6.preheader.i, label %print_array.exit

bb3.i3:                                           ; preds = %bb6.preheader.i, %bb5.i
  %indvar.i1 = phi i64 [ %indvar.next.i4, %bb5.i ], [ 0, %bb6.preheader.i ] ; <i64> [#uses=3]
  %scevgep.i2 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar7.i7, i64 %indvar.i1 ; <double*> [#uses=1]
  %tmp28 = add i64 %tmp27, %indvar.i1             ; <i64> [#uses=1]
  %tmp10.i = trunc i64 %tmp28 to i32              ; <i32> [#uses=1]
  %20 = load double* %scevgep.i2, align 8         ; <double> [#uses=1]
  %21 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %22 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %21, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %20) nounwind ; <i32> [#uses=0]
  %23 = srem i32 %tmp10.i, 80                     ; <i32> [#uses=1]
  %24 = icmp eq i32 %23, 20                       ; <i1> [#uses=1]
  br i1 %24, label %bb4.i, label %bb5.i

bb4.i:                                            ; preds = %bb3.i3
  %25 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %26 = bitcast %struct._IO_FILE* %25 to i8*      ; <i8*> [#uses=1]
  %27 = tail call i32 @fputc(i32 10, i8* %26) nounwind ; <i32> [#uses=0]
  br label %bb5.i

bb5.i:                                            ; preds = %bb4.i, %bb3.i3
  %indvar.next.i4 = add i64 %indvar.i1, 1         ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next.i4, 1024   ; <i1> [#uses=1]
  br i1 %exitcond, label %bb7.i, label %bb3.i3

bb7.i:                                            ; preds = %bb5.i
  %indvar.next8.i6 = add i64 %indvar7.i7, 1       ; <i64> [#uses=2]
  %exitcond26 = icmp eq i64 %indvar.next8.i6, 1024 ; <i1> [#uses=1]
  br i1 %exitcond26, label %bb9.i, label %bb6.preheader.i

bb6.preheader.i:                                  ; preds = %bb7.i, %bb.i
  %indvar7.i7 = phi i64 [ %indvar.next8.i6, %bb7.i ], [ 0, %bb.i ] ; <i64> [#uses=3]
  %tmp27 = shl i64 %indvar7.i7, 10                ; <i64> [#uses=1]
  br label %bb3.i3

bb9.i:                                            ; preds = %bb7.i
  %28 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %29 = bitcast %struct._IO_FILE* %28 to i8*      ; <i8*> [#uses=1]
  %30 = tail call i32 @fputc(i32 10, i8* %29) nounwind ; <i32> [#uses=0]
  ret i32 0

print_array.exit:                                 ; preds = %bb.i, %bb11
  ret i32 0
}
