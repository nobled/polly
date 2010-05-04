; RUN: opt -indvars -polly-scop-detect  -analyze %s | FileCheck %s
; XFAIL: *
; ModuleID = '<stdin>'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }

@X = common global [1024 x [1024 x double]] zeroinitializer, align 32 ; <[1024 x [1024 x double]]*> [#uses=12]
@A = common global [1024 x [1024 x double]] zeroinitializer, align 32 ; <[1024 x [1024 x double]]*> [#uses=8]
@B = common global [1024 x [1024 x double]] zeroinitializer, align 32 ; <[1024 x [1024 x double]]*> [#uses=10]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]

define void @init_array() nounwind inlinehint {
bb.nph7.bb.nph7.split_crit_edge:
  br label %bb.nph

bb.nph:                                           ; preds = %bb3, %bb.nph7.bb.nph7.split_crit_edge
  %indvar8 = phi i64 [ 0, %bb.nph7.bb.nph7.split_crit_edge ], [ %indvar.next9, %bb3 ] ; <i64> [#uses=5]
  %storemerge3 = trunc i64 %indvar8 to i32        ; <i32> [#uses=1]
  %0 = sitofp i32 %storemerge3 to double          ; <double> [#uses=1]
  br label %bb1

bb1:                                              ; preds = %bb1, %bb.nph
  %indvar = phi i64 [ 0, %bb.nph ], [ %indvar.next, %bb1 ] ; <i64> [#uses=5]
  %scevgep11 = getelementptr [1024 x [1024 x double]]* @X, i64 0, i64 %indvar8, i64 %indvar ; <double*> [#uses=1]
  %scevgep10 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar8, i64 %indvar ; <double*> [#uses=1]
  %scevgep = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %indvar8, i64 %indvar ; <double*> [#uses=1]
  %storemerge12 = trunc i64 %indvar to i32        ; <i32> [#uses=1]
  %1 = sitofp i32 %storemerge12 to double         ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=3]
  %3 = fadd double %2, 1.000000e+00               ; <double> [#uses=1]
  %4 = fdiv double %3, 1.024000e+03               ; <double> [#uses=1]
  store double %4, double* %scevgep11, align 8
  %5 = fadd double %2, 2.000000e+00               ; <double> [#uses=1]
  %6 = fdiv double %5, 1.024000e+03               ; <double> [#uses=1]
  store double %6, double* %scevgep10, align 8
  %7 = fadd double %2, 3.000000e+00               ; <double> [#uses=1]
  %8 = fdiv double %7, 1.024000e+03               ; <double> [#uses=1]
  store double %8, double* %scevgep, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 1024      ; <i1> [#uses=1]
  br i1 %exitcond, label %bb3, label %bb1

bb3:                                              ; preds = %bb1
  %indvar.next9 = add i64 %indvar8, 1             ; <i64> [#uses=2]
  %exitcond12 = icmp eq i64 %indvar.next9, 1024   ; <i1> [#uses=1]
  br i1 %exitcond12, label %return, label %bb.nph

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
bb.nph79:
  br label %bb5.preheader

bb.nph:                                           ; preds = %bb5.preheader, %bb4
  %storemerge112 = phi i64 [ %11, %bb4 ], [ 0, %bb5.preheader ] ; <i64> [#uses=6]
  %scevgep82.phi.trans.insert = getelementptr [1024 x [1024 x double]]* @X, i64 0, i64 %storemerge112, i64 0 ; <double*> [#uses=1]
  %.pre = load double* %scevgep82.phi.trans.insert, align 32 ; <double> [#uses=1]
  %scevgep83.phi.trans.insert = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %storemerge112, i64 0 ; <double*> [#uses=1]
  %.pre143 = load double* %scevgep83.phi.trans.insert, align 32 ; <double> [#uses=1]
  br label %bb2

bb2:                                              ; preds = %bb2, %bb.nph
  %0 = phi double [ %.pre143, %bb.nph ], [ %10, %bb2 ] ; <double> [#uses=2]
  %1 = phi double [ %.pre, %bb.nph ], [ %6, %bb2 ] ; <double> [#uses=1]
  %indvar = phi i64 [ 0, %bb.nph ], [ %tmp87, %bb2 ] ; <i64> [#uses=1]
  %tmp87 = add i64 %indvar, 1                     ; <i64> [#uses=5]
  %scevgep81 = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %storemerge112, i64 %tmp87 ; <double*> [#uses=2]
  %scevgep80 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %storemerge112, i64 %tmp87 ; <double*> [#uses=1]
  %scevgep = getelementptr [1024 x [1024 x double]]* @X, i64 0, i64 %storemerge112, i64 %tmp87 ; <double*> [#uses=2]
  %2 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %3 = load double* %scevgep80, align 8           ; <double> [#uses=3]
  %4 = fmul double %1, %3                         ; <double> [#uses=1]
  %5 = fdiv double %4, %0                         ; <double> [#uses=1]
  %6 = fsub double %2, %5                         ; <double> [#uses=2]
  store double %6, double* %scevgep, align 8
  %7 = load double* %scevgep81, align 8           ; <double> [#uses=1]
  %8 = fmul double %3, %3                         ; <double> [#uses=1]
  %9 = fdiv double %8, %0                         ; <double> [#uses=1]
  %10 = fsub double %7, %9                        ; <double> [#uses=2]
  store double %10, double* %scevgep81, align 8
  %exitcond = icmp eq i64 %tmp87, 1023            ; <i1> [#uses=1]
  br i1 %exitcond, label %bb4, label %bb2

bb4:                                              ; preds = %bb2
  %11 = add nsw i64 %storemerge112, 1             ; <i64> [#uses=2]
  %exitcond84 = icmp eq i64 %11, 1024             ; <i1> [#uses=1]
  br i1 %exitcond84, label %bb7, label %bb.nph

bb7:                                              ; preds = %bb7, %bb4
  %storemerge217 = phi i64 [ %15, %bb7 ], [ 0, %bb4 ] ; <i64> [#uses=3]
  %scevgep92 = getelementptr [1024 x [1024 x double]]* @X, i64 0, i64 %storemerge217, i64 1023 ; <double*> [#uses=2]
  %scevgep93 = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %storemerge217, i64 1023 ; <double*> [#uses=1]
  %12 = load double* %scevgep92, align 8          ; <double> [#uses=1]
  %13 = load double* %scevgep93, align 8          ; <double> [#uses=1]
  %14 = fdiv double %12, %13                      ; <double> [#uses=1]
  store double %14, double* %scevgep92, align 8
  %15 = add nsw i64 %storemerge217, 1             ; <i64> [#uses=2]
  %exitcond91 = icmp eq i64 %15, 1024             ; <i1> [#uses=1]
  br i1 %exitcond91, label %bb12.preheader, label %bb7

bb11:                                             ; preds = %bb12.preheader, %bb11
  %storemerge920 = phi i64 [ %23, %bb11 ], [ 0, %bb12.preheader ] ; <i64> [#uses=3]
  %tmp103 = sub i64 1021, %storemerge920          ; <i64> [#uses=3]
  %scevgep100 = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %storemerge323, i64 %tmp103 ; <double*> [#uses=1]
  %scevgep99 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %storemerge323, i64 %tmp103 ; <double*> [#uses=1]
  %scevgep98 = getelementptr [1024 x [1024 x double]]* @X, i64 0, i64 %storemerge323, i64 %tmp103 ; <double*> [#uses=1]
  %tmp107 = sub i64 1022, %storemerge920          ; <i64> [#uses=1]
  %scevgep96 = getelementptr [1024 x [1024 x double]]* @X, i64 0, i64 %storemerge323, i64 %tmp107 ; <double*> [#uses=2]
  %16 = load double* %scevgep96, align 8          ; <double> [#uses=1]
  %17 = load double* %scevgep98, align 8          ; <double> [#uses=1]
  %18 = load double* %scevgep99, align 8          ; <double> [#uses=1]
  %19 = fmul double %17, %18                      ; <double> [#uses=1]
  %20 = fsub double %16, %19                      ; <double> [#uses=1]
  %21 = load double* %scevgep100, align 8         ; <double> [#uses=1]
  %22 = fdiv double %20, %21                      ; <double> [#uses=1]
  store double %22, double* %scevgep96, align 8
  %23 = add nsw i64 %storemerge920, 1             ; <i64> [#uses=2]
  %exitcond94 = icmp eq i64 %23, 1022             ; <i1> [#uses=1]
  br i1 %exitcond94, label %bb13, label %bb11

bb13:                                             ; preds = %bb11
  %24 = add nsw i64 %storemerge323, 1             ; <i64> [#uses=2]
  %exitcond101 = icmp eq i64 %24, 1024            ; <i1> [#uses=1]
  br i1 %exitcond101, label %bb18.preheader, label %bb12.preheader

bb12.preheader:                                   ; preds = %bb13, %bb7
  %storemerge323 = phi i64 [ %24, %bb13 ], [ 0, %bb7 ] ; <i64> [#uses=5]
  br label %bb11

bb17:                                             ; preds = %bb18.preheader, %bb17
  %storemerge828 = phi i64 [ %36, %bb17 ], [ 0, %bb18.preheader ] ; <i64> [#uses=6]
  %scevgep114 = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %indvar110, i64 %storemerge828 ; <double*> [#uses=1]
  %scevgep113 = getelementptr [1024 x [1024 x double]]* @X, i64 0, i64 %indvar110, i64 %storemerge828 ; <double*> [#uses=1]
  %scevgep116 = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %tmp120, i64 %storemerge828 ; <double*> [#uses=2]
  %scevgep115 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp120, i64 %storemerge828 ; <double*> [#uses=1]
  %scevgep112 = getelementptr [1024 x [1024 x double]]* @X, i64 0, i64 %tmp120, i64 %storemerge828 ; <double*> [#uses=2]
  %25 = load double* %scevgep112, align 8         ; <double> [#uses=1]
  %26 = load double* %scevgep113, align 8         ; <double> [#uses=1]
  %27 = load double* %scevgep115, align 8         ; <double> [#uses=3]
  %28 = fmul double %26, %27                      ; <double> [#uses=1]
  %29 = load double* %scevgep114, align 8         ; <double> [#uses=2]
  %30 = fdiv double %28, %29                      ; <double> [#uses=1]
  %31 = fsub double %25, %30                      ; <double> [#uses=1]
  store double %31, double* %scevgep112, align 8
  %32 = load double* %scevgep116, align 8         ; <double> [#uses=1]
  %33 = fmul double %27, %27                      ; <double> [#uses=1]
  %34 = fdiv double %33, %29                      ; <double> [#uses=1]
  %35 = fsub double %32, %34                      ; <double> [#uses=1]
  store double %35, double* %scevgep116, align 8
  %36 = add nsw i64 %storemerge828, 1             ; <i64> [#uses=2]
  %exitcond109 = icmp eq i64 %36, 1024            ; <i1> [#uses=1]
  br i1 %exitcond109, label %bb19, label %bb17

bb19:                                             ; preds = %bb17
  %exitcond117 = icmp eq i64 %tmp120, 1023        ; <i1> [#uses=1]
  br i1 %exitcond117, label %bb22, label %bb18.preheader

bb18.preheader:                                   ; preds = %bb19, %bb13
  %indvar110 = phi i64 [ %tmp120, %bb19 ], [ 0, %bb13 ] ; <i64> [#uses=3]
  %tmp120 = add i64 %indvar110, 1                 ; <i64> [#uses=5]
  br label %bb17

bb22:                                             ; preds = %bb22, %bb19
  %storemerge535 = phi i64 [ %40, %bb22 ], [ 0, %bb19 ] ; <i64> [#uses=3]
  %scevgep125 = getelementptr [1024 x [1024 x double]]* @X, i64 0, i64 1023, i64 %storemerge535 ; <double*> [#uses=2]
  %scevgep126 = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 1023, i64 %storemerge535 ; <double*> [#uses=1]
  %37 = load double* %scevgep125, align 8         ; <double> [#uses=1]
  %38 = load double* %scevgep126, align 8         ; <double> [#uses=1]
  %39 = fdiv double %37, %38                      ; <double> [#uses=1]
  store double %39, double* %scevgep125, align 8
  %40 = add nsw i64 %storemerge535, 1             ; <i64> [#uses=2]
  %exitcond124 = icmp eq i64 %40, 1024            ; <i1> [#uses=1]
  br i1 %exitcond124, label %bb27.preheader, label %bb22

bb26:                                             ; preds = %bb27.preheader, %bb26
  %storemerge737 = phi i64 [ %48, %bb26 ], [ 0, %bb27.preheader ] ; <i64> [#uses=5]
  %scevgep132 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp136, i64 %storemerge737 ; <double*> [#uses=1]
  %scevgep131 = getelementptr [1024 x [1024 x double]]* @X, i64 0, i64 %tmp136, i64 %storemerge737 ; <double*> [#uses=1]
  %scevgep133 = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %tmp139, i64 %storemerge737 ; <double*> [#uses=1]
  %scevgep129 = getelementptr [1024 x [1024 x double]]* @X, i64 0, i64 %tmp139, i64 %storemerge737 ; <double*> [#uses=2]
  %41 = load double* %scevgep129, align 8         ; <double> [#uses=1]
  %42 = load double* %scevgep131, align 8         ; <double> [#uses=1]
  %43 = load double* %scevgep132, align 8         ; <double> [#uses=1]
  %44 = fmul double %42, %43                      ; <double> [#uses=1]
  %45 = fsub double %41, %44                      ; <double> [#uses=1]
  %46 = load double* %scevgep133, align 8         ; <double> [#uses=1]
  %47 = fdiv double %45, %46                      ; <double> [#uses=1]
  store double %47, double* %scevgep129, align 8
  %48 = add nsw i64 %storemerge737, 1             ; <i64> [#uses=2]
  %exitcond127 = icmp eq i64 %48, 1024            ; <i1> [#uses=1]
  br i1 %exitcond127, label %bb28, label %bb26

bb28:                                             ; preds = %bb26
  %49 = add nsw i64 %storemerge639, 1             ; <i64> [#uses=2]
  %exitcond134 = icmp eq i64 %49, 1022            ; <i1> [#uses=1]
  br i1 %exitcond134, label %bb30, label %bb27.preheader

bb27.preheader:                                   ; preds = %bb28, %bb22
  %storemerge639 = phi i64 [ %49, %bb28 ], [ 0, %bb22 ] ; <i64> [#uses=3]
  %tmp136 = sub i64 1021, %storemerge639          ; <i64> [#uses=2]
  %tmp139 = sub i64 1022, %storemerge639          ; <i64> [#uses=2]
  br label %bb26

bb30:                                             ; preds = %bb28
  %50 = add nsw i64 %storemerge44, 1              ; <i64> [#uses=2]
  %exitcond142 = icmp eq i64 %50, 10              ; <i1> [#uses=1]
  br i1 %exitcond142, label %return, label %bb5.preheader

bb5.preheader:                                    ; preds = %bb30, %bb.nph79
  %storemerge44 = phi i64 [ 0, %bb.nph79 ], [ %50, %bb30 ] ; <i64> [#uses=1]
  br label %bb.nph

return:                                           ; preds = %bb30
  ret void
}

define i32 @main(i32 %argc, i8** %argv) nounwind {
entry:
  br label %bb.nph.i

bb.nph.i:                                         ; preds = %bb3.i, %entry
  %indvar8.i = phi i64 [ 0, %entry ], [ %indvar.next9.i, %bb3.i ] ; <i64> [#uses=5]
  %storemerge3.i = trunc i64 %indvar8.i to i32    ; <i32> [#uses=1]
  %0 = sitofp i32 %storemerge3.i to double        ; <double> [#uses=1]
  br label %bb1.i

bb1.i:                                            ; preds = %bb1.i, %bb.nph.i
  %indvar.i = phi i64 [ 0, %bb.nph.i ], [ %indvar.next.i, %bb1.i ] ; <i64> [#uses=5]
  %scevgep11.i = getelementptr [1024 x [1024 x double]]* @X, i64 0, i64 %indvar8.i, i64 %indvar.i ; <double*> [#uses=1]
  %scevgep10.i = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar8.i, i64 %indvar.i ; <double*> [#uses=1]
  %scevgep.i = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %indvar8.i, i64 %indvar.i ; <double*> [#uses=1]
  %storemerge12.i = trunc i64 %indvar.i to i32    ; <i32> [#uses=1]
  %1 = sitofp i32 %storemerge12.i to double       ; <double> [#uses=1]
  %2 = fmul double %0, %1                         ; <double> [#uses=3]
  %3 = fadd double %2, 1.000000e+00               ; <double> [#uses=1]
  %4 = fdiv double %3, 1.024000e+03               ; <double> [#uses=1]
  store double %4, double* %scevgep11.i, align 8
  %5 = fadd double %2, 2.000000e+00               ; <double> [#uses=1]
  %6 = fdiv double %5, 1.024000e+03               ; <double> [#uses=1]
  store double %6, double* %scevgep10.i, align 8
  %7 = fadd double %2, 3.000000e+00               ; <double> [#uses=1]
  %8 = fdiv double %7, 1.024000e+03               ; <double> [#uses=1]
  store double %8, double* %scevgep.i, align 8
  %indvar.next.i = add i64 %indvar.i, 1           ; <i64> [#uses=2]
  %exitcond15 = icmp eq i64 %indvar.next.i, 1024  ; <i1> [#uses=1]
  br i1 %exitcond15, label %bb3.i, label %bb1.i

bb3.i:                                            ; preds = %bb1.i
  %indvar.next9.i = add i64 %indvar8.i, 1         ; <i64> [#uses=2]
  %exitcond16 = icmp eq i64 %indvar.next9.i, 1024 ; <i1> [#uses=1]
  br i1 %exitcond16, label %init_array.exit, label %bb.nph.i

init_array.exit:                                  ; preds = %bb3.i
  tail call void @scop_func() nounwind
  %9 = icmp sgt i32 %argc, 42                     ; <i1> [#uses=1]
  br i1 %9, label %bb.i, label %print_array.exit

bb.i:                                             ; preds = %init_array.exit
  %10 = load i8** %argv, align 1                  ; <i8*> [#uses=1]
  %11 = load i8* %10, align 1                     ; <i8> [#uses=1]
  %12 = icmp eq i8 %11, 0                         ; <i1> [#uses=1]
  br i1 %12, label %bb6.preheader.i, label %print_array.exit

bb3.i3:                                           ; preds = %bb6.preheader.i, %bb5.i
  %indvar.i1 = phi i64 [ %indvar.next.i4, %bb5.i ], [ 0, %bb6.preheader.i ] ; <i64> [#uses=3]
  %scevgep.i2 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %indvar8.i8, i64 %indvar.i1 ; <double*> [#uses=1]
  %tmp13 = add i64 %tmp12, %indvar.i1             ; <i64> [#uses=1]
  %tmp11.i = trunc i64 %tmp13 to i32              ; <i32> [#uses=1]
  %13 = load double* %scevgep.i2, align 8         ; <double> [#uses=1]
  %14 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %15 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %14, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %13) nounwind ; <i32> [#uses=0]
  %16 = srem i32 %tmp11.i, 80                     ; <i32> [#uses=1]
  %17 = icmp eq i32 %16, 20                       ; <i1> [#uses=1]
  br i1 %17, label %bb4.i, label %bb5.i

bb4.i:                                            ; preds = %bb3.i3
  %18 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %19 = bitcast %struct._IO_FILE* %18 to i8*      ; <i8*> [#uses=1]
  %20 = tail call i32 @fputc(i32 10, i8* %19) nounwind ; <i32> [#uses=0]
  br label %bb5.i

bb5.i:                                            ; preds = %bb4.i, %bb3.i3
  %indvar.next.i4 = add i64 %indvar.i1, 1         ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next.i4, 1024   ; <i1> [#uses=1]
  br i1 %exitcond, label %bb7.i, label %bb3.i3

bb7.i:                                            ; preds = %bb5.i
  %indvar.next9.i6 = add i64 %indvar8.i8, 1       ; <i64> [#uses=2]
  %exitcond11 = icmp eq i64 %indvar.next9.i6, 1024 ; <i1> [#uses=1]
  br i1 %exitcond11, label %bb9.i, label %bb6.preheader.i

bb6.preheader.i:                                  ; preds = %bb7.i, %bb.i
  %indvar8.i8 = phi i64 [ %indvar.next9.i6, %bb7.i ], [ 0, %bb.i ] ; <i64> [#uses=3]
  %tmp12 = shl i64 %indvar8.i8, 10                ; <i64> [#uses=1]
  br label %bb3.i3

bb9.i:                                            ; preds = %bb7.i
  %21 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %22 = bitcast %struct._IO_FILE* %21 to i8*      ; <i8*> [#uses=1]
  %23 = tail call i32 @fputc(i32 10, i8* %22) nounwind ; <i32> [#uses=0]
  ret i32 0

print_array.exit:                                 ; preds = %bb.i, %init_array.exit
  ret i32 0
}
