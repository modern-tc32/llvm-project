// RUN: %clang_cc1 -triple tc32 -emit-llvm -w -o - %s | FileCheck %s

// CHECK: target datalayout = "e-m:e-p:32:32-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f32:32:32-f64:32:32-n32-S32"

int check_float(void) {
  return sizeof(float);
// CHECK: ret i32 4
}

int check_double(void) {
  return sizeof(double);
// CHECK: ret i32 8
}

int check_longdouble(void) {
  return sizeof(long double);
// CHECK: ret i32 8
}

int check_double_align(void) {
  return __alignof__(double);
// CHECK: ret i32 4
}

int check_longdouble_align(void) {
  return __alignof__(long double);
// CHECK: ret i32 4
}

int check_struct_with_double(void) {
  return sizeof(struct { char c; double d; });
// CHECK: ret i32 12
}
