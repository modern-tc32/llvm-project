; RUN: llc -mtriple=tc32 -O1 --function-sections --data-sections < %s | FileCheck %s

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
  %last = add i32 %itemcount, -1
  br label %loop

loop:
  %remaining = phi i32 [ %next.remaining, %loop ], [ %last, %loop.preheader ]
  %cur = phi ptr [ %next.ptr, %loop ], [ %mem, %loop.preheader ]
  %cur.int = ptrtoint ptr %cur to i32
  %next.int = add i32 %cur.int, %block.size
  %next.ptr = inttoptr i32 %next.int to ptr
  store ptr %next.ptr, ptr %cur, align 1
  %next.remaining = add i32 %remaining, -1
  %done = icmp eq i32 %next.remaining, 0
  br i1 %done, label %tail, label %loop

tail:
  %tail.ptr = phi ptr [ %mem, %init ], [ %next.ptr, %loop ]
  store ptr null, ptr %tail.ptr, align 1
  br label %ret

ret:
  %result = phi ptr [ %pool, %tail ], [ null, %entry ]
  ret ptr %result
}

define ptr @mempool_alloc_like(ptr %pool) local_unnamed_addr {
entry:
  %head = load ptr, ptr %pool, align 1
  %empty = icmp eq ptr %head, null
  br i1 %empty, label %ret, label %have

have:
  %next = load ptr, ptr %head, align 1
  store ptr %next, ptr %pool, align 1
  %data = getelementptr inbounds i8, ptr %head, i32 4
  br label %ret

ret:
  %result = phi ptr [ null, %entry ], [ %data, %have ]
  ret ptr %result
}

define void @mempool_free_like(ptr %pool, ptr %p) local_unnamed_addr {
entry:
  %hdr = getelementptr inbounds i8, ptr %p, i32 -4
  %head = load ptr, ptr %pool, align 1
  store ptr %head, ptr %hdr, align 1
  store ptr %hdr, ptr %pool, align 1
  ret void
}

; CHECK: .section	.text.mempool_init_like,"ax",%progbits
; CHECK-LABEL: mempool_init_like:
; CHECK: .section	.text.mempool_alloc_like,"ax",%progbits
; CHECK-LABEL: mempool_alloc_like:
; CHECK: .section	.text.mempool_free_like,"ax",%progbits
; CHECK-LABEL: mempool_free_like:
