#include <cstdlib>
#include <cstring>

#include "plugin/module_exports.h"

namespace {

void require(bool condition) {
  if (!condition) {
    std::abort();
  }
}

} // namespace

int main() {
  auto const module_name{uwvm2_vulkan::plugin::GetModuleName()};
  require(module_name.name != nullptr);
  require(module_name.name_length == 12u);
  require(std::memcmp(module_name.name, "wasiu-vulkan", 12u) == 0);

  auto const function_vec{uwvm2_vulkan::plugin::GetFunctionVec()};
  require(function_vec.function_begin != nullptr);
  require(function_vec.function_size == 25u);

  auto const weak_module_vector{uwvm2_vulkan::plugin::GetWeakModuleVector()};
  require(weak_module_vector != nullptr);
  require(weak_module_vector->module_count == 1u);
  require(weak_module_vector->module_ptr != nullptr);
  require(weak_module_vector->module_ptr->function_vec.function_size ==
          function_vec.function_size);

  return 0;
}
