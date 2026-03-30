; RUN: llc -mtriple=tc32 -O2 -filetype=obj -o /dev/null %s

@g_zcl_colorCtrlAttrs = external global [32 x i8], align 1
@colorInfo = external global [64 x i8], align 4
@colorTimerEvt = external global ptr, align 4
@sampleLight_colorTimerEvtCb = external global i8, align 1

declare void @light_applyUpdate_16(ptr, ptr, ptr, ptr, i16, i16, i8)
declare void @light_refresh(i8)
declare zeroext i8 @ev_timer_taskCancel(ptr)
declare ptr @ev_timer_taskPost(ptr, ptr, i32)
declare i16 @llvm.umax.i16(i16, i16)

define internal fastcc void @sampleLight_enhancedStepHueProcess(ptr %0) unnamed_addr {
entry:
  store i8 0, ptr @g_zcl_colorCtrlAttrs, align 1
  %p1 = getelementptr inbounds nuw i8, ptr @g_zcl_colorCtrlAttrs, i32 1
  store i8 3, ptr %p1, align 1
  %p2 = getelementptr inbounds nuw i8, ptr @g_zcl_colorCtrlAttrs, i32 16
  %v2 = load i16, ptr %p2, align 1
  %v3 = zext i16 %v2 to i32
  %v4 = shl nuw nsw i32 %v3, 8
  %ci10 = getelementptr inbounds nuw i8, ptr @colorInfo, i32 10
  store i32 %v4, ptr %ci10, align 2
  %p5 = getelementptr inbounds nuw i8, ptr %0, i32 3
  %v6 = load i16, ptr %p5, align 1
  %v7 = tail call i16 @llvm.umax.i16(i16 %v6, i16 1)
  %ci38 = getelementptr inbounds nuw i8, ptr @colorInfo, i32 38
  store i16 %v7, ptr %ci38, align 2
  %p8 = getelementptr inbounds nuw i8, ptr %0, i32 1
  %v9 = load i16, ptr %p8, align 1
  %v10 = zext i16 %v9 to i32
  %v11 = shl nuw nsw i32 %v10, 8
  %v12 = zext i16 %v7 to i32
  %v13 = udiv i32 %v11, %v12
  %ci6 = getelementptr inbounds nuw i8, ptr @colorInfo, i32 6
  store i32 %v13, ptr %ci6, align 2
  %v14 = load i8, ptr %0, align 1
  %v15 = icmp eq i8 %v14, 3
  br i1 %v15, label %neg, label %cont

neg:
  %v17 = sub nsw i32 0, %v13
  store i32 %v17, ptr %ci6, align 2
  br label %cont

cont:
  tail call void @light_applyUpdate_16(ptr %p2, ptr %ci10, ptr %ci6, ptr %ci38, i16 0, i16 -1, i8 1)
  tail call void @light_refresh(i8 2)
  %v19 = load i16, ptr %ci38, align 2
  %v20 = icmp eq i16 %v19, 0
  %v21 = load ptr, ptr @colorTimerEvt, align 4
  %v22 = icmp eq ptr %v21, null
  br i1 %v20, label %zero, label %nonzero

nonzero:
  br i1 %v22, label %post, label %cancel1

cancel1:
  %v25 = tail call zeroext i8 @ev_timer_taskCancel(ptr @colorTimerEvt)
  br label %post

post:
  %v27 = tail call ptr @ev_timer_taskPost(ptr @sampleLight_colorTimerEvtCb, ptr null, i32 100)
  store ptr %v27, ptr @colorTimerEvt, align 4
  br label %done

zero:
  br i1 %v22, label %done, label %cancel2

cancel2:
  %v30 = tail call zeroext i8 @ev_timer_taskCancel(ptr @colorTimerEvt)
  br label %done

done:
  ret void
}
