; RUN: opt -indvars -polly-scop-detect  -analyze %s | FileCheck %s
; XFAIL: *
; ModuleID = '<stdin>'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }

@A = common global [1024 x [1024 x double]] zeroinitializer, align 32 ; <[1024 x [1024 x double]]*> [#uses=10]
@B = common global [1024 x [1024 x double]] zeroinitializer, align 32 ; <[1024 x [1024 x double]]*> [#uses=4]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]

define void @init_array() nounwind inlinehint {
bb.nph7.bb.nph7.split_crit_edge:
  br label %bb.nph

bb.nph:                                           ; preds = %bb3, %bb.nph7.bb.nph7.split_crit_edge
  %indvar8 = phi i64 [ 0, %bb.nph7.bb.nph7.split_crit_edge ], [ %indvar.next9, %bb3 ] ; <i64> [#uses=4]
  %storemerge3 = trunc i64 %indvar8 to i32        ; <i32> [#uses=1]
  %0 = sitofp i32 %storemerge3 to double          ; <double> [#uses=1]
  br label %bb1

bb1:                                              ; preds = %bb1, %bb.nph
  %indvar = phi i64 [ 0, %bb.nph ], [ %indvar.next, %bb1 ] ; <i64> [#uses=4]
  %scevgep10 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar8, i64 %indvar ; <double*> [#uses=1]
  %scevgep = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %indvar8, i64 %indvar ; <double*> [#uses=1]
  %storemerge12 = trunc i64 %indvar to i32        ; <i32> [#uses=1]
  %1 = sitofp i32 %storemerge12 to double         ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=2]
  %3 = fadd double %2, 1.000000e+01               ; <double> [#uses=1]
  %4 = fdiv double %3, 1.024000e+03               ; <double> [#uses=1]
  store double %4, double* %scevgep10, align 8
  %5 = fadd double %2, 1.100000e+01               ; <double> [#uses=1]
  %6 = fdiv double %5, 1.024000e+03               ; <double> [#uses=1]
  store double %6, double* %scevgep, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 1024      ; <i1> [#uses=1]
  br i1 %exitcond, label %bb3, label %bb1

bb3:                                              ; preds = %bb1
  %indvar.next9 = add i64 %indvar8, 1             ; <i64> [#uses=2]
  %exitcond11 = icmp eq i64 %indvar.next9, 1024   ; <i1> [#uses=1]
  br i1 %exitcond11, label %return, label %bb.nph

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
bb.nph35:
  %0 = add nsw i64 %n, -1                         ; <i64> [#uses=1]
  %1 = icmp sgt i64 %0, 2                         ; <i1> [#uses=2]
  %tmp = add i64 %n, -3                           ; <i64> [#uses=4]
  br label %bb5.preheader

bb.nph:                                           ; preds = %bb5.preheader, %bb4
  %indvar36 = phi i64 [ %tmp50, %bb4 ], [ 0, %bb5.preheader ] ; <i64> [#uses=3]
  %tmp50 = add i64 %indvar36, 1                   ; <i64> [#uses=3]
  %tmp53 = add i64 %indvar36, 3                   ; <i64> [#uses=1]
  %tmp55 = add i64 %indvar36, 2                   ; <i64> [#uses=4]
  %scevgep40.phi.trans.insert = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp55, i64 2 ; <double*> [#uses=1]
  %.pre = load double* %scevgep40.phi.trans.insert, align 16 ; <double> [#uses=1]
  br label %bb2

bb2:                                              ; preds = %bb2, %bb.nph
  %2 = phi double [ %.pre, %bb.nph ], [ %5, %bb2 ] ; <double> [#uses=1]
  %indvar = phi i64 [ 0, %bb.nph ], [ %tmp58, %bb2 ] ; <i64> [#uses=3]
  %tmp51 = add i64 %indvar, 2                     ; <i64> [#uses=3]
  %scevgep44 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp50, i64 %tmp51 ; <double*> [#uses=1]
  %scevgep42 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp53, i64 %tmp51 ; <double*> [#uses=1]
  %tmp56 = add i64 %indvar, 3                     ; <i64> [#uses=1]
  %scevgep48 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp55, i64 %tmp56 ; <double*> [#uses=1]
  %tmp58 = add i64 %indvar, 1                     ; <i64> [#uses=3]
  %scevgep46 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp55, i64 %tmp58 ; <double*> [#uses=1]
  %scevgep = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %tmp55, i64 %tmp51 ; <double*> [#uses=1]
  %3 = load double* %scevgep46, align 8           ; <double> [#uses=1]
  %4 = fadd double %2, %3                         ; <double> [#uses=1]
  %5 = load double* %scevgep48, align 8           ; <double> [#uses=2]
  %6 = fadd double %4, %5                         ; <double> [#uses=1]
  %7 = load double* %scevgep42, align 8           ; <double> [#uses=1]
  %8 = fadd double %6, %7                         ; <double> [#uses=1]
  %9 = load double* %scevgep44, align 8           ; <double> [#uses=1]
  %10 = fadd double %8, %9                        ; <double> [#uses=1]
  %11 = fmul double %10, 2.000000e-01             ; <double> [#uses=1]
  store double %11, double* %scevgep, align 8
  %exitcond = icmp eq i64 %tmp58, %tmp            ; <i1> [#uses=1]
  br i1 %exitcond, label %bb4, label %bb2

bb4:                                              ; preds = %bb2
  %exitcond49 = icmp eq i64 %tmp50, %tmp          ; <i1> [#uses=1]
  br i1 %exitcond49, label %bb11.loopexit, label %bb.nph

bb8:                                              ; preds = %bb9.preheader, %bb8
  %indvar62 = phi i64 [ %indvar.next63, %bb8 ], [ 0, %bb9.preheader ] ; <i64> [#uses=2]
  %tmp73 = add i64 %indvar62, 2                   ; <i64> [#uses=2]
  %scevgep70 = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %tmp72, i64 %tmp73 ; <double*> [#uses=1]
  %scevgep69 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp72, i64 %tmp73 ; <double*> [#uses=1]
  %12 = load double* %scevgep70, align 8          ; <double> [#uses=1]
  store double %12, double* %scevgep69, align 8
  %indvar.next63 = add i64 %indvar62, 1           ; <i64> [#uses=2]
  %exitcond64 = icmp eq i64 %indvar.next63, %tmp  ; <i1> [#uses=1]
  br i1 %exitcond64, label %bb10, label %bb8

bb10:                                             ; preds = %bb8
  %indvar.next66 = add i64 %indvar65, 1           ; <i64> [#uses=2]
  %exitcond71 = icmp eq i64 %indvar.next66, %tmp  ; <i1> [#uses=1]
  br i1 %exitcond71, label %bb12, label %bb9.preheader

bb11.loopexit:                                    ; preds = %bb4
  br i1 %1, label %bb9.preheader, label %bb12

bb9.preheader:                                    ; preds = %bb11.loopexit, %bb10
  %indvar65 = phi i64 [ %indvar.next66, %bb10 ], [ 0, %bb11.loopexit ] ; <i64> [#uses=2]
  %tmp72 = add i64 %indvar65, 2                   ; <i64> [#uses=2]
  br label %bb8

bb12:                                             ; preds = %bb5.preheader, %bb11.loopexit, %bb10
  %13 = add nsw i64 %storemerge20, 1              ; <i64> [#uses=2]
  %exitcond76 = icmp eq i64 %13, 20               ; <i1> [#uses=1]
  br i1 %exitcond76, label %return, label %bb5.preheader

bb5.preheader:                                    ; preds = %bb12, %bb.nph35
  %storemerge20 = phi i64 [ 0, %bb.nph35 ], [ %13, %bb12 ] ; <i64> [#uses=1]
  br i1 %1, label %bb.nph, label %bb12

return:                                           ; preds = %bb12
  ret void
}

define i32 @main(i32 %argc, i8** %argv) nounwind {
entry:
  br label %bb.nph.i

bb.nph.i:                                         ; preds = %bb3.i, %entry
  %indvar8.i = phi i64 [ 0, %entry ], [ %indvar.next9.i, %bb3.i ] ; <i64> [#uses=4]
  %storemerge3.i = trunc i64 %indvar8.i to i32    ; <i32> [#uses=1]
  %0 = sitofp i32 %storemerge3.i to double        ; <double> [#uses=1]
  br label %bb1.i

bb1.i:                                            ; preds = %bb1.i, %bb.nph.i
  %indvar.i = phi i64 [ 0, %bb.nph.i ], [ %indvar.next.i, %bb1.i ] ; <i64> [#uses=4]
  %scevgep10.i = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar8.i, i64 %indvar.i ; <double*> [#uses=1]
  %scevgep.i = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %indvar8.i, i64 %indvar.i ; <double*> [#uses=1]
  %storemerge12.i = trunc i64 %indvar.i to i32    ; <i32> [#uses=1]
  %1 = sitofp i32 %storemerge12.i to double       ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=2]
  %3 = fadd double %2, 1.000000e+01               ; <double> [#uses=1]
  %4 = fdiv double %3, 1.024000e+03               ; <double> [#uses=1]
  store double %4, double* %scevgep10.i, align 8
  %5 = fadd double %2, 1.100000e+01               ; <double> [#uses=1]
  %6 = fdiv double %5, 1.024000e+03               ; <double> [#uses=1]
  store double %6, double* %scevgep.i, align 8
  %indvar.next.i = add i64 %indvar.i, 1           ; <i64> [#uses=2]
  %exitcond14 = icmp eq i64 %indvar.next.i, 1024  ; <i1> [#uses=1]
  br i1 %exitcond14, label %bb3.i, label %bb1.i

bb3.i:                                            ; preds = %bb1.i
  %indvar.next9.i = add i64 %indvar8.i, 1         ; <i64> [#uses=2]
  %exitcond15 = icmp eq i64 %indvar.next9.i, 1024 ; <i1> [#uses=1]
  br i1 %exitcond15, label %init_array.exit, label %bb.nph.i

init_array.exit:                                  ; preds = %bb3.i
  tail call void @scop_func(i64 1024) nounwind
  %7 = icmp sgt i32 %argc, 42                     ; <i1> [#uses=1]
  br i1 %7, label %bb.i, label %print_array.exit

bb.i:                                             ; preds = %init_array.exit
  %8 = load i8** %argv, align 1                   ; <i8*> [#uses=1]
  %9 = load i8* %8, align 1                       ; <i8> [#uses=1]
  %10 = icmp eq i8 %9, 0                          ; <i1> [#uses=1]
  br i1 %10, label %bb6.preheader.i, label %print_array.exit

bb3.i3:                                           ; preds = %bb6.preheader.i, %bb5.i
  %indvar.i1 = phi i64 [ %indvar.next.i4, %bb5.i ], [ 0, %bb6.preheader.i ] ; <i64> [#uses=3]
  %scevgep.i2 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar8.i7, i64 %indvar.i1 ; <double*> [#uses=1]
  %tmp12 = add i64 %tmp11, %indvar.i1             ; <i64> [#uses=1]
  %tmp11.i = trunc i64 %tmp12 to i32              ; <i32> [#uses=1]
  %11 = load double* %scevgep.i2, align 8         ; <double> [#uses=1]
  %12 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %13 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %12, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %11) nounwind ; <i32> [#uses=0]
  %14 = srem i32 %tmp11.i, 80                     ; <i32> [#uses=1]
  %15 = icmp eq i32 %14, 20                       ; <i1> [#uses=1]
  br i1 %15, label %bb4.i, label %bb5.i

bb4.i:                                            ; preds = %bb3.i3
  %16 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %17 = bitcast %struct._IO_FILE* %16 to i8*      ; <i8*> [#uses=1]
  %18 = tail call i32 @fputc(i32 10, i8* %17) nounwind ; <i32> [#uses=0]
  br label %bb5.i

bb5.i:                                            ; preds = %bb4.i, %bb3.i3
  %indvar.next.i4 = add i64 %indvar.i1, 1         ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next.i4, 1024   ; <i1> [#uses=1]
  br i1 %exitcond, label %bb7.i, label %bb3.i3

bb7.i:                                            ; preds = %bb5.i
  %indvar.next9.i6 = add i64 %indvar8.i7, 1       ; <i64> [#uses=2]
  %exitcond10 = icmp eq i64 %indvar.next9.i6, 1024 ; <i1> [#uses=1]
  br i1 %exitcond10, label %bb9.i, label %bb6.preheader.i

bb6.preheader.i:                                  ; preds = %bb7.i, %bb.i
  %indvar8.i7 = phi i64 [ %indvar.next9.i6, %bb7.i ], [ 0, %bb.i ] ; <i64> [#uses=3]
  %tmp11 = shl i64 %indvar8.i7, 10                ; <i64> [#uses=1]
  br label %bb3.i3

bb9.i:                                            ; preds = %bb7.i
  %19 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %20 = bitcast %struct._IO_FILE* %19 to i8*      ; <i8*> [#uses=1]
  %21 = tail call i32 @fputc(i32 10, i8* %20) nounwind ; <i32> [#uses=0]
  ret i32 0

print_array.exit:                                 ; preds = %bb.i, %init_array.exit
  ret i32 0
}
