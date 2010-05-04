; RUN: opt -indvars -polly-scop-detect  -analyze %s | FileCheck %s
; XFAIL: *
; ModuleID = '<stdin>'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }

@A = common global [1024 x [1024 x double]] zeroinitializer, align 32 ; <[1024 x [1024 x double]]*> [#uses=14]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]

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
  %scevgep = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar8, i64 %indvar ; <double*> [#uses=1]
  %storemerge12 = trunc i64 %indvar to i32        ; <i32> [#uses=1]
  %1 = sitofp i32 %storemerge12 to double         ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=1]
  %3 = fadd double %2, 1.000000e+00               ; <double> [#uses=1]
  %4 = fdiv double %3, 1.024000e+03               ; <double> [#uses=1]
  store double %4, double* %scevgep, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 1024      ; <i1> [#uses=1]
  br i1 %exitcond, label %bb3, label %bb1

bb3:                                              ; preds = %bb1
  %indvar.next9 = add i64 %indvar8, 1             ; <i64> [#uses=2]
  %exitcond10 = icmp eq i64 %indvar.next9, 1024   ; <i1> [#uses=1]
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
  %scevgep = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar8, i64 %indvar ; <double*> [#uses=1]
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
  %exitcond = icmp eq i64 %indvar.next, 1024      ; <i1> [#uses=1]
  br i1 %exitcond, label %bb7, label %bb3

bb7:                                              ; preds = %bb5
  %indvar.next9 = add i64 %indvar8, 1             ; <i64> [#uses=2]
  %exitcond12 = icmp eq i64 %indvar.next9, 1024   ; <i1> [#uses=1]
  br i1 %exitcond12, label %bb9, label %bb6.preheader

bb6.preheader:                                    ; preds = %bb7, %bb
  %indvar8 = phi i64 [ %indvar.next9, %bb7 ], [ 0, %bb ] ; <i64> [#uses=3]
  %tmp13 = shl i64 %indvar8, 10                   ; <i64> [#uses=1]
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

define void @scop_func(i64 %n) nounwind {
entry:
  %0 = icmp sgt i64 %n, 0                         ; <i1> [#uses=1]
  br i1 %0, label %bb.nph28, label %return

bb1:                                              ; preds = %bb2.preheader, %bb1
  %indvar = phi i64 [ %indvar.next, %bb1 ], [ 0, %bb2.preheader ] ; <i64> [#uses=2]
  %tmp66 = add i64 %tmp60, %indvar                ; <i64> [#uses=1]
  %scevgep = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 0, i64 %tmp66 ; <double*> [#uses=2]
  %1 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %2 = load double* %scevgep69, align 8           ; <double> [#uses=1]
  %3 = fdiv double %1, %2                         ; <double> [#uses=1]
  store double %3, double* %scevgep, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, %tmp30    ; <i1> [#uses=1]
  br i1 %exitcond, label %bb8.loopexit, label %bb1

bb5:                                              ; preds = %bb6.preheader, %bb5
  %indvar34 = phi i64 [ %indvar.next35, %bb5 ], [ 0, %bb6.preheader ] ; <i64> [#uses=2]
  %tmp61 = add i64 %tmp60, %indvar34              ; <i64> [#uses=2]
  %scevgep45 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp57, i64 %tmp61 ; <double*> [#uses=2]
  %scevgep46 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 0, i64 %tmp61 ; <double*> [#uses=1]
  %4 = load double* %scevgep45, align 8           ; <double> [#uses=1]
  %5 = load double* %scevgep55, align 8           ; <double> [#uses=1]
  %6 = load double* %scevgep46, align 8           ; <double> [#uses=1]
  %7 = fmul double %5, %6                         ; <double> [#uses=1]
  %8 = fsub double %4, %7                         ; <double> [#uses=1]
  store double %8, double* %scevgep45, align 8
  %indvar.next35 = add i64 %indvar34, 1           ; <i64> [#uses=2]
  %exitcond38 = icmp eq i64 %indvar.next35, %tmp30 ; <i1> [#uses=1]
  br i1 %exitcond38, label %bb8.loopexit4, label %bb5

bb8.loopexit:                                     ; preds = %bb1
  br i1 %9, label %bb6.preheader, label %bb9

bb8.loopexit4:                                    ; preds = %bb5
  %exitcond49 = icmp eq i64 %tmp57, %tmp30        ; <i1> [#uses=1]
  br i1 %exitcond49, label %bb9, label %bb6.preheader

bb6.preheader:                                    ; preds = %bb8.loopexit4, %bb8.loopexit
  %indvar39 = phi i64 [ %tmp57, %bb8.loopexit4 ], [ 0, %bb8.loopexit ] ; <i64> [#uses=1]
  %tmp57 = add i64 %indvar39, 1                   ; <i64> [#uses=4]
  %scevgep55 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp57, i64 %tmp58 ; <double*> [#uses=1]
  br label %bb5

bb9:                                              ; preds = %bb2.preheader, %bb8.loopexit4, %bb8.loopexit
  %exitcond56 = icmp eq i64 %storemerge15, %n     ; <i1> [#uses=1]
  br i1 %exitcond56, label %return, label %bb2.preheader

bb.nph28:                                         ; preds = %entry
  %tmp29 = add i64 %n, -1                         ; <i64> [#uses=1]
  br label %bb2.preheader

bb2.preheader:                                    ; preds = %bb.nph28, %bb9
  %storemerge17 = phi i64 [ 0, %bb.nph28 ], [ %storemerge15, %bb9 ] ; <i64> [#uses=3]
  %tmp58 = mul i64 %storemerge17, 1025            ; <i64> [#uses=3]
  %tmp60 = add i64 %tmp58, 1                      ; <i64> [#uses=2]
  %tmp30 = sub i64 %tmp29, %storemerge17          ; <i64> [#uses=3]
  %storemerge15 = add i64 %storemerge17, 1        ; <i64> [#uses=3]
  %scevgep69 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 0, i64 %tmp58 ; <double*> [#uses=1]
  %9 = icmp slt i64 %storemerge15, %n             ; <i1> [#uses=2]
  br i1 %9, label %bb1, label %bb9

return:                                           ; preds = %bb9, %entry
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
  %scevgep.i = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar8.i, i64 %indvar.i ; <double*> [#uses=1]
  %storemerge12.i = trunc i64 %indvar.i to i32    ; <i32> [#uses=1]
  %1 = sitofp i32 %storemerge12.i to double       ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=1]
  %3 = fadd double %2, 1.000000e+00               ; <double> [#uses=1]
  %4 = fdiv double %3, 1.024000e+03               ; <double> [#uses=1]
  store double %4, double* %scevgep.i, align 8
  %indvar.next.i = add i64 %indvar.i, 1           ; <i64> [#uses=2]
  %exitcond51 = icmp eq i64 %indvar.next.i, 1024  ; <i1> [#uses=1]
  br i1 %exitcond51, label %bb3.i, label %bb1.i

bb3.i:                                            ; preds = %bb1.i
  %indvar.next9.i = add i64 %indvar8.i, 1         ; <i64> [#uses=2]
  %exitcond52 = icmp eq i64 %indvar.next9.i, 1024 ; <i1> [#uses=1]
  br i1 %exitcond52, label %bb2.preheader.i, label %bb.nph.i

bb1.i12:                                          ; preds = %bb2.preheader.i, %bb1.i12
  %indvar.i8 = phi i64 [ %indvar.next.i10, %bb1.i12 ], [ 0, %bb2.preheader.i ] ; <i64> [#uses=2]
  %tmp45 = add i64 %tmp44, %indvar.i8             ; <i64> [#uses=1]
  %scevgep.i9 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 0, i64 %tmp45 ; <double*> [#uses=2]
  %5 = load double* %scevgep.i9, align 8          ; <double> [#uses=1]
  %6 = load double* %scevgep69.i, align 8         ; <double> [#uses=1]
  %7 = fdiv double %5, %6                         ; <double> [#uses=1]
  store double %7, double* %scevgep.i9, align 8
  %indvar.next.i10 = add i64 %indvar.i8, 1        ; <i64> [#uses=2]
  %exitcond38 = icmp eq i64 %indvar.next.i10, %tmp23 ; <i1> [#uses=1]
  br i1 %exitcond38, label %bb8.loopexit.i, label %bb1.i12

bb5.i13:                                          ; preds = %bb6.preheader.i14, %bb5.i13
  %indvar34.i = phi i64 [ %indvar.next35.i, %bb5.i13 ], [ 0, %bb6.preheader.i14 ] ; <i64> [#uses=2]
  %tmp49 = add i64 %tmp44, %indvar34.i            ; <i64> [#uses=2]
  %scevgep45.i = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp48, i64 %tmp49 ; <double*> [#uses=2]
  %scevgep46.i = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 0, i64 %tmp49 ; <double*> [#uses=1]
  %8 = load double* %scevgep45.i, align 8         ; <double> [#uses=1]
  %9 = load double* %scevgep55.i, align 8         ; <double> [#uses=1]
  %10 = load double* %scevgep46.i, align 8        ; <double> [#uses=1]
  %11 = fmul double %9, %10                       ; <double> [#uses=1]
  %12 = fsub double %8, %11                       ; <double> [#uses=1]
  store double %12, double* %scevgep45.i, align 8
  %indvar.next35.i = add i64 %indvar34.i, 1       ; <i64> [#uses=2]
  %exitcond24 = icmp eq i64 %indvar.next35.i, %tmp23 ; <i1> [#uses=1]
  br i1 %exitcond24, label %bb8.loopexit4.i, label %bb5.i13

bb8.loopexit.i:                                   ; preds = %bb1.i12
  br i1 %13, label %bb6.preheader.i14, label %bb9.i15

bb8.loopexit4.i:                                  ; preds = %bb5.i13
  %exitcond31 = icmp eq i64 %tmp48, %tmp23        ; <i1> [#uses=1]
  br i1 %exitcond31, label %bb9.i15, label %bb6.preheader.i14

bb6.preheader.i14:                                ; preds = %bb8.loopexit4.i, %bb8.loopexit.i
  %indvar39.i = phi i64 [ %tmp48, %bb8.loopexit4.i ], [ 0, %bb8.loopexit.i ] ; <i64> [#uses=1]
  %tmp48 = add i64 %indvar39.i, 1                 ; <i64> [#uses=4]
  %scevgep55.i = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp48, i64 %tmp43 ; <double*> [#uses=1]
  br label %bb5.i13

bb9.i15:                                          ; preds = %bb2.preheader.i, %bb8.loopexit4.i, %bb8.loopexit.i
  %exitcond42 = icmp eq i64 %storemerge15.i, 1024 ; <i1> [#uses=1]
  br i1 %exitcond42, label %scop_func.exit, label %bb2.preheader.i

bb2.preheader.i:                                  ; preds = %bb9.i15, %bb3.i
  %storemerge17.i = phi i64 [ %storemerge15.i, %bb9.i15 ], [ 0, %bb3.i ] ; <i64> [#uses=3]
  %tmp43 = mul i64 %storemerge17.i, 1025          ; <i64> [#uses=3]
  %tmp44 = add i64 %tmp43, 1                      ; <i64> [#uses=2]
  %tmp23 = sub i64 1023, %storemerge17.i          ; <i64> [#uses=3]
  %storemerge15.i = add i64 %storemerge17.i, 1    ; <i64> [#uses=3]
  %scevgep69.i = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 0, i64 %tmp43 ; <double*> [#uses=1]
  %13 = icmp slt i64 %storemerge15.i, 1024        ; <i1> [#uses=2]
  br i1 %13, label %bb1.i12, label %bb9.i15

scop_func.exit:                                   ; preds = %bb9.i15
  %14 = icmp sgt i32 %argc, 42                    ; <i1> [#uses=1]
  br i1 %14, label %bb.i, label %print_array.exit

bb.i:                                             ; preds = %scop_func.exit
  %15 = load i8** %argv, align 1                  ; <i8*> [#uses=1]
  %16 = load i8* %15, align 1                     ; <i8> [#uses=1]
  %17 = icmp eq i8 %16, 0                         ; <i1> [#uses=1]
  br i1 %17, label %bb6.preheader.i, label %print_array.exit

bb3.i3:                                           ; preds = %bb6.preheader.i, %bb5.i
  %indvar.i1 = phi i64 [ %indvar.next.i4, %bb5.i ], [ 0, %bb6.preheader.i ] ; <i64> [#uses=3]
  %scevgep.i2 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar8.i7, i64 %indvar.i1 ; <double*> [#uses=1]
  %tmp21 = add i64 %tmp20, %indvar.i1             ; <i64> [#uses=1]
  %tmp11.i = trunc i64 %tmp21 to i32              ; <i32> [#uses=1]
  %18 = load double* %scevgep.i2, align 8         ; <double> [#uses=1]
  %19 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %20 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %19, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %18) nounwind ; <i32> [#uses=0]
  %21 = srem i32 %tmp11.i, 80                     ; <i32> [#uses=1]
  %22 = icmp eq i32 %21, 20                       ; <i1> [#uses=1]
  br i1 %22, label %bb4.i, label %bb5.i

bb4.i:                                            ; preds = %bb3.i3
  %23 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %24 = bitcast %struct._IO_FILE* %23 to i8*      ; <i8*> [#uses=1]
  %25 = tail call i32 @fputc(i32 10, i8* %24) nounwind ; <i32> [#uses=0]
  br label %bb5.i

bb5.i:                                            ; preds = %bb4.i, %bb3.i3
  %indvar.next.i4 = add i64 %indvar.i1, 1         ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next.i4, 1024   ; <i1> [#uses=1]
  br i1 %exitcond, label %bb7.i, label %bb3.i3

bb7.i:                                            ; preds = %bb5.i
  %indvar.next9.i6 = add i64 %indvar8.i7, 1       ; <i64> [#uses=2]
  %exitcond19 = icmp eq i64 %indvar.next9.i6, 1024 ; <i1> [#uses=1]
  br i1 %exitcond19, label %bb9.i, label %bb6.preheader.i

bb6.preheader.i:                                  ; preds = %bb7.i, %bb.i
  %indvar8.i7 = phi i64 [ %indvar.next9.i6, %bb7.i ], [ 0, %bb.i ] ; <i64> [#uses=3]
  %tmp20 = shl i64 %indvar8.i7, 10                ; <i64> [#uses=1]
  br label %bb3.i3

bb9.i:                                            ; preds = %bb7.i
  %26 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %27 = bitcast %struct._IO_FILE* %26 to i8*      ; <i8*> [#uses=1]
  %28 = tail call i32 @fputc(i32 10, i8* %27) nounwind ; <i32> [#uses=0]
  ret i32 0

print_array.exit:                                 ; preds = %bb.i, %scop_func.exit
  ret i32 0
}
