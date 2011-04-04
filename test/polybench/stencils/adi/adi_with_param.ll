; RUN: opt %loadPolly %polybenchOpts %defaultOpts -polly-analyze-ir  -print-top-scop-only -analyze %s | FileCheck %s
; Non Constant and non ICmpInst instruction
; XFAIL: *
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }
@X = common global [1024 x [1024 x double]] zeroinitializer, align 32 ; <[1024 x [1024 x double]]*> [#uses=12]
@A = common global [1024 x [1024 x double]] zeroinitializer, align 32 ; <[1024 x [1024 x double]]*> [#uses=8]
@B = common global [1024 x [1024 x double]] zeroinitializer, align 32 ; <[1024 x [1024 x double]]*> [#uses=10]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
define void @scop_func(i64 %n) nounwind {
bb.nph81:
  %0 = icmp sgt i64 %n, 0                         ; <i1> [#uses=6]
  %1 = icmp sgt i64 %n, 1                         ; <i1> [#uses=2]
  %2 = add nsw i64 %n, -2                         ; <i64> [#uses=5]
  %3 = icmp sgt i64 %2, 0                         ; <i1> [#uses=2]
  %4 = add nsw i64 %n, -3                         ; <i64> [#uses=2]
  %tmp = add i64 %n, -1                           ; <i64> [#uses=6]
  br label %bb5.preheader

bb.nph:                                           ; preds = %bb.nph16, %bb4
  %storemerge112 = phi i64 [ %16, %bb4 ], [ 0, %bb.nph16 ] ; <i64> [#uses=6]
  %scevgep85.phi.trans.insert = getelementptr [1024 x [1024 x double]]* @X, i64 0, i64 %storemerge112, i64 0 ; <double*> [#uses=1]
  %.pre = load double* %scevgep85.phi.trans.insert, align 32 ; <double> [#uses=1]
  %scevgep86.phi.trans.insert = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %storemerge112, i64 0 ; <double*> [#uses=1]
  %.pre149 = load double* %scevgep86.phi.trans.insert, align 32 ; <double> [#uses=1]
  br label %bb2

bb2:                                              ; preds = %bb2, %bb.nph
  %5 = phi double [ %.pre149, %bb.nph ], [ %15, %bb2 ] ; <double> [#uses=2]
  %6 = phi double [ %.pre, %bb.nph ], [ %11, %bb2 ] ; <double> [#uses=1]
  %indvar = phi i64 [ 0, %bb.nph ], [ %tmp90, %bb2 ] ; <i64> [#uses=1]
  %tmp90 = add i64 %indvar, 1                     ; <i64> [#uses=5]
  %scevgep84 = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %storemerge112, i64 %tmp90 ; <double*> [#uses=2]
  %scevgep83 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %storemerge112, i64 %tmp90 ; <double*> [#uses=1]
  %scevgep = getelementptr [1024 x [1024 x double]]* @X, i64 0, i64 %storemerge112, i64 %tmp90 ; <double*> [#uses=2]
  %7 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %8 = load double* %scevgep83, align 8           ; <double> [#uses=3]
  %9 = fmul double %6, %8                         ; <double> [#uses=1]
  %10 = fdiv double %9, %5                        ; <double> [#uses=1]
  %11 = fsub double %7, %10                       ; <double> [#uses=2]
  store double %11, double* %scevgep, align 8
  %12 = load double* %scevgep84, align 8          ; <double> [#uses=1]
  %13 = fmul double %8, %8                        ; <double> [#uses=1]
  %14 = fdiv double %13, %5                       ; <double> [#uses=1]
  %15 = fsub double %12, %14                      ; <double> [#uses=2]
  store double %15, double* %scevgep84, align 8
  %exitcond = icmp eq i64 %tmp90, %tmp            ; <i1> [#uses=1]
  br i1 %exitcond, label %bb4, label %bb2

bb4:                                              ; preds = %bb2
  %16 = add nsw i64 %storemerge112, 1             ; <i64> [#uses=2]
  %exitcond87 = icmp eq i64 %16, %n               ; <i1> [#uses=1]
  br i1 %exitcond87, label %bb8.loopexit, label %bb.nph

bb.nph16:                                         ; preds = %bb5.preheader
  br i1 %1, label %bb.nph, label %bb8.loopexit

bb7:                                              ; preds = %bb8.loopexit, %bb7
  %storemerge217 = phi i64 [ %20, %bb7 ], [ 0, %bb8.loopexit ] ; <i64> [#uses=3]
  %scevgep95 = getelementptr [1024 x [1024 x double]]* @X, i64 0, i64 %storemerge217, i64 %tmp ; <double*> [#uses=2]
  %scevgep96 = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %storemerge217, i64 %tmp ; <double*> [#uses=1]
  %17 = load double* %scevgep95, align 8          ; <double> [#uses=1]
  %18 = load double* %scevgep96, align 8          ; <double> [#uses=1]
  %19 = fdiv double %17, %18                      ; <double> [#uses=1]
  store double %19, double* %scevgep95, align 8
  %20 = add nsw i64 %storemerge217, 1             ; <i64> [#uses=2]
  %exitcond94 = icmp eq i64 %20, %n               ; <i1> [#uses=1]
  br i1 %exitcond94, label %bb14.loopexit, label %bb7

bb8.loopexit:                                     ; preds = %bb.nph16, %bb4
  br i1 %0, label %bb7, label %bb20.loopexit

bb11:                                             ; preds = %bb12.preheader, %bb11
  %storemerge920 = phi i64 [ %28, %bb11 ], [ 0, %bb12.preheader ] ; <i64> [#uses=3]
  %tmp107 = sub i64 %4, %storemerge920            ; <i64> [#uses=3]
  %scevgep104 = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %storemerge323, i64 %tmp107 ; <double*> [#uses=1]
  %scevgep103 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %storemerge323, i64 %tmp107 ; <double*> [#uses=1]
  %scevgep102 = getelementptr [1024 x [1024 x double]]* @X, i64 0, i64 %storemerge323, i64 %tmp107 ; <double*> [#uses=1]
  %tmp111 = sub i64 %2, %storemerge920            ; <i64> [#uses=1]
  %scevgep100 = getelementptr [1024 x [1024 x double]]* @X, i64 0, i64 %storemerge323, i64 %tmp111 ; <double*> [#uses=2]
  %21 = load double* %scevgep100, align 8         ; <double> [#uses=1]
  %22 = load double* %scevgep102, align 8         ; <double> [#uses=1]
  %23 = load double* %scevgep103, align 8         ; <double> [#uses=1]
  %24 = fmul double %22, %23                      ; <double> [#uses=1]
  %25 = fsub double %21, %24                      ; <double> [#uses=1]
  %26 = load double* %scevgep104, align 8         ; <double> [#uses=1]
  %27 = fdiv double %25, %26                      ; <double> [#uses=1]
  store double %27, double* %scevgep100, align 8
  %28 = add nsw i64 %storemerge920, 1             ; <i64> [#uses=2]
  %exitcond97 = icmp eq i64 %28, %2               ; <i1> [#uses=1]
  br i1 %exitcond97, label %bb13, label %bb11

bb13:                                             ; preds = %bb11
  %29 = add nsw i64 %storemerge323, 1             ; <i64> [#uses=2]
  %exitcond105 = icmp eq i64 %29, %n              ; <i1> [#uses=1]
  br i1 %exitcond105, label %bb20.loopexit, label %bb12.preheader

bb14.loopexit:                                    ; preds = %bb7
  %.not = xor i1 %0, true                         ; <i1> [#uses=1]
  %.not150 = xor i1 %3, true                      ; <i1> [#uses=1]
  %brmerge = or i1 %.not, %.not150                ; <i1> [#uses=1]
  br i1 %brmerge, label %bb20.loopexit, label %bb12.preheader

bb12.preheader:                                   ; preds = %bb14.loopexit, %bb13
  %storemerge323 = phi i64 [ %29, %bb13 ], [ 0, %bb14.loopexit ] ; <i64> [#uses=5]
  br label %bb11

bb17:                                             ; preds = %bb18.preheader, %bb17
  %storemerge828 = phi i64 [ %41, %bb17 ], [ 0, %bb18.preheader ] ; <i64> [#uses=6]
  %scevgep119 = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %indvar114, i64 %storemerge828 ; <double*> [#uses=1]
  %scevgep118 = getelementptr [1024 x [1024 x double]]* @X, i64 0, i64 %indvar114, i64 %storemerge828 ; <double*> [#uses=1]
  %scevgep121 = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %tmp125, i64 %storemerge828 ; <double*> [#uses=2]
  %scevgep120 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp125, i64 %storemerge828 ; <double*> [#uses=1]
  %scevgep117 = getelementptr [1024 x [1024 x double]]* @X, i64 0, i64 %tmp125, i64 %storemerge828 ; <double*> [#uses=2]
  %30 = load double* %scevgep117, align 8         ; <double> [#uses=1]
  %31 = load double* %scevgep118, align 8         ; <double> [#uses=1]
  %32 = load double* %scevgep120, align 8         ; <double> [#uses=3]
  %33 = fmul double %31, %32                      ; <double> [#uses=1]
  %34 = load double* %scevgep119, align 8         ; <double> [#uses=2]
  %35 = fdiv double %33, %34                      ; <double> [#uses=1]
  %36 = fsub double %30, %35                      ; <double> [#uses=1]
  store double %36, double* %scevgep117, align 8
  %37 = load double* %scevgep121, align 8         ; <double> [#uses=1]
  %38 = fmul double %32, %32                      ; <double> [#uses=1]
  %39 = fdiv double %38, %34                      ; <double> [#uses=1]
  %40 = fsub double %37, %39                      ; <double> [#uses=1]
  store double %40, double* %scevgep121, align 8
  %41 = add nsw i64 %storemerge828, 1             ; <i64> [#uses=2]
  %exitcond113 = icmp eq i64 %41, %n              ; <i1> [#uses=1]
  br i1 %exitcond113, label %bb19, label %bb17

bb19:                                             ; preds = %bb17
  %exitcond122 = icmp eq i64 %tmp125, %tmp        ; <i1> [#uses=1]
  br i1 %exitcond122, label %bb23.loopexit, label %bb18.preheader

bb20.loopexit:                                    ; preds = %bb5.preheader, %bb14.loopexit, %bb13, %bb8.loopexit
  br i1 %1, label %bb.nph34, label %bb23.loopexit

bb.nph34:                                         ; preds = %bb20.loopexit
  br i1 %0, label %bb18.preheader, label %bb29.loopexit

bb18.preheader:                                   ; preds = %bb.nph34, %bb19
  %indvar114 = phi i64 [ %tmp125, %bb19 ], [ 0, %bb.nph34 ] ; <i64> [#uses=3]
  %tmp125 = add i64 %indvar114, 1                 ; <i64> [#uses=5]
  br label %bb17

bb22:                                             ; preds = %bb23.loopexit, %bb22
  %storemerge535 = phi i64 [ %45, %bb22 ], [ 0, %bb23.loopexit ] ; <i64> [#uses=3]
  %scevgep130 = getelementptr [1024 x [1024 x double]]* @X, i64 0, i64 %tmp, i64 %storemerge535 ; <double*> [#uses=2]
  %scevgep131 = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %tmp, i64 %storemerge535 ; <double*> [#uses=1]
  %42 = load double* %scevgep130, align 8         ; <double> [#uses=1]
  %43 = load double* %scevgep131, align 8         ; <double> [#uses=1]
  %44 = fdiv double %42, %43                      ; <double> [#uses=1]
  store double %44, double* %scevgep130, align 8
  %45 = add nsw i64 %storemerge535, 1             ; <i64> [#uses=2]
  %exitcond129 = icmp eq i64 %45, %n              ; <i1> [#uses=1]
  br i1 %exitcond129, label %bb29.loopexit, label %bb22

bb23.loopexit:                                    ; preds = %bb20.loopexit, %bb19
  br i1 %0, label %bb22, label %bb29.loopexit

bb26:                                             ; preds = %bb27.preheader, %bb26
  %storemerge737 = phi i64 [ %53, %bb26 ], [ 0, %bb27.preheader ] ; <i64> [#uses=5]
  %scevgep138 = getelementptr [1024 x [1024 x double]]* @A, i64 0, i64 %tmp142, i64 %storemerge737 ; <double*> [#uses=1]
  %scevgep137 = getelementptr [1024 x [1024 x double]]* @X, i64 0, i64 %tmp142, i64 %storemerge737 ; <double*> [#uses=1]
  %scevgep139 = getelementptr [1024 x [1024 x double]]* @B, i64 0, i64 %tmp145, i64 %storemerge737 ; <double*> [#uses=1]
  %scevgep135 = getelementptr [1024 x [1024 x double]]* @X, i64 0, i64 %tmp145, i64 %storemerge737 ; <double*> [#uses=2]
  %46 = load double* %scevgep135, align 8         ; <double> [#uses=1]
  %47 = load double* %scevgep137, align 8         ; <double> [#uses=1]
  %48 = load double* %scevgep138, align 8         ; <double> [#uses=1]
  %49 = fmul double %47, %48                      ; <double> [#uses=1]
  %50 = fsub double %46, %49                      ; <double> [#uses=1]
  %51 = load double* %scevgep139, align 8         ; <double> [#uses=1]
  %52 = fdiv double %50, %51                      ; <double> [#uses=1]
  store double %52, double* %scevgep135, align 8
  %53 = add nsw i64 %storemerge737, 1             ; <i64> [#uses=2]
  %exitcond132 = icmp eq i64 %53, %n              ; <i1> [#uses=1]
  br i1 %exitcond132, label %bb28, label %bb26

bb28:                                             ; preds = %bb26
  %54 = add nsw i64 %storemerge640, 1             ; <i64> [#uses=2]
  %exitcond140 = icmp eq i64 %54, %2              ; <i1> [#uses=1]
  br i1 %exitcond140, label %bb30, label %bb27.preheader

bb29.loopexit:                                    ; preds = %bb23.loopexit, %bb22, %bb.nph34
  %.not151 = xor i1 %3, true                      ; <i1> [#uses=1]
  %.not152 = xor i1 %0, true                      ; <i1> [#uses=1]
  %brmerge153 = or i1 %.not151, %.not152          ; <i1> [#uses=1]
  br i1 %brmerge153, label %bb30, label %bb27.preheader

bb27.preheader:                                   ; preds = %bb29.loopexit, %bb28
  %storemerge640 = phi i64 [ %54, %bb28 ], [ 0, %bb29.loopexit ] ; <i64> [#uses=3]
  %tmp142 = sub i64 %4, %storemerge640            ; <i64> [#uses=2]
  %tmp145 = sub i64 %2, %storemerge640            ; <i64> [#uses=2]
  br label %bb26

bb30:                                             ; preds = %bb29.loopexit, %bb28
  %55 = add nsw i64 %storemerge46, 1              ; <i64> [#uses=2]
  %exitcond148 = icmp eq i64 %55, 10              ; <i1> [#uses=1]
  br i1 %exitcond148, label %return, label %bb5.preheader

bb5.preheader:                                    ; preds = %bb30, %bb.nph81
  %storemerge46 = phi i64 [ 0, %bb.nph81 ], [ %55, %bb30 ] ; <i64> [#uses=1]
  br i1 %0, label %bb.nph16, label %bb20.loopexit

return:                                           ; preds = %bb30
  ret void
}
