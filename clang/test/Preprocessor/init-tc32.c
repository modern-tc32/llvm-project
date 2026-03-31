// RUN: %clang_cc1 -triple tc32 -E -dM %s | FileCheck %s

// CHECK: #define __CHAR_UNSIGNED__ 1
// CHECK: #define __DBL_MANT_DIG__ 53
// CHECK: #define __LDBL_MANT_DIG__ 53
// CHECK: #define __SIZEOF_DOUBLE__ 8
// CHECK: #define __SIZEOF_LONG_DOUBLE__ 8
// CHECK: #define __TC32__ 1
// CHECK: #define __tc32__ 1
