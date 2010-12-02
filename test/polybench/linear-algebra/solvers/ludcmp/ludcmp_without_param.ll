; RUN: %opt -mem2reg -loopsimplify -indvars -polly-prepare -polly-detect -polly-cloog -analyze  %s | FileCheck %s
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }
@x = common global [1025 x double] zeroinitializer, align 32 ; <[1025 x double]*> [#uses=7]
@b = common global [1025 x double] zeroinitializer, align 32 ; <[1025 x double]*> [#uses=4]
@a = common global [1025 x [1025 x double]] zeroinitializer, align 32 ; <[1025 x [1025 x double]]*> [#uses=17]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
@w = common global double 0.000000e+00            ; <double*> [#uses=4]
@y = common global [1025 x double] zeroinitializer, align 32 ; <[1025 x double]*> [#uses=5]
define void @scop_func() nounwind {
bb.nph76:
  store double 1.000000e+00, double* getelementptr inbounds ([1025 x double]* @b, i64 0, i64 0), align 32
  %w.promoted = load double* @w                   ; <double> [#uses=1]
  br label %bb5.preheader

bb.nph38:                                         ; preds = %bb5.preheader
  %0 = icmp sgt i64 %storemerge55, 0              ; <i1> [#uses=1]
  %1 = getelementptr inbounds [1025 x [1025 x double]]* @a, i64 0, i64 %storemerge55, i64 %storemerge55 ; <double*> [#uses=2]
  %tmp137 = add i64 %storemerge55, 2              ; <i64> [#uses=2]
  br i1 %0, label %bb.nph38.split.us, label %bb4

bb.nph38.split.us:                                ; preds = %bb.nph38
  %tmp140 = mul i64 %storemerge55, 1026           ; <i64> [#uses=1]
  br label %bb.nph30.us

bb4.us:                                           ; preds = %bb2.us
  %2 = load double* %1, align 8                   ; <double> [#uses=1]
  %3 = fdiv double %9, %2                         ; <double> [#uses=1]
  store double %3, double* %scevgep141, align 8
  %4 = icmp sgt i64 %storemerge5.us, 1024         ; <i1> [#uses=1]
  br i1 %4, label %bb11.loopexit, label %bb.nph30.us

bb2.us:                                           ; preds = %bb.nph30.us, %bb2.us
  %5 = phi double [ %11, %bb.nph30.us ], [ %9, %bb2.us ] ; <double> [#uses=1]
  %storemerge829.us = phi i64 [ 0, %bb.nph30.us ], [ %10, %bb2.us ] ; <i64> [#uses=3]
  %scevgep134 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 %tmp135, i64 %storemerge829.us ; <double*> [#uses=1]
  %scevgep129 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 %storemerge829.us, i64 %storemerge55 ; <double*> [#uses=1]
  %6 = load double* %scevgep134, align 8          ; <double> [#uses=1]
  %7 = load double* %scevgep129, align 8          ; <double> [#uses=1]
  %8 = fmul double %6, %7                         ; <double> [#uses=1]
  %9 = fsub double %5, %8                         ; <double> [#uses=3]
  %10 = add nsw i64 %storemerge829.us, 1          ; <i64> [#uses=2]
  %exitcond128 = icmp eq i64 %10, %storemerge55   ; <i1> [#uses=1]
  br i1 %exitcond128, label %bb4.us, label %bb2.us

bb.nph30.us:                                      ; preds = %bb4.us, %bb.nph38.split.us
  %indvar130 = phi i64 [ %tmp139, %bb4.us ], [ 0, %bb.nph38.split.us ] ; <i64> [#uses=3]
  %tmp135 = add i64 %storemerge533, %indvar130    ; <i64> [#uses=1]
  %storemerge5.us = add i64 %tmp137, %indvar130   ; <i64> [#uses=1]
  %tmp139 = add i64 %indvar130, 1                 ; <i64> [#uses=2]
  %scevgep141 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 %tmp139, i64 %tmp140 ; <double*> [#uses=2]
  %11 = load double* %scevgep141, align 8         ; <double> [#uses=1]
  br label %bb2.us

bb4:                                              ; preds = %bb4, %bb.nph38
  %indvar145 = phi i64 [ %indvar.next146, %bb4 ], [ 0, %bb.nph38 ] ; <i64> [#uses=3]
  %storemerge57 = phi i64 [ %storemerge57, %bb4 ], [ %storemerge55, %bb.nph38 ] ; <i64> [#uses=3]
  %tmp148 = add i64 %storemerge533, %indvar145    ; <i64> [#uses=1]
  %storemerge5 = add i64 %tmp137, %indvar145      ; <i64> [#uses=1]
  %scevgep150 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 %tmp148, i64 %storemerge57 ; <double*> [#uses=2]
  %12 = load double* %scevgep150, align 8         ; <double> [#uses=2]
  %13 = load double* %1, align 8                  ; <double> [#uses=1]
  %14 = fdiv double %12, %13                      ; <double> [#uses=1]
  store double %14, double* %scevgep150, align 8
  %15 = icmp sgt i64 %storemerge5, 1024           ; <i1> [#uses=1]
  %indvar.next146 = add i64 %indvar145, 1         ; <i64> [#uses=1]
  br i1 %15, label %bb11.loopexit, label %bb4

bb.nph51:                                         ; preds = %bb11.loopexit
  %16 = icmp slt i64 %storemerge67, 0             ; <i1> [#uses=1]
  %tmp116 = add i64 %storemerge67, 2              ; <i64> [#uses=2]
  %tmp118 = mul i64 %storemerge67, 1026           ; <i64> [#uses=1]
  %tmp119175 = or i64 %tmp118, 1                  ; <i64> [#uses=2]
  br i1 %16, label %bb10.us, label %bb.nph42

bb10.us:                                          ; preds = %bb10.us, %bb.nph51
  %indvar114 = phi i64 [ %indvar.next115, %bb10.us ], [ 0, %bb.nph51 ] ; <i64> [#uses=4]
  %storemerge6.us = add i64 %tmp116, %indvar114   ; <i64> [#uses=1]
  %tmp120 = add i64 %tmp119175, %indvar114        ; <i64> [#uses=1]
  %scevgep121 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 1, i64 %tmp120 ; <double*> [#uses=1]
  %tmp123 = add i64 %storemerge646, %indvar114    ; <i64> [#uses=1]
  %tmp125 = mul i64 %storemerge646, 1025          ; <i64> [#uses=1]
  %scevgep124.sum = add i64 %tmp123, %tmp125      ; <i64> [#uses=1]
  %scevgep126 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 0, i64 %scevgep124.sum ; <double*> [#uses=1]
  %17 = load double* %scevgep126, align 8         ; <double> [#uses=2]
  store double %17, double* %scevgep121, align 8
  %18 = icmp sgt i64 %storemerge6.us, 1024        ; <i1> [#uses=1]
  %indvar.next115 = add i64 %indvar114, 1         ; <i64> [#uses=1]
  br i1 %18, label %bb13.loopexit, label %bb10.us

bb.nph42:                                         ; preds = %bb10, %bb.nph51
  %indvar155 = phi i64 [ %indvar.next156, %bb10 ], [ 0, %bb.nph51 ] ; <i64> [#uses=4]
  %tmp163 = add i64 %storemerge646, %indvar155    ; <i64> [#uses=2]
  %storemerge6 = add i64 %tmp116, %indvar155      ; <i64> [#uses=1]
  %tmp169 = add i64 %tmp119175, %indvar155        ; <i64> [#uses=1]
  %scevgep170 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 1, i64 %tmp169 ; <double*> [#uses=1]
  %tmp172 = mul i64 %storemerge646, 1025          ; <i64> [#uses=1]
  %scevgep171.sum = add i64 %tmp163, %tmp172      ; <i64> [#uses=1]
  %scevgep173 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 0, i64 %scevgep171.sum ; <double*> [#uses=1]
  %19 = load double* %scevgep173, align 8         ; <double> [#uses=1]
  br label %bb8

bb8:                                              ; preds = %bb8, %bb.nph42
  %w.tmp.043 = phi double [ %19, %bb.nph42 ], [ %23, %bb8 ] ; <double> [#uses=1]
  %storemerge741 = phi i64 [ 0, %bb.nph42 ], [ %24, %bb8 ] ; <i64> [#uses=3]
  %scevgep159 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 %storemerge741, i64 %tmp163 ; <double*> [#uses=1]
  %scevgep160 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 %storemerge646, i64 %storemerge741 ; <double*> [#uses=1]
  %20 = load double* %scevgep160, align 8         ; <double> [#uses=1]
  %21 = load double* %scevgep159, align 8         ; <double> [#uses=1]
  %22 = fmul double %20, %21                      ; <double> [#uses=1]
  %23 = fsub double %w.tmp.043, %22               ; <double> [#uses=3]
  %24 = add nsw i64 %storemerge741, 1             ; <i64> [#uses=2]
  %25 = icmp sgt i64 %24, %storemerge67           ; <i1> [#uses=1]
  br i1 %25, label %bb10, label %bb8

bb10:                                             ; preds = %bb8
  store double %23, double* %scevgep170, align 8
  %26 = icmp sgt i64 %storemerge6, 1024           ; <i1> [#uses=1]
  %indvar.next156 = add i64 %indvar155, 1         ; <i64> [#uses=1]
  br i1 %26, label %bb13.loopexit, label %bb.nph42

bb11.loopexit:                                    ; preds = %bb5.preheader, %bb4, %bb4.us
  %w.tmp.077 = phi double [ %w.tmp.1, %bb5.preheader ], [ %9, %bb4.us ], [ %12, %bb4 ] ; <double> [#uses=1]
  %storemerge67 = phi i64 [ %storemerge55, %bb5.preheader ], [ %storemerge55, %bb4.us ], [ %storemerge57, %bb4 ] ; <i64> [#uses=5]
  %storemerge646 = add i64 %storemerge67, 1       ; <i64> [#uses=8]
  %27 = icmp sgt i64 %storemerge646, 1024         ; <i1> [#uses=1]
  br i1 %27, label %bb13.loopexit, label %bb.nph51

bb13.loopexit:                                    ; preds = %bb11.loopexit, %bb10, %bb10.us
  %w.tmp.2 = phi double [ %w.tmp.077, %bb11.loopexit ], [ %17, %bb10.us ], [ %23, %bb10 ] ; <double> [#uses=2]
  %28 = icmp slt i64 %storemerge646, 1024         ; <i1> [#uses=1]
  br i1 %28, label %bb5.preheader, label %bb.nph25

bb5.preheader:                                    ; preds = %bb13.loopexit, %bb.nph76
  %w.tmp.1 = phi double [ %w.promoted, %bb.nph76 ], [ %w.tmp.2, %bb13.loopexit ] ; <double> [#uses=1]
  %storemerge55 = phi i64 [ 0, %bb.nph76 ], [ %storemerge646, %bb13.loopexit ] ; <i64> [#uses=11]
  %storemerge533 = add i64 %storemerge55, 1       ; <i64> [#uses=3]
  %29 = icmp sgt i64 %storemerge533, 1024         ; <i1> [#uses=1]
  br i1 %29, label %bb11.loopexit, label %bb.nph38

bb.nph25:                                         ; preds = %bb13.loopexit
  store double %w.tmp.2, double* @w
  store double 1.000000e+00, double* getelementptr inbounds ([1025 x double]* @y, i64 0, i64 0), align 32
  br label %bb.nph19

bb.nph19:                                         ; preds = %bb18, %bb.nph25
  %indvar102 = phi i64 [ 0, %bb.nph25 ], [ %tmp, %bb18 ] ; <i64> [#uses=1]
  %tmp = add i64 %indvar102, 1                    ; <i64> [#uses=6]
  %scevgep110 = getelementptr [1025 x double]* @y, i64 0, i64 %tmp ; <double*> [#uses=1]
  %scevgep111 = getelementptr [1025 x double]* @b, i64 0, i64 %tmp ; <double*> [#uses=1]
  %30 = load double* %scevgep111, align 8         ; <double> [#uses=1]
  br label %bb16

bb16:                                             ; preds = %bb16, %bb.nph19
  %31 = phi double [ %30, %bb.nph19 ], [ %35, %bb16 ] ; <double> [#uses=1]
  %storemerge418 = phi i64 [ 0, %bb.nph19 ], [ %36, %bb16 ] ; <i64> [#uses=3]
  %scevgep106 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 %tmp, i64 %storemerge418 ; <double*> [#uses=1]
  %scevgep105 = getelementptr [1025 x double]* @y, i64 0, i64 %storemerge418 ; <double*> [#uses=1]
  %32 = load double* %scevgep106, align 8         ; <double> [#uses=1]
  %33 = load double* %scevgep105, align 8         ; <double> [#uses=1]
  %34 = fmul double %32, %33                      ; <double> [#uses=1]
  %35 = fsub double %31, %34                      ; <double> [#uses=3]
  %36 = add nsw i64 %storemerge418, 1             ; <i64> [#uses=2]
  %exitcond104 = icmp eq i64 %36, %tmp            ; <i1> [#uses=1]
  br i1 %exitcond104, label %bb18, label %bb16

bb18:                                             ; preds = %bb16
  store double %35, double* %scevgep110, align 8
  %exitcond107 = icmp eq i64 %tmp, 1024           ; <i1> [#uses=1]
  br i1 %exitcond107, label %bb.nph14, label %bb.nph19

bb.nph14:                                         ; preds = %bb18
  store double %35, double* @w
  %37 = load double* getelementptr inbounds ([1025 x double]* @y, i64 0, i64 1024), align 32 ; <double> [#uses=1]
  %38 = load double* getelementptr inbounds ([1025 x [1025 x double]]* @a, i64 0, i64 1024, i64 1024), align 32 ; <double> [#uses=1]
  %39 = fdiv double %37, %38                      ; <double> [#uses=1]
  store double %39, double* getelementptr inbounds ([1025 x double]* @x, i64 0, i64 1024), align 32
  br label %bb.nph

bb.nph:                                           ; preds = %bb24, %bb.nph14
  %storemerge210 = phi i64 [ 0, %bb.nph14 ], [ %48, %bb24 ] ; <i64> [#uses=5]
  %tmp86 = mul i64 %storemerge210, -1026          ; <i64> [#uses=2]
  %tmp87 = add i64 %tmp86, 1024                   ; <i64> [#uses=1]
  %tmp91 = sub i64 1025, %storemerge210           ; <i64> [#uses=1]
  %tmp93 = sub i64 1024, %storemerge210           ; <i64> [#uses=1]
  %tmp96 = add i64 %tmp86, 1023                   ; <i64> [#uses=1]
  %scevgep97 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 1023, i64 %tmp96 ; <double*> [#uses=1]
  %tmp98 = sub i64 1023, %storemerge210           ; <i64> [#uses=2]
  %scevgep99 = getelementptr [1025 x double]* @x, i64 0, i64 %tmp98 ; <double*> [#uses=1]
  %scevgep100 = getelementptr [1025 x double]* @y, i64 0, i64 %tmp98 ; <double*> [#uses=1]
  %40 = load double* %scevgep100, align 8         ; <double> [#uses=1]
  br label %bb22

bb22:                                             ; preds = %bb22, %bb.nph
  %indvar = phi i64 [ 0, %bb.nph ], [ %indvar.next, %bb22 ] ; <i64> [#uses=4]
  %w.tmp.0 = phi double [ %40, %bb.nph ], [ %44, %bb22 ] ; <double> [#uses=1]
  %tmp88 = add i64 %tmp87, %indvar                ; <i64> [#uses=1]
  %scevgep83 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 1023, i64 %tmp88 ; <double*> [#uses=1]
  %tmp94 = add i64 %tmp93, %indvar                ; <i64> [#uses=1]
  %scevgep = getelementptr [1025 x double]* @x, i64 0, i64 %tmp94 ; <double*> [#uses=1]
  %41 = load double* %scevgep83, align 8          ; <double> [#uses=1]
  %42 = load double* %scevgep, align 8            ; <double> [#uses=1]
  %43 = fmul double %41, %42                      ; <double> [#uses=1]
  %44 = fsub double %w.tmp.0, %43                 ; <double> [#uses=3]
  %tmp85 = add i64 %tmp91, %indvar                ; <i64> [#uses=1]
  %45 = icmp sgt i64 %tmp85, 1024                 ; <i1> [#uses=1]
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=1]
  br i1 %45, label %bb24, label %bb22

bb24:                                             ; preds = %bb22
  %46 = load double* %scevgep97, align 8          ; <double> [#uses=1]
  %47 = fdiv double %44, %46                      ; <double> [#uses=1]
  store double %47, double* %scevgep99, align 8
  %48 = add nsw i64 %storemerge210, 1             ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %48, 1024               ; <i1> [#uses=1]
  br i1 %exitcond, label %return, label %bb.nph

return:                                           ; preds = %bb24
  store double %44, double* @w
  ret void
}

; CHECK: for region: 'bb5.preheader => return' in function 'scop_func':

