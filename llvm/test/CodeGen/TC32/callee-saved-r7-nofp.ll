; RUN: llc -mtriple=tc32 -O2 < %s | FileCheck %s
; RUN: llc -mtriple=tc32 -O2 -filetype=obj -o /dev/null %s

define ptr @mempool_init_like(ptr %pool, ptr %mem, i32 %itemsize, i32 %itemcount) local_unnamed_addr {
entry:
  %pool.nonnull = icmp ne ptr %pool, null
  %mem.nonnull = icmp ne ptr %mem, null
  %ok = and i1 %pool.nonnull, %mem.nonnull
  br i1 %ok, label %init, label %ret

init:
  store ptr %mem, ptr %pool, align 1
  %size.plus = add nsw i32 %itemsize, 3
  %block.size = and i32 %size.plus, -4
  %have.loop = icmp sgt i32 %itemcount, 1
  br i1 %have.loop, label %loop.preheader, label %tail

loop.preheader:
  %last = add nsw i32 %itemcount, -2
  br label %loop

loop:
  %i = phi i32 [ %next.i, %loop ], [ 0, %loop.preheader ]
  %cur = phi ptr [ %next.ptr, %loop ], [ %mem, %loop.preheader ]
  %cur.int = ptrtoint ptr %cur to i32
  %next.int = add i32 %cur.int, %block.size
  %next.ptr = inttoptr i32 %next.int to ptr
  store ptr %next.ptr, ptr %cur, align 1
  %next.i = add nuw nsw i32 %i, 1
  %done = icmp eq i32 %i, %last
  br i1 %done, label %tail, label %loop

tail:
  %tail.ptr = phi ptr [ %mem, %init ], [ %next.ptr, %loop ]
  store ptr null, ptr %tail.ptr, align 1
  br label %ret

ret:
  %result = phi ptr [ %pool, %tail ], [ null, %entry ]
  ret ptr %result
}

; CHECK-LABEL: mempool_init_like:
; CHECK: tsub	sp, #20
; CHECK: tstorer	r7, [sp, #4]
; CHECK: tloadr	r7, [sp, #4]
; CHECK-NOT: tpush	{r7, lr}
