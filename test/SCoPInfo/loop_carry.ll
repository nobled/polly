; RUN: opt -O3 -indvars -polly-scop-detect -polly-print-temp-scop-in-detail -print-top-scop-only  -analyze %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-linux-gnu"

;long f(long a[], long n) {
;  long i, k;
;  k = 1;
;  for (i = 1; i < n; ++i) {
;   a[i] = k * a[i - 1];
;   k = a[i + 3] + a[2 * i];
;  }
;  return 0;
;}

define i64 @f(i64* nocapture %a, i64 %n) nounwind {
entry:
  %0 = icmp sgt i64 %n, 1                         ; <i1> [#uses=1]
  br i1 %0, label %bb.nph, label %bb2

bb.nph:                                           ; preds = %entry
  %tmp = add i64 %n, -1                           ; <i64> [#uses=1]
  %.pre = load i64* %a, align 8                   ; <i64> [#uses=1]
  br label %bb

bb:                                               ; preds = %bb, %bb.nph
  %1 = phi i64 [ %.pre, %bb.nph ], [ %2, %bb ]    ; <i64> [#uses=1]
  %indvar = phi i64 [ 0, %bb.nph ], [ %tmp6, %bb ] ; <i64> [#uses=3]
  %k.05 = phi i64 [ 1, %bb.nph ], [ %5, %bb ]     ; <i64> [#uses=1]
  %tmp6 = add i64 %indvar, 1                      ; <i64> [#uses=3]
  %scevgep = getelementptr i64* %a, i64 %tmp6     ; <i64*> [#uses=1]
  %2 = mul nsw i64 %1, %k.05                      ; <i64> [#uses=2]
  store i64 %2, i64* %scevgep, align 8
  %tmp7 = shl i64 %indvar, 1                      ; <i64> [#uses=1]
  %tmp11 = add i64 %indvar, 4                     ; <i64> [#uses=1]
  %tmp8 = add i64 %tmp7, 2                        ; <i64> [#uses=1]
  %scevgep12 = getelementptr i64* %a, i64 %tmp11  ; <i64*> [#uses=1]
  %scevgep9 = getelementptr i64* %a, i64 %tmp8    ; <i64*> [#uses=1]
  %3 = load i64* %scevgep9, align 8               ; <i64> [#uses=1]
  %4 = load i64* %scevgep12, align 8              ; <i64> [#uses=1]
  %5 = add nsw i64 %3, %4                         ; <i64> [#uses=1]
  %exitcond = icmp eq i64 %tmp6, %tmp             ; <i1> [#uses=1]
  br i1 %exitcond, label %bb2, label %bb

bb2:                                              ; preds = %bb, %entry
  ret i64 0
}

; CHECK: SCoP: entry => <Function Return>        Parameters: (%n, ), Max Loop Depth: 1
; CHECK: BB: bb.nph{
; CHECK: Reads %a[0]
; CHECK: Writes %.pre[]
; CHECK: }
; CHECK: Bounds of Loop: bb:     { 0, 1 * %n + -2}
; CHECK:   BB: bb{
; CHECK:   Writes %a[8 * {0,+,1}<%bb> + 8]
; CHECK:   Reads %a[16 * {0,+,1}<%bb> + 16]
; CHECK:   Reads %a[8 * {0,+,1}<%bb> + 32]
; CHECK:   Reads %.pre[]
; CHECK:   Reads %2[]
; CHECK:   Reads %5[]
; CHECK:   Writes %2[]
; CHECK:   Writes %5[]
; CHECK:   }


