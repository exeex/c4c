#include "src/backend/bir/bir.hpp"
#include "src/backend/mir/aarch64/api/api.hpp"
#include "src/backend/mir/aarch64/module/module.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/target_profile.hpp"

#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace {

namespace aarch64_api = c4c::backend::aarch64::api;
namespace aarch64_module = c4c::backend::aarch64::module;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int fail(std::string_view message) {
  std::cerr << message << "\n";
  return 1;
}

prepare::PreparedBirModule prepared_data_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto visible_link = prepared.module.names.link_names.intern("visible.data");
  const auto tls_link = prepared.module.names.link_names.intern("tls.data");
  const auto const_ptr_link = prepared.module.names.link_names.intern("const.ptr");
  const auto target_link = prepared.module.names.link_names.intern("target.data");
  const auto array_link = prepared.module.names.link_names.intern("array.with.ptr");
  const auto extern_link = prepared.module.names.link_names.intern("extern.data");
  const auto string_name = prepared.module.names.texts.intern("string.literal.structured");

  prepared.module.globals.push_back(bir::Global{
      .name = "raw.visible.display.drift",
      .link_name_id = visible_link,
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .initializer = bir::Value::immediate_i32(42),
  });
  prepared.module.globals.push_back(bir::Global{
      .name = "raw.tls.display.drift",
      .link_name_id = tls_link,
      .type = bir::TypeKind::I64,
      .is_thread_local = true,
      .size_bytes = 8,
      .align_bytes = 8,
      .initializer = bir::Value::immediate_i64(7),
  });
  prepared.module.globals.push_back(bir::Global{
      .name = "raw.target.display.drift",
      .link_name_id = target_link,
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 8,
      .initializer = bir::Value::immediate_i64(99),
  });
  prepared.module.globals.push_back(bir::Global{
      .name = "raw.const.ptr.display.drift",
      .link_name_id = const_ptr_link,
      .type = bir::TypeKind::Ptr,
      .is_constant = true,
      .size_bytes = 8,
      .align_bytes = 8,
      .initializer_symbol_name = std::string("raw.target.initializer.drift"),
      .initializer_symbol_name_id = target_link,
  });
  prepared.module.globals.push_back(bir::Global{
      .name = "raw.array.display.drift",
      .link_name_id = array_link,
      .type = bir::TypeKind::I8,
      .size_bytes = 16,
      .align_bytes = 8,
      .initializer_elements =
          {
              bir::Value::immediate_i8(1),
              bir::Value::named_symbol_pointer("@target.data", target_link),
          },
  });
  prepared.module.globals.push_back(bir::Global{
      .name = "raw.extern.display.drift",
      .link_name_id = extern_link,
      .type = bir::TypeKind::I32,
      .is_extern = true,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  prepared.module.string_constants.push_back(bir::StringConstant{
      .name = "raw.string.display.drift",
      .name_id = string_name,
      .bytes = std::string("hello\000world", 11),
      .align_bytes = 16,
  });

  return prepared;
}

int records_preserve_prepared_data_identities() {
  auto prepared = prepared_data_module();

  const auto result = aarch64_api::build_prepared_module(prepared);
  if (result.error.has_value()) {
    return fail("expected AArch64 data identity module to pass handoff gate");
  }
  if (!result.module.has_value()) {
    return fail("expected AArch64 data identity module to build");
  }

  const auto& module = *result.module;
  if (module.globals.size() != prepared.module.globals.size()) {
    return fail("expected every prepared global to have a side-table record");
  }
  if (module.strings.size() != 1) {
    return fail("expected prepared string constant to have a side-table record");
  }
  if (module.relocation_needs.size() != 2) {
    return fail("expected future relocation side table to describe two symbolic initializers");
  }

  const auto& visible = module.globals[0];
  if (visible.label != "visible.data" || visible.link_name == c4c::kInvalidLinkName ||
      visible.source_global != &prepared.module.globals[0]) {
    return fail("expected global record to preserve structured link identity over raw name drift");
  }
  if (!visible.initializer.has_value() ||
      visible.initializer->kind != bir::Value::Kind::Immediate ||
      visible.initializer->immediate != 42) {
    return fail("expected scalar initializer constant to remain structured data");
  }

  const auto& tls = module.globals[1];
  if (tls.label != "tls.data" || !tls.is_thread_local ||
      tls.visibility != aarch64_module::SymbolVisibilityRecordKind::LinkVisibleDefinition) {
    return fail("expected TLS definition record to preserve TLS and visibility facts");
  }

  const auto& const_ptr = module.globals[3];
  if (const_ptr.label != "const.ptr" || !const_ptr.is_constant ||
      !const_ptr.initializer_symbol_label.has_value() ||
      *const_ptr.initializer_symbol_label != "target.data" ||
      const_ptr.initializer_symbol_name_id == c4c::kInvalidLinkName) {
    return fail("expected constant pointer global to preserve symbolic initializer identity");
  }
  if (const_ptr.relocation_needs.size() != 1 ||
      const_ptr.relocation_needs.front().kind !=
          aarch64_module::DataRelocationNeedKind::InitializerSymbol ||
      const_ptr.relocation_needs.front().target_label != "target.data") {
    return fail("expected pointer initializer to publish a descriptive relocation need");
  }

  const auto& array = module.globals[4];
  if (array.initializer_elements.size() != 2 ||
      array.initializer_elements[1].pointer_symbol_link_name_id == c4c::kInvalidLinkName) {
    return fail("expected aggregate initializer elements to preserve structured pointer identity");
  }
  if (array.relocation_needs.size() != 1 ||
      array.relocation_needs.front().kind !=
          aarch64_module::DataRelocationNeedKind::InitializerElementSymbol ||
      !array.relocation_needs.front().initializer_element_index.has_value() ||
      *array.relocation_needs.front().initializer_element_index != 1) {
    return fail("expected pointer initializer element to publish descriptive relocation identity");
  }

  const auto& external = module.globals[5];
  if (!external.is_extern ||
      external.visibility != aarch64_module::SymbolVisibilityRecordKind::ExternalDeclaration ||
      external.initializer.has_value() || !external.initializer_elements.empty()) {
    return fail("expected extern global record to remain declaration-only");
  }

  const auto& string = module.strings.front();
  if (string.label != "string.literal.structured" || string.name_id == c4c::kInvalidText ||
      string.bytes != std::string_view("hello\000world", 11) || string.align_bytes != 16 ||
      string.source_string != &prepared.module.string_constants.front()) {
    return fail("expected string record to preserve text id, bytes, alignment, and source pointer");
  }

  return 0;
}

}  // namespace

int main() {
  if (const int status = records_preserve_prepared_data_identities(); status != 0) {
    return status;
  }
  return 0;
}
