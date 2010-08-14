; RUN: opt -polly-analyze-ir  -analyze %s | not FileCheck %s
; ModuleID = 'bugpoint-reduced-simplified.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

declare i16 @llvm.bswap.i16(i16) nounwind readnone

define fastcc void @cli_vba_scandir() nounwind {
entry:
  br i1 undef, label %bb.i1, label %cli_dbgmsg.exit.i

bb.i1:                                            ; preds = %entry
  unreachable

cli_dbgmsg.exit.i:                                ; preds = %entry
  br i1 undef, label %bb.i2, label %bb4.i

bb.i2:                                            ; preds = %cli_dbgmsg.exit.i
  br i1 undef, label %bb.i4.i, label %bb18

bb.i4.i:                                          ; preds = %bb.i2
  unreachable

bb4.i:                                            ; preds = %cli_dbgmsg.exit.i
  br i1 undef, label %bb7.i, label %bb6.i

bb6.i:                                            ; preds = %bb4.i
  br label %bb18

bb7.i:                                            ; preds = %bb4.i
  br i1 undef, label %bb12.preheader.i, label %bb8.i

bb12.preheader.i:                                 ; preds = %bb7.i
  br i1 undef, label %bb10.i, label %bb14.i

bb8.i:                                            ; preds = %bb7.i
  br label %bb18

bb10.i:                                           ; preds = %bb12.preheader.i
  unreachable

bb14.i:                                           ; preds = %bb12.preheader.i
  switch i32 undef, label %bb17.i [
    i32 1, label %bb15.i
    i32 14, label %bb16.i
  ]

bb15.i:                                           ; preds = %bb14.i
  br i1 undef, label %bb3.i.i, label %bb20.i

bb16.i:                                           ; preds = %bb14.i
  unreachable

bb17.i:                                           ; preds = %bb14.i
  unreachable

bb3.i.i:                                          ; preds = %bb15.i
  %0 = load i16* undef, align 2                   ; <i16> [#uses=1]
  br i1 false, label %vba_endian_convert_16.exit.i.i, label %bb.i.i.i

bb.i.i.i:                                         ; preds = %bb3.i.i
  %1 = call i16 @llvm.bswap.i16(i16 %0) nounwind  ; <i16> [#uses=1]
  store i16 %1, i16* undef
  br label %vba_endian_convert_16.exit.i.i

vba_endian_convert_16.exit.i.i:                   ; preds = %bb.i.i.i, %bb3.i.i
  unreachable

bb20.i:                                           ; preds = %bb15.i
  br label %bb18

bb18:                                             ; preds = %bb20.i, %bb8.i, %bb6.i, %bb.i2
  unreachable
}

; CHECK: SCoP
