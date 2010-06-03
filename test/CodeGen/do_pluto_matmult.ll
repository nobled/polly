; RUN: opt -polly-print-scop -S -analyze < %s | FileCheck %s
; RUN: opt -polly-codegen < %s
; RUN: opt -polly-import -polly-import-dir=`dirname %s` -polly-print-scop -S -analyze < %s | FileCheck -check-prefix=IMPORT %s
; RUN: opt -polly-import -polly-import-dir=`dirname %s` -polly-print-scop < %s | lli | diff %s.result -

; ModuleID = 'do_pluto_matmult.s'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }

@A = common global [36 x [49 x double]] zeroinitializer, align 8 ; <[36 x [49 x double]]*> [#uses=3]
@B = common global [36 x [49 x double]] zeroinitializer, align 8 ; <[36 x [49 x double]]*> [#uses=3]
@C = common global [36 x [49 x double]] zeroinitializer, align 8 ; <[36 x [49 x double]]*> [#uses=4]
@stdout = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=3]
@.str = private constant [5 x i8] c"%lf \00"      ; <[5 x i8]*> [#uses=1]
@.str1 = private constant [2 x i8] c"\0A\00"      ; <[2 x i8]*> [#uses=1]

define void @init_array() nounwind {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc29, %entry
  %indvar1 = phi i64 [ %indvar.next2, %for.inc29 ], [ 0, %entry ] ; <i64> [#uses=7]
  %exitcond6 = icmp ne i64 %indvar1, 36           ; <i1> [#uses=1]
  br i1 %exitcond6, label %for.body, label %for.end32

for.body:                                         ; preds = %for.cond
  br label %for.cond1

for.cond1:                                        ; preds = %for.inc, %for.body
  %indvar = phi i64 [ %indvar.next, %for.inc ], [ 0, %for.body ] ; <i64> [#uses=7]
  %tmp7 = add i64 %indvar1, %indvar               ; <i64> [#uses=1]
  %add = trunc i64 %tmp7 to i32                   ; <i32> [#uses=1]
  %arrayidx10 = getelementptr [36 x [49 x double]]* @A, i64 0, i64 %indvar1, i64 %indvar ; <double*> [#uses=1]
  %tmp9 = mul i64 %indvar1, %indvar               ; <i64> [#uses=1]
  %mul = trunc i64 %tmp9 to i32                   ; <i32> [#uses=1]
  %arrayidx20 = getelementptr [36 x [49 x double]]* @B, i64 0, i64 %indvar1, i64 %indvar ; <double*> [#uses=1]
  %arrayidx27 = getelementptr [36 x [49 x double]]* @C, i64 0, i64 %indvar1, i64 %indvar ; <double*> [#uses=1]
  %exitcond = icmp ne i64 %indvar, 36             ; <i1> [#uses=1]
  br i1 %exitcond, label %for.body4, label %for.end

for.body4:                                        ; preds = %for.cond1
  %conv = sitofp i32 %add to double               ; <double> [#uses=1]
  store double %conv, double* %arrayidx10
  call void @llvm.memory.barrier(i1 true, i1 true, i1 true, i1 true, i1 false)
  %conv13 = sitofp i32 %mul to double             ; <double> [#uses=1]
  store double %conv13, double* %arrayidx20
  store double 0.000000e+00, double* %arrayidx27
  br label %for.inc

for.inc:                                          ; preds = %for.body4
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=1]
  br label %for.cond1

for.end:                                          ; preds = %for.cond1
  br label %for.inc29

for.inc29:                                        ; preds = %for.end
  %indvar.next2 = add i64 %indvar1, 1             ; <i64> [#uses=1]
  br label %for.cond

for.end32:                                        ; preds = %for.cond
  ret void
}

declare void @llvm.memory.barrier(i1, i1, i1, i1, i1) nounwind

define void @print_array() nounwind {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc18, %entry
  %indvar1 = phi i64 [ %indvar.next2, %for.inc18 ], [ 0, %entry ] ; <i64> [#uses=3]
  %exitcond3 = icmp ne i64 %indvar1, 36           ; <i1> [#uses=1]
  br i1 %exitcond3, label %for.body, label %for.end21

for.body:                                         ; preds = %for.cond
  br label %for.cond1

for.cond1:                                        ; preds = %for.inc, %for.body
  %indvar = phi i64 [ %indvar.next, %for.inc ], [ 0, %for.body ] ; <i64> [#uses=3]
  %j.0 = phi i32 [ 0, %for.body ], [ %inc, %for.inc ] ; <i32> [#uses=2]
  %arrayidx9 = getelementptr [36 x [49 x double]]* @C, i64 0, i64 %indvar1, i64 %indvar ; <double*> [#uses=1]
  %exitcond = icmp ne i64 %indvar, 36             ; <i1> [#uses=1]
  br i1 %exitcond, label %for.body4, label %for.end

for.body4:                                        ; preds = %for.cond1
  %tmp5 = load %struct._IO_FILE** @stdout         ; <%struct._IO_FILE*> [#uses=1]
  %tmp10 = load double* %arrayidx9                ; <double> [#uses=1]
  %call = call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* %tmp5, i8* getelementptr inbounds ([5 x i8]* @.str, i32 0, i32 0), double %tmp10) ; <i32> [#uses=0]
  %cmp12 = icmp eq i32 %j.0, 79                   ; <i1> [#uses=1]
  br i1 %cmp12, label %if.then, label %if.end

if.then:                                          ; preds = %for.body4
  %tmp13 = load %struct._IO_FILE** @stdout        ; <%struct._IO_FILE*> [#uses=1]
  %call14 = call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* %tmp13, i8* getelementptr inbounds ([2 x i8]* @.str1, i32 0, i32 0)) ; <i32> [#uses=0]
  br label %if.end

if.end:                                           ; preds = %if.then, %for.body4
  br label %for.inc

for.inc:                                          ; preds = %if.end
  %inc = add nsw i32 %j.0, 1                      ; <i32> [#uses=1]
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=1]
  br label %for.cond1

for.end:                                          ; preds = %for.cond1
  %tmp16 = load %struct._IO_FILE** @stdout        ; <%struct._IO_FILE*> [#uses=1]
  %call17 = call i32 (%struct._IO_FILE*, i8*, ...)* @fprintf(%struct._IO_FILE* %tmp16, i8* getelementptr inbounds ([2 x i8]* @.str1, i32 0, i32 0)) ; <i32> [#uses=0]
  br label %for.inc18

for.inc18:                                        ; preds = %for.end
  %indvar.next2 = add i64 %indvar1, 1             ; <i64> [#uses=1]
  br label %for.cond

for.end21:                                        ; preds = %for.cond
  ret void
}

declare i32 @fprintf(%struct._IO_FILE*, i8*, ...)

define void @do_pluto_matmult() nounwind {
entry:
  call void @llvm.memory.barrier(i1 true, i1 true, i1 true, i1 true, i1 false)
  br label %do.body

do.body:                                          ; preds = %do.cond42, %entry
  %indvar3 = phi i64 [ %indvar.next4, %do.cond42 ], [ 0, %entry ] ; <i64> [#uses=3]
  br label %do.body1

do.body1:                                         ; preds = %do.cond36, %do.body
  %indvar1 = phi i64 [ %indvar.next2, %do.cond36 ], [ 0, %do.body ] ; <i64> [#uses=3]
  %arrayidx5 = getelementptr [36 x [49 x double]]* @C, i64 0, i64 %indvar3, i64 %indvar1 ; <double*> [#uses=2]
  br label %do.body2

do.body2:                                         ; preds = %do.cond, %do.body1
  %indvar = phi i64 [ %indvar.next, %do.cond ], [ 0, %do.body1 ] ; <i64> [#uses=3]
  %arrayidx13 = getelementptr [36 x [49 x double]]* @A, i64 0, i64 %indvar3, i64 %indvar ; <double*> [#uses=1]
  %arrayidx22 = getelementptr [36 x [49 x double]]* @B, i64 0, i64 %indvar, i64 %indvar1 ; <double*> [#uses=1]
  %tmp6 = load double* %arrayidx5                 ; <double> [#uses=1]
  %mul = fmul double 1.000000e+00, %tmp6          ; <double> [#uses=1]
  %tmp14 = load double* %arrayidx13               ; <double> [#uses=1]
  %mul15 = fmul double 1.000000e+00, %tmp14       ; <double> [#uses=1]
  %tmp23 = load double* %arrayidx22               ; <double> [#uses=1]
  %mul24 = fmul double %mul15, %tmp23             ; <double> [#uses=1]
  %add = fadd double %mul, %mul24                 ; <double> [#uses=1]
  store double %add, double* %arrayidx5
  br label %do.cond

do.cond:                                          ; preds = %do.body2
  %indvar.next = add i64 %indvar, 1               ; <i64> [#uses=2]
  %exitcond = icmp ne i64 %indvar.next, 36        ; <i1> [#uses=1]
  br i1 %exitcond, label %do.body2, label %do.end

do.end:                                           ; preds = %do.cond
  br label %do.cond36

do.cond36:                                        ; preds = %do.end
  %indvar.next2 = add i64 %indvar1, 1             ; <i64> [#uses=2]
  %exitcond5 = icmp ne i64 %indvar.next2, 36      ; <i1> [#uses=1]
  br i1 %exitcond5, label %do.body1, label %do.end39

do.end39:                                         ; preds = %do.cond36
  br label %do.cond42

do.cond42:                                        ; preds = %do.end39
  %indvar.next4 = add i64 %indvar3, 1             ; <i64> [#uses=2]
  %exitcond6 = icmp ne i64 %indvar.next4, 36      ; <i1> [#uses=1]
  br i1 %exitcond6, label %do.body, label %do.end45

do.end45:                                         ; preds = %do.cond42
  call void @llvm.memory.barrier(i1 true, i1 true, i1 true, i1 true, i1 false)
  ret void
}

define i32 @main() nounwind {
entry:
  call void @init_array()
  call void @do_pluto_matmult()
  call void @print_array()
  ret i32 0
}

; CHECK: for (s1=0;s1<=35;s1++) {
; CHECK:     for (s3=0;s3<=35;s3++) {
; CHECK:           for (s5=0;s5<=35;s5++) {
; CHECK:                   S0(s1,s3,s5);
; CHECK:                       }
; CHECK:                         }
; CHECK: }


; IMPORT: for (s1=0;s1<=35;s1+=4) {
; IMPORT:   for (s2=max(0,s1);s2<=min(35,s1+3);s2++) {
; IMPORT:     for (s5=0;s5<=35;s5+=4) {
; IMPORT:       for (s6=max(0,s5);s6<=min(35,s5+3);s6++) {
; IMPORT:         for (s9=0;s9<=35;s9+=4) {
; IMPORT:           for (s10=max(0,s9);s10<=min(35,s9+3);s10++)
; IMPORT:           {
; IMPORT:             S0(s2,s6,s10);
; IMPORT:           }
; IMPORT:         }
; IMPORT:       }
; IMPORT:     }
; IMPORT:   }
; IMPORT: }

