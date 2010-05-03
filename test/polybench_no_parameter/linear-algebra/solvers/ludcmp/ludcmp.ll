; RUN: opt  -indvars -polly-scop-detect -print-top-scop-only -polly-print-scop -analyze  %s | FileCheck %s
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

define void @init_array() nounwind inlinehint {
bb.nph6.split.us:
  br label %bb.nph.us

bb3.us:                                           ; preds = %bb1.us
  %indvar.next8 = add i64 %indvar7, 1             ; <i64> [#uses=2]
  %exitcond9 = icmp eq i64 %indvar.next8, 1025    ; <i1> [#uses=1]
  br i1 %exitcond9, label %return, label %bb.nph.us

bb1.us:                                           ; preds = %bb.nph.us, %bb1.us
  %indvar = phi i64 [ 0, %bb.nph.us ], [ %indvar.next, %bb1.us ] ; <i64> [#uses=3]
  %scevgep = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 %indvar7, i64 %indvar ; <double*> [#uses=1]
  %j.01.us = trunc i64 %indvar to i32             ; <i32> [#uses=1]
  %0 = sitofp i32 %j.01.us to double              ; <double> [#uses=1]
  %1 = fmul double %4, %0                         ; <double> [#uses=1]
  %2 = fadd double %1, 1.000000e+00               ; <double> [#uses=1]
  %3 = fdiv double %2, 1.024000e+03               ; <double> [#uses=1]
  store double %3, double* %scevgep, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 1025      ; <i1> [#uses=1]
  br i1 %exitcond, label %bb3.us, label %bb1.us

bb.nph.us:                                        ; preds = %bb3.us, %bb.nph6.split.us
  %indvar7 = phi i64 [ %indvar.next8, %bb3.us ], [ 0, %bb.nph6.split.us ] ; <i64> [#uses=5]
  %i.02.us = trunc i64 %indvar7 to i32            ; <i32> [#uses=1]
  %scevgep11 = getelementptr [1025 x double]* @b, i64 0, i64 %indvar7 ; <double*> [#uses=1]
  %scevgep12 = getelementptr [1025 x double]* @x, i64 0, i64 %indvar7 ; <double*> [#uses=1]
  %4 = sitofp i32 %i.02.us to double              ; <double> [#uses=3]
  %5 = fadd double %4, 1.000000e+00               ; <double> [#uses=1]
  %6 = fdiv double %5, 1.024000e+03               ; <double> [#uses=1]
  store double %6, double* %scevgep12, align 8
  %7 = fadd double %4, 2.000000e+00               ; <double> [#uses=1]
  %8 = fdiv double %7, 1.024000e+03               ; <double> [#uses=1]
  store double %8, double* %scevgep11, align 8
  br label %bb1.us

return:                                           ; preds = %bb3.us
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
  br i1 %3, label %bb2, label %return

bb2:                                              ; preds = %bb4, %bb
  %indvar = phi i64 [ %indvar.next, %bb4 ], [ 0, %bb ] ; <i64> [#uses=3]
  %scevgep = getelementptr [1025 x double]* @x, i64 0, i64 %indvar ; <double*> [#uses=1]
  %i.01 = trunc i64 %indvar to i32                ; <i32> [#uses=1]
  %4 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %5 = load %struct._IO_FILE** @stderr, align 8   ; <%struct._IO_FILE*> [#uses=1]
  %6 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %5, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %4) nounwind ; <i32> [#uses=0]
  %7 = srem i32 %i.01, 80                         ; <i32> [#uses=1]
  %8 = icmp eq i32 %7, 20                         ; <i1> [#uses=1]
  br i1 %8, label %bb3, label %bb4

bb3:                                              ; preds = %bb2
  %9 = load %struct._IO_FILE** @stderr, align 8   ; <%struct._IO_FILE*> [#uses=1]
  %10 = bitcast %struct._IO_FILE* %9 to i8*       ; <i8*> [#uses=1]
  %11 = tail call i32 @fputc(i32 10, i8* %10) nounwind ; <i32> [#uses=0]
  br label %bb4

bb4:                                              ; preds = %bb3, %bb2
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 1025      ; <i1> [#uses=1]
  br i1 %exitcond, label %bb6, label %bb2

bb6:                                              ; preds = %bb4
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
bb.nph70:
  store double 1.000000e+00, double* getelementptr inbounds ([1025 x double]* @b, i64 0, i64 0), align 32
  %w.promoted = load double* @w                   ; <double> [#uses=1]
  br label %bb5.preheader

bb.nph32:                                         ; preds = %bb5.preheader
  %0 = icmp sgt i32 %i.049, 0                     ; <i1> [#uses=1]
  %1 = sext i32 %i.049 to i64                     ; <i64> [#uses=5]
  %2 = getelementptr inbounds [1025 x [1025 x double]]* @a, i64 0, i64 %1, i64 %1 ; <double*> [#uses=2]
  br i1 %0, label %bb.nph32.split.us, label %bb.nph32.bb.nph32.split_crit_edge

bb.nph32.bb.nph32.split_crit_edge:                ; preds = %bb.nph32
  %tmp155 = sub i32 1023, %i.049                  ; <i32> [#uses=1]
  %tmp156 = zext i32 %tmp155 to i64               ; <i64> [#uses=1]
  %tmp157 = add i64 %tmp156, 1                    ; <i64> [#uses=1]
  %tmp160 = sext i32 %j.027 to i64                ; <i64> [#uses=1]
  br label %bb4

bb.nph32.split.us:                                ; preds = %bb.nph32
  %tmp131 = zext i32 %i.049 to i64                ; <i64> [#uses=1]
  %tmp141 = sub i32 1023, %i.049                  ; <i32> [#uses=1]
  %tmp142 = zext i32 %tmp141 to i64               ; <i64> [#uses=1]
  %tmp143 = add i64 %tmp142, 1                    ; <i64> [#uses=1]
  %tmp145 = sext i32 %j.027 to i64                ; <i64> [#uses=1]
  br label %bb.nph24.us

bb4.us:                                           ; preds = %bb2.us
  %3 = load double* %2, align 8                   ; <double> [#uses=1]
  %4 = fdiv double %9, %3                         ; <double> [#uses=1]
  store double %4, double* %scevgep149, align 8
  %indvar.next136 = add i64 %indvar135, 1         ; <i64> [#uses=2]
  %exitcond144 = icmp eq i64 %indvar.next136, %tmp143 ; <i1> [#uses=1]
  br i1 %exitcond144, label %bb11.loopexit, label %bb.nph24.us

bb2.us:                                           ; preds = %bb.nph24.us, %bb2.us
  %indvar129 = phi i64 [ 0, %bb.nph24.us ], [ %indvar.next130, %bb2.us ] ; <i64> [#uses=3]
  %5 = phi double [ %10, %bb.nph24.us ], [ %9, %bb2.us ] ; <double> [#uses=1]
  %scevgep140 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 %tmp146, i64 %indvar129 ; <double*> [#uses=1]
  %scevgep134 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 %indvar129, i64 %1 ; <double*> [#uses=1]
  %6 = load double* %scevgep140, align 8          ; <double> [#uses=1]
  %7 = load double* %scevgep134, align 8          ; <double> [#uses=1]
  %8 = fmul double %6, %7                         ; <double> [#uses=1]
  %9 = fsub double %5, %8                         ; <double> [#uses=3]
  %indvar.next130 = add i64 %indvar129, 1         ; <i64> [#uses=2]
  %exitcond132 = icmp eq i64 %indvar.next130, %tmp131 ; <i1> [#uses=1]
  br i1 %exitcond132, label %bb4.us, label %bb2.us

bb.nph24.us:                                      ; preds = %bb4.us, %bb.nph32.split.us
  %indvar135 = phi i64 [ %indvar.next136, %bb4.us ], [ 0, %bb.nph32.split.us ] ; <i64> [#uses=2]
  %tmp146 = add i64 %tmp145, %indvar135           ; <i64> [#uses=2]
  %scevgep149 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 %tmp146, i64 %1 ; <double*> [#uses=2]
  %10 = load double* %scevgep149, align 8         ; <double> [#uses=1]
  br label %bb2.us

bb4:                                              ; preds = %bb4, %bb.nph32.bb.nph32.split_crit_edge
  %indvar153 = phi i64 [ 0, %bb.nph32.bb.nph32.split_crit_edge ], [ %indvar.next154, %bb4 ] ; <i64> [#uses=2]
  %i.051 = phi i32 [ %i.049, %bb.nph32.bb.nph32.split_crit_edge ], [ %i.051, %bb4 ] ; <i32> [#uses=2]
  %tmp161 = add i64 %tmp160, %indvar153           ; <i64> [#uses=1]
  %scevgep163 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 %tmp161, i64 %1 ; <double*> [#uses=2]
  %11 = load double* %scevgep163, align 8         ; <double> [#uses=2]
  %12 = load double* %2, align 8                  ; <double> [#uses=1]
  %13 = fdiv double %11, %12                      ; <double> [#uses=1]
  store double %13, double* %scevgep163, align 8
  %indvar.next154 = add i64 %indvar153, 1         ; <i64> [#uses=2]
  %exitcond158 = icmp eq i64 %indvar.next154, %tmp157 ; <i1> [#uses=1]
  br i1 %exitcond158, label %bb11.loopexit, label %bb4

bb.nph45:                                         ; preds = %bb11.loopexit
  %14 = icmp slt i32 %i.061, 0                    ; <i1> [#uses=1]
  br i1 %14, label %bb.nph45.split.us, label %bb.nph45.bb.nph45.split_crit_edge

bb.nph45.bb.nph45.split_crit_edge:                ; preds = %bb.nph45
  %tmp168 = sext i32 %j.140 to i64                ; <i64> [#uses=4]
  %tmp177 = sub i32 1023, %i.061                  ; <i32> [#uses=1]
  %tmp178 = zext i32 %tmp177 to i64               ; <i64> [#uses=1]
  %tmp179 = add i64 %tmp178, 1                    ; <i64> [#uses=1]
  %tmp184 = mul i64 %tmp168, 1026                 ; <i64> [#uses=1]
  br label %bb.nph36

bb.nph45.split.us:                                ; preds = %bb.nph45
  %tmp114 = sub i32 1023, %i.061                  ; <i32> [#uses=1]
  %tmp115 = zext i32 %tmp114 to i64               ; <i64> [#uses=1]
  %tmp116 = add i64 %tmp115, 1                    ; <i64> [#uses=1]
  %tmp119 = sext i32 %j.140 to i64                ; <i64> [#uses=3]
  %tmp120 = mul i64 %tmp119, 1026                 ; <i64> [#uses=1]
  br label %bb10.us

bb10.us:                                          ; preds = %bb10.us, %bb.nph45.split.us
  %indvar112 = phi i64 [ %indvar.next113, %bb10.us ], [ 0, %bb.nph45.split.us ] ; <i64> [#uses=3]
  %tmp121 = add i64 %tmp120, %indvar112           ; <i64> [#uses=1]
  %scevgep122 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 0, i64 %tmp121 ; <double*> [#uses=1]
  %tmp123 = add i64 %tmp119, %indvar112           ; <i64> [#uses=1]
  %tmp126 = mul i64 %tmp119, 1025                 ; <i64> [#uses=1]
  %scevgep124.sum = add i64 %tmp123, %tmp126      ; <i64> [#uses=1]
  %scevgep127 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 0, i64 %scevgep124.sum ; <double*> [#uses=1]
  %15 = load double* %scevgep127, align 8         ; <double> [#uses=2]
  store double %15, double* %scevgep122, align 8
  %indvar.next113 = add i64 %indvar112, 1         ; <i64> [#uses=2]
  %exitcond117 = icmp eq i64 %indvar.next113, %tmp116 ; <i1> [#uses=1]
  br i1 %exitcond117, label %bb13.loopexit, label %bb10.us

bb.nph36:                                         ; preds = %bb10, %bb.nph45.bb.nph45.split_crit_edge
  %indvar170 = phi i64 [ 0, %bb.nph45.bb.nph45.split_crit_edge ], [ %indvar.next171, %bb10 ] ; <i64> [#uses=3]
  %tmp182 = add i64 %tmp168, %indvar170           ; <i64> [#uses=2]
  %tmp185 = add i64 %tmp184, %indvar170           ; <i64> [#uses=1]
  %scevgep186 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 0, i64 %tmp185 ; <double*> [#uses=1]
  %tmp189 = mul i64 %tmp168, 1025                 ; <i64> [#uses=1]
  %scevgep187.sum = add i64 %tmp182, %tmp189      ; <i64> [#uses=1]
  %scevgep190 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 0, i64 %scevgep187.sum ; <double*> [#uses=1]
  %16 = load double* %scevgep190, align 8         ; <double> [#uses=1]
  br label %bb8

bb8:                                              ; preds = %bb8, %bb.nph36
  %indvar165 = phi i64 [ 0, %bb.nph36 ], [ %tmp174, %bb8 ] ; <i64> [#uses=3]
  %w.tmp.037 = phi double [ %16, %bb.nph36 ], [ %20, %bb8 ] ; <double> [#uses=1]
  %scevgep173 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 %indvar165, i64 %tmp182 ; <double*> [#uses=1]
  %scevgep169 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 %tmp168, i64 %indvar165 ; <double*> [#uses=1]
  %17 = load double* %scevgep169, align 8         ; <double> [#uses=1]
  %18 = load double* %scevgep173, align 8         ; <double> [#uses=1]
  %19 = fmul double %17, %18                      ; <double> [#uses=1]
  %20 = fsub double %w.tmp.037, %19               ; <double> [#uses=3]
  %tmp174 = add i64 %indvar165, 1                 ; <i64> [#uses=2]
  %tmp175 = trunc i64 %tmp174 to i32              ; <i32> [#uses=1]
  %21 = icmp sgt i32 %tmp175, %i.061              ; <i1> [#uses=1]
  br i1 %21, label %bb10, label %bb8

bb10:                                             ; preds = %bb8
  store double %20, double* %scevgep186, align 8
  %indvar.next171 = add i64 %indvar170, 1         ; <i64> [#uses=2]
  %exitcond180 = icmp eq i64 %indvar.next171, %tmp179 ; <i1> [#uses=1]
  br i1 %exitcond180, label %bb13.loopexit, label %bb.nph36

bb11.loopexit:                                    ; preds = %bb5.preheader, %bb4, %bb4.us
  %w.tmp.071 = phi double [ %w.tmp.1, %bb5.preheader ], [ %9, %bb4.us ], [ %11, %bb4 ] ; <double> [#uses=1]
  %i.061 = phi i32 [ %i.049, %bb5.preheader ], [ %i.049, %bb4.us ], [ %i.051, %bb4 ] ; <i32> [#uses=5]
  %j.140 = add i32 %i.061, 1                      ; <i32> [#uses=5]
  %22 = icmp slt i32 %j.140, 1025                 ; <i1> [#uses=1]
  br i1 %22, label %bb.nph45, label %bb13.loopexit

bb13.loopexit:                                    ; preds = %bb11.loopexit, %bb10, %bb10.us
  %w.tmp.2 = phi double [ %w.tmp.071, %bb11.loopexit ], [ %15, %bb10.us ], [ %20, %bb10 ] ; <double> [#uses=2]
  %23 = icmp slt i32 %j.140, 1024                 ; <i1> [#uses=1]
  br i1 %23, label %bb5.preheader, label %bb.nph19

bb5.preheader:                                    ; preds = %bb13.loopexit, %bb.nph70
  %w.tmp.1 = phi double [ %w.promoted, %bb.nph70 ], [ %w.tmp.2, %bb13.loopexit ] ; <double> [#uses=1]
  %i.049 = phi i32 [ 0, %bb.nph70 ], [ %j.140, %bb13.loopexit ] ; <i32> [#uses=9]
  %j.027 = add i32 %i.049, 1                      ; <i32> [#uses=3]
  %24 = icmp slt i32 %j.027, 1025                 ; <i1> [#uses=1]
  br i1 %24, label %bb.nph32, label %bb11.loopexit

bb.nph19:                                         ; preds = %bb13.loopexit
  store double %w.tmp.2, double* @w
  store double 1.000000e+00, double* getelementptr inbounds ([1025 x double]* @y, i64 0, i64 0), align 32
  br label %bb.nph12

bb.nph12:                                         ; preds = %bb18, %bb.nph19
  %indvar99 = phi i64 [ 0, %bb.nph19 ], [ %tmp101, %bb18 ] ; <i64> [#uses=1]
  %tmp101 = add i64 %indvar99, 1                  ; <i64> [#uses=6]
  %scevgep109 = getelementptr [1025 x double]* @b, i64 0, i64 %tmp101 ; <double*> [#uses=1]
  %25 = load double* %scevgep109, align 8         ; <double> [#uses=1]
  br label %bb16

bb16:                                             ; preds = %bb16, %bb.nph12
  %indvar97 = phi i64 [ 0, %bb.nph12 ], [ %indvar.next98, %bb16 ] ; <i64> [#uses=3]
  %26 = phi double [ %25, %bb.nph12 ], [ %30, %bb16 ] ; <double> [#uses=1]
  %scevgep106 = getelementptr [1025 x double]* @y, i64 0, i64 %indvar97 ; <double*> [#uses=1]
  %tmp104 = mul i64 %tmp101, 1025                 ; <i64> [#uses=1]
  %scevgep103.sum = add i64 %indvar97, %tmp104    ; <i64> [#uses=1]
  %scevgep105 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 0, i64 %scevgep103.sum ; <double*> [#uses=1]
  %27 = load double* %scevgep105, align 8         ; <double> [#uses=1]
  %28 = load double* %scevgep106, align 8         ; <double> [#uses=1]
  %29 = fmul double %27, %28                      ; <double> [#uses=1]
  %30 = fsub double %26, %29                      ; <double> [#uses=3]
  %indvar.next98 = add i64 %indvar97, 1           ; <i64> [#uses=2]
  %exitcond102 = icmp eq i64 %indvar.next98, %tmp101 ; <i1> [#uses=1]
  br i1 %exitcond102, label %bb18, label %bb16

bb18:                                             ; preds = %bb16
  %31 = getelementptr inbounds [1025 x double]* @y, i64 0, i64 %tmp101 ; <double*> [#uses=1]
  store double %30, double* %31, align 8
  %exitcond107 = icmp eq i64 %tmp101, 1024        ; <i1> [#uses=1]
  br i1 %exitcond107, label %bb.nph6, label %bb.nph12

bb.nph6:                                          ; preds = %bb18
  store double %30, double* @w
  %32 = load double* getelementptr inbounds ([1025 x double]* @y, i64 0, i64 1024), align 32 ; <double> [#uses=1]
  %33 = load double* getelementptr inbounds ([1025 x [1025 x double]]* @a, i64 0, i64 1024, i64 1024), align 32 ; <double> [#uses=1]
  %34 = fdiv double %32, %33                      ; <double> [#uses=1]
  store double %34, double* getelementptr inbounds ([1025 x double]* @x, i64 0, i64 1024), align 32
  br label %bb.nph

bb.nph:                                           ; preds = %bb24, %bb.nph6
  %indvar72 = phi i64 [ 0, %bb.nph6 ], [ %tmp, %bb24 ] ; <i64> [#uses=4]
  %tmp83 = sub i64 1024, %indvar72                ; <i64> [#uses=1]
  %tmp86 = mul i64 %indvar72, -1026               ; <i64> [#uses=2]
  %tmp87 = add i64 %tmp86, 1024                   ; <i64> [#uses=1]
  %tmp = add i64 %indvar72, 1                     ; <i64> [#uses=3]
  %tmp91 = add i64 %tmp86, 1023                   ; <i64> [#uses=1]
  %scevgep92 = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 1023, i64 %tmp91 ; <double*> [#uses=1]
  %tmp93 = sub i64 1023, %indvar72                ; <i64> [#uses=2]
  %scevgep94 = getelementptr [1025 x double]* @x, i64 0, i64 %tmp93 ; <double*> [#uses=1]
  %scevgep95 = getelementptr [1025 x double]* @y, i64 0, i64 %tmp93 ; <double*> [#uses=1]
  %35 = load double* %scevgep95, align 8          ; <double> [#uses=1]
  br label %bb22

bb22:                                             ; preds = %bb22, %bb.nph
  %indvar = phi i64 [ 0, %bb.nph ], [ %indvar.next, %bb22 ] ; <i64> [#uses=3]
  %w.tmp.0 = phi double [ %35, %bb.nph ], [ %39, %bb22 ] ; <double> [#uses=1]
  %tmp84 = add i64 %tmp83, %indvar                ; <i64> [#uses=1]
  %scevgep80 = getelementptr [1025 x double]* @x, i64 0, i64 %tmp84 ; <double*> [#uses=1]
  %tmp88 = add i64 %tmp87, %indvar                ; <i64> [#uses=1]
  %scevgep = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 1023, i64 %tmp88 ; <double*> [#uses=1]
  %36 = load double* %scevgep, align 8            ; <double> [#uses=1]
  %37 = load double* %scevgep80, align 8          ; <double> [#uses=1]
  %38 = fmul double %36, %37                      ; <double> [#uses=1]
  %39 = fsub double %w.tmp.0, %38                 ; <double> [#uses=3]
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, %tmp      ; <i1> [#uses=1]
  br i1 %exitcond, label %bb24, label %bb22

bb24:                                             ; preds = %bb22
  %40 = load double* %scevgep92, align 8          ; <double> [#uses=1]
  %41 = fdiv double %39, %40                      ; <double> [#uses=1]
  store double %41, double* %scevgep94, align 8
  %exitcond81 = icmp eq i64 %tmp, 1024            ; <i1> [#uses=1]
  br i1 %exitcond81, label %return, label %bb.nph

return:                                           ; preds = %bb24
  store double %39, double* @w
  ret void
}

define i32 @main(i32 %argc, i8** nocapture %argv) nounwind {
entry:
  br label %bb.nph.us.i

bb3.us.i:                                         ; preds = %bb1.us.i
  %indvar.next8.i = add i64 %indvar7.i, 1         ; <i64> [#uses=2]
  %exitcond6 = icmp eq i64 %indvar.next8.i, 1025  ; <i1> [#uses=1]
  br i1 %exitcond6, label %init_array.exit, label %bb.nph.us.i

bb1.us.i:                                         ; preds = %bb.nph.us.i, %bb1.us.i
  %indvar.i = phi i64 [ 0, %bb.nph.us.i ], [ %indvar.next.i, %bb1.us.i ] ; <i64> [#uses=3]
  %scevgep.i = getelementptr [1025 x [1025 x double]]* @a, i64 0, i64 %indvar7.i, i64 %indvar.i ; <double*> [#uses=1]
  %j.01.us.i = trunc i64 %indvar.i to i32         ; <i32> [#uses=1]
  %0 = sitofp i32 %j.01.us.i to double            ; <double> [#uses=1]
  %1 = fmul double %4, %0                         ; <double> [#uses=1]
  %2 = fadd double %1, 1.000000e+00               ; <double> [#uses=1]
  %3 = fdiv double %2, 1.024000e+03               ; <double> [#uses=1]
  store double %3, double* %scevgep.i, align 8
  %indvar.next.i = add i64 %indvar.i, 1           ; <i64> [#uses=2]
  %exitcond5 = icmp eq i64 %indvar.next.i, 1025   ; <i1> [#uses=1]
  br i1 %exitcond5, label %bb3.us.i, label %bb1.us.i

bb.nph.us.i:                                      ; preds = %bb3.us.i, %entry
  %indvar7.i = phi i64 [ %indvar.next8.i, %bb3.us.i ], [ 0, %entry ] ; <i64> [#uses=5]
  %scevgep12.i = getelementptr [1025 x double]* @x, i64 0, i64 %indvar7.i ; <double*> [#uses=1]
  %scevgep11.i = getelementptr [1025 x double]* @b, i64 0, i64 %indvar7.i ; <double*> [#uses=1]
  %i.02.us.i = trunc i64 %indvar7.i to i32        ; <i32> [#uses=1]
  %4 = sitofp i32 %i.02.us.i to double            ; <double> [#uses=3]
  %5 = fadd double %4, 1.000000e+00               ; <double> [#uses=1]
  %6 = fdiv double %5, 1.024000e+03               ; <double> [#uses=1]
  store double %6, double* %scevgep12.i, align 8
  %7 = fadd double %4, 2.000000e+00               ; <double> [#uses=1]
  %8 = fdiv double %7, 1.024000e+03               ; <double> [#uses=1]
  store double %8, double* %scevgep11.i, align 8
  br label %bb1.us.i

init_array.exit:                                  ; preds = %bb3.us.i
  tail call void @scop_func() nounwind
  %9 = icmp sgt i32 %argc, 42                     ; <i1> [#uses=1]
  br i1 %9, label %bb.i, label %print_array.exit

bb.i:                                             ; preds = %init_array.exit
  %10 = load i8** %argv, align 1                  ; <i8*> [#uses=1]
  %11 = load i8* %10, align 1                     ; <i8> [#uses=1]
  %12 = icmp eq i8 %11, 0                         ; <i1> [#uses=1]
  br i1 %12, label %bb2.i, label %print_array.exit

bb2.i:                                            ; preds = %bb4.i, %bb.i
  %indvar.i1 = phi i64 [ %indvar.next.i3, %bb4.i ], [ 0, %bb.i ] ; <i64> [#uses=3]
  %i.01.i = trunc i64 %indvar.i1 to i32           ; <i32> [#uses=1]
  %scevgep.i2 = getelementptr [1025 x double]* @x, i64 0, i64 %indvar.i1 ; <double*> [#uses=1]
  %13 = load double* %scevgep.i2, align 8         ; <double> [#uses=1]
  %14 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %15 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %14, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %13) nounwind ; <i32> [#uses=0]
  %16 = srem i32 %i.01.i, 80                      ; <i32> [#uses=1]
  %17 = icmp eq i32 %16, 20                       ; <i1> [#uses=1]
  br i1 %17, label %bb3.i, label %bb4.i

bb3.i:                                            ; preds = %bb2.i
  %18 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %19 = bitcast %struct._IO_FILE* %18 to i8*      ; <i8*> [#uses=1]
  %20 = tail call i32 @fputc(i32 10, i8* %19) nounwind ; <i32> [#uses=0]
  br label %bb4.i

bb4.i:                                            ; preds = %bb3.i, %bb2.i
  %indvar.next.i3 = add i64 %indvar.i1, 1         ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next.i3, 1025   ; <i1> [#uses=1]
  br i1 %exitcond, label %bb6.i, label %bb2.i

bb6.i:                                            ; preds = %bb4.i
  %21 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %22 = bitcast %struct._IO_FILE* %21 to i8*      ; <i8*> [#uses=1]
  %23 = tail call i32 @fputc(i32 10, i8* %22) nounwind ; <i32> [#uses=0]
  ret i32 0

print_array.exit:                                 ; preds = %bb.i, %init_array.exit
  ret i32 0
}
