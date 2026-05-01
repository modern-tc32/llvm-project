// RUN: %clang -### %s --target=tc32-unknown-none-elf \
// RUN:   -nostartfiles -nostdlib -fuse-ld=lld -B%S/Inputs/lld -o %t 2>&1 \
// RUN:   | FileCheck %s

// CHECK: "{{[^"]*}}ld.lld" "-Bstatic" "-m" "tc32elf"
// CHECK-SAME: "-o" "{{.*}}"

// RUN: %clang -### %s --target=tc32-unknown-none-elf -mcpu=tc32 -c 2>&1 \
// RUN:   | FileCheck --check-prefix=MCPU %s

// MCPU-NOT: unsupported option '-mcpu='
// MCPU: "-target-cpu" "tc32"
