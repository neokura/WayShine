/**
 * @file src/platform/linux/virtual_display.cpp
 * @brief Linux virtual display helpers for WayShine.
 */
#include "src/platform/linux/virtual_display.h"

#include "src/config.h"
#include "src/logging.h"

#include <boost/algorithm/string.hpp>
#include <boost/process/v1/child.hpp>
#include <boost/process/v1/io.hpp>
#include <boost/process/v1/search_path.hpp>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <sys/wait.h>

namespace display_device::linux_vdisplay {
  namespace bp = boost::process::v1;
  using namespace std::literals;

  namespace {
    constexpr auto DEFAULT_PROFILE_NAME = "sdr-default"sv;
    constexpr auto DEFAULT_EDID_NAME = "wayshine-sdr-default.bin"sv;

    std::vector<std::byte> make_default_edid() {
      std::vector<std::byte> edid {
        std::byte {0x00}, std::byte {0xff}, std::byte {0xff}, std::byte {0xff}, std::byte {0xff}, std::byte {0xff}, std::byte {0xff}, std::byte {0x00},
        std::byte {0x5a}, std::byte {0x63}, std::byte {0x01}, std::byte {0x00}, std::byte {0x01}, std::byte {0x00}, std::byte {0x00}, std::byte {0x00},
        std::byte {0x01}, std::byte {0x22}, std::byte {0x01}, std::byte {0x04}, std::byte {0xa5}, std::byte {0x3c}, std::byte {0x22}, std::byte {0x78},
        std::byte {0x3a}, std::byte {0xee}, std::byte {0x95}, std::byte {0xa3}, std::byte {0x54}, std::byte {0x4c}, std::byte {0x99}, std::byte {0x26},
        std::byte {0x0f}, std::byte {0x50}, std::byte {0x54}, std::byte {0x21}, std::byte {0x08}, std::byte {0x00}, std::byte {0xd1}, std::byte {0xc0},
        std::byte {0x81}, std::byte {0x80}, std::byte {0x81}, std::byte {0x40}, std::byte {0x81}, std::byte {0xc0}, std::byte {0x95}, std::byte {0x00},
        std::byte {0xb3}, std::byte {0x00}, std::byte {0xa9}, std::byte {0x40}, std::byte {0x01}, std::byte {0x01}, std::byte {0x08}, std::byte {0xe8},
        std::byte {0x00}, std::byte {0x30}, std::byte {0xf2}, std::byte {0x70}, std::byte {0x5a}, std::byte {0x80}, std::byte {0xb0}, std::byte {0x58},
        std::byte {0x8a}, std::byte {0x00}, std::byte {0x58}, std::byte {0x54}, std::byte {0x21}, std::byte {0x00}, std::byte {0x00}, std::byte {0x1e},
        std::byte {0x04}, std::byte {0x74}, std::byte {0x00}, std::byte {0x30}, std::byte {0xf2}, std::byte {0x70}, std::byte {0x5a}, std::byte {0x80},
        std::byte {0xb0}, std::byte {0x58}, std::byte {0x8a}, std::byte {0x00}, std::byte {0x58}, std::byte {0x54}, std::byte {0x21}, std::byte {0x00},
        std::byte {0x00}, std::byte {0x1e}, std::byte {0x00}, std::byte {0x00}, std::byte {0x00}, std::byte {0xfc}, std::byte {0x00}, std::byte {0x57},
        std::byte {0x61}, std::byte {0x79}, std::byte {0x53}, std::byte {0x68}, std::byte {0x69}, std::byte {0x6e}, std::byte {0x65}, std::byte {0x20},
        std::byte {0x53}, std::byte {0x44}, std::byte {0x52}, std::byte {0x0a}, std::byte {0x00}, std::byte {0x00}, std::byte {0x00}, std::byte {0xfd},
        std::byte {0x00}, std::byte {0x30}, std::byte {0x78}, std::byte {0x1e}, std::byte {0xff}, std::byte {0x3c}, std::byte {0x01}, std::byte {0x0a},
        std::byte {0x20}, std::byte {0x20}, std::byte {0x20}, std::byte {0x20}, std::byte {0x20}, std::byte {0x20}, std::byte {0x00}, std::byte {0x00}
      };

      unsigned int sum = 0;
      for (std::size_t i = 0; i + 1 < edid.size(); ++i) {
        sum += std::to_integer<unsigned int>(edid[i]);
      }
      edid.back() = std::byte {static_cast<unsigned char>((256 - (sum & 0xff)) & 0xff)};
      return edid;
    }

    std::string shell_quote(const std::string &value) {
      std::string quoted {"'"};
      for (const char ch : value) {
        if (ch == '\'') {
          quoted += "'\\''";
        } else {
          quoted += ch;
        }
      }
      quoted += "'";
      return quoted;
    }

    std::string run_capture(const std::string &cmd, int *exit_code = nullptr) {
      std::array<char, 256> buffer {};
      std::string output;
      FILE *pipe = ::popen((cmd + " 2>&1").c_str(), "r");
      if (!pipe) {
        if (exit_code) {
          *exit_code = errno ? errno : 1;
        }
        return {};
      }

      while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe)) {
        output += buffer.data();
      }

      const int status = ::pclose(pipe);
      if (exit_code) {
        *exit_code = status >= 0 && WIFEXITED(status) ? WEXITSTATUS(status) : status;
      }
      return output;
    }

    bool command_exists(const std::string &command) {
      return !bp::search_path(command).empty();
    }

    std::vector<std::string> read_lines(const std::filesystem::path &path) {
      std::ifstream file {path};
      std::vector<std::string> lines;
      std::string line;
      while (std::getline(file, line)) {
        boost::algorithm::trim(line);
        if (!line.empty()) {
          lines.emplace_back(std::move(line));
        }
      }
      return lines;
    }

    std::filesystem::path drm_path_for_connector(const std::string &connector) {
      return std::filesystem::path {"/sys/class/drm"} / connector;
    }

    std::string refresh_to_string(const unsigned int refresh_hz) {
      return std::to_string(refresh_hz);
    }

    bool refresh_matches(const LinuxVirtualDisplayMode &expected, const LinuxVirtualDisplayMode &actual) {
      return expected.width == actual.width && expected.height == actual.height && expected.refresh_hz == actual.refresh_hz;
    }

    std::string build_profile_metadata(const std::string &connector, const LinuxVirtualDisplayProfile &profile) {
      std::ostringstream out;
      out << "{\n";
      out << "  \"name\": \"" << profile.name << "\",\n";
      out << "  \"connector\": \"" << connector << "\",\n";
      out << "  \"edid\": \"" << profile.edid_filename << "\",\n";
      out << "  \"mode_policy\": \"exact\",\n";
      out << "  \"modes\": [";
      for (std::size_t i = 0; i < profile.modes.size(); ++i) {
        if (i != 0) {
          out << ", ";
        }
        out << "\"" << mode_to_string(profile.modes[i]) << "\"";
      }
      out << "]\n";
      out << "}\n";
      return out.str();
    }

    std::string kscreen_output_name_for_command(const Output &output) {
      return output.name.empty() ? output.id : output.name;
    }

    std::optional<LinuxVirtualDisplayMode> find_mode_in_output(const Output &output, const LinuxVirtualDisplayMode &mode) {
      const auto it = std::ranges::find_if(output.modes, [&](const auto &candidate) {
        return refresh_matches(mode, candidate);
      });
      if (it == output.modes.end()) {
        return std::nullopt;
      }
      return *it;
    }

    std::string json_escape(const std::string &value) {
      std::string escaped;
      for (const char ch : value) {
        switch (ch) {
          case '\\':
            escaped += "\\\\";
            break;
          case '"':
            escaped += "\\\"";
            break;
          case '\n':
            escaped += "\\n";
            break;
          case '\r':
            escaped += "\\r";
            break;
          case '\t':
            escaped += "\\t";
            break;
          default:
            escaped += ch;
            break;
        }
      }
      return escaped;
    }
  }  // namespace

  std::string mode_to_string(const LinuxVirtualDisplayMode &mode) {
    return std::to_string(mode.width) + "x" + std::to_string(mode.height) + "@" + refresh_to_string(mode.refresh_hz);
  }

  std::optional<LinuxVirtualDisplayMode> parse_mode(std::string_view text) {
    const std::regex pattern {R"(^\s*(\d+)x(\d+)@(\d+)(?:\.\d+)?\s*$)"};
    std::cmatch match;
    const std::string value {text};
    if (!std::regex_match(value.c_str(), match, pattern)) {
      return std::nullopt;
    }

    return LinuxVirtualDisplayMode {
      static_cast<unsigned int>(std::stoul(match[1].str())),
      static_cast<unsigned int>(std::stoul(match[2].str())),
      static_cast<unsigned int>(std::stoul(match[3].str()))
    };
  }

  bool edid_checksum_valid(const std::vector<std::byte> &edid) {
    if (edid.empty() || edid.size() % 128 != 0) {
      return false;
    }

    for (std::size_t block = 0; block < edid.size(); block += 128) {
      unsigned int sum = 0;
      for (std::size_t i = block; i < block + 128; ++i) {
        sum += std::to_integer<unsigned int>(edid[i]);
      }
      if ((sum & 0xff) != 0) {
        return false;
      }
    }
    return true;
  }

  const LinuxVirtualDisplayProfile &LinuxVirtualDisplayProvisioner::default_profile() {
    static const LinuxVirtualDisplayProfile profile {
      .name = std::string {DEFAULT_PROFILE_NAME},
      .edid_filename = std::string {DEFAULT_EDID_NAME},
      .modes = {
        {1920, 1080, 60},
        {1920, 1080, 120},
        {2560, 1440, 60},
        {2560, 1440, 120},
        {3840, 2160, 60}
      },
      .edid = make_default_edid()
    };
    return profile;
  }

  Expected<void> LinuxVirtualDisplayProvisioner::install(const std::string &connector, const std::string &profile_name) {
    if (profile_name != DEFAULT_PROFILE_NAME) {
      return "Only the sdr-default EDID profile is available in P0."s;
    }
    if (connector.empty()) {
      return "A DRM connector name is required, for example DP-2."s;
    }
    if (command_exists("rpm-ostree") && !command_exists("grubby")) {
      return "Atomic rpm-ostree systems are partially supported in P0. Use --linux-vdisplay-doctor and apply EDID firmware, initramfs, and kernel args manually until persistence has been verified."s;
    }

    const auto &profile = default_profile();
    if (!edid_checksum_valid(profile.edid)) {
      return "Bundled WayShine EDID profile failed checksum validation."s;
    }

    std::error_code ec;
    const std::filesystem::path edid_dir {"/lib/firmware/edid"};
    std::filesystem::create_directories(edid_dir, ec);
    if (ec) {
      return "Failed to create /lib/firmware/edid: "s + ec.message();
    }

    const auto edid_path = edid_dir / profile.edid_filename;
    std::ofstream edid_file {edid_path, std::ios::binary};
    if (!edid_file) {
      return "Failed to open "s + edid_path.string() + " for writing. Run this command with sudo.";
    }
    for (const auto byte : profile.edid) {
      edid_file.put(static_cast<char>(std::to_integer<unsigned char>(byte)));
    }
    edid_file.close();

    const auto metadata_path = edid_dir / (profile.name + ".json");
    std::ofstream metadata_file {metadata_path};
    if (!metadata_file) {
      return "Failed to write "s + metadata_path.string();
    }
    metadata_file << build_profile_metadata(connector, profile);

    const auto drm_arg = "drm.edid_firmware="s + connector + ":edid/"s + profile.edid_filename;
    const auto video_arg = "video="s + connector + ":e"s;

    if (command_exists("grubby")) {
      int exit_code = 0;
      const auto output = run_capture("grubby --update-kernel=ALL --args=" + shell_quote(drm_arg + " " + video_arg), &exit_code);
      if (exit_code != 0) {
        return "grubby failed while adding kernel args:\n"s + output;
      }
    } else if (command_exists("rpm-ostree")) {
      return "Atomic rpm-ostree systems are partially supported in P0. EDID files were written, but kargs/initramfs persistence must be applied manually and verified with the doctor."s;
    } else {
      return "EDID files were written, but no supported kernel-arg tool was found. Add these kernel args manually: "s + drm_arg + " " + video_arg;
    }

    if (command_exists("dracut")) {
      int exit_code = 0;
      const auto output = run_capture("dracut -f", &exit_code);
      if (exit_code != 0) {
        return "dracut failed while rebuilding initramfs:\n"s + output;
      }
    }

    std::cout << "WayShine virtual display EDID installed for " << connector << ". Reboot, then run --linux-vdisplay-doctor.\n";
    return {};
  }

  Expected<void> LinuxVirtualDisplayProvisioner::remove(const std::string &connector) {
    if (connector.empty()) {
      return "A DRM connector name is required, for example DP-2."s;
    }

    const auto &profile = default_profile();
    const auto drm_arg = "drm.edid_firmware="s + connector + ":edid/"s + profile.edid_filename;
    const auto video_arg = "video="s + connector + ":e"s;

    if (command_exists("grubby")) {
      int exit_code = 0;
      const auto output = run_capture("grubby --update-kernel=ALL --remove-args=" + shell_quote(drm_arg + " " + video_arg), &exit_code);
      if (exit_code != 0) {
        return "grubby failed while removing kernel args:\n"s + output;
      }
    } else {
      std::cout << "Remove these kernel args manually: " << drm_arg << " " << video_arg << "\n";
    }

    std::error_code ec;
    std::filesystem::remove(std::filesystem::path {"/lib/firmware/edid"} / profile.edid_filename, ec);
    std::filesystem::remove(std::filesystem::path {"/lib/firmware/edid"} / (profile.name + ".json"), ec);
    std::cout << "WayShine virtual display EDID removal requested. Rebuild initramfs if needed and reboot.\n";
    return {};
  }

  Expected<OutputList> KscreenDoctorBackend::enumerateOutputs() {
    if (!command_exists("kscreen-doctor")) {
      return "kscreen-doctor was not found in PATH."s;
    }

    int exit_code = 0;
    const auto output = run_capture("kscreen-doctor -o", &exit_code);
    if (exit_code != 0) {
      return "kscreen-doctor -o failed:\n"s + output;
    }

    OutputList outputs;
    Output *current = nullptr;
    std::istringstream lines {output};
    std::string line;
    const std::regex output_line {R"(^\s*Output:\s+(\S+)\s+(\S+).*)"};
    const std::regex mode_line {R"((\d+):(\d+)x(\d+)@(\d+)(?:\.\d+)?)"};

    while (std::getline(lines, line)) {
      std::smatch match;
      if (std::regex_match(line, match, output_line)) {
        outputs.push_back({
          .id = match[1].str(),
          .name = match[2].str(),
          .enabled = line.find("enabled") != std::string::npos,
          .connected = line.find("connected") != std::string::npos,
          .primary = line.find("primary") != std::string::npos
        });
        current = &outputs.back();
        continue;
      }

      if (!current) {
        continue;
      }

      current->enabled = current->enabled || line.find("enabled") != std::string::npos;
      current->connected = current->connected || line.find("connected") != std::string::npos;
      current->primary = current->primary || line.find("primary") != std::string::npos;

      for (std::sregex_iterator it {line.begin(), line.end(), mode_line}, end; it != end; ++it) {
        LinuxVirtualDisplayMode mode {
          static_cast<unsigned int>(std::stoul((*it)[2].str())),
          static_cast<unsigned int>(std::stoul((*it)[3].str())),
          static_cast<unsigned int>(std::stoul((*it)[4].str()))
        };
        current->modes.push_back(mode);
        current->mode_ids.push_back(std::stoi((*it)[1].str()));
        if (line.find((*it)[0].str() + "*") != std::string::npos || line.find((*it)[0].str() + "!") != std::string::npos) {
          current->applied_mode = mode;
        }
      }
    }

    return outputs;
  }

  Expected<void> KscreenDoctorBackend::applyAtomic(const DisplayTransaction &transaction) {
    const auto outputs = enumerateOutputs();
    if (!outputs) {
      return outputs.error();
    }

    auto target = std::ranges::find_if(outputs.value(), [&](const auto &output) {
      return output.name == transaction.target_output || output.id == transaction.target_output;
    });
    if (target == outputs.value().end()) {
      return "KScreen output not found: "s + transaction.target_output;
    }

    const auto mode_it = std::ranges::find_if(target->modes, [&](const auto &mode) {
      return refresh_matches(transaction.mode, mode);
    });
    if (mode_it == target->modes.end()) {
      return "KScreen output "s + transaction.target_output + " does not expose requested mode "s + mode_to_string(transaction.mode);
    }
    const auto mode_offset = static_cast<std::size_t>(std::distance(target->modes.begin(), mode_it));
    const auto mode_id = mode_offset < target->mode_ids.size() ? target->mode_ids[mode_offset] : static_cast<int>(mode_offset);
    const auto target_name = kscreen_output_name_for_command(*target);

    std::ostringstream cmd;
    cmd << "kscreen-doctor"
        << " output." << shell_quote(target_name) << ".enable"
        << " output." << shell_quote(target_name) << ".mode." << mode_id
        << " output." << shell_quote(target_name) << ".position.0,0";
    if (transaction.make_primary || transaction.only_display) {
      cmd << " output." << shell_quote(target_name) << ".primary";
    }
    if (transaction.only_display) {
      for (const auto &output : outputs.value()) {
        const auto name = kscreen_output_name_for_command(output);
        if (name != target_name) {
          cmd << " output." << shell_quote(name) << ".disable";
        }
      }
    }

    int exit_code = 0;
    const auto out = run_capture(cmd.str(), &exit_code);
    if (exit_code != 0) {
      return "kscreen-doctor apply failed:\n"s + out;
    }

    return {};
  }

  Expected<void> KscreenDoctorBackend::restore(const DisplaySnapshot &snapshot) {
    if (snapshot.outputs.empty()) {
      return {};
    }

    std::ostringstream cmd;
    cmd << "kscreen-doctor";
    for (const auto &output : snapshot.outputs) {
      const auto name = kscreen_output_name_for_command(output);
      cmd << " output." << shell_quote(name) << (output.enabled ? ".enable" : ".disable");
      if (output.enabled && output.applied_mode) {
        auto mode_index = std::ranges::find_if(output.modes, [&](const auto &mode) {
          return refresh_matches(mode, *output.applied_mode);
        });
        if (mode_index != output.modes.end()) {
          const auto mode_offset = static_cast<std::size_t>(std::distance(output.modes.begin(), mode_index));
          const auto mode_id = mode_offset < output.mode_ids.size() ? output.mode_ids[mode_offset] : static_cast<int>(mode_offset);
          cmd << " output." << shell_quote(name) << ".mode." << mode_id;
        }
      }
      if (output.primary) {
        cmd << " output." << shell_quote(name) << ".primary";
      }
    }

    int exit_code = 0;
    const auto out = run_capture(cmd.str(), &exit_code);
    if (exit_code != 0) {
      return "kscreen-doctor restore failed:\n"s + out;
    }

    return {};
  }

  LinuxDisplaySettingsManager::LinuxDisplaySettingsManager(const config::video_t &video_config):
      _video_config(video_config),
      _backend(std::make_unique<KscreenDoctorBackend>()) {}

  EnumeratedDeviceList LinuxDisplaySettingsManager::enumAvailableDevices() const {
    auto *backend = const_cast<LinuxDisplayControlBackend *>(_backend.get());
    const auto outputs = backend->enumerateOutputs();
    if (!outputs) {
      return {};
    }

    EnumeratedDeviceList devices;
    for (const auto &output : outputs.value()) {
      EnumeratedDevice device;
      device.m_device_id = output.name;
      device.m_display_name = output.name;
      device.m_friendly_name = output.name.empty() ? output.id : output.name;
      if (output.enabled && output.applied_mode) {
        device.m_info = EnumeratedDevice::Info {
          .m_resolution = {output.applied_mode->width, output.applied_mode->height},
          .m_resolution_scale = Rational {1, 1},
          .m_refresh_rate = Rational {output.applied_mode->refresh_hz, 1},
          .m_primary = output.primary,
          .m_origin_point = {},
          .m_hdr_state = HdrState::Disabled
        };
      }
      devices.emplace_back(std::move(device));
    }
    return devices;
  }

  std::string LinuxDisplaySettingsManager::getDisplayName(const std::string &device_id) const {
    if (device_id.empty()) {
      return {};
    }

    auto *backend = const_cast<LinuxDisplayControlBackend *>(_backend.get());
    const auto outputs = backend->enumerateOutputs();
    if (!outputs) {
      return {};
    }

    auto matches = 0;
    std::string display_name;
    for (const auto &output : outputs.value()) {
      if (output.name == device_id || output.id == device_id) {
        ++matches;
        display_name = output.name;
      }
    }

    if (matches != 1) {
      return {};
    }
    return display_name;
  }

  SettingsManagerInterface::ApplyResult LinuxDisplaySettingsManager::applySettings(const SingleDisplayConfiguration &config) {
    const auto lease = prepareSession(config);
    if (!lease) {
      BOOST_LOG(error) << "[WayShine] Linux virtual display preflight failed: " << lease.error();
      return ApplyResult::DisplayModePrepFailed;
    }

    BOOST_LOG(info) << "[WayShine] Virtual display lease active on " << _lease->connector() << " at " << mode_to_string(_lease->requested_mode());
    return ApplyResult::Ok;
  }

  Expected<VirtualDisplayLease> LinuxDisplaySettingsManager::prepareSession(const SingleDisplayConfiguration &config) {
    const auto lease = prepareVirtualDisplay(config);
    if (lease) {
      _lease = lease.value();
    }
    return lease;
  }

  SettingsManagerInterface::RevertResult LinuxDisplaySettingsManager::revertSettings() {
    if (!_snapshot) {
      _lease.reset();
      return RevertResult::Ok;
    }

    const auto result = _backend->restore(*_snapshot);
    if (!result) {
      BOOST_LOG(error) << "[WayShine] Failed to restore Linux virtual display snapshot: " << result.error();
      return RevertResult::SwitchingTopologyFailed;
    }

    _snapshot.reset();
    _lease.reset();
    return RevertResult::Ok;
  }

  bool LinuxDisplaySettingsManager::resetPersistence() {
    _snapshot.reset();
    _lease.reset();
    return true;
  }

  Expected<VirtualDisplayLease> LinuxDisplaySettingsManager::prepareVirtualDisplay(const SingleDisplayConfiguration &config) {
    if (!_video_config.linux_vdisplay.enabled) {
      return VirtualDisplayLease {};
    }

    if (_video_config.linux_vdisplay.backend != "kscreen_doctor") {
      return "Unsupported Linux virtual display backend: "s + _video_config.linux_vdisplay.backend;
    }
    if (_video_config.linux_vdisplay.mode_policy != "exact") {
      return "WayShine P0 only supports linux_vdisplay_mode_policy=exact."s;
    }
    if (config.m_hdr_state == HdrState::Enabled) {
      return "HDR is not supported by WayShine Linux virtual display v1."s;
    }

    const auto requested_mode = resolveRequestedMode(config);
    if (!requested_mode) {
      return requested_mode.error();
    }
    if (!profileContainsMode(requested_mode.value())) {
      return "Requested mode "s + mode_to_string(requested_mode.value()) + " is not declared in profile "s + _video_config.linux_vdisplay.profile;
    }

    const auto connector = config.m_device_id.empty() ? _video_config.linux_vdisplay.connector : config.m_device_id;
    if (connector.empty()) {
      return "linux_vdisplay_connector or output_name must be set."s;
    }
    if (config.m_device_id.empty() &&
        config.m_device_prep != SingleDisplayConfiguration::DevicePreparation::EnsurePrimary &&
        config.m_device_prep != SingleDisplayConfiguration::DevicePreparation::EnsureOnlyDisplay) {
      return "Strict capture mapping requires output_name to be set to the virtual connector, or dd_configuration_option to make the virtual output primary/only display."s;
    }
    if (!sysfsConnectorContainsMode(connector, requested_mode.value())) {
      return "DRM connector "s + connector + " does not expose requested EDID mode "s + mode_to_string(requested_mode.value());
    }

    const auto outputs = _backend->enumerateOutputs();
    if (!outputs) {
      return outputs.error();
    }
    const auto target = findTargetOutput(outputs.value(), connector);
    if (!target) {
      return target.error();
    }
    if (!find_mode_in_output(target.value(), requested_mode.value())) {
      return "KScreen output "s + target.value().name + " does not expose requested mode "s + mode_to_string(requested_mode.value());
    }

    _snapshot = DisplaySnapshot {outputs.value()};

    const bool only_display = config.m_device_prep == SingleDisplayConfiguration::DevicePreparation::EnsureOnlyDisplay;
    const bool make_primary = only_display || config.m_device_prep == SingleDisplayConfiguration::DevicePreparation::EnsurePrimary;
    const DisplayTransaction transaction {
      .target_output = target.value().name,
      .mode = requested_mode.value(),
      .make_primary = make_primary,
      .only_display = only_display
    };

    BOOST_LOG(info) << "[WayShine] Strict virtual display mapping:"
                    << " drm_connector=" << connector
                    << " kscreen_output=" << target.value().name
                    << " sunshine_capture_output=" << target.value().name
                    << " requested_mode=" << mode_to_string(requested_mode.value());

    const auto apply = _backend->applyAtomic(transaction);
    if (!apply) {
      return apply.error();
    }

    const auto verified_outputs = _backend->enumerateOutputs();
    if (!verified_outputs) {
      return verified_outputs.error();
    }
    const auto verified_target = findTargetOutput(verified_outputs.value(), connector);
    if (!verified_target) {
      return verified_target.error();
    }
    if (!verified_target.value().applied_mode || !refresh_matches(requested_mode.value(), *verified_target.value().applied_mode)) {
      return "Applied mode could not be verified. requested="s + mode_to_string(requested_mode.value()) + " applied="s +
             (verified_target.value().applied_mode ? mode_to_string(*verified_target.value().applied_mode) : "unknown"s);
    }

    BOOST_LOG(info) << "[WayShine] Verified virtual display mode: " << mode_to_string(*verified_target.value().applied_mode);
    return VirtualDisplayLease {connector, requested_mode.value()};
  }

  Expected<LinuxVirtualDisplayMode> LinuxDisplaySettingsManager::resolveRequestedMode(const SingleDisplayConfiguration &config) const {
    if (!config.m_resolution || !config.m_refresh_rate) {
      return "Exact mode policy requires both resolution and refresh rate."s;
    }

    unsigned int refresh = 0;
    if (const auto *rational = std::get_if<Rational>(&*config.m_refresh_rate)) {
      if (rational->m_denominator == 0) {
        return "Invalid refresh rate denominator."s;
      }
      refresh = static_cast<unsigned int>((rational->m_numerator + rational->m_denominator / 2) / rational->m_denominator);
    } else {
      refresh = static_cast<unsigned int>(std::get<double>(*config.m_refresh_rate) + 0.5);
    }

    return LinuxVirtualDisplayMode {
      config.m_resolution->m_width,
      config.m_resolution->m_height,
      refresh
    };
  }

  Expected<Output> LinuxDisplaySettingsManager::findTargetOutput(const OutputList &outputs, const std::string &device_id) const {
    std::vector<Output> matches;
    for (const auto &output : outputs) {
      if (output.name == device_id || output.id == device_id) {
        matches.push_back(output);
      }
    }

    if (matches.empty()) {
      return "No KScreen output maps to DRM connector "s + device_id;
    }
    if (matches.size() > 1) {
      return "Ambiguous KScreen mapping for DRM connector "s + device_id;
    }
    return matches.front();
  }

  bool LinuxDisplaySettingsManager::profileContainsMode(const LinuxVirtualDisplayMode &mode) const {
    const auto &profile = LinuxVirtualDisplayProvisioner::default_profile();
    if (_video_config.linux_vdisplay.profile != profile.name) {
      return false;
    }
    return std::ranges::any_of(profile.modes, [&](const auto &candidate) {
      return refresh_matches(mode, candidate);
    });
  }

  bool LinuxDisplaySettingsManager::sysfsConnectorContainsMode(const std::string &connector, const LinuxVirtualDisplayMode &mode) const {
    const auto modes = read_lines(drm_path_for_connector(connector) / "modes");
    const auto expected_prefix = std::to_string(mode.width) + "x" + std::to_string(mode.height);
    return std::ranges::any_of(modes, [&](const auto &line) {
      return line == expected_prefix || line.starts_with(expected_prefix + "@");
    });
  }

  LinuxVirtualDisplayDoctor::LinuxVirtualDisplayDoctor(const config::video_t &video_config):
      _video_config(video_config) {}

  int LinuxVirtualDisplayDoctor::run(bool json_output) const {
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    const auto &profile = LinuxVirtualDisplayProvisioner::default_profile();
    const auto connector = _video_config.linux_vdisplay.connector;

    if (connector.empty()) {
      errors.emplace_back("linux_vdisplay_connector is not configured.");
    } else {
      const auto path = drm_path_for_connector(connector);
      if (!std::filesystem::exists(path)) {
        errors.emplace_back("DRM connector not found: " + connector);
      } else {
        const auto status = read_lines(path / "status");
        if (!status.empty() && status.front() != "connected") {
          warnings.emplace_back("DRM connector " + connector + " status is " + status.front() + ".");
        }
        const auto modes = read_lines(path / "modes");
        for (const auto &mode : profile.modes) {
          const auto expected = std::to_string(mode.width) + "x" + std::to_string(mode.height);
          if (std::ranges::find(modes, expected) == modes.end()) {
            errors.emplace_back("EDID mode missing from /sys/class/drm/" + connector + "/modes: " + mode_to_string(mode));
          }
        }
      }
    }

    if (!edid_checksum_valid(profile.edid)) {
      errors.emplace_back("Bundled EDID profile checksum is invalid.");
    }
    if (command_exists("edid-decode")) {
      const auto installed_edid = std::filesystem::path {"/lib/firmware/edid"} / profile.edid_filename;
      if (std::filesystem::exists(installed_edid)) {
        int exit_code = 0;
        const auto output = run_capture("edid-decode " + shell_quote(installed_edid.string()), &exit_code);
        if (exit_code != 0) {
          errors.emplace_back("edid-decode rejected installed EDID profile: " + output);
        }
      } else {
        warnings.emplace_back("edid-decode is available, but the WayShine EDID file is not installed at " + installed_edid.string() + ".");
      }
    } else {
      warnings.emplace_back("edid-decode was not found; EDID semantic validation was skipped.");
    }
    if (!command_exists("kscreen-doctor")) {
      errors.emplace_back("kscreen-doctor is not installed or not in PATH.");
    } else {
      KscreenDoctorBackend backend;
      const auto outputs = backend.enumerateOutputs();
      if (!outputs) {
        errors.emplace_back(outputs.error());
      } else if (!connector.empty()) {
        const auto matches = std::ranges::count_if(outputs.value(), [&](const auto &output) {
          return output.name == connector || output.id == connector;
        });
        if (matches == 0) {
          errors.emplace_back("KScreen does not expose output " + connector + ".");
        } else if (matches > 1) {
          errors.emplace_back("KScreen mapping is ambiguous for " + connector + ".");
        }
      }
    }

    if (!command_exists("nvidia-smi")) {
      warnings.emplace_back("nvidia-smi was not found; NVIDIA proprietary driver could not be verified.");
    }
    if (command_exists("rpm-ostree")) {
      warnings.emplace_back("rpm-ostree system detected. WayShine P0 treats Bazzite/atomic EDID installation as partially supported until persistence is verified.");
    }
    if (command_exists("kreadconfig6") || command_exists("kreadconfig5")) {
      const auto kreadconfig = command_exists("kreadconfig6") ? "kreadconfig6"s : "kreadconfig5"s;
      int exit_code = 0;
      const auto lock = run_capture(kreadconfig + " --file kscreenlockerrc --group Daemon --key Autolock", &exit_code);
      if (exit_code == 0 && lock.find("true") != std::string::npos) {
        warnings.emplace_back("KDE autolock appears enabled and can interrupt headless streaming.");
      }
      const auto dpms = run_capture(kreadconfig + " --file powermanagementprofilesrc --group AC --group DPMSControl --key idleTime", &exit_code);
      if (exit_code == 0 && !boost::algorithm::trim_copy(dpms).empty()) {
        warnings.emplace_back("KDE DPMS/energy-saving setting detected; verify it will not disable the virtual output.");
      }
    } else {
      warnings.emplace_back("kreadconfig was not found; KDE power-management settings could not be diagnosed.");
    }

    if (json_output) {
      std::cout << "{\n  \"errors\": [";
      for (std::size_t i = 0; i < errors.size(); ++i) {
        std::cout << (i ? ", " : "") << "\"" << json_escape(errors[i]) << "\"";
      }
      std::cout << "],\n  \"warnings\": [";
      for (std::size_t i = 0; i < warnings.size(); ++i) {
        std::cout << (i ? ", " : "") << "\"" << json_escape(warnings[i]) << "\"";
      }
      std::cout << "]\n}\n";
    } else {
      std::cout << "WayShine Linux virtual display doctor\n";
      for (const auto &error : errors) {
        std::cout << "ERROR: " << error << "\n";
      }
      for (const auto &warning : warnings) {
        std::cout << "WARN: " << warning << "\n";
      }
      if (errors.empty()) {
        std::cout << "OK: strict preflight prerequisites are satisfied.\n";
      }
    }

    return errors.empty() ? 0 : 2;
  }
}  // namespace display_device::linux_vdisplay
