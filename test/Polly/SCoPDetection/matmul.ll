; RUN: opt -polly-scop-detection -analyze < %s | FileCheck %s
; ModuleID = 'matmul.c'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

@C = common global [2048 x [2061 x double]] zeroinitializer, align 8 ; <[2048 x [2061 x double]]*> [#uses=1]
@A = common global [2048 x [2061 x double]] zeroinitializer, align 8 ; <[2048 x [2061 x double]]*> [#uses=1]
@B = common global [2048 x [2061 x double]] zeroinitializer, align 8 ; <[2048 x [2061 x double]]*> [#uses=1]

define i32 @main() nounwind {
entry:
  %retval = alloca i32, align 4                   ; <i32*> [#uses=3]
  %i = alloca i32, align 4                        ; <i32*> [#uses=7]
  %j = alloca i32, align 4                        ; <i32*> [#uses=7]
  %k = alloca i32, align 4                        ; <i32*> [#uses=6]
  %s = alloca double, align 8                     ; <double*> [#uses=0]
  store i32 0, i32* %retval
  call void (...)* @foo()
  store i32 0, i32* %i
  br label %for.cond

for.cond:                                         ; preds = %for.inc44, %entry
  %tmp = load i32* %i                             ; <i32> [#uses=1]
  %cmp = icmp slt i32 %tmp, 2048                  ; <i1> [#uses=1]
  br i1 %cmp, label %for.body, label %for.end47

for.body:                                         ; preds = %for.cond
  store i32 0, i32* %j
  br label %for.cond1

for.cond1:                                        ; preds = %for.inc40, %for.body
  %tmp2 = load i32* %j                            ; <i32> [#uses=1]
  %cmp3 = icmp slt i32 %tmp2, 2048                ; <i1> [#uses=1]
  br i1 %cmp3, label %for.body4, label %for.end43

for.body4:                                        ; preds = %for.cond1
  store i32 0, i32* %k
  br label %for.cond5

for.cond5:                                        ; preds = %for.inc, %for.body4
  %tmp6 = load i32* %k                            ; <i32> [#uses=1]
  %cmp7 = icmp slt i32 %tmp6, 2048                ; <i1> [#uses=1]
  br i1 %cmp7, label %for.body8, label %for.end

for.body8:                                        ; preds = %for.cond5
  %tmp9 = load i32* %j                            ; <i32> [#uses=1]
  %tmp10 = load i32* %i                           ; <i32> [#uses=1]
  %idxprom = sext i32 %tmp10 to i64               ; <i64> [#uses=1]
  %arrayidx = getelementptr inbounds [2061 x double]* getelementptr inbounds ([2048 x [2061 x double]]* @C, i32 0, i32 0), i64 %idxprom ; <[2061 x double]*> [#uses=1]
  %arraydecay = getelementptr inbounds [2061 x double]* %arrayidx, i32 0, i32 0 ; <double*> [#uses=1]
  %idxprom11 = sext i32 %tmp9 to i64              ; <i64> [#uses=1]
  %arrayidx12 = getelementptr inbounds double* %arraydecay, i64 %idxprom11 ; <double*> [#uses=1]
  %tmp13 = load double* %arrayidx12               ; <double> [#uses=1]
  %mul = fmul double 1.000000e+00, %tmp13         ; <double> [#uses=1]
  %tmp14 = load i32* %k                           ; <i32> [#uses=1]
  %tmp15 = load i32* %i                           ; <i32> [#uses=1]
  %idxprom16 = sext i32 %tmp15 to i64             ; <i64> [#uses=1]
  %arrayidx17 = getelementptr inbounds [2061 x double]* getelementptr inbounds ([2048 x [2061 x double]]* @A, i32 0, i32 0), i64 %idxprom16 ; <[2061 x double]*> [#uses=1]
  %arraydecay18 = getelementptr inbounds [2061 x double]* %arrayidx17, i32 0, i32 0 ; <double*> [#uses=1]
  %idxprom19 = sext i32 %tmp14 to i64             ; <i64> [#uses=1]
  %arrayidx20 = getelementptr inbounds double* %arraydecay18, i64 %idxprom19 ; <double*> [#uses=1]
  %tmp21 = load double* %arrayidx20               ; <double> [#uses=1]
  %mul22 = fmul double 1.000000e+00, %tmp21       ; <double> [#uses=1]
  %tmp23 = load i32* %j                           ; <i32> [#uses=1]
  %tmp24 = load i32* %k                           ; <i32> [#uses=1]
  %idxprom25 = sext i32 %tmp24 to i64             ; <i64> [#uses=1]
  %arrayidx26 = getelementptr inbounds [2061 x double]* getelementptr inbounds ([2048 x [2061 x double]]* @B, i32 0, i32 0), i64 %idxprom25 ; <[2061 x double]*> [#uses=1]
  %arraydecay27 = getelementptr inbounds [2061 x double]* %arrayidx26, i32 0, i32 0 ; <double*> [#uses=1]
  %idxprom28 = sext i32 %tmp23 to i64             ; <i64> [#uses=1]
  %arrayidx29 = getelementptr inbounds double* %arraydecay27, i64 %idxprom28 ; <double*> [#uses=1]
  %tmp30 = load double* %arrayidx29               ; <double> [#uses=1]
  %mul31 = fmul double %mul22, %tmp30             ; <double> [#uses=1]
  %add = fadd double %mul, %mul31                 ; <double> [#uses=1]
  %tmp32 = load i32* %j                           ; <i32> [#uses=1]
  %tmp33 = load i32* %i                           ; <i32> [#uses=1]
  %idxprom34 = sext i32 %tmp33 to i64             ; <i64> [#uses=1]
  %arrayidx35 = getelementptr inbounds [2061 x double]* getelementptr inbounds ([2048 x [2061 x double]]* @C, i32 0, i32 0), i64 %idxprom34 ; <[2061 x double]*> [#uses=1]
  %arraydecay36 = getelementptr inbounds [2061 x double]* %arrayidx35, i32 0, i32 0 ; <double*> [#uses=1]
  %idxprom37 = sext i32 %tmp32 to i64             ; <i64> [#uses=1]
  %arrayidx38 = getelementptr inbounds double* %arraydecay36, i64 %idxprom37 ; <double*> [#uses=1]
  store double %add, double* %arrayidx38
  br label %for.inc

for.inc:                                          ; preds = %for.body8
  %tmp39 = load i32* %k                           ; <i32> [#uses=1]
  %inc = add nsw i32 %tmp39, 1                    ; <i32> [#uses=1]
  store i32 %inc, i32* %k
  br label %for.cond5

for.end:                                          ; preds = %for.cond5
  br label %for.inc40

for.inc40:                                        ; preds = %for.end
  %tmp41 = load i32* %j                           ; <i32> [#uses=1]
  %inc42 = add nsw i32 %tmp41, 1                  ; <i32> [#uses=1]
  store i32 %inc42, i32* %j
  br label %for.cond1

for.end43:                                        ; preds = %for.cond1
  br label %for.inc44

for.inc44:                                        ; preds = %for.end43
  %tmp45 = load i32* %i                           ; <i32> [#uses=1]
  %inc46 = add nsw i32 %tmp45, 1                  ; <i32> [#uses=1]
  store i32 %inc46, i32* %i
  br label %for.cond

for.end47:                                        ; preds = %for.cond
  call void (...)* @foo()
  store i32 0, i32* %retval
  %0 = load i32* %retval                          ; <i32> [#uses=1]
  ret i32 %0
}

declare void @foo(...)
