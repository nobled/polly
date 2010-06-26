; RUN: opt  -O3 -loopsimplify -indvars -polly-scop-extract  -print-top-scop-only -analyze %s | FileCheck %s
; XFAIL: *
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
define void @scop_func(i64 %n) nounwind {
entry:
  store double 1.000000e+00, double* getelementptr inbounds ([1025 x double]* @b, i64 0, i64 0), align 32
  %0 = icmp sgt i64 %n, 0                         ; <i1> [#uses=1]
  br i1 %0, label %bb.nph81, label %bb14

bb.nph43:                                         ; preds = %bb5.preheader
  %1 = icmp sgt i64 %storemerge60, 0              ; <i1> [#uses=1]
  %2 = getelementptr inbounds [1025 x [1025 x double]]* @a, i64 0, i64 %storemerge60, i64 %storemerge60 ; <double*> [#uses=2]
  %tmp144 = add i64 %storemerge60, 2              ; <i64> [#uses=2]
  br i1 %1, label %bb.nph43.split.us, label %bb4

bb.nph43.split.us:                                ; preds = %bb.nph43
  %tmp147 = mul i64 %storemerge60, 1026           ; <i64> [#uses=1]
  br label %bb.nph35.us

bb4.us:                                           ; preds = %bb2.us
  %3 = load double* %2, align 8                   ; <double> [#uses=1]
  %4 = fdiv double %10, %3                        ; <double> [#uses=1]
  store double %4, double* %scevgep148, align 8
  %5 = icmp sgt i64 %storemerge5.us, %n           ; <i1> [#uses=1]
  br i1 %5, label %bb11.loopexit, label %bb.nph35.us

bb2.us:                                           ; preds = %bb.nph35.us, %bb2.us
  %6 = phi double [ %12, %bb.nph35.us ], [ %10, %bb2.us ] ; <double> [#uses=1]
  %storemerge834.us = phi i64 [ 0, %bb.nph35.us ], [ %11, %bb2.us ] ; <i64> [#uses=3]
  %scevgep141 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 %tmp142, i64 %storemerge834.us ; <double*> [#uses=1]
  %scevgep136 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 %storemerge834.us, i64 %storemerge60 ; <double*> [#uses=1]
  %7 = load double* %scevgep141, align 8          ; <double> [#uses=1]
  %8 = load double* %scevgep136, align 8          ; <double> [#uses=1]
  %9 = fmul double %7, %8                         ; <double> [#uses=1]
  %10 = fsub double %6, %9                        ; <double> [#uses=3]
  %11 = add nsw i64 %storemerge834.us, 1          ; <i64> [#uses=2]
  %exitcond135 = icmp eq i64 %11, %storemerge60   ; <i1> [#uses=1]
  br i1 %exitcond135, label %bb4.us, label %bb2.us

bb.nph35.us:                                      ; preds = %bb4.us, %bb.nph43.split.us
  %indvar137 = phi i64 [ %tmp146, %bb4.us ], [ 0, %bb.nph43.split.us ] ; <i64> [#uses=3]
  %tmp142 = add i64 %storemerge538, %indvar137    ; <i64> [#uses=1]
  %storemerge5.us = add i64 %tmp144, %indvar137   ; <i64> [#uses=1]
  %tmp146 = add i64 %indvar137, 1                 ; <i64> [#uses=2]
  %scevgep148 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 %tmp146, i64 %tmp147 ; <double*> [#uses=2]
  %12 = load double* %scevgep148, align 8         ; <double> [#uses=1]
  br label %bb2.us

bb4:                                              ; preds = %bb4, %bb.nph43
  %indvar152 = phi i64 [ %indvar.next153, %bb4 ], [ 0, %bb.nph43 ] ; <i64> [#uses=3]
  %storemerge62 = phi i64 [ %storemerge62, %bb4 ], [ %storemerge60, %bb.nph43 ] ; <i64> [#uses=3]
  %tmp155 = add i64 %storemerge538, %indvar152    ; <i64> [#uses=1]
  %storemerge5 = add i64 %tmp144, %indvar152      ; <i64> [#uses=1]
  %scevgep157 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 %tmp155, i64 %storemerge62 ; <double*> [#uses=2]
  %13 = load double* %scevgep157, align 8         ; <double> [#uses=2]
  %14 = load double* %2, align 8                  ; <double> [#uses=1]
  %15 = fdiv double %13, %14                      ; <double> [#uses=1]
  store double %15, double* %scevgep157, align 8
  %16 = icmp sgt i64 %storemerge5, %n             ; <i1> [#uses=1]
  %indvar.next153 = add i64 %indvar152, 1         ; <i64> [#uses=1]
  br i1 %16, label %bb11.loopexit, label %bb4

bb.nph56:                                         ; preds = %bb11.loopexit
  %17 = icmp slt i64 %storemerge72, 0             ; <i1> [#uses=1]
  %tmp = add i64 %storemerge72, 2                 ; <i64> [#uses=2]
  %tmp125 = mul i64 %storemerge72, 1026           ; <i64> [#uses=1]
  %tmp126182 = or i64 %tmp125, 1                  ; <i64> [#uses=2]
  br i1 %17, label %bb10.us, label %bb.nph47

bb10.us:                                          ; preds = %bb10.us, %bb.nph56
  %indvar122 = phi i64 [ %indvar.next123, %bb10.us ], [ 0, %bb.nph56 ] ; <i64> [#uses=4]
  %storemerge6.us = add i64 %tmp, %indvar122      ; <i64> [#uses=1]
  %tmp127 = add i64 %tmp126182, %indvar122        ; <i64> [#uses=1]
  %scevgep128 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 1, i64 %tmp127 ; <double*> [#uses=1]
  %tmp130 = add i64 %storemerge651, %indvar122    ; <i64> [#uses=1]
  %tmp132 = mul i64 %storemerge651, 1025          ; <i64> [#uses=1]
  %scevgep131.sum = add i64 %tmp130, %tmp132      ; <i64> [#uses=1]
  %scevgep133 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 0, i64 %scevgep131.sum ; <double*> [#uses=1]
  %18 = load double* %scevgep133, align 8         ; <double> [#uses=2]
  store double %18, double* %scevgep128, align 8
  %19 = icmp sgt i64 %storemerge6.us, %n          ; <i1> [#uses=1]
  %indvar.next123 = add i64 %indvar122, 1         ; <i64> [#uses=1]
  br i1 %19, label %bb13.loopexit, label %bb10.us

bb.nph47:                                         ; preds = %bb10, %bb.nph56
  %indvar162 = phi i64 [ %indvar.next163, %bb10 ], [ 0, %bb.nph56 ] ; <i64> [#uses=4]
  %tmp170 = add i64 %storemerge651, %indvar162    ; <i64> [#uses=2]
  %storemerge6 = add i64 %tmp, %indvar162         ; <i64> [#uses=1]
  %tmp176 = add i64 %tmp126182, %indvar162        ; <i64> [#uses=1]
  %scevgep177 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 1, i64 %tmp176 ; <double*> [#uses=1]
  %tmp179 = mul i64 %storemerge651, 1025          ; <i64> [#uses=1]
  %scevgep178.sum = add i64 %tmp170, %tmp179      ; <i64> [#uses=1]
  %scevgep180 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 0, i64 %scevgep178.sum ; <double*> [#uses=1]
  %20 = load double* %scevgep180, align 8         ; <double> [#uses=1]
  br label %bb8

bb8:                                              ; preds = %bb8, %bb.nph47
  %w.tmp.048 = phi double [ %20, %bb.nph47 ], [ %24, %bb8 ] ; <double> [#uses=1]
  %storemerge746 = phi i64 [ 0, %bb.nph47 ], [ %25, %bb8 ] ; <i64> [#uses=3]
  %scevgep166 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 %storemerge746, i64 %tmp170 ; <double*> [#uses=1]
  %scevgep167 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 %storemerge651, i64 %storemerge746 ; <double*> [#uses=1]
  %21 = load double* %scevgep167, align 8         ; <double> [#uses=1]
  %22 = load double* %scevgep166, align 8         ; <double> [#uses=1]
  %23 = fmul double %21, %22                      ; <double> [#uses=1]
  %24 = fsub double %w.tmp.048, %23               ; <double> [#uses=3]
  %25 = add nsw i64 %storemerge746, 1             ; <i64> [#uses=2]
  %26 = icmp sgt i64 %25, %storemerge72           ; <i1> [#uses=1]
  br i1 %26, label %bb10, label %bb8

bb10:                                             ; preds = %bb8
  store double %24, double* %scevgep177, align 8
  %27 = icmp sgt i64 %storemerge6, %n             ; <i1> [#uses=1]
  %indvar.next163 = add i64 %indvar162, 1         ; <i64> [#uses=1]
  br i1 %27, label %bb13.loopexit, label %bb.nph47

bb11.loopexit:                                    ; preds = %bb5.preheader, %bb4, %bb4.us
  %w.tmp.082 = phi double [ %w.tmp.1, %bb5.preheader ], [ %10, %bb4.us ], [ %13, %bb4 ] ; <double> [#uses=1]
  %storemerge72 = phi i64 [ %storemerge60, %bb5.preheader ], [ %storemerge60, %bb4.us ], [ %storemerge62, %bb4 ] ; <i64> [#uses=5]
  %storemerge651 = add i64 %storemerge72, 1       ; <i64> [#uses=8]
  %28 = icmp sgt i64 %storemerge651, %n           ; <i1> [#uses=1]
  br i1 %28, label %bb13.loopexit, label %bb.nph56

bb13.loopexit:                                    ; preds = %bb11.loopexit, %bb10, %bb10.us
  %w.tmp.2 = phi double [ %w.tmp.082, %bb11.loopexit ], [ %18, %bb10.us ], [ %24, %bb10 ] ; <double> [#uses=2]
  %29 = icmp slt i64 %storemerge651, %n           ; <i1> [#uses=1]
  br i1 %29, label %bb5.preheader, label %bb13.bb14_crit_edge

bb13.bb14_crit_edge:                              ; preds = %bb13.loopexit
  store double %w.tmp.2, double* @w
  br label %bb14

bb.nph81:                                         ; preds = %entry
  %w.promoted = load double* @w                   ; <double> [#uses=1]
  br label %bb5.preheader

bb5.preheader:                                    ; preds = %bb.nph81, %bb13.loopexit
  %w.tmp.1 = phi double [ %w.promoted, %bb.nph81 ], [ %w.tmp.2, %bb13.loopexit ] ; <double> [#uses=1]
  %storemerge60 = phi i64 [ 0, %bb.nph81 ], [ %storemerge651, %bb13.loopexit ] ; <i64> [#uses=11]
  %storemerge538 = add i64 %storemerge60, 1       ; <i64> [#uses=3]
  %30 = icmp sgt i64 %storemerge538, %n           ; <i1> [#uses=1]
  br i1 %30, label %bb11.loopexit, label %bb.nph43

bb14:                                             ; preds = %bb13.bb14_crit_edge, %entry
  store double 1.000000e+00, double* getelementptr inbounds ([1025 x double]* @y, i64 0, i64 0), align 32
  %31 = icmp slt i64 %n, 1                        ; <i1> [#uses=1]
  br i1 %31, label %bb20, label %bb15

bb15:                                             ; preds = %bb18, %bb14
  %indvar111 = phi i64 [ %storemerge126, %bb18 ], [ 0, %bb14 ] ; <i64> [#uses=2]
  %storemerge126 = add i64 %indvar111, 1          ; <i64> [#uses=6]
  %tmp117 = add i64 %indvar111, 2                 ; <i64> [#uses=1]
  %scevgep118 = getelementptr [1025 x double]* @y, i64 0, i64 %storemerge126 ; <double*> [#uses=1]
  %scevgep119 = getelementptr [1025 x double]* @b, i64 0, i64 %storemerge126 ; <double*> [#uses=1]
  %32 = load double* %scevgep119, align 8         ; <double> [#uses=2]
  %33 = icmp sgt i64 %storemerge126, 0            ; <i1> [#uses=1]
  br i1 %33, label %bb16, label %bb18

bb16:                                             ; preds = %bb16, %bb15
  %34 = phi double [ %32, %bb15 ], [ %38, %bb16 ] ; <double> [#uses=1]
  %storemerge423 = phi i64 [ 0, %bb15 ], [ %39, %bb16 ] ; <i64> [#uses=3]
  %scevgep114 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 %storemerge126, i64 %storemerge423 ; <double*> [#uses=1]
  %scevgep113 = getelementptr [1025 x double]* @y, i64 0, i64 %storemerge423 ; <double*> [#uses=1]
  %35 = load double* %scevgep114, align 8         ; <double> [#uses=1]
  %36 = load double* %scevgep113, align 8         ; <double> [#uses=1]
  %37 = fmul double %35, %36                      ; <double> [#uses=1]
  %38 = fsub double %34, %37                      ; <double> [#uses=2]
  %39 = add nsw i64 %storemerge423, 1             ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %39, %storemerge126     ; <i1> [#uses=1]
  br i1 %exitcond, label %bb18, label %bb16

bb18:                                             ; preds = %bb16, %bb15
  %w.tmp.032 = phi double [ %32, %bb15 ], [ %38, %bb16 ] ; <double> [#uses=2]
  store double %w.tmp.032, double* %scevgep118, align 8
  %40 = icmp sgt i64 %tmp117, %n                  ; <i1> [#uses=1]
  br i1 %40, label %bb19.bb20_crit_edge, label %bb15

bb19.bb20_crit_edge:                              ; preds = %bb18
  store double %w.tmp.032, double* @w
  br label %bb20

bb20:                                             ; preds = %bb19.bb20_crit_edge, %bb14
  %41 = getelementptr inbounds [1025 x double]* @y, i64 0, i64 %n ; <double*> [#uses=1]
  %42 = load double* %41, align 8                 ; <double> [#uses=1]
  %43 = getelementptr inbounds [1025 x [1025 x double]]* @a, i64 0, i64 %n, i64 %n ; <double*> [#uses=1]
  %44 = load double* %43, align 8                 ; <double> [#uses=1]
  %45 = fdiv double %42, %44                      ; <double> [#uses=1]
  %46 = getelementptr inbounds [1025 x double]* @x, i64 0, i64 %n ; <double*> [#uses=1]
  store double %45, double* %46, align 8
  %47 = add nsw i64 %n, -1                        ; <i64> [#uses=3]
  %48 = icmp slt i64 %47, 0                       ; <i1> [#uses=1]
  br i1 %48, label %return, label %bb.nph19

bb.nph19:                                         ; preds = %bb20
  %tmp86 = mul i64 %n, 1026                       ; <i64> [#uses=2]
  %tmp90 = add i64 %n, 1                          ; <i64> [#uses=1]
  %tmp94 = add i64 %tmp86, -1                     ; <i64> [#uses=1]
  br label %bb21

bb21:                                             ; preds = %bb24, %bb.nph19
  %storemerge211 = phi i64 [ 0, %bb.nph19 ], [ %tmp109, %bb24 ] ; <i64> [#uses=5]
  %tmp93 = mul i64 %storemerge211, -1026          ; <i64> [#uses=2]
  %tmp95 = add i64 %tmp94, %tmp93                 ; <i64> [#uses=1]
  %scevgep96 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 -1, i64 %tmp95 ; <double*> [#uses=1]
  %tmp98 = sub i64 %47, %storemerge211            ; <i64> [#uses=2]
  %scevgep99 = getelementptr [1025 x double]* @x, i64 0, i64 %tmp98 ; <double*> [#uses=1]
  %scevgep100 = getelementptr [1025 x double]* @y, i64 0, i64 %tmp98 ; <double*> [#uses=1]
  %tmp101 = add i64 %tmp86, %tmp93                ; <i64> [#uses=1]
  %tmp104 = sub i64 %tmp90, %storemerge211        ; <i64> [#uses=1]
  %tmp106 = sub i64 %n, %storemerge211            ; <i64> [#uses=2]
  %tmp109 = add i64 %storemerge211, 1             ; <i64> [#uses=2]
  %49 = load double* %scevgep100, align 8         ; <double> [#uses=2]
  %50 = icmp sgt i64 %tmp106, %n                  ; <i1> [#uses=1]
  br i1 %50, label %bb24, label %bb22

bb22:                                             ; preds = %bb22, %bb21
  %indvar = phi i64 [ %indvar.next, %bb22 ], [ 0, %bb21 ] ; <i64> [#uses=4]
  %w.tmp.0 = phi double [ %54, %bb22 ], [ %49, %bb21 ] ; <double> [#uses=1]
  %tmp102 = add i64 %tmp101, %indvar              ; <i64> [#uses=1]
  %scevgep89 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 -1, i64 %tmp102 ; <double*> [#uses=1]
  %tmp107 = add i64 %tmp106, %indvar              ; <i64> [#uses=1]
  %scevgep = getelementptr [1025 x double]* @x, i64 0, i64 %tmp107 ; <double*> [#uses=1]
  %51 = load double* %scevgep89, align 8          ; <double> [#uses=1]
  %52 = load double* %scevgep, align 8            ; <double> [#uses=1]
  %53 = fmul double %51, %52                      ; <double> [#uses=1]
  %54 = fsub double %w.tmp.0, %53                 ; <double> [#uses=2]
  %tmp92 = add i64 %tmp104, %indvar               ; <i64> [#uses=1]
  %55 = icmp sgt i64 %tmp92, %n                   ; <i1> [#uses=1]
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=1]
  br i1 %55, label %bb24, label %bb22

bb24:                                             ; preds = %bb22, %bb21
  %w.tmp.021 = phi double [ %49, %bb21 ], [ %54, %bb22 ] ; <double> [#uses=2]
  %56 = load double* %scevgep96, align 8          ; <double> [#uses=1]
  %57 = fdiv double %w.tmp.021, %56               ; <double> [#uses=1]
  store double %57, double* %scevgep99, align 8
  %58 = icmp slt i64 %47, %tmp109                 ; <i1> [#uses=1]
  br i1 %58, label %bb25.return_crit_edge, label %bb21

bb25.return_crit_edge:                            ; preds = %bb24
  store double %w.tmp.021, double* @w
  ret void

return:                                           ; preds = %bb20
  ret void
}
