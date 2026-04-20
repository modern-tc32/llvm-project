; RUN: llc -mtriple=tc32-unknown-none-elf -filetype=obj -o /dev/null %s
;
; TC32 emits Thumb1 jump tables inline from tBR_JTr, so layout accounting must
; include the hidden jump-table bytes before placing later constant-island
; entries. This reduced testcase used to fail in object emission with:
;   invalid TC32 literal load target

source_filename = "core.8d7ff32b2be6acb0-cgu.0"
target datalayout = "e-m:e-p:32:32-Fi8-i64:64-v128:64:128-a:0:32-n32-S64"
target triple = "tc32-unknown-none-elf"

; Function Attrs: nounwind
define noundef zeroext i1 @_RNvXs20_NtNtCsc9cpkn6bpzE_4core3str4iterNtB6_11EscapeDebugNtNtBa_3fmt7Display3fmt(ptr noalias noundef readonly align 4 captures(none) dereferenceable(108) %self, ptr noalias noundef readonly align 4 captures(none) dereferenceable(16) %f, i32 %0, i1 %1, ptr %2, ptr %_4.sroa.5.sroa.0.i.i, ptr %x.i.i, i8 %_4.1.i.i.i, i1 %3, ptr %_4.sroa.5.sroa.0.i, i8 %_4.0.i.i, i8 %_4.1.i.i, ptr %4, ptr %_6.sroa.5.sroa.0.i, i8 %_4.0.i5.i, i8 %_4.1.i6.i, ptr %_8.sroa.5.sroa.13, ptr %_8.sroa.5.sroa.0, ptr %_8.sroa.5.sroa.8, i8 %_4.sroa.5.sroa.4.0.i, i8 %_4.sroa.5.sroa.5.0.i, i32 %_6.sroa.0.0.i, i8 %_6.sroa.5.sroa.4.0.i, i8 %_6.sroa.5.sroa.5.0.i, i32 %_8.sroa.0.0.i, i8 %_8.sroa.5.sroa.4.0.i, i8 %_8.sroa.5.sroa.5.0.i, ptr %5, i1 %6, ptr %7, ptr %_4.sroa.5.sroa.0.i5, i8 %_4.1.i.i27, i8 %_4.0.i.i26, i1 %8, ptr %9, ptr %_6.sroa.5.sroa.0.i4, ptr %x2.i23, i8 %_4.0.i4.i, ptr %_10.sroa.5.sroa.0, i8 %_4.sroa.5.sroa.4.0.i9, i8 %_4.sroa.5.sroa.5.0.i8, i32 %_6.sroa.0.0.i14, i8 %_6.sroa.5.sroa.4.0.i12, ptr %10, ptr %.val3.i, ptr %_4, ptr %_6.sroa.4.0._4.sroa_idx, ptr %_6.sroa.4.sroa.8.0._6.sroa.4.0._4.sroa_idx.sroa_idx, ptr %_6.sroa.4.sroa.13.0._6.sroa.4.0._4.sroa_idx.sroa_idx, ptr %_6.sroa.5.sroa.4.0._6.sroa.5.0._4.sroa_idx.sroa_idx, ptr %_6.sroa.5.sroa.4.sroa.8.0._6.sroa.5.sroa.4.0._6.sroa.5.0._4.sroa_idx.sroa_idx.sroa_idx, ptr %_10.sroa.5.sroa.8, ptr %_6.sroa.4.sroa.4.0._6.sroa.4.0._4.sroa_idx.sroa_idx, i32 %_8.sroa.5.sroa.7.0, ptr %_6.sroa.4.sroa.7.0._6.sroa.4.0._4.sroa_idx.sroa_idx, i8 %_8.sroa.5.sroa.9.0, ptr %_6.sroa.4.sroa.9.0._6.sroa.4.0._4.sroa_idx.sroa_idx, i8 %_8.sroa.5.sroa.10.0, ptr %_6.sroa.4.sroa.10.0._6.sroa.4.0._4.sroa_idx.sroa_idx, i32 %_8.sroa.5.sroa.12.0, ptr %_6.sroa.4.sroa.12.0._6.sroa.4.0._4.sroa_idx.sroa_idx, i8 %_8.sroa.5.sroa.14.0, ptr %_6.sroa.4.sroa.14.0._6.sroa.4.0._4.sroa_idx.sroa_idx, i8 %_8.sroa.5.sroa.15.0, ptr %_6.sroa.4.sroa.15.0._6.sroa.4.0._4.sroa_idx.sroa_idx, i32 %_10.sroa.0.0, ptr %_6.sroa.5.sroa.4.sroa.4.0._6.sroa.5.sroa.4.0._6.sroa.5.0._4.sroa_idx.sroa_idx.sroa_idx, ptr %_6.sroa.5.sroa.4.sroa.5.0._6.sroa.5.sroa.4.0._6.sroa.5.0._4.sroa_idx.sroa_idx.sroa_idx, i32 %_10.sroa.5.sroa.7.0, ptr %_6.sroa.5.sroa.4.sroa.7.0._6.sroa.5.sroa.4.0._6.sroa.5.0._4.sroa_idx.sroa_idx.sroa_idx, i8 %_10.sroa.5.sroa.9.0, ptr %_6.sroa.5.sroa.4.sroa.9.0._6.sroa.5.sroa.4.0._6.sroa.5.0._4.sroa_idx.sroa_idx.sroa_idx, ptr %_6.sroa.5.sroa.4.sroa.10.0._6.sroa.5.sroa.4.0._6.sroa.5.0._4.sroa_idx.sroa_idx.sroa_idx, ptr %_10.sroa.5.sroa.12.0, ptr %_6.sroa.5.sroa.4.sroa.12.0._6.sroa.5.sroa.4.0._6.sroa.5.0._4.sroa_idx.sroa_idx.sroa_idx, ptr %_10.sroa.5.sroa.13.0, ptr %_6.sroa.5.sroa.4.sroa.13.0._6.sroa.5.sroa.4.0._6.sroa.5.0._4.sroa_idx.sroa_idx.sroa_idx, i1 %.not.i28, i1 %11, i1 %_24.i.i.i.i.i.i.i.i, i8 %.promoted.i.i.i.i.i.i, i8 %_21.i.i.i.i.i.i.i.i, i1 %_15.i.i.us.i.i.i.i.i.i105, i1 %exitcond.not.not.i.not.i.i.i.i.i103.not, i8 %_6.sroa.4.sroa.4.0._6.sroa.4.0._4.sroa_idx.sroa_idx.promoted85, i8 %.lcssa.sink, ptr %_3.0.i.i.i.i.i.us.i.i35.i.i.i.i, i32 %_26.i.i.i.i11.i.i.i.i, i8 %_10.sroa.5.sroa.4.0, i8 %_19.i.i5.us.i.i.i.i.i50.i119, i8 %_19.i.i.us.i.i.i.i.i53.i, i8 %_10.sroa.5.sroa.5.0, ptr %_16.val.i, i32 %_8.i.i.i.i.i.i.i.i.i) #0 {
start:
  %output.i19.i.i.i.i.i.i.i.i.i = alloca [10 x i8], align 4
  %_6.sroa.5.sroa.0.i41 = alloca [12 x i8], align 4
  %_4.sroa.5.sroa.0.i52 = alloca [12 x i8], align 4
  %_4.sroa.5.sroa.0.i.i3 = alloca [12 x i8], align 4
  %_6.sroa.5.sroa.0.i5 = alloca [12 x i8], align 4
  %_4.sroa.5.sroa.0.i6 = alloca [12 x i8], align 4
  %_10.sroa.5.sroa.07 = alloca [12 x i8], align 4
  %_10.sroa.5.sroa.88 = alloca [12 x i8], align 4
  %_8.sroa.5.sroa.09 = alloca [12 x i8], align 4
  %_8.sroa.5.sroa.810 = alloca [12 x i8], align 4
  %_8.sroa.5.sroa.1311 = alloca [12 x i8], align 4
  %_412 = alloca [108 x i8], align 4
  br i1 false, label %bb1, label %bb4

bb4:                                              ; preds = %start
  %12 = load i32, ptr null, align 4
  %13 = icmp eq i32 %0, 1
  br i1 %1, label %bb4.i.i, label %bb1.i

bb4.i.i:                                          ; preds = %bb4
  %x.i.i13 = getelementptr inbounds nuw i8, ptr %self, i32 44
  %_4.0.i.i.i = load i8, ptr null, align 4
  %14 = getelementptr inbounds nuw i8, ptr %self, i32 57
  %_4.1.i.i.i14 = load i8, ptr %2, align 1
  call void @llvm.memcpy.p0.p0.i32(ptr noundef nonnull align 4 dereferenceable(12) %_4.sroa.5.sroa.0.i.i, ptr noundef nonnull readonly align 4 dereferenceable(12) %x.i.i, i32 12, i1 false)
  br label %bb1.i

bb1.i:                                            ; preds = %bb4.i.i, %bb4
  %_8.sroa.5.sroa.5.0.i15 = phi i8 [ 0, %bb4 ], [ %_4.1.i.i.i, %bb4.i.i ]
  %_8.sroa.5.sroa.4.0.i16 = phi i8 [ 0, %bb4 ], [ 1, %bb4.i.i ]
  %_8.sroa.0.0.i17 = phi i32 [ 1, %bb4 ], [ 0, %bb4.i.i ]
  %15 = trunc nuw i32 0 to i1
  br i1 %3, label %bb8.i, label %bb6.i

bb8.i:                                            ; preds = %bb1.i
  %_4.0.i.i18 = load i8, ptr null, align 4
  %_4.1.i.i19 = load i8, ptr null, align 1
  call void @llvm.memcpy.p0.p0.i32(ptr noundef nonnull align 4 dereferenceable(12) %_4.sroa.5.sroa.0.i, ptr noundef nonnull readonly align 4 dereferenceable(12) null, i32 12, i1 false)
  br label %bb6.i

bb6.i:                                            ; preds = %bb8.i, %bb1.i
  %_4.sroa.5.sroa.4.0.i20 = phi i8 [ %_4.0.i.i, %bb8.i ], [ undef, %bb1.i ]
  %_4.sroa.5.sroa.5.0.i21 = phi i8 [ %_4.1.i.i, %bb8.i ], [ undef, %bb1.i ]
  %16 = getelementptr inbounds nuw i8, ptr %self, i32 20
  %_16.i = load i32, ptr null, align 4
  %17 = trunc nuw i32 0 to i1
  br i1 true, label %bb12.i, label %_RNvXsK_NtNtNtCsc9cpkn6bpzE_4core4iter8adapters7flattenINtB5_13FlattenCompatINtNtBb_6option8IntoIterNtNtBb_4char11EscapeDebugEB1z_ENtNtBb_5clone5Clone5cloneBb_.exit

bb12.i:                                           ; preds = %bb6.i
  %_4.0.i5.i22 = load i8, ptr null, align 4
  %18 = getelementptr inbounds nuw i8, ptr %self, i32 37
  %_4.1.i6.i23 = load i8, ptr %4, align 1
  call void @llvm.memcpy.p0.p0.i32(ptr noundef nonnull align 4 dereferenceable(12) %_6.sroa.5.sroa.0.i, ptr noundef nonnull readonly align 4 dereferenceable(12) null, i32 12, i1 false)
  br label %_RNvXsK_NtNtNtCsc9cpkn6bpzE_4core4iter8adapters7flattenINtB5_13FlattenCompatINtNtBb_6option8IntoIterNtNtBb_4char11EscapeDebugEB1z_ENtNtBb_5clone5Clone5cloneBb_.exit

_RNvXsK_NtNtNtCsc9cpkn6bpzE_4core4iter8adapters7flattenINtB5_13FlattenCompatINtNtBb_6option8IntoIterNtNtBb_4char11EscapeDebugEB1z_ENtNtBb_5clone5Clone5cloneBb_.exit: ; preds = %bb12.i, %bb6.i
  %_6.sroa.5.sroa.4.0.i24 = phi i8 [ %_4.0.i5.i, %bb12.i ], [ 0, %bb6.i ]
  %_6.sroa.5.sroa.5.0.i25 = phi i8 [ %_4.1.i6.i, %bb12.i ], [ 0, %bb6.i ]
  %_6.sroa.0.0.i26 = phi i32 [ 1, %bb12.i ], [ 0, %bb6.i ]
  call void @llvm.memcpy.p0.p0.i32(ptr noundef nonnull align 4 dereferenceable(12) %_8.sroa.5.sroa.13, ptr noundef nonnull align 4 dereferenceable(12) %_4.sroa.5.sroa.0.i.i, i32 12, i1 false)
  call void @llvm.memcpy.p0.p0.i32(ptr noundef nonnull align 4 dereferenceable(12) %_8.sroa.5.sroa.0, ptr noundef nonnull align 4 dereferenceable(12) %_4.sroa.5.sroa.0.i, i32 12, i1 false)
  call void @llvm.memcpy.p0.p0.i32(ptr noundef nonnull align 4 dereferenceable(12) %_8.sroa.5.sroa.8, ptr noundef nonnull align 4 dereferenceable(12) %_6.sroa.5.sroa.0.i, i32 12, i1 false)
  br label %bb1

bb1:                                              ; preds = %_RNvXsK_NtNtNtCsc9cpkn6bpzE_4core4iter8adapters7flattenINtB5_13FlattenCompatINtNtBb_6option8IntoIterNtNtBb_4char11EscapeDebugEB1z_ENtNtBb_5clone5Clone5cloneBb_.exit, %start
  %.promoted.i.i.i.i.i.i27 = phi i8 [ 0, %start ], [ %_4.sroa.5.sroa.4.0.i, %_RNvXsK_NtNtNtCsc9cpkn6bpzE_4core4iter8adapters7flattenINtB5_13FlattenCompatINtNtBb_6option8IntoIterNtNtBb_4char11EscapeDebugEB1z_ENtNtBb_5clone5Clone5cloneBb_.exit ]
  %_21.i.i.i.i.i.i.i.i28 = phi i8 [ 0, %start ], [ %_4.sroa.5.sroa.5.0.i, %_RNvXsK_NtNtNtCsc9cpkn6bpzE_4core4iter8adapters7flattenINtB5_13FlattenCompatINtNtBb_6option8IntoIterNtNtBb_4char11EscapeDebugEB1z_ENtNtBb_5clone5Clone5cloneBb_.exit ]
  %_8.sroa.5.sroa.7.029 = phi i32 [ 0, %start ], [ %_6.sroa.0.0.i, %_RNvXsK_NtNtNtCsc9cpkn6bpzE_4core4iter8adapters7flattenINtB5_13FlattenCompatINtNtBb_6option8IntoIterNtNtBb_4char11EscapeDebugEB1z_ENtNtBb_5clone5Clone5cloneBb_.exit ]
  %_8.sroa.5.sroa.9.030 = phi i8 [ 0, %start ], [ %_6.sroa.5.sroa.4.0.i, %_RNvXsK_NtNtNtCsc9cpkn6bpzE_4core4iter8adapters7flattenINtB5_13FlattenCompatINtNtBb_6option8IntoIterNtNtBb_4char11EscapeDebugEB1z_ENtNtBb_5clone5Clone5cloneBb_.exit ]
  %_8.sroa.5.sroa.10.031 = phi i8 [ 0, %start ], [ %_6.sroa.5.sroa.5.0.i, %_RNvXsK_NtNtNtCsc9cpkn6bpzE_4core4iter8adapters7flattenINtB5_13FlattenCompatINtNtBb_6option8IntoIterNtNtBb_4char11EscapeDebugEB1z_ENtNtBb_5clone5Clone5cloneBb_.exit ]
  %_8.sroa.5.sroa.12.032 = phi i32 [ 0, %start ], [ %_8.sroa.0.0.i, %_RNvXsK_NtNtNtCsc9cpkn6bpzE_4core4iter8adapters7flattenINtB5_13FlattenCompatINtNtBb_6option8IntoIterNtNtBb_4char11EscapeDebugEB1z_ENtNtBb_5clone5Clone5cloneBb_.exit ]
  %_8.sroa.5.sroa.14.033 = phi i8 [ 0, %start ], [ %_8.sroa.5.sroa.4.0.i, %_RNvXsK_NtNtNtCsc9cpkn6bpzE_4core4iter8adapters7flattenINtB5_13FlattenCompatINtNtBb_6option8IntoIterNtNtBb_4char11EscapeDebugEB1z_ENtNtBb_5clone5Clone5cloneBb_.exit ]
  %_8.sroa.5.sroa.15.034 = phi i8 [ 0, %start ], [ %_8.sroa.5.sroa.5.0.i, %_RNvXsK_NtNtNtCsc9cpkn6bpzE_4core4iter8adapters7flattenINtB5_13FlattenCompatINtNtBb_6option8IntoIterNtNtBb_4char11EscapeDebugEB1z_ENtNtBb_5clone5Clone5cloneBb_.exit ]
  %.not3 = icmp eq i32 0, 1
  br i1 true, label %bb5, label %bb7

bb7:                                              ; preds = %bb1
  %19 = getelementptr inbounds nuw i8, ptr %self, i32 100
  %20 = load ptr, ptr %5, align 4
  %.val3.i35 = load ptr, ptr null, align 4
  call void @llvm.lifetime.start.p0(ptr nonnull %_4.sroa.5.sroa.0.i52)
  %21 = trunc nuw i32 0 to i1
  br i1 %6, label %bb8.i24, label %bb6.i7

bb8.i24:                                          ; preds = %bb7
  %_4.0.i.i2636 = load i8, ptr null, align 4
  %22 = getelementptr inbounds nuw i8, ptr %self, i32 77
  %_4.1.i.i2737 = load i8, ptr %7, align 1
  call void @llvm.memcpy.p0.p0.i32(ptr noundef nonnull align 4 dereferenceable(12) %_4.sroa.5.sroa.0.i5, ptr noundef nonnull readonly align 4 dereferenceable(12) null, i32 12, i1 false)
  br label %bb6.i7

bb6.i7:                                           ; preds = %bb8.i24, %bb7
  %_4.sroa.5.sroa.5.0.i838 = phi i8 [ %_4.1.i.i27, %bb8.i24 ], [ 0, %bb7 ]
  %_4.sroa.5.sroa.4.0.i939 = phi i8 [ %_4.0.i.i26, %bb8.i24 ], [ 0, %bb7 ]
  %_4.sroa.0.0.i10 = phi i32 [ 1, %bb8.i24 ], [ 0, %bb7 ]
  %23 = trunc nuw i32 0 to i1
  br i1 %8, label %bb12.i22, label %_RNvXsK_NtNtNtCsc9cpkn6bpzE_4core4iter8adapters7flattenINtB5_13FlattenCompatINtNtB7_3map3MapNtNtNtBb_3str4iter5CharsNtB1v_23CharEscapeDebugContinueENtNtBb_4char11EscapeDebugENtNtBb_5clone5Clone5cloneBb_.exit

bb12.i22:                                         ; preds = %bb6.i7
  %x2.i2340 = getelementptr inbounds nuw i8, ptr %self, i32 84
  %24 = getelementptr inbounds nuw i8, ptr %self, i32 96
  %_4.0.i4.i41 = load i8, ptr %9, align 4
  %_4.1.i5.i = load i8, ptr null, align 1
  call void @llvm.memcpy.p0.p0.i32(ptr noundef nonnull align 4 dereferenceable(12) %_6.sroa.5.sroa.0.i4, ptr noundef nonnull readonly align 4 dereferenceable(12) %x2.i23, i32 12, i1 false)
  br label %_RNvXsK_NtNtNtCsc9cpkn6bpzE_4core4iter8adapters7flattenINtB5_13FlattenCompatINtNtB7_3map3MapNtNtNtBb_3str4iter5CharsNtB1v_23CharEscapeDebugContinueENtNtBb_4char11EscapeDebugENtNtBb_5clone5Clone5cloneBb_.exit

_RNvXsK_NtNtNtCsc9cpkn6bpzE_4core4iter8adapters7flattenINtB5_13FlattenCompatINtNtB7_3map3MapNtNtNtBb_3str4iter5CharsNtB1v_23CharEscapeDebugContinueENtNtBb_4char11EscapeDebugENtNtBb_5clone5Clone5cloneBb_.exit: ; preds = %bb12.i22, %bb6.i7
  %_6.sroa.5.sroa.4.0.i1242 = phi i8 [ %_4.0.i4.i, %bb12.i22 ], [ 0, %bb6.i7 ]
  %_6.sroa.5.sroa.5.0.i13 = phi i8 [ 0, %bb12.i22 ], [ 0, %bb6.i7 ]
  %_6.sroa.0.0.i1443 = phi i32 [ 1, %bb12.i22 ], [ 0, %bb6.i7 ]
  call void @llvm.memcpy.p0.p0.i32(ptr noundef nonnull align 4 dereferenceable(12) %_10.sroa.5.sroa.0, ptr noundef nonnull align 4 dereferenceable(12) %_4.sroa.5.sroa.0.i5, i32 12, i1 false)
  call void @llvm.memcpy.p0.p0.i32(ptr noundef nonnull align 4 dereferenceable(12) null, ptr noundef nonnull align 4 dereferenceable(12) null, i32 12, i1 false)
  call void @llvm.lifetime.end.p0(ptr nonnull %_4.sroa.5.sroa.0.i52)
  br label %bb5

bb5:                                              ; preds = %_RNvXsK_NtNtNtCsc9cpkn6bpzE_4core4iter8adapters7flattenINtB5_13FlattenCompatINtNtB7_3map3MapNtNtNtBb_3str4iter5CharsNtB1v_23CharEscapeDebugContinueENtNtBb_4char11EscapeDebugENtNtBb_5clone5Clone5cloneBb_.exit, %bb1
  %_10.sroa.5.sroa.4.044 = phi i8 [ 0, %bb1 ], [ %_4.sroa.5.sroa.4.0.i9, %_RNvXsK_NtNtNtCsc9cpkn6bpzE_4core4iter8adapters7flattenINtB5_13FlattenCompatINtNtB7_3map3MapNtNtNtBb_3str4iter5CharsNtB1v_23CharEscapeDebugContinueENtNtBb_4char11EscapeDebugENtNtBb_5clone5Clone5cloneBb_.exit ]
  %_10.sroa.5.sroa.5.045 = phi i8 [ undef, %bb1 ], [ %_4.sroa.5.sroa.5.0.i8, %_RNvXsK_NtNtNtCsc9cpkn6bpzE_4core4iter8adapters7flattenINtB5_13FlattenCompatINtNtB7_3map3MapNtNtNtBb_3str4iter5CharsNtB1v_23CharEscapeDebugContinueENtNtBb_4char11EscapeDebugENtNtBb_5clone5Clone5cloneBb_.exit ]
  %_10.sroa.5.sroa.7.046 = phi i32 [ 0, %bb1 ], [ %_6.sroa.0.0.i14, %_RNvXsK_NtNtNtCsc9cpkn6bpzE_4core4iter8adapters7flattenINtB5_13FlattenCompatINtNtB7_3map3MapNtNtNtBb_3str4iter5CharsNtB1v_23CharEscapeDebugContinueENtNtBb_4char11EscapeDebugENtNtBb_5clone5Clone5cloneBb_.exit ]
  %_10.sroa.5.sroa.9.047 = phi i8 [ 0, %bb1 ], [ %_6.sroa.5.sroa.4.0.i12, %_RNvXsK_NtNtNtCsc9cpkn6bpzE_4core4iter8adapters7flattenINtB5_13FlattenCompatINtNtB7_3map3MapNtNtNtBb_3str4iter5CharsNtB1v_23CharEscapeDebugContinueENtNtBb_4char11EscapeDebugENtNtBb_5clone5Clone5cloneBb_.exit ]
  %_10.sroa.5.sroa.10.0 = phi i8 [ 0, %bb1 ], [ 0, %_RNvXsK_NtNtNtCsc9cpkn6bpzE_4core4iter8adapters7flattenINtB5_13FlattenCompatINtNtB7_3map3MapNtNtNtBb_3str4iter5CharsNtB1v_23CharEscapeDebugContinueENtNtBb_4char11EscapeDebugENtNtBb_5clone5Clone5cloneBb_.exit ]
  %_10.sroa.5.sroa.12.048 = phi ptr [ null, %bb1 ], [ %10, %_RNvXsK_NtNtNtCsc9cpkn6bpzE_4core4iter8adapters7flattenINtB5_13FlattenCompatINtNtB7_3map3MapNtNtNtBb_3str4iter5CharsNtB1v_23CharEscapeDebugContinueENtNtBb_4char11EscapeDebugENtNtBb_5clone5Clone5cloneBb_.exit ]
  %_10.sroa.5.sroa.13.049 = phi ptr [ null, %bb1 ], [ %.val3.i, %_RNvXsK_NtNtNtCsc9cpkn6bpzE_4core4iter8adapters7flattenINtB5_13FlattenCompatINtNtB7_3map3MapNtNtNtBb_3str4iter5CharsNtB1v_23CharEscapeDebugContinueENtNtBb_4char11EscapeDebugENtNtBb_5clone5Clone5cloneBb_.exit ]
  %_10.sroa.0.050 = phi i32 [ 2, %bb1 ], [ 1, %_RNvXsK_NtNtNtCsc9cpkn6bpzE_4core4iter8adapters7flattenINtB5_13FlattenCompatINtNtB7_3map3MapNtNtNtBb_3str4iter5CharsNtB1v_23CharEscapeDebugContinueENtNtBb_4char11EscapeDebugENtNtBb_5clone5Clone5cloneBb_.exit ]
  %_6.sroa.4.0._4.sroa_idx51 = getelementptr inbounds nuw i8, ptr %_4, i32 4
  call void @llvm.memcpy.p0.p0.i32(ptr noundef nonnull align 4 dereferenceable(12) %_6.sroa.4.0._4.sroa_idx, ptr noundef nonnull align 4 dereferenceable(12) %_8.sroa.5.sroa.0, i32 12, i1 false)
  %_6.sroa.4.sroa.8.0._6.sroa.4.0._4.sroa_idx.sroa_idx52 = getelementptr inbounds nuw i8, ptr %_4, i32 24
  call void @llvm.memcpy.p0.p0.i32(ptr noundef nonnull align 4 dereferenceable(12) %_6.sroa.4.sroa.8.0._6.sroa.4.0._4.sroa_idx.sroa_idx, ptr noundef nonnull align 4 dereferenceable(12) %_8.sroa.5.sroa.8, i32 12, i1 false)
  %_6.sroa.4.sroa.13.0._6.sroa.4.0._4.sroa_idx.sroa_idx53 = getelementptr inbounds nuw i8, ptr %_4, i32 44
  call void @llvm.memcpy.p0.p0.i32(ptr noundef nonnull align 4 dereferenceable(12) %_6.sroa.4.sroa.13.0._6.sroa.4.0._4.sroa_idx.sroa_idx, ptr noundef nonnull align 4 dereferenceable(12) null, i32 12, i1 false)
  %_6.sroa.5.sroa.4.0._6.sroa.5.0._4.sroa_idx.sroa_idx54 = getelementptr inbounds nuw i8, ptr %_4, i32 64
  call void @llvm.memcpy.p0.p0.i32(ptr noundef nonnull align 4 dereferenceable(12) %_6.sroa.5.sroa.4.0._6.sroa.5.0._4.sroa_idx.sroa_idx, ptr noundef nonnull align 4 dereferenceable(12) %_10.sroa.5.sroa.0, i32 12, i1 false)
  %_6.sroa.5.sroa.4.sroa.8.0._6.sroa.5.sroa.4.0._6.sroa.5.0._4.sroa_idx.sroa_idx.sroa_idx55 = getelementptr inbounds nuw i8, ptr %_4, i32 84
  call void @llvm.memcpy.p0.p0.i32(ptr noundef nonnull align 4 dereferenceable(12) %_6.sroa.5.sroa.4.sroa.8.0._6.sroa.5.sroa.4.0._6.sroa.5.0._4.sroa_idx.sroa_idx.sroa_idx, ptr noundef nonnull align 4 dereferenceable(12) %_10.sroa.5.sroa.8, i32 12, i1 false)
  store i32 0, ptr null, align 4
  %_6.sroa.4.sroa.4.0._6.sroa.4.0._4.sroa_idx.sroa_idx56 = getelementptr inbounds nuw i8, ptr %_4, i32 16
  store i8 1, ptr %_6.sroa.4.sroa.4.0._6.sroa.4.0._4.sroa_idx.sroa_idx, align 4
  %_6.sroa.4.sroa.5.0._6.sroa.4.0._4.sroa_idx.sroa_idx = getelementptr inbounds nuw i8, ptr %_4, i32 17
  store i8 0, ptr null, align 1
  %_6.sroa.4.sroa.7.0._6.sroa.4.0._4.sroa_idx.sroa_idx57 = getelementptr inbounds nuw i8, ptr %_4, i32 20
  store i32 %_8.sroa.5.sroa.7.0, ptr %_6.sroa.4.sroa.7.0._6.sroa.4.0._4.sroa_idx.sroa_idx, align 4
  %_6.sroa.4.sroa.9.0._6.sroa.4.0._4.sroa_idx.sroa_idx58 = getelementptr inbounds nuw i8, ptr %_4, i32 36
  store i8 %_8.sroa.5.sroa.9.0, ptr %_6.sroa.4.sroa.9.0._6.sroa.4.0._4.sroa_idx.sroa_idx, align 4
  %_6.sroa.4.sroa.10.0._6.sroa.4.0._4.sroa_idx.sroa_idx59 = getelementptr inbounds nuw i8, ptr %_4, i32 37
  store i8 %_8.sroa.5.sroa.10.0, ptr %_6.sroa.4.sroa.10.0._6.sroa.4.0._4.sroa_idx.sroa_idx, align 1
  %_6.sroa.4.sroa.12.0._6.sroa.4.0._4.sroa_idx.sroa_idx60 = getelementptr inbounds nuw i8, ptr %_4, i32 40
  store i32 %_8.sroa.5.sroa.12.0, ptr %_6.sroa.4.sroa.12.0._6.sroa.4.0._4.sroa_idx.sroa_idx, align 4
  %_6.sroa.4.sroa.14.0._6.sroa.4.0._4.sroa_idx.sroa_idx61 = getelementptr inbounds nuw i8, ptr %_4, i32 56
  store i8 %_8.sroa.5.sroa.14.0, ptr %_6.sroa.4.sroa.14.0._6.sroa.4.0._4.sroa_idx.sroa_idx, align 4
  %_6.sroa.4.sroa.15.0._6.sroa.4.0._4.sroa_idx.sroa_idx62 = getelementptr inbounds nuw i8, ptr %_4, i32 57
  store i8 %_8.sroa.5.sroa.15.0, ptr %_6.sroa.4.sroa.15.0._6.sroa.4.0._4.sroa_idx.sroa_idx, align 1
  %_6.sroa.5.0._4.sroa_idx = getelementptr inbounds nuw i8, ptr %_4, i32 60
  store i32 %_10.sroa.0.0, ptr null, align 4
  %_6.sroa.5.sroa.4.sroa.4.0._6.sroa.5.sroa.4.0._6.sroa.5.0._4.sroa_idx.sroa_idx.sroa_idx63 = getelementptr inbounds nuw i8, ptr %_4, i32 76
  store i8 1, ptr %_6.sroa.5.sroa.4.sroa.4.0._6.sroa.5.sroa.4.0._6.sroa.5.0._4.sroa_idx.sroa_idx.sroa_idx, align 4
  %_6.sroa.5.sroa.4.sroa.5.0._6.sroa.5.sroa.4.0._6.sroa.5.0._4.sroa_idx.sroa_idx.sroa_idx64 = getelementptr inbounds nuw i8, ptr %_4, i32 77
  store i8 0, ptr %_6.sroa.5.sroa.4.sroa.5.0._6.sroa.5.sroa.4.0._6.sroa.5.0._4.sroa_idx.sroa_idx.sroa_idx, align 1
  %_6.sroa.5.sroa.4.sroa.7.0._6.sroa.5.sroa.4.0._6.sroa.5.0._4.sroa_idx.sroa_idx.sroa_idx65 = getelementptr inbounds nuw i8, ptr %_4, i32 80
  store i32 %_10.sroa.5.sroa.7.0, ptr %_6.sroa.5.sroa.4.sroa.7.0._6.sroa.5.sroa.4.0._6.sroa.5.0._4.sroa_idx.sroa_idx.sroa_idx, align 4
  %_6.sroa.5.sroa.4.sroa.9.0._6.sroa.5.sroa.4.0._6.sroa.5.0._4.sroa_idx.sroa_idx.sroa_idx66 = getelementptr inbounds nuw i8, ptr %_4, i32 96
  store i8 %_10.sroa.5.sroa.9.0, ptr %_6.sroa.5.sroa.4.sroa.9.0._6.sroa.5.sroa.4.0._6.sroa.5.0._4.sroa_idx.sroa_idx.sroa_idx, align 4
  %_6.sroa.5.sroa.4.sroa.10.0._6.sroa.5.sroa.4.0._6.sroa.5.0._4.sroa_idx.sroa_idx.sroa_idx67 = getelementptr inbounds nuw i8, ptr %_4, i32 97
  store i8 0, ptr %_6.sroa.5.sroa.4.sroa.10.0._6.sroa.5.sroa.4.0._6.sroa.5.0._4.sroa_idx.sroa_idx.sroa_idx, align 1
  %_6.sroa.5.sroa.4.sroa.12.0._6.sroa.5.sroa.4.0._6.sroa.5.0._4.sroa_idx.sroa_idx.sroa_idx68 = getelementptr inbounds nuw i8, ptr %_4, i32 100
  store ptr %_10.sroa.5.sroa.12.0, ptr %_6.sroa.5.sroa.4.sroa.12.0._6.sroa.5.sroa.4.0._6.sroa.5.0._4.sroa_idx.sroa_idx.sroa_idx, align 4
  %_6.sroa.5.sroa.4.sroa.13.0._6.sroa.5.sroa.4.0._6.sroa.5.0._4.sroa_idx.sroa_idx.sroa_idx69 = getelementptr inbounds nuw i8, ptr %_4, i32 104
  store ptr %_10.sroa.5.sroa.13.0, ptr %_6.sroa.5.sroa.4.sroa.13.0._6.sroa.5.sroa.4.0._6.sroa.5.0._4.sroa_idx.sroa_idx.sroa_idx, align 4
  %.not.i2870 = icmp eq i32 0, 1
  br i1 %.not.i28, label %bb9.i, label %bb1.i29

bb1.i29:                                          ; preds = %bb5
  %25 = trunc nuw i32 0 to i1
  br i1 %11, label %bb1.i.i.i.i, label %bb8.i.i.i.i

bb1.i.i.i.i:                                      ; preds = %bb1.i29
  %_24.i.i.i.i.i.i.i.i71 = icmp ugt i8 0, 0
  br i1 %_24.i.i.i.i.i.i.i.i, label %bb1.us.i.i.preheader.i.i.i.i, label %bb1.preheader.i.i.i.i.i.i

bb1.us.i.i.preheader.i.i.i.i:                     ; preds = %bb1.i.i.i.i
  %_3.0.i.i.i.i.i.us.i.i.i.i.i.i = load ptr, ptr null, align 4
  %_6.sroa.4.sroa.4.0._6.sroa.4.0._4.sroa_idx.sroa_idx.promoted8572 = load i8, ptr %_6.sroa.4.sroa.4.0._6.sroa.4.0._4.sroa_idx.sroa_idx, align 4
  %_15.i.i.us.i.i.i.i.i.i10573 = icmp ult i8 %.promoted.i.i.i.i.i.i, %_21.i.i.i.i.i.i.i.i
  br i1 %_15.i.i.us.i.i.i.i.i.i105, label %bb3.i.i.us.i.i.i.i.i.i.preheader, label %bb8.i.i.i.i.sink.split

bb3.i.i.us.i.i.i.i.i.i.preheader:                 ; preds = %bb1.us.i.i.preheader.i.i.i.i
  %26 = load ptr, ptr null, align 4
  %_19.i.i.us.i.i.i.i.i.i = add nuw i8 0, 0
  %_0.i.i.i.i.i.us.i.i.i.i.i.i = tail call noundef zeroext i1 %26(ptr noundef nonnull null, i32 noundef range(i32 0, 1114112) 0) #8
  store i8 0, ptr null, align 4
  ret i1 false

bb1.preheader.i.i.i.i.i.i:                        ; preds = %bb1.i.i.i.i
  %umax.i.i.i.i.i.i = tail call i8 @llvm.umax.i8(i8 %.promoted.i.i.i.i.i.i, i8 0)
  %_3.0.i.i.i.i.i.i.i.i.i.i.i = load ptr, ptr null, align 4
  %exitcond.not.not.i.not.i.i.i.i.i103.not74 = icmp ult i8 %.promoted.i.i.i.i.i.i, %_21.i.i.i.i.i.i.i.i
  br i1 %exitcond.not.not.i.not.i.i.i.i.i103.not, label %bb3.i.i.i.i.i.i.i.i.lr.ph, label %bb8.i.i.i.i.sink.split

bb3.i.i.i.i.i.i.i.i.lr.ph:                        ; preds = %bb1.preheader.i.i.i.i.i.i
  %self2.i.i.i.i.i.i.i.i = load i8, ptr null, align 1
  %_9.i.i.i.i.i.i.i.i = zext nneg i8 0 to i32
  %_0.i.i.i.i.i.i.i.i.i.i.i = tail call noundef zeroext i1 null(ptr noundef nonnull null, i32 noundef range(i32 0, 1114112) 0) #8
  ret i1 false

bb8.i.i.i.i.sink.split:                           ; preds = %bb1.preheader.i.i.i.i.i.i, %bb1.us.i.i.preheader.i.i.i.i
  %.lcssa.sink75 = phi i8 [ 0, %bb1.preheader.i.i.i.i.i.i ], [ %_6.sroa.4.sroa.4.0._6.sroa.4.0._4.sroa_idx.sroa_idx.promoted85, %bb1.us.i.i.preheader.i.i.i.i ]
  store i8 %.lcssa.sink, ptr null, align 4
  br label %bb8.i.i.i.i

bb8.i.i.i.i:                                      ; preds = %bb8.i.i.i.i.sink.split, %bb1.i29
  %_26.i.i.i.i11.i.i.i.i76 = load i32, ptr %_6.sroa.4.sroa.8.0._6.sroa.4.0._4.sroa_idx.sroa_idx, align 4
  %_3.0.i.i.i.i.i.us.i.i35.i.i.i.i77 = load ptr, ptr %f, align 4
  %27 = load ptr, ptr null, align 4
  br label %bb3.i.i.us.i.i32.i.i.i.i

bb1.us.i.i29.i.i.i.i:                             ; preds = %bb3.i.i.us.i.i32.i.i.i.i
  %_19.i.i.us.i.i33.i.i.i.i = add nuw i8 1, 1
  %_15.i.i.us.i.i31.i.i.i.i = icmp ult i8 1, 0
  br label %bb3.i.i.us.i.i32.i.i.i.i

bb3.i.i.us.i.i32.i.i.i.i:                         ; preds = %bb1.us.i.i29.i.i.i.i, %bb8.i.i.i.i
  %_19.i.i5.us.i.i30.i.i.i.i115 = phi i8 [ 1, %bb1.us.i.i29.i.i.i.i ], [ 1, %bb8.i.i.i.i ]
  %_0.i.i.i.i.i.us.i.i37.i.i.i.i = tail call noundef zeroext i1 %27(ptr noundef nonnull %_3.0.i.i.i.i.i.us.i.i35.i.i.i.i, i32 noundef range(i32 0, 1114112) %_26.i.i.i.i11.i.i.i.i) #8
  br i1 true, label %_RINvXs_NtNtNtCsc9cpkn6bpzE_4core4iter8adapters5chainINtB5_5ChainINtNtB7_7flatten7FlattenINtNtBb_6option8IntoIterNtNtBb_4char11EscapeDebugEEINtB13_7FlatMapNtNtNtBb_3str4iter5CharsB1M_NtB2w_23CharEscapeDebugContinueEENtNtNtB9_6traits8iterator8Iterator8try_folduNCINvNvB3r_12try_for_each4callcINtNtBb_6result6ResultuNtNtBb_3fmt5ErrorENCNvXs20_B2u_NtB2u_11EscapeDebugNtB53_7Display3fmt0E0B4E_EBb_.exit, label %bb1.us.i.i29.i.i.i.i

bb9.i:                                            ; preds = %bb5
  %.not1.i = icmp eq i32 0, 0
  br i1 false, label %_RINvXs_NtNtNtCsc9cpkn6bpzE_4core4iter8adapters5chainINtB5_5ChainINtNtB7_7flatten7FlattenINtNtBb_6option8IntoIterNtNtBb_4char11EscapeDebugEEINtB13_7FlatMapNtNtNtBb_3str4iter5CharsB1M_NtB2w_23CharEscapeDebugContinueEENtNtNtB9_6traits8iterator8Iterator8try_folduNCINvNvB3r_12try_for_each4callcINtNtBb_6result6ResultuNtNtBb_3fmt5ErrorENCNvXs20_B2u_NtB2u_11EscapeDebugNtB53_7Display3fmt0E0B4E_EBb_.exit, label %bb10.i

bb10.i:                                           ; preds = %bb9.i
  %_16.val.i78 = load ptr, ptr null, align 4
  %28 = trunc nuw i32 0 to i1
  br i1 true, label %bb1.i.i.i30.i, label %bb8.i.i.i4.i

bb1.i.i.i30.i:                                    ; preds = %bb10.i
  %_24.i.i.i.i.i.i.i34.i = icmp ugt i8 1, 1
  %_26.i.i.i.i.i.i.i35.i = load i32, ptr null, align 4
  br i1 true, label %bb1.us.i.i.preheader.i.i.i48.i, label %bb1.preheader.i.i.i.i.i36.i

bb1.us.i.i.preheader.i.i.i48.i:                   ; preds = %bb1.i.i.i30.i
  %_15.i.i.us.i.i.i.i.i51.i118 = icmp ult i8 0, 0
  br i1 true, label %bb3.i.i.us.i.i.i.i.i52.i.preheader, label %bb8.i.i.i4.i

bb3.i.i.us.i.i.i.i.i52.i.preheader:               ; preds = %bb1.us.i.i.preheader.i.i.i48.i
  %29 = load ptr, ptr null, align 4
  br label %bb3.i.i.us.i.i.i.i.i52.i

bb1.preheader.i.i.i.i.i36.i:                      ; preds = %bb1.i.i.i30.i
  %umax.i.i.i.i.i37.i = tail call i8 @llvm.umax.i8(i8 %_10.sroa.5.sroa.4.0, i8 0)
  %exitcond.not.not.i.not.i.i.i.i41.i116.not = icmp ult i8 0, 1
  br i1 true, label %bb3.i.i.i.i.i.i.i42.i.lr.ph, label %bb8.i.i.i4.i

bb3.i.i.i.i.i.i.i42.i.lr.ph:                      ; preds = %bb1.preheader.i.i.i.i.i36.i
  %_0.i.i.i.i.i.i.i.i.i.i = tail call noundef zeroext i1 null(ptr noundef nonnull null, i32 noundef range(i32 0, 1114112) 0) #8
  br i1 true, label %_RINvXs_NtNtNtCsc9cpkn6bpzE_4core4iter8adapters5chainINtB5_5ChainINtNtB7_7flatten7FlattenINtNtBb_6option8IntoIterNtNtBb_4char11EscapeDebugEEINtB13_7FlatMapNtNtNtBb_3str4iter5CharsB1M_NtB2w_23CharEscapeDebugContinueEENtNtNtB9_6traits8iterator8Iterator8try_folduNCINvNvB3r_12try_for_each4callcINtNtBb_6result6ResultuNtNtBb_3fmt5ErrorENCNvXs20_B2u_NtB2u_11EscapeDebugNtB53_7Display3fmt0E0B4E_EBb_.exit, label %bb1.i.i.i.i.i39.i

bb1.us.i.i.i.i.i49.i:                             ; preds = %bb3.i.i.us.i.i.i.i.i52.i
  %_19.i.i.us.i.i.i.i.i53.i79 = add nuw i8 %_19.i.i5.us.i.i.i.i.i50.i119, 0
  %_15.i.i.us.i.i.i.i.i51.i = icmp ult i8 %_19.i.i.us.i.i.i.i.i53.i, %_10.sroa.5.sroa.5.0
  br i1 %_15.i.i.us.i.i.i.i.i51.i, label %bb3.i.i.us.i.i.i.i.i52.i, label %bb8.i.i.i4.i

bb3.i.i.us.i.i.i.i.i52.i:                         ; preds = %bb1.us.i.i.i.i.i49.i, %bb3.i.i.us.i.i.i.i.i52.i.preheader
  %_19.i.i5.us.i.i.i.i.i50.i11980 = phi i8 [ 0, %bb1.us.i.i.i.i.i49.i ], [ %_10.sroa.5.sroa.4.0, %bb3.i.i.us.i.i.i.i.i52.i.preheader ]
  %_0.i.i.i.i.us.i.i.i.i.i.i = tail call noundef zeroext i1 %29(ptr noundef nonnull %_16.val.i, i32 noundef range(i32 0, 1114112) 0) #8
  br i1 true, label %_RINvXs_NtNtNtCsc9cpkn6bpzE_4core4iter8adapters5chainINtB5_5ChainINtNtB7_7flatten7FlattenINtNtBb_6option8IntoIterNtNtBb_4char11EscapeDebugEEINtB13_7FlatMapNtNtNtBb_3str4iter5CharsB1M_NtB2w_23CharEscapeDebugContinueEENtNtNtB9_6traits8iterator8Iterator8try_folduNCINvNvB3r_12try_for_each4callcINtNtBb_6result6ResultuNtNtBb_3fmt5ErrorENCNvXs20_B2u_NtB2u_11EscapeDebugNtB53_7Display3fmt0E0B4E_EBb_.exit, label %bb1.us.i.i.i.i.i49.i

bb1.i.i.i.i.i39.i:                                ; preds = %bb3.i.i.i.i.i.i.i42.i.lr.ph
  %indvars.iv.next.i.i.i.i.i43.i = add nuw nsw i32 0, 0
  br label %bb8.i.i.i4.i

bb8.i.i.i4.i:                                     ; preds = %bb1.i.i.i.i.i39.i, %bb1.us.i.i.i.i.i49.i, %bb1.preheader.i.i.i.i.i36.i, %bb1.us.i.i.preheader.i.i.i48.i, %bb10.i
  br i1 false, label %bb11.i.i.i9.i, label %bb1.i.i.i.i7.i

bb1.i.i.i.i7.i:                                   ; preds = %bb8.i.i.i4.i
  %_6.i.i.not.i12.i.i.i.i.i.i.i = icmp eq ptr %_10.sroa.5.sroa.12.0, null
  br i1 true, label %bb11.i.i.i9.i, label %bb15.i.i.lr.ph.i.i.i.i.i.i.i

bb15.i.i.lr.ph.i.i.i.i.i.i.i:                     ; preds = %bb1.i.i.i.i7.i
  %_8.i.i.i.i.i.i.i.i.i81 = zext nneg i8 0 to i32
  call void @llvm.lifetime.start.p0(ptr nonnull %output.i19.i.i.i.i.i.i.i.i.i)
  switch i32 %_8.i.i.i.i.i.i.i.i.i, label %bb2.i.i.i.i.i.i.i.i.i [
    i32 0, label %bb9.i.i.i.i.i.i.i.i.i
    i32 9, label %bb8.i.i6.i.i.i.i.i.i.i
    i32 13, label %bb7.i.i.i.i.i.i.i.i.i
    i32 39, label %bb3.i.i3.i.i.i.i.i.i.i
  ]

bb2.i.i.i.i.i.i.i.i.i:                            ; preds = %bb15.i.i.lr.ph.i.i.i.i.i.i.i
  call void @llvm.memcpy.p0.p0.i32(ptr noundef nonnull align 4 dereferenceable(10) %_6.sroa.5.sroa.4.0._6.sroa.5.0._4.sroa_idx.sroa_idx, ptr noundef nonnull align 4 dereferenceable(10) null, i32 1, i1 false)
  ret i1 false

bb9.i.i.i.i.i.i.i.i.i:                            ; preds = %bb15.i.i.lr.ph.i.i.i.i.i.i.i
  ret i1 false

bb8.i.i6.i.i.i.i.i.i.i:                           ; preds = %bb15.i.i.lr.ph.i.i.i.i.i.i.i
  ret i1 false

bb7.i.i.i.i.i.i.i.i.i:                            ; preds = %bb15.i.i.lr.ph.i.i.i.i.i.i.i
  ret i1 false

bb3.i.i3.i.i.i.i.i.i.i:                           ; preds = %bb15.i.i.lr.ph.i.i.i.i.i.i.i
  store i64 0, ptr null, align 2
  ret i1 false

bb11.i.i.i9.i:                                    ; preds = %bb1.i.i.i.i7.i, %bb8.i.i.i4.i
  %30 = load ptr, ptr null, align 4
  br label %bb3.i.i.us.i.i31.i.i.i.i

bb3.i.i.us.i.i31.i.i.i.i:                         ; preds = %bb3.i.i.us.i.i31.i.i.i.i, %bb11.i.i.i9.i
  %_19.i.i5.us.i.i29.i.i.i.i127 = phi i8 [ %_19.i.i.us.i.i32.i.i.i.i, %bb3.i.i.us.i.i31.i.i.i.i ], [ %_10.sroa.5.sroa.9.047, %bb11.i.i.i9.i ]
  %_0.i.i.i.i.us.i.i35.i.i.i.i = tail call noundef zeroext i1 %30(ptr noundef nonnull %_16.val.i78, i32 noundef range(i32 0, 1114112) 0) #8
  %_19.i.i.us.i.i32.i.i.i.i = add nuw i8 %_19.i.i5.us.i.i29.i.i.i.i127, 1
  %_15.i.i.us.i.i30.i.i.i.i = icmp ult i8 %_19.i.i.us.i.i32.i.i.i.i, 0
  br label %bb3.i.i.us.i.i31.i.i.i.i

_RINvXs_NtNtNtCsc9cpkn6bpzE_4core4iter8adapters5chainINtB5_5ChainINtNtB7_7flatten7FlattenINtNtBb_6option8IntoIterNtNtBb_4char11EscapeDebugEEINtB13_7FlatMapNtNtNtBb_3str4iter5CharsB1M_NtB2w_23CharEscapeDebugContinueEENtNtNtB9_6traits8iterator8Iterator8try_folduNCINvNvB3r_12try_for_each4callcINtNtBb_6result6ResultuNtNtBb_3fmt5ErrorENCNvXs20_B2u_NtB2u_11EscapeDebugNtB53_7Display3fmt0E0B4E_EBb_.exit: ; preds = %bb3.i.i.us.i.i.i.i.i52.i, %bb3.i.i.i.i.i.i.i42.i.lr.ph, %bb9.i, %bb3.i.i.us.i.i32.i.i.i.i
  %_0.sroa.0.0.off0.i = phi i1 [ false, %bb3.i.i.us.i.i.i.i.i52.i ], [ false, %bb9.i ], [ false, %bb3.i.i.i.i.i.i.i42.i.lr.ph ], [ true, %bb3.i.i.us.i.i32.i.i.i.i ]
  ret i1 %_0.sroa.0.0.off0.i
}

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p0(ptr captures(none)) #1

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.end.p0(ptr captures(none)) #1

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write)
declare void @llvm.assume(i1 noundef) #2

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p0.p0.i32(ptr noalias writeonly captures(none), ptr noalias readonly captures(none), i32, i1 immarg) #3

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: write)
declare void @llvm.memset.p0.i32(ptr writeonly captures(none), i8, i32, i1 immarg) #4

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.ctlz.i32(i32, i1 immarg) #5

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: readwrite)
declare void @llvm.experimental.noalias.scope.decl(metadata) #6

; Function Attrs: nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none)
declare i8 @llvm.umax.i8(i8, i8) #7

attributes #0 = { nounwind "frame-pointer"="non-leaf" "target-cpu"="generic" "target-features"="+strict-align" }
attributes #1 = { nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #2 = { nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write) }
attributes #3 = { nocallback nofree nounwind willreturn memory(argmem: readwrite) }
attributes #4 = { nocallback nofree nounwind willreturn memory(argmem: write) }
attributes #5 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
attributes #6 = { nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: readwrite) }
attributes #7 = { nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none) }
attributes #8 = { inlinehint nounwind }
