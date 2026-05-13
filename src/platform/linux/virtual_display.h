/**
 * @file src/platform/linux/virtual_display.h
 * @brief Linux virtual display helpers for WayShine.
 */
#pragma once

#include <display_device/settings_manager_interface.h>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace config {
  struct video_t;
}

namespace display_device::linux_vdisplay {
  template<typename T>
  class Expected {
  public:
    Expected(T value):
        _value(std::move(value)) {}

    Expected(std::string error):
        _error(std::move(error)) {}

    [[nodiscard]] bool has_value() const {
      return _value.has_value();
    }

    explicit operator bool() const {
      return has_value();
    }

    [[nodiscard]] const T &value() const {
      return *_value;
    }

    [[nodiscard]] T &value() {
      return *_value;
    }

    [[nodiscard]] const std::string &error() const {
      return _error;
    }

  private:
    std::optional<T> _value;
    std::string _error;
  };

  template<>
  class Expected<void> {
  public:
    Expected() = default;

    Expected(std::string error):
        _error(std::move(error)) {}

    [[nodiscard]] bool has_value() const {
      return _error.empty();
    }

    explicit operator bool() const {
      return has_value();
    }

    [[nodiscard]] const std::string &error() const {
      return _error;
    }

  private:
    std::string _error;
  };

  struct LinuxVirtualDisplayMode {
    unsigned int width {};
    unsigned int height {};
    unsigned int refresh_hz {};
  };

  struct LinuxVirtualDisplayProfile {
    std::string name;
    std::string edid_filename;
    std::vector<LinuxVirtualDisplayMode> modes;
    std::vector<std::byte> edid;
  };

  struct Output {
    std::string id;
    std::string name;
    bool enabled {};
    bool connected {};
    bool primary {};
    std::vector<LinuxVirtualDisplayMode> modes;
    std::vector<int> mode_ids;
    std::optional<LinuxVirtualDisplayMode> applied_mode;
  };

  using OutputList = std::vector<Output>;

  struct DisplaySnapshot {
    OutputList outputs;
  };

  struct DisplayTransaction {
    std::string target_output;
    LinuxVirtualDisplayMode mode;
    bool make_primary {};
    bool only_display {};
  };

  class LinuxDisplayControlBackend {
  public:
    virtual ~LinuxDisplayControlBackend() = default;

    [[nodiscard]] virtual Expected<OutputList> enumerateOutputs() = 0;
    [[nodiscard]] virtual Expected<void> applyAtomic(const DisplayTransaction &transaction) = 0;
    [[nodiscard]] virtual Expected<void> restore(const DisplaySnapshot &snapshot) = 0;
  };

  class VirtualDisplayLease {
  public:
    VirtualDisplayLease() = default;

    VirtualDisplayLease(std::string connector, LinuxVirtualDisplayMode requested_mode):
        _connector(std::move(connector)),
        _requested_mode(requested_mode),
        _valid(true) {}

    [[nodiscard]] bool valid() const {
      return _valid;
    }

    [[nodiscard]] const std::string &connector() const {
      return _connector;
    }

    [[nodiscard]] LinuxVirtualDisplayMode requested_mode() const {
      return _requested_mode;
    }

  private:
    std::string _connector;
    LinuxVirtualDisplayMode _requested_mode {};
    bool _valid {};
  };

  class LinuxVirtualDisplayProvisioner {
  public:
    [[nodiscard]] static const LinuxVirtualDisplayProfile &default_profile();
    [[nodiscard]] static Expected<void> install(const std::string &connector, const std::string &profile_name);
    [[nodiscard]] static Expected<void> remove(const std::string &connector);
  };

  class LinuxVirtualDisplayDoctor {
  public:
    explicit LinuxVirtualDisplayDoctor(const config::video_t &video_config);

    [[nodiscard]] int run(bool json_output) const;

  private:
    const config::video_t &_video_config;
  };

  class KscreenDoctorBackend: public LinuxDisplayControlBackend {
  public:
    [[nodiscard]] Expected<OutputList> enumerateOutputs() override;
    [[nodiscard]] Expected<void> applyAtomic(const DisplayTransaction &transaction) override;
    [[nodiscard]] Expected<void> restore(const DisplaySnapshot &snapshot) override;
  };

  class LinuxDisplaySettingsManager: public SettingsManagerInterface {
  public:
    explicit LinuxDisplaySettingsManager(const config::video_t &video_config);

    [[nodiscard]] EnumeratedDeviceList enumAvailableDevices() const override;
    [[nodiscard]] std::string getDisplayName(const std::string &device_id) const override;
    [[nodiscard]] ApplyResult applySettings(const SingleDisplayConfiguration &config) override;
    [[nodiscard]] RevertResult revertSettings() override;
    [[nodiscard]] bool resetPersistence() override;
    [[nodiscard]] Expected<VirtualDisplayLease> prepareSession(const SingleDisplayConfiguration &config);

  private:
    [[nodiscard]] Expected<VirtualDisplayLease> prepareVirtualDisplay(const SingleDisplayConfiguration &config);
    [[nodiscard]] Expected<LinuxVirtualDisplayMode> resolveRequestedMode(const SingleDisplayConfiguration &config) const;
    [[nodiscard]] Expected<Output> findTargetOutput(const OutputList &outputs, const std::string &device_id) const;
    [[nodiscard]] bool profileContainsMode(const LinuxVirtualDisplayMode &mode) const;
    [[nodiscard]] bool sysfsConnectorContainsMode(const std::string &connector, const LinuxVirtualDisplayMode &mode) const;

    const config::video_t &_video_config;
    std::unique_ptr<LinuxDisplayControlBackend> _backend;
    std::optional<DisplaySnapshot> _snapshot;
    std::optional<VirtualDisplayLease> _lease;
  };

  [[nodiscard]] std::string mode_to_string(const LinuxVirtualDisplayMode &mode);
  [[nodiscard]] std::optional<LinuxVirtualDisplayMode> parse_mode(std::string_view text);
  [[nodiscard]] bool edid_checksum_valid(const std::vector<std::byte> &edid);
}  // namespace display_device::linux_vdisplay
