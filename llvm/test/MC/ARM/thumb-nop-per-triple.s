// The nop mnemonic has a different encoding per target: TC32 uses 0x06c0,
// v6-M uses the hint 0xbf00, and other Thumb1 targets use mov r8, r8 (0x46c0).
// 0x06c0 is lsls r0, r0, #27 on ARM Thumb1, so TC32's nop must not leak there.

// RUN: llvm-mc -triple=thumbv4t-none-eabi -show-encoding %s | FileCheck %s --check-prefix=V4T
// RUN: llvm-mc -triple=thumbv6m-none-eabi -show-encoding %s | FileCheck %s --check-prefix=V6M
// RUN: llvm-mc -triple=tc32 -show-encoding %s | FileCheck %s --check-prefix=TC32

// V4T: mov r8, r8 @ encoding: [0xc0,0x46]
// V6M: nop @ encoding: [0x00,0xbf]
// TC32: nop @ encoding: [0xc0,0x06]
nop
