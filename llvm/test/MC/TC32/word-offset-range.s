# RUN: split-file %s %t
# RUN: llvm-mc -triple=tc32 -show-encoding %t/valid.s | FileCheck %s --check-prefix=ENC
# RUN: not llvm-mc -triple=tc32 %t/invalid.s 2>&1 | FileCheck %s --check-prefix=ERR

#--- valid.s
        tloadr r2, [r6, #124]
        tstorer r2, [r6, #124]

# ENC: tloadr r2, [r6, #124]
# ENC-SAME: encoding: [0xf2,0x5f]
# ENC: tstorer r2, [r6, #124]
# ENC-SAME: encoding: [0xf2,0x57]

#--- invalid.s
        tloadr r2, [r6, #128]
        tstorer r2, [r6, #128]

# ERR: error: word offset must be 0..124 step 4
# ERR: tloadr r2, [r6, #128]
# ERR: error: word offset must be 0..124 step 4
# ERR: tstorer r2, [r6, #128]
