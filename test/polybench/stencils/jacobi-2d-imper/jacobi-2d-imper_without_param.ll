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

define void @scop_func() nounwind {
bb.nph35:
  br label %bb5.preheader

bb.nph:                                           ; preds = %bb5.preheader, %bb4
  %indvar36 = phi i64 [ %tmp49, %bb4 ], [ 0, %bb5.preheader ] ; <i64> [#uses=3]
  %tmp49 = add i64 %indvar36, 1                   ; <i64> [#uses=3]
  %tmp52 = add i64 %indvar36, 3                   ; <i64> [#uses=1]
  %tmp54 = add i64 %indvar36, 2                   ; <i64> [#uses=4]
  %scevgep39.phi.trans.insert = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp54, i64 2 ; <double*> [#uses=1]
  %.pre = load double* %scevgep39.phi.trans.insert, align 16 ; <double> [#uses=1]
  br label %bb2

bb2:                                              ; preds = %bb2, %bb.nph
  %0 = phi double [ %.pre, %bb.nph ], [ %3, %bb2 ] ; <double> [#uses=1]
  %indvar = phi i64 [ 0, %bb.nph ], [ %tmp57, %bb2 ] ; <i64> [#uses=3]
  %tmp50 = add i64 %indvar, 2                     ; <i64> [#uses=3]
  %scevgep43 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp49, i64 %tmp50 ; <double*> [#uses=1]
  %scevgep41 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp52, i64 %tmp50 ; <double*> [#uses=1]
  %tmp55 = add i64 %indvar, 3                     ; <i64> [#uses=1]
  %scevgep47 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp54, i64 %tmp55 ; <double*> [#uses=1]
  %tmp57 = add i64 %indvar, 1                     ; <i64> [#uses=3]
  %scevgep45 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp54, i64 %tmp57 ; <double*> [#uses=1]
  %scevgep = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %tmp54, i64 %tmp50 ; <double*> [#uses=1]
  %1 = load double* %scevgep45, align 8           ; <double> [#uses=1]
  %2 = fadd double %0, %1                         ; <double> [#uses=1]
  %3 = load double* %scevgep47, align 8           ; <double> [#uses=2]
  %4 = fadd double %2, %3                         ; <double> [#uses=1]
  %5 = load double* %scevgep41, align 8           ; <double> [#uses=1]
  %6 = fadd double %4, %5                         ; <double> [#uses=1]
  %7 = load double* %scevgep43, align 8           ; <double> [#uses=1]
  %8 = fadd double %6, %7                         ; <double> [#uses=1]
  %9 = fmul double %8, 2.000000e-01               ; <double> [#uses=1]
  store double %9, double* %scevgep, align 8
  %exitcond = icmp eq i64 %tmp57, 1021            ; <i1> [#uses=1]
  br i1 %exitcond, label %bb4, label %bb2

bb4:                                              ; preds = %bb2
  %exitcond48 = icmp eq i64 %tmp49, 1021          ; <i1> [#uses=1]
  br i1 %exitcond48, label %bb9.preheader, label %bb.nph

bb8:                                              ; preds = %bb9.preheader, %bb8
  %indvar61 = phi i64 [ %indvar.next62, %bb8 ], [ 0, %bb9.preheader ] ; <i64> [#uses=2]
  %tmp71 = add i64 %indvar61, 2                   ; <i64> [#uses=2]
  %scevgep68 = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %tmp70, i64 %tmp71 ; <double*> [#uses=1]
  %scevgep67 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp70, i64 %tmp71 ; <double*> [#uses=1]
  %10 = load double* %scevgep68, align 8          ; <double> [#uses=1]
  store double %10, double* %scevgep67, align 8
  %indvar.next62 = add i64 %indvar61, 1           ; <i64> [#uses=2]
  %exitcond63 = icmp eq i64 %indvar.next62, 1021  ; <i1> [#uses=1]
  br i1 %exitcond63, label %bb10, label %bb8

bb10:                                             ; preds = %bb8
  %indvar.next65 = add i64 %indvar64, 1           ; <i64> [#uses=2]
  %exitcond69 = icmp eq i64 %indvar.next65, 1021  ; <i1> [#uses=1]
  br i1 %exitcond69, label %bb12, label %bb9.preheader

bb9.preheader:                                    ; preds = %bb10, %bb4
  %indvar64 = phi i64 [ %indvar.next65, %bb10 ], [ 0, %bb4 ] ; <i64> [#uses=2]
  %tmp70 = add i64 %indvar64, 2                   ; <i64> [#uses=2]
  br label %bb8

bb12:                                             ; preds = %bb10
  %11 = add nsw i64 %storemerge20, 1              ; <i64> [#uses=2]
  %exitcond74 = icmp eq i64 %11, 20               ; <i1> [#uses=1]
  br i1 %exitcond74, label %return, label %bb5.preheader

bb5.preheader:                                    ; preds = %bb12, %bb.nph35
  %storemerge20 = phi i64 [ 0, %bb.nph35 ], [ %11, %bb12 ] ; <i64> [#uses=1]
  br label %bb.nph

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
  tail call void @scop_func() nounwind
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
