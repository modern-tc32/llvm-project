//===-- ArchitecturePluginTest.cpp ---------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Plugins/Architecture/Arm/ArchitectureArm.h"
#include "lldb/Core/PluginManager.h"
#include "lldb/Utility/ArchSpec.h"
#include "gtest/gtest.h"

using namespace lldb_private;

namespace {

class ArchitectureArmRegistration {
public:
  ArchitectureArmRegistration() { ArchitectureArm::Initialize(); }
  ~ArchitectureArmRegistration() { ArchitectureArm::Terminate(); }
};

} // namespace

TEST(ArchitecturePluginTest, CreatesArmPluginForTC32Targets) {
  ArchitectureArmRegistration registration;

  auto plugin = PluginManager::CreateArchitectureInstance(
      ArchSpec("tc32-unknown-none-elf"));

  ASSERT_NE(plugin, nullptr);
  EXPECT_EQ(plugin->GetPluginName(), ArchitectureArm::GetPluginNameStatic());
}
