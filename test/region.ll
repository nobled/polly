; run "opt region.ll  -regions -analyze -debug"
; ModuleID = 'lnr.bc'
target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-f80:128:128-v64:64:64-v128:128:128-a0:0:64-f80:32:32-n8:16:32"
target triple = "i686-pc-win32"
;  e
; / \
;b0 b1
; \ /
;  r
define void @f0() nounwind {
e:
    %c1 = call i1 @rnd(i32 0)
    br i1 %c1, label %b0, label %b1
b0:
    br label %r
b1:
    br label %r
r:
    ret void
}

;  e
; / \
;b0  b1
;|\ /|
;b2  b3
; \ /
;  r
define void @f1() nounwind {
e:
    %c1 = call i1 @rnd(i32 0)
    br i1 %c1, label %b0, label %b1
b0:
    %c2 = call i1 @rnd(i32 1)
    br i1 %c2, label %b2, label %b3
b1:
    %c3 = call i1 @rnd(i32 2)
    br i1 %c3, label %b2, label %b3
b2:
    br label %r
b3:
    br label %r
r:
    ret void
}

;should e0->r be a region?
;  e0
; / \
;b0  b1
; \ /
;  e1
; / \
;b2  b3
; \ /
;  r
define void @f2() nounwind {
e0:
    %c1 = call i1 @rnd(i32 0)
    br i1 %c1, label %b0, label %b1
b0:
    br label %e1
b1:
    br label %e1
e1:
    %c2 = call i1 @rnd(i32 0)
    br i1 %c2, label %b2, label %b3
b2:
    br label %r
b3:
    br label %r
r:
    ret void
}

;  e0
; / \
;b0  b1
;|\ /|
;b2  b3
; \ /
;  e1
; / \
;b4  b5
; \ /
;  r
define void @f3() nounwind {
e0:
    %c1 = call i1 @rnd(i32 0)
    br i1 %c1, label %b0, label %b1
b0:
    %c2 = call i1 @rnd(i32 1)
    br i1 %c2, label %b2, label %b3
b1:
    %c3 = call i1 @rnd(i32 2)
    br i1 %c3, label %b2, label %b3
b2:
    br label %e1
b3:
    br label %e1
e1:
    %c4 = call i1 @rnd(i32 0)
    br i1 %c4, label %b4, label %b5
b4:
    br label %r
b5:
    br label %r
r:
    ret void
}

;  e
; / \
;b0  b1
;| \ |
;b2  b3
; \ /
;  r
define void @f4() nounwind {
e:
    %c1 = call i1 @rnd(i32 0)
    br i1 %c1, label %b0, label %b1
b0:
    %c2 = call i1 @rnd(i32 1)
    br i1 %c2, label %b2, label %b3
b1:
    br label %b3
b2:
    br label %r
b3:
    br label %r
r:
    ret void
}




;  e0
; / \
;b0  b1
;| \   \
;b2  b3 |
; \ /  /
;  r -
define void @f5() nounwind {
e:
    %c1 = call i1 @rnd(i32 0)
    br i1 %c1, label %b0, label %b1
b0:
    %c2 = call i1 @rnd(i32 1)
    br i1 %c2, label %b2, label %b3
b1:
    br label %r
b2:
    br label %r
b3:
    br label %r
r:
    ret void
}

;  e0
; / \
;b0  b1
;|   | \
;|   b3 b2 
; \ /  /
;  r -
define void @f6() nounwind {
e:
    %c1 = call i1 @rnd(i32 0)
    br i1 %c1, label %b0, label %b1
b0:
    br label %r
b1:
    %c2 = call i1 @rnd(i32 1)
    br i1 %c2, label %b2, label %b3
b2:
    br label %r
b3:
    br label %r
r:
    ret void
}


;b0
;|
;b1
;|
;b2
;|
;r
define void @f7() nounwind {
b0:
    br label %b1
b1:
    br label %b2
b2:
    br label %r
r:
    ret void
}

;     e0
;    /| 
;   / |/\
;  b0 b1 |
;   \ |\/
;    \|
;     r
define void @f8() nounwind {
e0:
    %c1 = call i1 @rnd(i32 0)
    br i1 %c1, label %b0, label %b1
b0:
    br label %r
b1:
    %c2 = call i1 @rnd(i32 0)
    br i1 %c2, label %r, label %b1
r:
    ret void
}

;     e0
;    /| 
;   / |/\
;  b0 b1 b2
;   \ |\/
;    \|
;     r
define void @f9() nounwind {
e0:
    %c1 = call i1 @rnd(i32 0)
    br i1 %c1, label %b0, label %b1
b0:
    br label %r
b1:
    %c2 = call i1 @rnd(i32 0)
    br i1 %c2, label %r, label %b2
b2:
    br label %b1
r:
    ret void
}

;     e0
;    /| 
;   / |/\
;  | b1  |
;  b0 |  b3
;  | b2  |
;   \ |\/
;    \|
;     r
define void @f10() nounwind {
e0:
    %c1 = call i1 @rnd(i32 0)
    br i1 %c1, label %b0, label %b1
b0:
    br label %r
b1:
    br label %b2
b2:
    %c2 = call i1 @rnd(i32 0)
    br i1 %c2, label %r, label %b3
b3:
    br label %b1
r:
    ret void
}

;     e0-
;    /|  |
;   / |/\|
;  | b1  |
;  b0 |  b3
;  | b2  |
;   \ |\/
;    \|
;     r
define void @f11() nounwind {
e0:
    %c1 = call i32 @rnd32(i32 0)
    switch i32 %c1, label %b0 [i32 1, label %b1
                                i32 2, label %b3]
b0:
    br label %r
b1:
    br label %b2
b2:
    %c2 = call i1 @rnd(i32 0)
    br i1 %c2, label %r, label %b3
b3:
    br label %b1
r:
    ret void
}

declare i1  @rnd(i32 %c) 
declare i32 @rnd32(i32 %c) 
