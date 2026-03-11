/*************************************************************
 * UWVM2 Vulkan Wrapper                                     *
 * Copyright (c) 2026-present MacroModel. All rights        *
 * reserved.                                                *
 *************************************************************/

/**
 * @author      MacroModel
 * @version     0.1.0
 * @date        2026-03-11
 */

/****************************************
 *  _   _ __        ____     __ __  __  *
 * | | | |\ \      / /\ \   / /|  \/  | *
 * | | | | \ \ /\ / /  \ \ / / | |\/| | *
 * | |_| |  \ V  V /    \ V /  | |  | | *
 *  \___/    \_/\_/      \_/   |_|  |_| *
 *                                      *
 ****************************************/

#ifndef UWVM2_VULKAN_SRC_RUNTIME_MEMORY_ACCESS_H_
#define UWVM2_VULKAN_SRC_RUNTIME_MEMORY_ACCESS_H_

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <type_traits>

#include "abi/uwvm_abi.h"

namespace uwvm2_vulkan::runtime {

class GuestMemoryAccessor {
public:
  void SetHostApi(uwvm_preload_host_api_v1 const *host_api) noexcept;
  [[nodiscard]] uwvm_preload_host_api_v1 const *host_api() const noexcept;
  [[nodiscard]] bool IsAvailable() const noexcept;

  [[nodiscard]] std::optional<uwvm_preload_memory_descriptor_t>
  FindDescriptor(std::size_t memory_index) const noexcept;
  [[nodiscard]] bool Read(std::size_t memory_index, std::uint64_t address,
                          void *destination, std::size_t size) const noexcept;
  [[nodiscard]] bool Write(std::size_t memory_index, std::uint64_t address,
                           void const *source, std::size_t size) const noexcept;
  [[nodiscard]] bool ReadString(std::size_t memory_index, std::uint64_t address,
                                std::uint32_t size, std::string &out) const;

  template <typename T>
  [[nodiscard]] bool ReadObject(std::size_t memory_index, std::uint64_t address,
                                T &out) const noexcept {
    static_assert(std::is_trivially_copyable_v<T>);
    return Read(memory_index, address, std::addressof(out), sizeof(T));
  }

  template <typename T>
  [[nodiscard]] bool WriteObject(std::size_t memory_index,
                                 std::uint64_t address,
                                 T const &value) const noexcept {
    static_assert(std::is_trivially_copyable_v<T>);
    return Write(memory_index, address, std::addressof(value), sizeof(T));
  }

private:
  [[nodiscard]] static bool RangeIsValid(std::uint64_t byte_length,
                                         std::uint64_t address,
                                         std::size_t size) noexcept;
  [[nodiscard]] static bool
  DirectRangeIsValid(uwvm_preload_memory_descriptor_t const &descriptor,
                     std::uint64_t address, std::size_t size) noexcept;

  uwvm_preload_host_api_v1 const *host_api_{};
};

} // namespace uwvm2_vulkan::runtime

#endif
