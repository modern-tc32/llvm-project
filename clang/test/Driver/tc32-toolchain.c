// RUN: %clang -### %s --target=tc32-unknown-none-elf \
// RUN:   -nostartfiles -nostdlib -fuse-ld=lld -B%S/Inputs/lld -o %t 2>&1 \
// RUN:   | FileCheck %s

// CHECK: "{{[^"]*}}ld.lld" "-Bstatic" "-m" "tc32elf"
// CHECK-SAME: "-o" "{{.*}}"
