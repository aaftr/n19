/*
* Copyright (c) 2024 Diago Lima
* SPDX-License-Identifier: BSD-3-Clause
*/

#ifndef NATIVE_IODEVICE_HPP
#define NATIVE_IODEVICE_HPP
#include <Sys/Handle.hpp>
#include <Sys/String.hpp>
#include <Core/Bytes.hpp>
#include <Core/Result.hpp>
#include <array>
#include <cstdint>
#include <span>

#if defined(N19_WIN32)
#   include <windows.h>
#else // POSIX
#   include <unistd.h>
#   include <fcntl.h>
#   include <poll.h>
#endif

BEGIN_NAMESPACE(n19::sys);
#if defined(N19_WIN32)
using IODeviceBase_ = Handle<::HANDLE>;
#else
using IODeviceBase_ = Handle<int>;
#endif

class IODevice : public IODeviceBase_ {
public:
  enum Permissions : uint8_t {
    NoAccess = 0x00,
    Read     = 0x01,
    Write    = 0x01 << 1,
    Execute  = 0x01 << 2,
  };

  auto close()      -> void override;
  auto invalidate() -> void override;
  auto is_invalid() -> bool override;

  auto write(const Bytes& bytes) const -> Result<void>;
  auto read_into(WritableBytes& bytes) const -> Result<void>;
  auto flush_handle() const -> void;

  template<typename T> auto operator<<(const T&) -> IODevice&;
  template<typename T> auto operator>>(T& val)   -> IODevice&;

  static auto from_stdout() -> IODevice;
  static auto from_stderr() -> IODevice;
  static auto from_stdin()  -> IODevice;
  static auto create_pipe() -> Result<std::array<IODevice, 2>>;

  IODevice() = default;
 ~IODevice() override = default;

  uint8_t perms_ = NoAccess;
};

template<typename T>
auto IODevice::operator<<(const T& val) -> IODevice& {
  if constexpr(std::ranges::contiguous_range<T>) {
    auto bytes = as_bytes(val);
    write(bytes);
  } else if constexpr (std::is_trivially_constructible_v<T>){
    auto copy = as_bytecopy(val);
    write(copy.bytes());
  } else {
    static_assert(
    "IODevice::operator<< must be called with "
    "a type easily convertible to n19::Bytes.");
  }

  return *this;
}

template<typename T>
auto IODevice::operator>>(T& val) -> IODevice & {
  static_assert(std::ranges::contiguous_range<T>);
  auto bytes = as_writable_bytes(val);
  read_into(bytes);
  return *this;
}

#if defined(N19_POSIX)
FORCEINLINE_ auto IODevice::invalidate() -> void {
  value_ = -1;
  perms_ = IODevice::NoAccess;
}

FORCEINLINE_ auto IODevice::close() -> void {
  ::close(value_);
  invalidate();
}

FORCEINLINE_ auto IODevice::is_invalid() -> bool {
  return value_ == -1;
}

FORCEINLINE_ auto IODevice::flush_handle() const -> void {
  ::fsync(value_);
}

#else // IF WINDOWS
FORCEINLINE_ auto IODevice::invalidate() -> void {
  value_ = (::HANDLE)0x00;
  perms_ = IODevice::NoAccess;
}

FORCEINLINE_ auto IODevice::close() -> void {
  ::CancelIoEx(value_, nullptr);
  ::CloseHandle(value_);
  invalidate();
}

FORCEINLINE_ auto IODevice::is_invalid() -> bool {
  return value_ == (::HANDLE)nullptr;
}

FORCEINLINE_ auto IODevice::flush_handle() const -> void {
  ::FlushFileBuffers(value_); // Flush Win32 file buff.
}

#endif //IF defined(N19_POSIX)
END_NAMESPACE(n19::sys);
#endif //NATIVE_IODEVICE_HPP
