// RUN: %clang_cc1 -triple tc32 -ffunction-sections -fdata-sections -emit-llvm -o - %s | FileCheck %s

void f(void) {}
int g;

// CHECK: !llvm.module.flags = !{
// CHECK-DAG: [[FS:![0-9]+]] = !{i32 1, !"function-sections", i32 1}
// CHECK-DAG: [[DS:![0-9]+]] = !{i32 1, !"data-sections", i32 1}
