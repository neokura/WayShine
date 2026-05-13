/**
 * @file tests/unit/test_linux_virtual_display.cpp
 * @brief Test WayShine Linux virtual display helpers.
 */
#include "../tests_common.h"

#ifdef __linux__
  #include <src/platform/linux/virtual_display.h>

  #include <algorithm>

using namespace display_device::linux_vdisplay;

TEST(LinuxVirtualDisplayTest, ParseMode) {
  const auto mode = parse_mode("2560x1440@120");

  ASSERT_TRUE(mode);
  EXPECT_EQ(mode->width, 2560);
  EXPECT_EQ(mode->height, 1440);
  EXPECT_EQ(mode->refresh_hz, 120);
  EXPECT_EQ(mode_to_string(*mode), "2560x1440@120");
}

TEST(LinuxVirtualDisplayTest, RejectInvalidMode) {
  EXPECT_FALSE(parse_mode("2560x1440"));
  EXPECT_FALSE(parse_mode("2560@120"));
  EXPECT_FALSE(parse_mode("not-a-mode"));
}

TEST(LinuxVirtualDisplayTest, DefaultEdidChecksumIsValid) {
  EXPECT_TRUE(edid_checksum_valid(LinuxVirtualDisplayProvisioner::default_profile().edid));
}

TEST(LinuxVirtualDisplayTest, DefaultProfileContainsFiniteModes) {
  const auto &profile = LinuxVirtualDisplayProvisioner::default_profile();

  EXPECT_EQ(profile.name, "sdr-default");
  ASSERT_FALSE(profile.modes.empty());
  EXPECT_NE(std::ranges::find_if(profile.modes, [](const auto &mode) {
    return mode.width == 1920 && mode.height == 1080 && mode.refresh_hz == 120;
  }),
            profile.modes.end());
}
#endif
