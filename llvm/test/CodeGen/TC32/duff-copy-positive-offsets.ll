; RUN: llc -mtriple=tc32 -O2 -filetype=asm %s -o - | FileCheck %s

define void @duff_copy(ptr nocapture writeonly %dst, ptr nocapture readonly %src) {
entry:
  br label %loop

loop:                                             ; preds = %entry, %loop
  %d = phi ptr [ %dst, %entry ], [ %d.next, %loop ]
  %s = phi ptr [ %src, %entry ], [ %s.next, %loop ]
  %n = phi i32 [ 5, %entry ], [ %n.next, %loop ]

  %d14 = getelementptr inbounds nuw i8, ptr %d, i32 14
  %s14 = getelementptr inbounds nuw i8, ptr %s, i32 14
  %v14 = load i16, ptr %s14, align 2
  store i16 %v14, ptr %d14, align 2

  %d16 = getelementptr inbounds nuw i8, ptr %d, i32 16
  %s16 = getelementptr inbounds nuw i8, ptr %s, i32 16
  %v16 = load i16, ptr %s16, align 2
  store i16 %v16, ptr %d16, align 2

  %d18 = getelementptr inbounds nuw i8, ptr %d, i32 18
  %s18 = getelementptr inbounds nuw i8, ptr %s, i32 18
  %v18 = load i16, ptr %s18, align 2
  store i16 %v18, ptr %d18, align 2

  %d20 = getelementptr inbounds nuw i8, ptr %d, i32 20
  %s20 = getelementptr inbounds nuw i8, ptr %s, i32 20
  %v20 = load i16, ptr %s20, align 2
  store i16 %v20, ptr %d20, align 2

  %d22 = getelementptr inbounds nuw i8, ptr %d, i32 22
  %s22 = getelementptr inbounds nuw i8, ptr %s, i32 22
  %v22 = load i16, ptr %s22, align 2
  store i16 %v22, ptr %d22, align 2

  %d24 = getelementptr inbounds nuw i8, ptr %d, i32 24
  %s24 = getelementptr inbounds nuw i8, ptr %s, i32 24
  %v24 = load i16, ptr %s24, align 2
  store i16 %v24, ptr %d24, align 2

  %d26 = getelementptr inbounds nuw i8, ptr %d, i32 26
  %s26 = getelementptr inbounds nuw i8, ptr %s, i32 26
  %v26 = load i16, ptr %s26, align 2
  store i16 %v26, ptr %d26, align 2

  %d28 = getelementptr inbounds nuw i8, ptr %d, i32 28
  %s28 = getelementptr inbounds nuw i8, ptr %s, i32 28
  %v28 = load i16, ptr %s28, align 2
  store i16 %v28, ptr %d28, align 2

  %d.next = getelementptr inbounds nuw i8, ptr %d, i32 16
  %s.next = getelementptr inbounds nuw i8, ptr %s, i32 16
  %n.next = add nsw i32 %n, -1
  %cont = icmp samesign ugt i32 %n, 2
  br i1 %cont, label %loop, label %exit

exit:
  ret void
}

; CHECK-LABEL: duff_copy:
; CHECK: .LBB0_1:
; CHECK-NOT: tloadr	r{{[0-7]}}, [pc, #.Ltmp
; CHECK: tloadrh
; CHECK: #14]
; CHECK: tstorerh
; CHECK: #14]
; CHECK: tloadrh
; CHECK: #28]
; CHECK: tstorerh
; CHECK: #28]
; CHECK: tmovs r4, #16
; CHECK: tadds r2, r2, r4
; CHECK: tsub r3, r3, #1
; CHECK: tj .LBB0_1
