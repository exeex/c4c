#include "src/backend/backend.hpp"
#include "src/backend/bir/bir.hpp"
#include "src/backend/mir/x86/codegen/route_debug.hpp"
#include "src/backend/mir/x86/api/api.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace {

namespace bir = c4c::backend::bir;
using c4c::backend::BackendModuleInput;
using c4c::backend::BackendOptions;

c4c::TargetProfile x86_target_profile() {
  return c4c::default_target_profile(c4c::TargetArch::X86_64);
}

const c4c::TargetProfile& target_profile_from_module_triple(std::string_view target_triple,
                                                            c4c::TargetProfile& storage) {
  storage = c4c::target_profile_from_triple(
      target_triple.empty() ? c4c::default_host_target_triple() : target_triple);
  return storage;
}

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

bool expect_contains(const std::string& text,
                     std::string_view needle,
                     const char* failure_context) {
  if (text.find(needle) != std::string::npos) {
    return true;
  }
  std::cerr << failure_context << ": missing expected text: " << needle << "\n";
  std::cerr << "--- text ---\n" << text << "\n";
  return false;
}

bir::Module make_x86_multi_defined_global_function_pointer_boundary_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.string_constants.push_back(bir::StringConstant{
      .name = ".str0",
      .bytes = "%d\n",
  });
  module.globals.push_back(bir::Global{
      .name = "f",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
      .initializer_symbol_name = std::string("fred"),
  });
  module.globals.push_back(bir::Global{
      .name = "printfptr",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
      .initializer_symbol_name = std::string("printf"),
  });

  bir::Function fred;
  fred.name = "fred";
  fred.return_type = bir::TypeKind::I32;
  fred.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "%p0",
  });
  fred.blocks.push_back(bir::Block{
      .label = "entry",
      .terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(42)},
  });

  bir::Function printf_decl;
  printf_decl.name = "printf";
  printf_decl.is_declaration = true;
  printf_decl.return_type = bir::TypeKind::I32;
  printf_decl.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p._anon_param_",
  });

  bir::Function main_function;
  main_function.name = "main";
  main_function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "%fred.ptr"),
      .global_name = "f",
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t1"),
      .callee_value = bir::Value::named(bir::TypeKind::Ptr, "%fred.ptr"),
      .args = {bir::Value::immediate_i32(24)},
      .arg_types = {bir::TypeKind::I32},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_indirect = true,
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "%printf.ptr"),
      .global_name = "printfptr",
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t2"),
      .callee_value = bir::Value::named(bir::TypeKind::Ptr, "%printf.ptr"),
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@.str0"),
               bir::Value::named(bir::TypeKind::I32, "%t1")},
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::I32},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_indirect = true,
      .is_variadic = true,
  });
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  main_function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(fred));
  module.functions.push_back(std::move(printf_decl));
  module.functions.push_back(std::move(main_function));
  return module;
}

int check_route_rejection(const bir::Module& module,
                          std::string_view expected_message_fragment,
                          const char* failure_context) {
  c4c::TargetProfile target_profile;
  const auto prepared =
      c4c::backend::prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));

  try {
    (void)c4c::backend::x86::api::emit_prepared_module(prepared);
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer unexpectedly accepted an out-of-scope route")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find(expected_message_fragment) == std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer rejected the out-of-scope route with the wrong contract message")
                      .c_str());
    }
  }

  try {
    (void)c4c::backend::emit_x86_bir_module_entry(module, target_profile);
    return fail((std::string(failure_context) +
                 ": explicit x86 BIR entry unexpectedly kept a mixed bootstrap fallback alive")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find(expected_message_fragment) == std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": explicit x86 BIR entry rejected the out-of-scope route with the wrong contract message")
                      .c_str());
    }
  }

  try {
    (void)c4c::backend::emit_module(
        BackendModuleInput{module}, BackendOptions{.target_profile = x86_target_profile()});
    return fail((std::string(failure_context) +
                 ": generic x86 backend emit path unexpectedly kept a mixed bootstrap fallback alive")
                    .c_str());
  } catch (const std::invalid_argument& error) {
    if (std::string_view(error.what()).find(expected_message_fragment) == std::string_view::npos) {
      return fail((std::string(failure_context) +
                   ": generic x86 backend emit path rejected the out-of-scope route with the wrong contract message")
                      .c_str());
    }
  }

  return 0;
}

int check_route_debug_surface(const bir::Module& module,
                              std::string_view expected_message_fragment,
                              const char* failure_context) {
  c4c::TargetProfile target_profile;
  const auto prepared =
      c4c::backend::prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));

  const auto summary = c4c::backend::x86::summarize_prepared_module_routes(prepared);
  if (!expect_contains(
          summary,
          "module-level bounded multi-function lane: rejected",
          failure_context) ||
      !expect_contains(
          summary,
          "- module-level final rejection: bounded multi-function handoff recognized the module, but the prepared shape is outside the current x86 support",
          failure_context) ||
      !expect_contains(summary,
                       expected_message_fragment,
                       failure_context) ||
      !expect_contains(
          summary,
          "- module-level next inspect: inspect the current x86 bounded multi-function shape support in src/backend/mir/x86/codegen/module/module_emit.cpp",
          failure_context)) {
    return 1;
  }

  const auto trace = c4c::backend::x86::trace_prepared_module_routes(prepared);
  if (!expect_contains(
          trace,
          "final: rejected: bounded multi-function handoff recognized the module, but the prepared shape is outside the current x86 support",
          failure_context) ||
      !expect_contains(trace, expected_message_fragment, failure_context) ||
      !expect_contains(
          trace,
          "next inspect: inspect the current x86 bounded multi-function shape support in src/backend/mir/x86/codegen/module/module_emit.cpp",
          failure_context)) {
    return 1;
  }

  return 0;
}

}  // namespace

int run_backend_x86_handoff_boundary_multi_defined_rejection_tests() {
  // Keep the residual multi-defined rejection-only boundary isolated so Step 5
  // can finish shrinking the monolithic handoff file without mixing it back
  // into the supported-path same-module call lane.
  if (const auto status = check_route_rejection(
          make_x86_multi_defined_global_function_pointer_boundary_module(),
          "x86 backend emitter only supports direct immediate i32 returns, constant-evaluable straight-line no-parameter i32 return expressions, direct single-parameter i32 passthrough returns, single-parameter i32 add-immediate/sub-immediate/mul-immediate/and-immediate/or-immediate/xor-immediate/shl-immediate/lshr-immediate/ashr-immediate returns, a bounded equality-against-immediate guard family with immediate return leaves, or one bounded compare-against-zero branch family through the canonical prepared-module handoff",
          "multi-defined global-function-pointer and indirect variadic-runtime boundary");
      status != 0) {
    return status;
  }

  if (const auto status = check_route_debug_surface(
          make_x86_multi_defined_global_function_pointer_boundary_module(),
          "one bounded multi-defined-function main-entry lane with same-module symbol calls and direct variadic runtime calls",
          "multi-defined global-function-pointer and indirect variadic-runtime route-debug surface");
      status != 0) {
    return status;
  }

  return 0;
}
