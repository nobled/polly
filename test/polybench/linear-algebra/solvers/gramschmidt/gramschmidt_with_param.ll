; RUN: opt -indvars -polly-scop-detect  -analyze %s | FileCheck %s
; XFAIL: *
; ModuleID = '<stdin>'
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
bb.nph7.bb.nph7.split_crit_edge:
  br label %bb.nph

bb.nph:                                           ; preds = %bb3, %bb.nph7.bb.nph7.split_crit_edge
  %indvar8 = phi i64 [ 0, %bb.nph7.bb.nph7.split_crit_edge ], [ %indvar.next9, %bb3 ] ; <i64> [#uses=3]
  %storemerge3 = trunc i64 %indvar8 to i32        ; <i32> [#uses=1]
  %0 = sitofp i32 %storemerge3 to double          ; <double> [#uses=1]
  br label %bb1

bb1:                                              ; preds = %bb1, %bb.nph
  %indvar = phi i64 [ 0, %bb.nph ], [ %indvar.next, %bb1 ] ; <i64> [#uses=3]
  %scevgep = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %indvar8, i64 %indvar ; <double*> [#uses=1]
  %storemerge12 = trunc i64 %indvar to i32        ; <i32> [#uses=1]
  %1 = sitofp i32 %storemerge12 to double         ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=1]
  %3 = fdiv double %2, 5.120000e+02               ; <double> [#uses=1]
  store double %3, double* %scevgep, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 512       ; <i1> [#uses=1]
  br i1 %exitcond, label %bb3, label %bb1

bb3:                                              ; preds = %bb1
  %indvar.next9 = add i64 %indvar8, 1             ; <i64> [#uses=2]
  %exitcond10 = icmp eq i64 %indvar.next9, 512    ; <i1> [#uses=1]
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
  %scevgep = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %indvar8, i64 %indvar ; <double*> [#uses=1]
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
  %exitcond = icmp eq i64 %indvar.next, 512       ; <i1> [#uses=1]
  br i1 %exitcond, label %bb7, label %bb3

bb7:                                              ; preds = %bb5
  %indvar.next9 = add i64 %indvar8, 1             ; <i64> [#uses=2]
  %exitcond12 = icmp eq i64 %indvar.next9, 512    ; <i1> [#uses=1]
  br i1 %exitcond12, label %bb9, label %bb6.preheader

bb6.preheader:                                    ; preds = %bb7, %bb
  %indvar8 = phi i64 [ %indvar.next9, %bb7 ], [ 0, %bb ] ; <i64> [#uses=3]
  %tmp13 = shl i64 %indvar8, 9                    ; <i64> [#uses=1]
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
  %0 = icmp sgt i64 %n, 0                         ; <i1> [#uses=1]
  br i1 %0, label %bb.nph41, label %return

bb.nph41:                                         ; preds = %entry
  %1 = icmp sgt i64 %m, 0                         ; <i1> [#uses=4]
  %tmp56 = add i64 %n, -1                         ; <i64> [#uses=1]
  br label %bb

bb:                                               ; preds = %bb15, %bb.nph41
  %storemerge24 = phi i64 [ 0, %bb.nph41 ], [ %storemerge315, %bb15 ] ; <i64> [#uses=8]
  %tmp = mul i64 %storemerge24, 513               ; <i64> [#uses=2]
  %tmp68 = add i64 %tmp, 1                        ; <i64> [#uses=1]
  %storemerge315 = add i64 %storemerge24, 1       ; <i64> [#uses=4]
  %tmp57 = sub i64 %tmp56, %storemerge24          ; <i64> [#uses=1]
  %scevgep82 = getelementptr [512 x [512 x double]]* @R, i64 0, i64 0, i64 %tmp ; <double*> [#uses=2]
  store double 0.000000e+00, double* @nrm, align 8
  br i1 %1, label %bb1, label %bb3.thread

bb3.thread:                                       ; preds = %bb
  store double 0.000000e+00, double* %scevgep82, align 8
  br label %bb14.loopexit

bb1:                                              ; preds = %bb1, %bb
  %nrm.tmp.0 = phi double [ %4, %bb1 ], [ 0.000000e+00, %bb ] ; <double> [#uses=1]
  %storemerge17 = phi i64 [ %5, %bb1 ], [ 0, %bb ] ; <i64> [#uses=2]
  %scevgep = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %storemerge17, i64 %storemerge24 ; <double*> [#uses=1]
  %2 = load double* %scevgep, align 8             ; <double> [#uses=2]
  %3 = fmul double %2, %2                         ; <double> [#uses=1]
  %4 = fadd double %3, %nrm.tmp.0                 ; <double> [#uses=3]
  %5 = add nsw i64 %storemerge17, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %5, %m                  ; <i1> [#uses=1]
  br i1 %exitcond, label %bb3, label %bb1

bb3:                                              ; preds = %bb1
  store double %4, double* @nrm
  %6 = tail call double @sqrt(double %4) nounwind readonly ; <double> [#uses=2]
  store double %6, double* %scevgep82, align 8
  br i1 %1, label %bb4, label %bb14.loopexit

bb4:                                              ; preds = %bb4, %bb3
  %storemerge28 = phi i64 [ %9, %bb4 ], [ 0, %bb3 ] ; <i64> [#uses=3]
  %scevgep44 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %storemerge28, i64 %storemerge24 ; <double*> [#uses=1]
  %scevgep43 = getelementptr [512 x [512 x double]]* @Q, i64 0, i64 %storemerge28, i64 %storemerge24 ; <double*> [#uses=1]
  %7 = load double* %scevgep44, align 8           ; <double> [#uses=1]
  %8 = fdiv double %7, %6                         ; <double> [#uses=1]
  store double %8, double* %scevgep43, align 8
  %9 = add nsw i64 %storemerge28, 1               ; <i64> [#uses=2]
  %exitcond42 = icmp eq i64 %9, %m                ; <i1> [#uses=1]
  br i1 %exitcond42, label %bb14.loopexit, label %bb4

bb7:                                              ; preds = %bb14.loopexit6, %bb14.loopexit
  %indvar = phi i64 [ %indvar.next, %bb14.loopexit6 ], [ 0, %bb14.loopexit ] ; <i64> [#uses=3]
  %tmp69 = add i64 %tmp68, %indvar                ; <i64> [#uses=1]
  %scevgep66 = getelementptr [512 x [512 x double]]* @R, i64 0, i64 0, i64 %tmp69 ; <double*> [#uses=2]
  %tmp72 = add i64 %storemerge315, %indvar        ; <i64> [#uses=2]
  store double 0.000000e+00, double* %scevgep66, align 8
  br i1 %1, label %bb8, label %bb14.loopexit6

bb8:                                              ; preds = %bb8, %bb7
  %.tmp.0 = phi double [ %13, %bb8 ], [ 0.000000e+00, %bb7 ] ; <double> [#uses=1]
  %storemerge410 = phi i64 [ %14, %bb8 ], [ 0, %bb7 ] ; <i64> [#uses=3]
  %scevgep48 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %storemerge410, i64 %tmp72 ; <double*> [#uses=1]
  %scevgep49 = getelementptr [512 x [512 x double]]* @Q, i64 0, i64 %storemerge410, i64 %storemerge24 ; <double*> [#uses=1]
  %10 = load double* %scevgep49, align 8          ; <double> [#uses=1]
  %11 = load double* %scevgep48, align 8          ; <double> [#uses=1]
  %12 = fmul double %10, %11                      ; <double> [#uses=1]
  %13 = fadd double %.tmp.0, %12                  ; <double> [#uses=3]
  %14 = add nsw i64 %storemerge410, 1             ; <i64> [#uses=2]
  %exitcond46 = icmp eq i64 %14, %m               ; <i1> [#uses=1]
  br i1 %exitcond46, label %bb12.loopexit, label %bb8

bb11:                                             ; preds = %bb12.loopexit, %bb11
  %storemerge513 = phi i64 [ %19, %bb11 ], [ 0, %bb12.loopexit ] ; <i64> [#uses=3]
  %scevgep53 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %storemerge513, i64 %tmp72 ; <double*> [#uses=2]
  %scevgep54 = getelementptr [512 x [512 x double]]* @Q, i64 0, i64 %storemerge513, i64 %storemerge24 ; <double*> [#uses=1]
  %15 = load double* %scevgep53, align 8          ; <double> [#uses=1]
  %16 = load double* %scevgep54, align 8          ; <double> [#uses=1]
  %17 = fmul double %16, %13                      ; <double> [#uses=1]
  %18 = fsub double %15, %17                      ; <double> [#uses=1]
  store double %18, double* %scevgep53, align 8
  %19 = add nsw i64 %storemerge513, 1             ; <i64> [#uses=2]
  %exitcond50 = icmp eq i64 %19, %m               ; <i1> [#uses=1]
  br i1 %exitcond50, label %bb14.loopexit6, label %bb11

bb12.loopexit:                                    ; preds = %bb8
  store double %13, double* %scevgep66
  br i1 %1, label %bb11, label %bb14.loopexit6

bb14.loopexit:                                    ; preds = %bb4, %bb3, %bb3.thread
  %20 = icmp slt i64 %storemerge315, %n           ; <i1> [#uses=1]
  br i1 %20, label %bb7, label %bb15

bb14.loopexit6:                                   ; preds = %bb12.loopexit, %bb11, %bb7
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond58 = icmp eq i64 %indvar.next, %tmp57  ; <i1> [#uses=1]
  br i1 %exitcond58, label %bb15, label %bb7

bb15:                                             ; preds = %bb14.loopexit6, %bb14.loopexit
  %exitcond67 = icmp eq i64 %storemerge315, %n    ; <i1> [#uses=1]
  br i1 %exitcond67, label %return, label %bb

return:                                           ; preds = %bb15, %entry
  ret void
}

declare double @sqrt(double) nounwind readonly

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
  %scevgep.i = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %indvar8.i, i64 %indvar.i ; <double*> [#uses=1]
  %storemerge12.i = trunc i64 %indvar.i to i32    ; <i32> [#uses=1]
  %1 = sitofp i32 %storemerge12.i to double       ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=1]
  %3 = fdiv double %2, 5.120000e+02               ; <double> [#uses=1]
  store double %3, double* %scevgep.i, align 8
  %indvar.next.i = add i64 %indvar.i, 1           ; <i64> [#uses=2]
  %exitcond14 = icmp eq i64 %indvar.next.i, 512   ; <i1> [#uses=1]
  br i1 %exitcond14, label %bb3.i, label %bb1.i

bb3.i:                                            ; preds = %bb1.i
  %indvar.next9.i = add i64 %indvar8.i, 1         ; <i64> [#uses=2]
  %exitcond15 = icmp eq i64 %indvar.next9.i, 512  ; <i1> [#uses=1]
  br i1 %exitcond15, label %init_array.exit, label %bb.nph.i

init_array.exit:                                  ; preds = %bb3.i
  tail call void @scop_func(i64 512, i64 512) nounwind
  %4 = icmp sgt i32 %argc, 42                     ; <i1> [#uses=1]
  br i1 %4, label %bb.i, label %print_array.exit

bb.i:                                             ; preds = %init_array.exit
  %5 = load i8** %argv, align 1                   ; <i8*> [#uses=1]
  %6 = load i8* %5, align 1                       ; <i8> [#uses=1]
  %7 = icmp eq i8 %6, 0                           ; <i1> [#uses=1]
  br i1 %7, label %bb6.preheader.i, label %print_array.exit

bb3.i3:                                           ; preds = %bb6.preheader.i, %bb5.i
  %indvar.i1 = phi i64 [ %indvar.next.i4, %bb5.i ], [ 0, %bb6.preheader.i ] ; <i64> [#uses=3]
  %scevgep.i2 = getelementptr [512 x [512 x double]]* @A, i64 0, i64 %indvar8.i7, i64 %indvar.i1 ; <double*> [#uses=1]
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
  %exitcond = icmp eq i64 %indvar.next.i4, 512    ; <i1> [#uses=1]
  br i1 %exitcond, label %bb7.i, label %bb3.i3

bb7.i:                                            ; preds = %bb5.i
  %indvar.next9.i6 = add i64 %indvar8.i7, 1       ; <i64> [#uses=2]
  %exitcond10 = icmp eq i64 %indvar.next9.i6, 512 ; <i1> [#uses=1]
  br i1 %exitcond10, label %bb9.i, label %bb6.preheader.i

bb6.preheader.i:                                  ; preds = %bb7.i, %bb.i
  %indvar8.i7 = phi i64 [ %indvar.next9.i6, %bb7.i ], [ 0, %bb.i ] ; <i64> [#uses=3]
  %tmp11 = shl i64 %indvar8.i7, 9                 ; <i64> [#uses=1]
  br label %bb3.i3

bb9.i:                                            ; preds = %bb7.i
  %16 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %17 = bitcast %struct._IO_FILE* %16 to i8*      ; <i8*> [#uses=1]
  %18 = tail call i32 @fputc(i32 10, i8* %17) nounwind ; <i32> [#uses=0]
  ret i32 0

print_array.exit:                                 ; preds = %bb.i, %init_array.exit
  ret i32 0
}
