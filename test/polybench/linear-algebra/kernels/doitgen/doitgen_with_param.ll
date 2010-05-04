; RUN: opt -indvars -polly-scop-detect  -analyze %s | FileCheck %s
; XFAIL: *
; ModuleID = '<stdin>'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }

@A = common global [128 x [128 x [128 x double]]] zeroinitializer, align 32 ; <[128 x [128 x [128 x double]]]*> [#uses=8]
@C4 = common global [128 x [128 x double]] zeroinitializer, align 32 ; <[128 x [128 x double]]*> [#uses=4]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=6]
@.str = private constant [8 x i8] c"%0.2lf \00", align 1 ; <[8 x i8]*> [#uses=1]
@sum = common global [128 x [128 x [128 x double]]] zeroinitializer, align 32 ; <[128 x [128 x [128 x double]]]*> [#uses=4]

define void @init_array() nounwind inlinehint {
bb.nph29.bb.nph29.split_crit_edge:
  br label %bb.nph17.bb.nph17.split_crit_edge

bb.nph12:                                         ; preds = %bb.nph17.bb.nph17.split_crit_edge, %bb4
  %indvar39 = phi i64 [ 0, %bb.nph17.bb.nph17.split_crit_edge ], [ %indvar.next40, %bb4 ] ; <i64> [#uses=3]
  %storemerge313 = trunc i64 %indvar39 to i32     ; <i32> [#uses=1]
  %0 = sitofp i32 %storemerge313 to double        ; <double> [#uses=1]
  %1 = fmul double %5, %0                         ; <double> [#uses=1]
  br label %bb2

bb2:                                              ; preds = %bb2, %bb.nph12
  %indvar34 = phi i64 [ 0, %bb.nph12 ], [ %indvar.next35, %bb2 ] ; <i64> [#uses=3]
  %scevgep41 = getelementptr [128 x [128 x [128 x double]]]* @A, i64 0, i64 %indvar37, i64 %indvar39, i64 %indvar34 ; <double*> [#uses=1]
  %storemerge411 = trunc i64 %indvar34 to i32     ; <i32> [#uses=1]
  %2 = sitofp i32 %storemerge411 to double        ; <double> [#uses=1]
  %3 = fadd double %1, %2                         ; <double> [#uses=1]
  %4 = fdiv double %3, 1.280000e+02               ; <double> [#uses=1]
  store double %4, double* %scevgep41, align 8
  %indvar.next35 = add i64 %indvar34, 1           ; <i64> [#uses=2]
  %exitcond36 = icmp eq i64 %indvar.next35, 128   ; <i1> [#uses=1]
  br i1 %exitcond36, label %bb4, label %bb2

bb4:                                              ; preds = %bb2
  %indvar.next40 = add i64 %indvar39, 1           ; <i64> [#uses=2]
  %exitcond42 = icmp eq i64 %indvar.next40, 128   ; <i1> [#uses=1]
  br i1 %exitcond42, label %bb6, label %bb.nph12

bb.nph17.bb.nph17.split_crit_edge:                ; preds = %bb6, %bb.nph29.bb.nph29.split_crit_edge
  %indvar37 = phi i64 [ 0, %bb.nph29.bb.nph29.split_crit_edge ], [ %indvar.next38, %bb6 ] ; <i64> [#uses=3]
  %storemerge20 = trunc i64 %indvar37 to i32      ; <i32> [#uses=1]
  %5 = sitofp i32 %storemerge20 to double         ; <double> [#uses=1]
  br label %bb.nph12

bb6:                                              ; preds = %bb4
  %indvar.next38 = add i64 %indvar37, 1           ; <i64> [#uses=2]
  %exitcond44 = icmp eq i64 %indvar.next38, 128   ; <i1> [#uses=1]
  br i1 %exitcond44, label %bb.nph, label %bb.nph17.bb.nph17.split_crit_edge

bb.nph:                                           ; preds = %bb12, %bb6
  %indvar30 = phi i64 [ %indvar.next31, %bb12 ], [ 0, %bb6 ] ; <i64> [#uses=3]
  %storemerge16 = trunc i64 %indvar30 to i32      ; <i32> [#uses=1]
  %6 = sitofp i32 %storemerge16 to double         ; <double> [#uses=1]
  br label %bb10

bb10:                                             ; preds = %bb10, %bb.nph
  %indvar = phi i64 [ 0, %bb.nph ], [ %indvar.next, %bb10 ] ; <i64> [#uses=3]
  %scevgep = getelementptr [128 x [128 x double]]* @C4, i64 0, i64 %indvar30, i64 %indvar ; <double*> [#uses=1]
  %storemerge25 = trunc i64 %indvar to i32        ; <i32> [#uses=1]
  %7 = sitofp i32 %storemerge25 to double         ; <double> [#uses=1]
  %8 = fmul double %6, %7                         ; <double> [#uses=1]
  %9 = fdiv double %8, 1.280000e+02               ; <double> [#uses=1]
  store double %9, double* %scevgep, align 8
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 128       ; <i1> [#uses=1]
  br i1 %exitcond, label %bb12, label %bb10

bb12:                                             ; preds = %bb10
  %indvar.next31 = add i64 %indvar30, 1           ; <i64> [#uses=2]
  %exitcond32 = icmp eq i64 %indvar.next31, 128   ; <i1> [#uses=1]
  br i1 %exitcond32, label %return, label %bb.nph

return:                                           ; preds = %bb12
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
  br i1 %3, label %bb9.preheader, label %return

bb4:                                              ; preds = %bb7.preheader, %bb6
  %indvar = phi i64 [ %indvar.next, %bb6 ], [ 0, %bb7.preheader ] ; <i64> [#uses=3]
  %scevgep = getelementptr [128 x [128 x [128 x double]]]* @A, i64 0, i64 %indvar21, i64 %indvar23, i64 %indvar ; <double*> [#uses=1]
  %tmp40 = add i64 %tmp39, %indvar                ; <i64> [#uses=1]
  %tmp28 = trunc i64 %tmp40 to i32                ; <i32> [#uses=1]
  %4 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %5 = load %struct._IO_FILE** @stderr, align 8   ; <%struct._IO_FILE*> [#uses=1]
  %6 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %5, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %4) nounwind ; <i32> [#uses=0]
  %7 = srem i32 %tmp28, 80                        ; <i32> [#uses=1]
  %8 = icmp eq i32 %7, 20                         ; <i1> [#uses=1]
  br i1 %8, label %bb5, label %bb6

bb5:                                              ; preds = %bb4
  %9 = load %struct._IO_FILE** @stderr, align 8   ; <%struct._IO_FILE*> [#uses=1]
  %10 = bitcast %struct._IO_FILE* %9 to i8*       ; <i8*> [#uses=1]
  %11 = tail call i32 @fputc(i32 10, i8* %10) nounwind ; <i32> [#uses=0]
  br label %bb6

bb6:                                              ; preds = %bb5, %bb4
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next, 128       ; <i1> [#uses=1]
  br i1 %exitcond, label %bb8, label %bb4

bb8:                                              ; preds = %bb6
  %indvar.next24 = add i64 %indvar23, 1           ; <i64> [#uses=2]
  %exitcond29 = icmp eq i64 %indvar.next24, 128   ; <i1> [#uses=1]
  br i1 %exitcond29, label %bb10, label %bb7.preheader

bb7.preheader:                                    ; preds = %bb9.preheader, %bb8
  %indvar23 = phi i64 [ %indvar.next24, %bb8 ], [ 0, %bb9.preheader ] ; <i64> [#uses=3]
  %tmp = shl i64 %indvar23, 7                     ; <i64> [#uses=1]
  %tmp39 = add i64 %tmp38, %tmp                   ; <i64> [#uses=1]
  br label %bb4

bb10:                                             ; preds = %bb8
  %indvar.next22 = add i64 %indvar21, 1           ; <i64> [#uses=2]
  %exitcond36 = icmp eq i64 %indvar.next22, 128   ; <i1> [#uses=1]
  br i1 %exitcond36, label %bb12, label %bb9.preheader

bb9.preheader:                                    ; preds = %bb10, %bb
  %indvar21 = phi i64 [ %indvar.next22, %bb10 ], [ 0, %bb ] ; <i64> [#uses=3]
  %tmp38 = shl i64 %indvar21, 7                   ; <i64> [#uses=1]
  br label %bb7.preheader

bb12:                                             ; preds = %bb10
  %12 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %13 = bitcast %struct._IO_FILE* %12 to i8*      ; <i8*> [#uses=1]
  %14 = tail call i32 @fputc(i32 10, i8* %13) nounwind ; <i32> [#uses=0]
  ret void

return:                                           ; preds = %bb, %entry
  ret void
}

declare i32 @fprintf(%struct._IO_FILE* noalias nocapture, i8* noalias nocapture, ...) nounwind

declare i32 @fputc(i32, i8* nocapture) nounwind

define void @scop_func(i64 %nr, i64 %nq, i64 %np) nounwind {
entry:
  %0 = icmp sgt i64 %nr, 0                        ; <i1> [#uses=1]
  br i1 %0, label %bb.nph50, label %return

bb5.us:                                           ; preds = %bb3.us
  store double %5, double* %scevgep54
  %1 = add nsw i64 %storemerge26.us, 1            ; <i64> [#uses=2]
  %exitcond52 = icmp eq i64 %1, %np               ; <i1> [#uses=1]
  br i1 %exitcond52, label %bb9.loopexit, label %bb.nph.us

bb3.us:                                           ; preds = %bb.nph.us, %bb3.us
  %.tmp.0.us = phi double [ 0.000000e+00, %bb.nph.us ], [ %5, %bb3.us ] ; <double> [#uses=1]
  %storemerge45.us = phi i64 [ 0, %bb.nph.us ], [ %6, %bb3.us ] ; <i64> [#uses=3]
  %scevgep = getelementptr [128 x [128 x double]]* @C4, i64 0, i64 %storemerge45.us, i64 %storemerge26.us ; <double*> [#uses=1]
  %scevgep51 = getelementptr [128 x [128 x [128 x double]]]* @A, i64 0, i64 %storemerge43, i64 %storemerge113, i64 %storemerge45.us ; <double*> [#uses=1]
  %2 = load double* %scevgep51, align 8           ; <double> [#uses=1]
  %3 = load double* %scevgep, align 8             ; <double> [#uses=1]
  %4 = fmul double %2, %3                         ; <double> [#uses=1]
  %5 = fadd double %.tmp.0.us, %4                 ; <double> [#uses=2]
  %6 = add nsw i64 %storemerge45.us, 1            ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %6, %np                 ; <i1> [#uses=1]
  br i1 %exitcond, label %bb5.us, label %bb3.us

bb.nph.us:                                        ; preds = %bb6.preheader, %bb5.us
  %storemerge26.us = phi i64 [ %1, %bb5.us ], [ 0, %bb6.preheader ] ; <i64> [#uses=3]
  %scevgep54 = getelementptr [128 x [128 x [128 x double]]]* @sum, i64 0, i64 %storemerge43, i64 %storemerge113, i64 %storemerge26.us ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep54, align 8
  br label %bb3.us

bb8:                                              ; preds = %bb9.loopexit, %bb8
  %storemerge311 = phi i64 [ %8, %bb8 ], [ 0, %bb9.loopexit ] ; <i64> [#uses=3]
  %scevgep61 = getelementptr [128 x [128 x [128 x double]]]* @A, i64 0, i64 %storemerge43, i64 %storemerge113, i64 %storemerge311 ; <double*> [#uses=1]
  %scevgep62 = getelementptr [128 x [128 x [128 x double]]]* @sum, i64 0, i64 %storemerge43, i64 %storemerge113, i64 %storemerge311 ; <double*> [#uses=1]
  %7 = load double* %scevgep62, align 8           ; <double> [#uses=1]
  store double %7, double* %scevgep61, align 8
  %8 = add nsw i64 %storemerge311, 1              ; <i64> [#uses=2]
  %exitcond60 = icmp eq i64 %8, %np               ; <i1> [#uses=1]
  br i1 %exitcond60, label %bb10, label %bb8

bb9.loopexit:                                     ; preds = %bb5.us
  br i1 %14, label %bb8, label %bb10

bb10:                                             ; preds = %bb6.preheader, %bb9.loopexit, %bb8
  %storemerge12566 = phi i64 [ %storemerge113, %bb9.loopexit ], [ %storemerge113, %bb8 ], [ %storemerge113, %bb6.preheader ] ; <i64> [#uses=1]
  %storemerge4464 = phi i64 [ %storemerge43, %bb9.loopexit ], [ %storemerge43, %bb8 ], [ %storemerge43, %bb6.preheader ] ; <i64> [#uses=2]
  %9 = add nsw i64 %storemerge12566, 1            ; <i64> [#uses=2]
  %10 = icmp slt i64 %9, %nq                      ; <i1> [#uses=1]
  br i1 %10, label %bb6.preheader, label %bb12

bb6.preheader:                                    ; preds = %bb.nph50, %bb12, %bb10
  %storemerge43 = phi i64 [ %storemerge4464, %bb10 ], [ %11, %bb12 ], [ 0, %bb.nph50 ] ; <i64> [#uses=7]
  %storemerge113 = phi i64 [ %9, %bb10 ], [ 0, %bb.nph50 ], [ 0, %bb12 ] ; <i64> [#uses=7]
  br i1 %14, label %bb.nph.us, label %bb10

bb12:                                             ; preds = %bb10
  %11 = add nsw i64 %storemerge4464, 1            ; <i64> [#uses=2]
  %12 = icmp slt i64 %11, %nr                     ; <i1> [#uses=1]
  br i1 %12, label %bb6.preheader, label %return

bb.nph50:                                         ; preds = %entry
  %13 = icmp sgt i64 %nq, 0                       ; <i1> [#uses=1]
  %14 = icmp sgt i64 %np, 0                       ; <i1> [#uses=2]
  br i1 %13, label %bb6.preheader, label %return

return:                                           ; preds = %bb.nph50, %bb12, %entry
  ret void
}

define i32 @main(i32 %argc, i8** %argv) nounwind {
entry:
  br label %bb.nph17.bb.nph17.split_crit_edge.i

bb.nph12.i:                                       ; preds = %bb.nph17.bb.nph17.split_crit_edge.i, %bb4.i
  %indvar39.i = phi i64 [ 0, %bb.nph17.bb.nph17.split_crit_edge.i ], [ %indvar.next40.i, %bb4.i ] ; <i64> [#uses=3]
  %storemerge313.i = trunc i64 %indvar39.i to i32 ; <i32> [#uses=1]
  %0 = sitofp i32 %storemerge313.i to double      ; <double> [#uses=1]
  %1 = fmul double %5, %0                         ; <double> [#uses=1]
  br label %bb2.i

bb2.i:                                            ; preds = %bb2.i, %bb.nph12.i
  %indvar34.i = phi i64 [ 0, %bb.nph12.i ], [ %indvar.next35.i, %bb2.i ] ; <i64> [#uses=3]
  %scevgep41.i = getelementptr [128 x [128 x [128 x double]]]* @A, i64 0, i64 %indvar37.i, i64 %indvar39.i, i64 %indvar34.i ; <double*> [#uses=1]
  %storemerge411.i = trunc i64 %indvar34.i to i32 ; <i32> [#uses=1]
  %2 = sitofp i32 %storemerge411.i to double      ; <double> [#uses=1]
  %3 = fadd double %1, %2                         ; <double> [#uses=1]
  %4 = fdiv double %3, 1.280000e+02               ; <double> [#uses=1]
  store double %4, double* %scevgep41.i, align 8
  %indvar.next35.i = add i64 %indvar34.i, 1       ; <i64> [#uses=2]
  %exitcond40 = icmp eq i64 %indvar.next35.i, 128 ; <i1> [#uses=1]
  br i1 %exitcond40, label %bb4.i, label %bb2.i

bb4.i:                                            ; preds = %bb2.i
  %indvar.next40.i = add i64 %indvar39.i, 1       ; <i64> [#uses=2]
  %exitcond42 = icmp eq i64 %indvar.next40.i, 128 ; <i1> [#uses=1]
  br i1 %exitcond42, label %bb6.i, label %bb.nph12.i

bb.nph17.bb.nph17.split_crit_edge.i:              ; preds = %bb6.i, %entry
  %indvar37.i = phi i64 [ 0, %entry ], [ %indvar.next38.i, %bb6.i ] ; <i64> [#uses=3]
  %storemerge20.i = trunc i64 %indvar37.i to i32  ; <i32> [#uses=1]
  %5 = sitofp i32 %storemerge20.i to double       ; <double> [#uses=1]
  br label %bb.nph12.i

bb6.i:                                            ; preds = %bb4.i
  %indvar.next38.i = add i64 %indvar37.i, 1       ; <i64> [#uses=2]
  %exitcond44 = icmp eq i64 %indvar.next38.i, 128 ; <i1> [#uses=1]
  br i1 %exitcond44, label %bb.nph.i, label %bb.nph17.bb.nph17.split_crit_edge.i

bb.nph.i:                                         ; preds = %bb12.i, %bb6.i
  %indvar30.i = phi i64 [ %indvar.next31.i, %bb12.i ], [ 0, %bb6.i ] ; <i64> [#uses=3]
  %storemerge16.i = trunc i64 %indvar30.i to i32  ; <i32> [#uses=1]
  %6 = sitofp i32 %storemerge16.i to double       ; <double> [#uses=1]
  br label %bb10.i

bb10.i:                                           ; preds = %bb10.i, %bb.nph.i
  %indvar.i = phi i64 [ 0, %bb.nph.i ], [ %indvar.next.i, %bb10.i ] ; <i64> [#uses=3]
  %scevgep.i = getelementptr [128 x [128 x double]]* @C4, i64 0, i64 %indvar30.i, i64 %indvar.i ; <double*> [#uses=1]
  %storemerge25.i = trunc i64 %indvar.i to i32    ; <i32> [#uses=1]
  %7 = sitofp i32 %storemerge25.i to double       ; <double> [#uses=1]
  %8 = fmul double %6, %7                         ; <double> [#uses=1]
  %9 = fdiv double %8, 1.280000e+02               ; <double> [#uses=1]
  store double %9, double* %scevgep.i, align 8
  %indvar.next.i = add i64 %indvar.i, 1           ; <i64> [#uses=2]
  %exitcond36 = icmp eq i64 %indvar.next.i, 128   ; <i1> [#uses=1]
  br i1 %exitcond36, label %bb12.i, label %bb10.i

bb12.i:                                           ; preds = %bb10.i
  %indvar.next31.i = add i64 %indvar30.i, 1       ; <i64> [#uses=2]
  %exitcond38 = icmp eq i64 %indvar.next31.i, 128 ; <i1> [#uses=1]
  br i1 %exitcond38, label %bb6.preheader.i.outer, label %bb.nph.i

bb6.preheader.i.outer:                            ; preds = %bb12.i14, %bb12.i
  %storemerge43.i.ph = phi i64 [ %19, %bb12.i14 ], [ 0, %bb12.i ] ; <i64> [#uses=5]
  br label %bb6.preheader.i

bb5.us.i:                                         ; preds = %bb3.us.i
  store double %14, double* %scevgep54.i
  %10 = add nsw i64 %storemerge26.us.i, 1         ; <i64> [#uses=2]
  %exitcond32 = icmp eq i64 %10, 128              ; <i1> [#uses=1]
  br i1 %exitcond32, label %bb8.i12, label %bb.nph.us.i

bb3.us.i:                                         ; preds = %bb.nph.us.i, %bb3.us.i
  %.tmp.0.us.i = phi double [ 0.000000e+00, %bb.nph.us.i ], [ %14, %bb3.us.i ] ; <double> [#uses=1]
  %storemerge45.us.i = phi i64 [ 0, %bb.nph.us.i ], [ %15, %bb3.us.i ] ; <i64> [#uses=3]
  %scevgep51.i = getelementptr [128 x [128 x [128 x double]]]* @A, i64 0, i64 %storemerge43.i.ph, i64 %storemerge113.i, i64 %storemerge45.us.i ; <double*> [#uses=1]
  %scevgep.i10 = getelementptr [128 x [128 x double]]* @C4, i64 0, i64 %storemerge45.us.i, i64 %storemerge26.us.i ; <double*> [#uses=1]
  %11 = load double* %scevgep51.i, align 8        ; <double> [#uses=1]
  %12 = load double* %scevgep.i10, align 8        ; <double> [#uses=1]
  %13 = fmul double %11, %12                      ; <double> [#uses=1]
  %14 = fadd double %.tmp.0.us.i, %13             ; <double> [#uses=2]
  %15 = add nsw i64 %storemerge45.us.i, 1         ; <i64> [#uses=2]
  %exitcond31 = icmp eq i64 %15, 128              ; <i1> [#uses=1]
  br i1 %exitcond31, label %bb5.us.i, label %bb3.us.i

bb.nph.us.i:                                      ; preds = %bb6.preheader.i, %bb5.us.i
  %storemerge26.us.i = phi i64 [ %10, %bb5.us.i ], [ 0, %bb6.preheader.i ] ; <i64> [#uses=3]
  %scevgep54.i = getelementptr [128 x [128 x [128 x double]]]* @sum, i64 0, i64 %storemerge43.i.ph, i64 %storemerge113.i, i64 %storemerge26.us.i ; <double*> [#uses=2]
  store double 0.000000e+00, double* %scevgep54.i, align 8
  br label %bb3.us.i

bb8.i12:                                          ; preds = %bb8.i12, %bb5.us.i
  %storemerge311.i = phi i64 [ %17, %bb8.i12 ], [ 0, %bb5.us.i ] ; <i64> [#uses=3]
  %scevgep62.i = getelementptr [128 x [128 x [128 x double]]]* @sum, i64 0, i64 %storemerge43.i.ph, i64 %storemerge113.i, i64 %storemerge311.i ; <double*> [#uses=1]
  %scevgep61.i = getelementptr [128 x [128 x [128 x double]]]* @A, i64 0, i64 %storemerge43.i.ph, i64 %storemerge113.i, i64 %storemerge311.i ; <double*> [#uses=1]
  %16 = load double* %scevgep62.i, align 8        ; <double> [#uses=1]
  store double %16, double* %scevgep61.i, align 8
  %17 = add nsw i64 %storemerge311.i, 1           ; <i64> [#uses=2]
  %exitcond33 = icmp eq i64 %17, 128              ; <i1> [#uses=1]
  br i1 %exitcond33, label %bb10.i13, label %bb8.i12

bb10.i13:                                         ; preds = %bb8.i12
  %18 = add nsw i64 %storemerge113.i, 1           ; <i64> [#uses=2]
  %exitcond34 = icmp eq i64 %18, 128              ; <i1> [#uses=1]
  br i1 %exitcond34, label %bb12.i14, label %bb6.preheader.i

bb6.preheader.i:                                  ; preds = %bb10.i13, %bb6.preheader.i.outer
  %storemerge113.i = phi i64 [ %18, %bb10.i13 ], [ 0, %bb6.preheader.i.outer ] ; <i64> [#uses=5]
  br label %bb.nph.us.i

bb12.i14:                                         ; preds = %bb10.i13
  %19 = add nsw i64 %storemerge43.i.ph, 1         ; <i64> [#uses=2]
  %exitcond35 = icmp eq i64 %19, 128              ; <i1> [#uses=1]
  br i1 %exitcond35, label %scop_func.exit, label %bb6.preheader.i.outer

scop_func.exit:                                   ; preds = %bb12.i14
  %20 = icmp sgt i32 %argc, 42                    ; <i1> [#uses=1]
  br i1 %20, label %bb.i, label %print_array.exit

bb.i:                                             ; preds = %scop_func.exit
  %21 = load i8** %argv, align 1                  ; <i8*> [#uses=1]
  %22 = load i8* %21, align 1                     ; <i8> [#uses=1]
  %23 = icmp eq i8 %22, 0                         ; <i1> [#uses=1]
  br i1 %23, label %bb9.preheader.i, label %print_array.exit

bb4.i3:                                           ; preds = %bb7.preheader.i, %bb6.i6
  %indvar.i1 = phi i64 [ %indvar.next.i4, %bb6.i6 ], [ 0, %bb7.preheader.i ] ; <i64> [#uses=3]
  %tmp29 = add i64 %tmp28, %indvar.i1             ; <i64> [#uses=1]
  %tmp28.i = trunc i64 %tmp29 to i32              ; <i32> [#uses=1]
  %scevgep.i2 = getelementptr [128 x [128 x [128 x double]]]* @A, i64 0, i64 %indvar21.i, i64 %indvar23.i, i64 %indvar.i1 ; <double*> [#uses=1]
  %24 = load double* %scevgep.i2, align 8         ; <double> [#uses=1]
  %25 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %26 = tail call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* noalias %25, i8* noalias getelementptr inbounds ([8 x i8]* @.str, i64 0, i64 0), double %24) nounwind ; <i32> [#uses=0]
  %27 = srem i32 %tmp28.i, 80                     ; <i32> [#uses=1]
  %28 = icmp eq i32 %27, 20                       ; <i1> [#uses=1]
  br i1 %28, label %bb5.i, label %bb6.i6

bb5.i:                                            ; preds = %bb4.i3
  %29 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %30 = bitcast %struct._IO_FILE* %29 to i8*      ; <i8*> [#uses=1]
  %31 = tail call i32 @fputc(i32 10, i8* %30) nounwind ; <i32> [#uses=0]
  br label %bb6.i6

bb6.i6:                                           ; preds = %bb5.i, %bb4.i3
  %indvar.next.i4 = add i64 %indvar.i1, 1         ; <i64> [#uses=2]
  %exitcond = icmp eq i64 %indvar.next.i4, 128    ; <i1> [#uses=1]
  br i1 %exitcond, label %bb8.i, label %bb4.i3

bb8.i:                                            ; preds = %bb6.i6
  %indvar.next24.i = add i64 %indvar23.i, 1       ; <i64> [#uses=2]
  %exitcond20 = icmp eq i64 %indvar.next24.i, 128 ; <i1> [#uses=1]
  br i1 %exitcond20, label %bb10.i8, label %bb7.preheader.i

bb7.preheader.i:                                  ; preds = %bb9.preheader.i, %bb8.i
  %indvar23.i = phi i64 [ %indvar.next24.i, %bb8.i ], [ 0, %bb9.preheader.i ] ; <i64> [#uses=3]
  %tmp = shl i64 %indvar23.i, 7                   ; <i64> [#uses=1]
  %tmp28 = add i64 %tmp27, %tmp                   ; <i64> [#uses=1]
  br label %bb4.i3

bb10.i8:                                          ; preds = %bb8.i
  %indvar.next22.i = add i64 %indvar21.i, 1       ; <i64> [#uses=2]
  %exitcond26 = icmp eq i64 %indvar.next22.i, 128 ; <i1> [#uses=1]
  br i1 %exitcond26, label %bb12.i9, label %bb9.preheader.i

bb9.preheader.i:                                  ; preds = %bb10.i8, %bb.i
  %indvar21.i = phi i64 [ %indvar.next22.i, %bb10.i8 ], [ 0, %bb.i ] ; <i64> [#uses=3]
  %tmp27 = shl i64 %indvar21.i, 7                 ; <i64> [#uses=1]
  br label %bb7.preheader.i

bb12.i9:                                          ; preds = %bb10.i8
  %32 = load %struct._IO_FILE** @stderr, align 8  ; <%struct._IO_FILE*> [#uses=1]
  %33 = bitcast %struct._IO_FILE* %32 to i8*      ; <i8*> [#uses=1]
  %34 = tail call i32 @fputc(i32 10, i8* %33) nounwind ; <i32> [#uses=0]
  ret i32 0

print_array.exit:                                 ; preds = %bb.i, %scop_func.exit
  ret i32 0
}
