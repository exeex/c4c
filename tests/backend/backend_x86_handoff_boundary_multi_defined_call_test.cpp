#include "src/backend/backend.hpp"
#include "src/backend/bir/bir.hpp"
#include "src/backend/bir/lir_to_bir/lowering.hpp"
#include "src/backend/mir/x86/abi/abi.hpp"
#include "src/backend/mir/x86/codegen/x86_codegen.hpp"  // Compatibility holdout for prepared helper/render reach-throughs without narrower owners yet.
#include "src/backend/mir/x86/api/api.hpp"
#include "src/backend/prealloc/target_register_profile.hpp"

#include <algorithm>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace {

namespace bir = c4c::backend::bir;
namespace lir = c4c::codegen::lir;
namespace prepare = c4c::backend::prepare;
using c4c::backend::BackendModuleInput;
using c4c::backend::BackendOptions;

const c4c::TargetProfile& target_profile_from_module_triple(std::string_view target_triple,
                                                            c4c::TargetProfile& storage) {
  storage = c4c::target_profile_from_triple(
      target_triple.empty() ? c4c::default_host_target_triple() : target_triple);
  return storage;
}

c4c::TargetProfile x86_target_profile() {
  return c4c::default_target_profile(c4c::TargetArch::X86_64);
}

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

std::string minimal_i32_return_register() {
  const auto abi =
      c4c::backend::lir_to_bir_detail::compute_function_return_abi(x86_target_profile(),
                                                                   bir::TypeKind::I32,
                                                                   false);
  if (!abi.has_value()) {
    throw std::runtime_error("missing canonical i32 return ABI for x86 handoff test");
  }
  const auto wide_register =
      prepare::call_result_destination_register_name(x86_target_profile(), *abi);
  if (!wide_register.has_value()) {
    throw std::runtime_error("missing canonical i32 return register for x86 handoff test");
  }
  return c4c::backend::x86::abi::narrow_i32_register_name(*wide_register);
}

std::string asm_header(const char* function_name) {
  return std::string(".intel_syntax noprefix\n.text\n.globl ") + function_name +
         "\n.type " + function_name + ", @function\n" + function_name + ":\n";
}

std::string expected_minimal_multi_defined_direct_call_lane_asm() {
  return asm_header("foo") + "    ret\n" +
         asm_header("actual_function") + "    mov " + minimal_i32_return_register() +
         ", 42\n    ret\n" +
         asm_header("main") + "    sub rsp, 24\n"
         "    xor eax, eax\n"
         "    call actual_function\n"
         "    mov r11d, eax\n"
         "    mov DWORD PTR [rsp + 16], r11d\n"
         "    mov eax, DWORD PTR [rsp + 16]\n"
         "    lea rdi, [rip + .L.str0]\n"
         "    mov esi, eax\n"
         "    xor eax, eax\n"
         "    call printf\n"
         "    mov DWORD PTR [rsp + 8], eax\n"
         "    xor eax, eax\n"
         "    call actual_function\n"
         "    mov r11d, eax\n"
         "    mov DWORD PTR [rsp + 20], r11d\n"
         "    mov eax, DWORD PTR [rsp + 20]\n"
         "    lea rdi, [rip + .L.str0]\n"
         "    mov esi, eax\n"
         "    xor eax, eax\n"
         "    call printf\n"
         "    mov DWORD PTR [rsp + 12], eax\n"
         "    mov eax, 0\n"
         "    add rsp, 24\n"
         "    ret\n"
         ".section .rodata\n"
         ".L.str0:\n"
         "    .asciz \"%i\\n\"\n";
}

std::string expected_multi_defined_direct_call_lane_contract_drift_asm() {
  return asm_header("foo") + "    ret\n" +
         asm_header("actual_function") + "    mov " + minimal_i32_return_register() +
         ", 42\n    ret\n" +
         asm_header("main") + "    sub rsp, 24\n"
         "    xor eax, eax\n"
         "    call actual_function\n"
         "    mov r10d, eax\n"
         "    mov DWORD PTR [rsp + 16], r10d\n"
         "    mov eax, DWORD PTR [rsp + 16]\n"
         "    lea rdi, [rip + .L.str0]\n"
         "    mov edx, eax\n"
         "    xor eax, eax\n"
         "    call printf\n"
         "    mov DWORD PTR [rsp + 8], eax\n"
         "    xor eax, eax\n"
         "    call actual_function\n"
         "    mov r11d, eax\n"
         "    mov DWORD PTR [rsp + 20], r11d\n"
         "    mov eax, DWORD PTR [rsp + 20]\n"
         "    lea rdi, [rip + .L.str0]\n"
         "    mov esi, eax\n"
         "    xor eax, eax\n"
         "    call printf\n"
         "    mov DWORD PTR [rsp + 12], eax\n"
         "    mov eax, 0\n"
         "    add rsp, 24\n"
         "    ret\n"
         ".section .rodata\n"
         ".L.str0:\n"
         "    .asciz \"%i\\n\"\n";
}

bir::Module make_x86_multi_defined_direct_call_lane_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.string_constants.push_back(bir::StringConstant{
      .name = ".str0",
      .bytes = "%i\n",
  });

  bir::Function foo;
  foo.name = "foo";
  foo.return_type = bir::TypeKind::Void;
  foo.blocks.push_back(bir::Block{
      .label = "entry",
      .terminator = bir::ReturnTerminator{},
  });

  bir::Function actual_function;
  actual_function.name = "actual_function";
  actual_function.return_type = bir::TypeKind::I32;
  actual_function.blocks.push_back(bir::Block{
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
  main_function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.function_pointer",
      .type = bir::TypeKind::Ptr,
      .align_bytes = 8,
  });
  main_function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.a",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_address_taken = true,
  });
  main_function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.b",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_address_taken = true,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t1"),
      .callee_value = bir::Value::named(bir::TypeKind::Ptr, "@actual_function"),
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_indirect = true,
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.a",
      .value = bir::Value::named(bir::TypeKind::I32, "%t1"),
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t3"),
      .slot_name = "%lv.a",
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t4"),
      .callee = "printf",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@.str0"),
               bir::Value::named(bir::TypeKind::I32, "%t3")},
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::I32},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_variadic = true,
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t6"),
      .callee_value = bir::Value::named(bir::TypeKind::Ptr, "@actual_function"),
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_indirect = true,
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.b",
      .value = bir::Value::named(bir::TypeKind::I32, "%t6"),
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t8"),
      .slot_name = "%lv.b",
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t9"),
      .callee = "printf",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@.str0"),
               bir::Value::named(bir::TypeKind::I32, "%t8")},
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::I32},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_variadic = true,
  });
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  main_function.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(foo));
  module.functions.push_back(std::move(actual_function));
  module.functions.push_back(std::move(printf_decl));
  module.functions.push_back(std::move(main_function));
  return module;
}

bir::Module make_x86_multi_defined_byval_helper_call_lane_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.string_constants.push_back(bir::StringConstant{
      .name = ".str0",
      .bytes = "%d %d\n",
  });

  bir::Function printf_decl;
  printf_decl.name = "printf";
  printf_decl.is_declaration = true;
  printf_decl.return_type = bir::TypeKind::I32;
  printf_decl.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p._anon_param_",
  });

  bir::Function show;
  show.name = "show";
  show.return_type = bir::TypeKind::Void;
  show.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p.p",
      .size_bytes = 8,
      .align_bytes = 4,
      .is_byval = true,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.p.p.0",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_byval_copy = true,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.p.p.4",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_byval_copy = true,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.p.0",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_byval_copy = true,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.p.4",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_byval_copy = true,
  });

  bir::Block show_entry;
  show_entry.label = "entry";
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%lv.param.p.p.aggregate.param.copy.0"),
      .slot_name = "%p.p",
      .byte_offset = 0,
      .align_bytes = 4,
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.p.p.0",
      .value = bir::Value::named(bir::TypeKind::I32, "%lv.param.p.p.aggregate.param.copy.0"),
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%lv.param.p.p.aggregate.param.copy.4"),
      .slot_name = "%p.p",
      .byte_offset = 4,
      .align_bytes = 4,
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.p.p.4",
      .value = bir::Value::named(bir::TypeKind::I32, "%lv.param.p.p.aggregate.param.copy.4"),
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%lv.param.p.byval.copy.0"),
      .slot_name = "%p.p",
      .byte_offset = 0,
      .align_bytes = 4,
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.p.0",
      .value = bir::Value::named(bir::TypeKind::I32, "%lv.param.p.byval.copy.0"),
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%lv.param.p.byval.copy.4"),
      .slot_name = "%p.p",
      .byte_offset = 4,
      .align_bytes = 4,
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.p.4",
      .value = bir::Value::named(bir::TypeKind::I32, "%lv.param.p.byval.copy.4"),
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t2"),
      .slot_name = "%lv.param.p.0",
      .align_bytes = 4,
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t4"),
      .slot_name = "%lv.param.p.4",
      .align_bytes = 4,
  });
  show_entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t5"),
      .callee = "printf",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@.str0"),
               bir::Value::named(bir::TypeKind::I32, "%t2"),
               bir::Value::named(bir::TypeKind::I32, "%t4")},
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::I32, bir::TypeKind::I32},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_variadic = true,
  });
  show_entry.terminator = bir::ReturnTerminator{};
  show.blocks.push_back(std::move(show_entry));

  bir::Function main_function;
  main_function.name = "main";
  main_function.return_type = bir::TypeKind::I32;
  main_function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.p.0",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_address_taken = true,
  });
  main_function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.p.4",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_address_taken = true,
  });

  bir::Block main_entry;
  main_entry.label = "entry";
  main_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.p.0",
      .value = bir::Value::immediate_i32(7),
  });
  main_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.p.4",
      .value = bir::Value::immediate_i32(9),
  });
  main_entry.insts.push_back(bir::CallInst{
      .callee = "show",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "%lv.p")},
      .arg_types = {bir::TypeKind::Ptr},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 4,
          .passed_on_stack = true,
          .byval_copy = true,
      }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  main_entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};
  main_function.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(show));
  module.functions.push_back(std::move(printf_decl));
  module.functions.push_back(std::move(main_function));
  return module;
}

bir::Module make_x86_multi_defined_byval_i8_helper_copy_lane_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  bir::Function show;
  show.name = "show";
  show.return_type = bir::TypeKind::Void;
  show.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p.a",
      .size_bytes = 1,
      .align_bytes = 1,
      .is_byval = true,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.p.a.0",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .is_byval_copy = true,
  });

  bir::Block show_entry;
  show_entry.label = "entry";
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "%lv.param.p.a.aggregate.param.copy.0"),
      .slot_name = "%lv.param.p.a.0",
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%p.a",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%p.a"),
          .size_bytes = 1,
          .align_bytes = 1,
      },
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.p.a.0",
      .value = bir::Value::named(bir::TypeKind::I8, "%lv.param.p.a.aggregate.param.copy.0"),
      .align_bytes = 1,
  });
  show_entry.terminator = bir::ReturnTerminator{};
  show.blocks.push_back(std::move(show_entry));

  bir::Function main_function;
  main_function.name = "main";
  main_function.return_type = bir::TypeKind::I32;
  main_function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.a.0",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .is_address_taken = true,
  });

  bir::Block main_entry;
  main_entry.label = "entry";
  main_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.a.0",
      .value = bir::Value::immediate_i8('0'),
  });
  main_entry.insts.push_back(bir::CallInst{
      .callee = "show",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "%lv.a")},
      .arg_types = {bir::TypeKind::Ptr},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 1,
          .align_bytes = 1,
          .passed_on_stack = true,
          .byval_copy = true,
      }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  main_entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};
  main_function.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(show));
  module.functions.push_back(std::move(main_function));
  return module;
}

bir::Module make_x86_multi_defined_byval_i8_helper_pointer_arg_lane_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.string_constants.push_back(bir::StringConstant{
      .name = ".str0",
      .bytes = "%.1s\n",
  });

  bir::Function printf_decl;
  printf_decl.name = "printf";
  printf_decl.is_declaration = true;
  printf_decl.return_type = bir::TypeKind::I32;
  printf_decl.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p._anon_param_",
  });

  bir::Function show;
  show.name = "show";
  show.return_type = bir::TypeKind::Void;
  show.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p.a",
      .size_bytes = 1,
      .align_bytes = 1,
      .is_byval = true,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.p.a.0",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .is_byval_copy = true,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.a.0",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .is_byval_copy = true,
  });

  bir::Block show_entry;
  show_entry.label = "entry";
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "%lv.param.p.a.aggregate.param.copy.0"),
      .slot_name = "%lv.param.p.a.0",
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%p.a",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%p.a"),
          .size_bytes = 1,
          .align_bytes = 1,
      },
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.p.a.0",
      .value = bir::Value::named(bir::TypeKind::I8, "%lv.param.p.a.aggregate.param.copy.0"),
      .align_bytes = 1,
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "%lv.param.a.byval.copy.0"),
      .slot_name = "%lv.param.a.0",
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%p.a",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%p.a"),
          .size_bytes = 1,
          .align_bytes = 1,
      },
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.a.0",
      .value = bir::Value::named(bir::TypeKind::I8, "%lv.param.a.byval.copy.0"),
      .align_bytes = 1,
  });
  show_entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t3"),
      .callee = "printf",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@.str0"),
               bir::Value::named(bir::TypeKind::Ptr, "%lv.param.a.0")},
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::Ptr},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_variadic = true,
  });
  show_entry.terminator = bir::ReturnTerminator{};
  show.blocks.push_back(std::move(show_entry));

  bir::Function main_function;
  main_function.name = "main";
  main_function.return_type = bir::TypeKind::I32;
  main_function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.a.0",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .is_address_taken = true,
  });

  bir::Block main_entry;
  main_entry.label = "entry";
  main_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.a.0",
      .value = bir::Value::immediate_i8('0'),
  });
  main_entry.insts.push_back(bir::CallInst{
      .callee = "show",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "%lv.a")},
      .arg_types = {bir::TypeKind::Ptr},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 1,
          .align_bytes = 1,
          .passed_on_stack = true,
          .byval_copy = true,
      }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  main_entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};
  main_function.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(show));
  module.functions.push_back(std::move(printf_decl));
  module.functions.push_back(std::move(main_function));
  return module;
}

bir::Module make_x86_multi_defined_byval_i8x2_helper_pointer_arg_lane_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.string_constants.push_back(bir::StringConstant{
      .name = ".str0",
      .bytes = "%.2s\n",
  });

  bir::Function printf_decl;
  printf_decl.name = "printf";
  printf_decl.is_declaration = true;
  printf_decl.return_type = bir::TypeKind::I32;
  printf_decl.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p._anon_param_",
  });

  bir::Function show;
  show.name = "show";
  show.return_type = bir::TypeKind::Void;
  show.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p.a",
      .size_bytes = 2,
      .align_bytes = 1,
      .is_byval = true,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.p.a.0",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .is_byval_copy = true,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.p.a.1",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .is_byval_copy = true,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.a.0",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .is_byval_copy = true,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.a.1",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .is_byval_copy = true,
  });

  bir::Block show_entry;
  show_entry.label = "entry";
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "%lv.param.p.a.aggregate.param.copy.0"),
      .slot_name = "%lv.param.p.a.0",
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%p.a",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%p.a"),
          .size_bytes = 2,
          .align_bytes = 1,
      },
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.p.a.0",
      .value = bir::Value::named(bir::TypeKind::I8, "%lv.param.p.a.aggregate.param.copy.0"),
      .align_bytes = 1,
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "%lv.param.p.a.aggregate.param.copy.1"),
      .slot_name = "%lv.param.p.a.1",
      .byte_offset = 1,
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%p.a",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%p.a"),
          .byte_offset = 1,
          .size_bytes = 2,
          .align_bytes = 1,
      },
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.p.a.1",
      .value = bir::Value::named(bir::TypeKind::I8, "%lv.param.p.a.aggregate.param.copy.1"),
      .align_bytes = 1,
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "%lv.param.a.byval.copy.0"),
      .slot_name = "%lv.param.a.0",
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%p.a",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%p.a"),
          .size_bytes = 2,
          .align_bytes = 1,
      },
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.a.0",
      .value = bir::Value::named(bir::TypeKind::I8, "%lv.param.a.byval.copy.0"),
      .align_bytes = 1,
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "%lv.param.a.byval.copy.1"),
      .slot_name = "%lv.param.a.1",
      .byte_offset = 1,
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%p.a",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%p.a"),
          .byte_offset = 1,
          .size_bytes = 2,
          .align_bytes = 1,
      },
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.a.1",
      .value = bir::Value::named(bir::TypeKind::I8, "%lv.param.a.byval.copy.1"),
      .align_bytes = 1,
  });
  show_entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t3"),
      .callee = "printf",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@.str0"),
               bir::Value::named(bir::TypeKind::Ptr, "%lv.param.a.0")},
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::Ptr},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_variadic = true,
  });
  show_entry.terminator = bir::ReturnTerminator{};
  show.blocks.push_back(std::move(show_entry));

  bir::Function main_function;
  main_function.name = "main";
  main_function.return_type = bir::TypeKind::I32;
  main_function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.a.0",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .is_address_taken = true,
  });
  main_function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.a.1",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .is_address_taken = true,
  });

  bir::Block main_entry;
  main_entry.label = "entry";
  main_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.a.0",
      .value = bir::Value::immediate_i8('0'),
  });
  main_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.a.1",
      .value = bir::Value::immediate_i8('1'),
  });
  main_entry.insts.push_back(bir::CallInst{
      .callee = "show",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "%lv.a")},
      .arg_types = {bir::TypeKind::Ptr},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 2,
          .align_bytes = 1,
          .passed_on_stack = true,
          .byval_copy = true,
      }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  main_entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};
  main_function.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(show));
  module.functions.push_back(std::move(printf_decl));
  module.functions.push_back(std::move(main_function));
  return module;
}

bir::Module make_x86_multi_defined_helper_same_module_byval_i8x2_call_lane_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.string_constants.push_back(bir::StringConstant{
      .name = ".str0",
      .bytes = "%.2s\n",
  });
  module.globals.push_back(bir::Global{
      .name = "g",
      .type = bir::TypeKind::I8,
      .size_bytes = 2,
      .align_bytes = 1,
      .initializer_elements = {
          bir::Value::immediate_i8('0'),
          bir::Value::immediate_i8('1'),
      },
  });

  bir::Function printf_decl;
  printf_decl.name = "printf";
  printf_decl.is_declaration = true;
  printf_decl.return_type = bir::TypeKind::I32;
  printf_decl.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p._anon_param_",
  });

  bir::Function show;
  show.name = "show";
  show.return_type = bir::TypeKind::Void;
  show.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p.a",
      .size_bytes = 2,
      .align_bytes = 1,
      .is_byval = true,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.p.a.0",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .is_byval_copy = true,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.p.a.1",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .is_byval_copy = true,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.a.0",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .is_byval_copy = true,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.a.1",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .is_byval_copy = true,
  });

  bir::Block show_entry;
  show_entry.label = "entry";
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "%lv.param.p.a.aggregate.param.copy.0"),
      .slot_name = "%lv.param.p.a.0",
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%p.a",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%p.a"),
          .size_bytes = 2,
          .align_bytes = 1,
      },
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.p.a.0",
      .value = bir::Value::named(bir::TypeKind::I8, "%lv.param.p.a.aggregate.param.copy.0"),
      .align_bytes = 1,
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "%lv.param.p.a.aggregate.param.copy.1"),
      .slot_name = "%lv.param.p.a.1",
      .byte_offset = 1,
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%p.a",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%p.a"),
          .byte_offset = 1,
          .size_bytes = 2,
          .align_bytes = 1,
      },
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.p.a.1",
      .value = bir::Value::named(bir::TypeKind::I8, "%lv.param.p.a.aggregate.param.copy.1"),
      .align_bytes = 1,
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "%lv.param.a.byval.copy.0"),
      .slot_name = "%lv.param.a.0",
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%p.a",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%p.a"),
          .size_bytes = 2,
          .align_bytes = 1,
      },
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.a.0",
      .value = bir::Value::named(bir::TypeKind::I8, "%lv.param.a.byval.copy.0"),
      .align_bytes = 1,
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "%lv.param.a.byval.copy.1"),
      .slot_name = "%lv.param.a.1",
      .byte_offset = 1,
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%p.a",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%p.a"),
          .byte_offset = 1,
          .size_bytes = 2,
          .align_bytes = 1,
      },
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.a.1",
      .value = bir::Value::named(bir::TypeKind::I8, "%lv.param.a.byval.copy.1"),
      .align_bytes = 1,
  });
  show_entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t3"),
      .callee = "printf",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@.str0"),
               bir::Value::named(bir::TypeKind::Ptr, "%lv.param.a.0")},
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::Ptr},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_variadic = true,
  });
  show_entry.terminator = bir::ReturnTerminator{};
  show.blocks.push_back(std::move(show_entry));

  bir::Function arg_function;
  arg_function.name = "arg";
  arg_function.return_type = bir::TypeKind::Void;

  bir::Block arg_entry;
  arg_entry.label = "entry";
  arg_entry.insts.push_back(bir::CallInst{
      .callee = "show",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@g")},
      .arg_types = {bir::TypeKind::Ptr},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 2,
          .align_bytes = 1,
          .passed_on_stack = true,
          .byval_copy = true,
      }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  arg_entry.terminator = bir::ReturnTerminator{};
  arg_function.blocks.push_back(std::move(arg_entry));

  bir::Function main_function;
  main_function.name = "main";
  main_function.return_type = bir::TypeKind::I32;

  bir::Block main_entry;
  main_entry.label = "entry";
  main_entry.insts.push_back(bir::CallInst{
      .callee = "arg",
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  main_entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};
  main_function.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(show));
  module.functions.push_back(std::move(arg_function));
  module.functions.push_back(std::move(printf_decl));
  module.functions.push_back(std::move(main_function));
  return module;
}

bir::Module make_x86_multi_defined_helper_same_module_local_byval_i8_call_lane_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.string_constants.push_back(bir::StringConstant{
      .name = ".str0",
      .bytes = "%.1s\n",
  });
  module.string_constants.push_back(bir::StringConstant{
      .name = ".str1",
      .bytes = "Arguments:\n",
  });
  module.globals.push_back(bir::Global{
      .name = "s1",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .initializer_elements = {bir::Value::immediate_i8('0')},
  });

  bir::Function printf_decl;
  printf_decl.name = "printf";
  printf_decl.is_declaration = true;
  printf_decl.return_type = bir::TypeKind::I32;
  printf_decl.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p._anon_param_",
  });

  bir::Function show;
  show.name = "show";
  show.return_type = bir::TypeKind::Void;
  show.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p.a",
      .size_bytes = 1,
      .align_bytes = 1,
      .is_byval = true,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.p.a.0",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .is_byval_copy = true,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.a.0",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .is_byval_copy = true,
  });

  bir::Block show_entry;
  show_entry.label = "entry";
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "%lv.param.p.a.aggregate.param.copy.0"),
      .slot_name = "%lv.param.p.a.0",
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%p.a",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%p.a"),
          .size_bytes = 1,
          .align_bytes = 1,
      },
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.p.a.0",
      .value = bir::Value::named(bir::TypeKind::I8, "%lv.param.p.a.aggregate.param.copy.0"),
      .align_bytes = 1,
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "%lv.param.a.byval.copy.0"),
      .slot_name = "%lv.param.a.0",
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%p.a",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%p.a"),
          .size_bytes = 1,
          .align_bytes = 1,
      },
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.a.0",
      .value = bir::Value::named(bir::TypeKind::I8, "%lv.param.a.byval.copy.0"),
      .align_bytes = 1,
  });
  show_entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t3"),
      .callee = "printf",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@.str0"),
               bir::Value::named(bir::TypeKind::Ptr, "%lv.param.a.0")},
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::Ptr},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_variadic = true,
  });
  show_entry.terminator = bir::ReturnTerminator{};
  show.blocks.push_back(std::move(show_entry));

  bir::Function arg_function;
  arg_function.name = "arg";
  arg_function.return_type = bir::TypeKind::Void;
  arg_function.local_slots.push_back(bir::LocalSlot{
      .name = "%t2.0",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .storage_kind = bir::LocalSlotStorageKind::LoweringScratch,
  });

  bir::Block arg_entry;
  arg_entry.label = "entry";
  arg_entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t1"),
      .callee = "printf",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@.str1")},
      .arg_types = {bir::TypeKind::Ptr},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_variadic = true,
  });
  arg_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "%t2.global.aggregate.load.0"),
      .slot_name = "%t2.0",
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
          .base_name = "s1",
          .size_bytes = 1,
          .align_bytes = 1,
      },
  });
  arg_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%t2.0",
      .value = bir::Value::named(bir::TypeKind::I8, "%t2.global.aggregate.load.0"),
      .align_bytes = 1,
  });
  arg_entry.insts.push_back(bir::CallInst{
      .callee = "show",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "%t2")},
      .arg_types = {bir::TypeKind::Ptr},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 1,
          .align_bytes = 1,
          .passed_on_stack = true,
          .byval_copy = true,
      }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  arg_entry.terminator = bir::ReturnTerminator{};
  arg_function.blocks.push_back(std::move(arg_entry));

  bir::Function main_function;
  main_function.name = "main";
  main_function.return_type = bir::TypeKind::I32;

  bir::Block main_entry;
  main_entry.label = "entry";
  main_entry.insts.push_back(bir::CallInst{
      .callee = "arg",
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  main_entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};
  main_function.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(show));
  module.functions.push_back(std::move(arg_function));
  module.functions.push_back(std::move(printf_decl));
  module.functions.push_back(std::move(main_function));
  return module;
}

bir::Module make_x86_multi_defined_helper_same_module_sret_i8_call_lane_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.globals.push_back(bir::Global{
      .name = "s1",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .initializer_elements = {bir::Value::immediate_i8('0')},
  });

  bir::Function fr_s1;
  fr_s1.name = "fr_s1";
  fr_s1.return_type = bir::TypeKind::Void;
  fr_s1.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%ret.sret",
      .size_bytes = 1,
      .align_bytes = 1,
      .is_sret = true,
  });
  fr_s1.local_slots.push_back(bir::LocalSlot{
      .name = "%t0.0",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .storage_kind = bir::LocalSlotStorageKind::LoweringScratch,
  });

  bir::Block fr_s1_entry;
  fr_s1_entry.label = "entry";
  fr_s1_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "%t0.global.aggregate.load.0"),
      .slot_name = "%t0.0",
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
          .base_name = "s1",
          .size_bytes = 1,
          .align_bytes = 1,
      },
  });
  fr_s1_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%t0.0",
      .value = bir::Value::named(bir::TypeKind::I8, "%t0.global.aggregate.load.0"),
      .align_bytes = 1,
  });
  fr_s1_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "fr_s1.ret.sret.copy.0"),
      .slot_name = "%t0.0",
      .align_bytes = 1,
  });
  fr_s1_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%t0.0",
      .value = bir::Value::named(bir::TypeKind::I8, "fr_s1.ret.sret.copy.0"),
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%ret.sret",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%ret.sret"),
          .size_bytes = 1,
          .align_bytes = 1,
      },
  });
  fr_s1_entry.terminator = bir::ReturnTerminator{};
  fr_s1.blocks.push_back(std::move(fr_s1_entry));

  bir::Function ret_function;
  ret_function.name = "ret";
  ret_function.return_type = bir::TypeKind::Void;
  ret_function.local_slots.push_back(bir::LocalSlot{
      .name = "%t0.0",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .storage_kind = bir::LocalSlotStorageKind::LoweringScratch,
  });

  bir::Block ret_entry;
  ret_entry.label = "entry";
  ret_entry.insts.push_back(bir::CallInst{
      .callee = "fr_s1",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "%t0")},
      .arg_types = {bir::TypeKind::Ptr},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 1,
          .align_bytes = 1,
          .sret_pointer = true,
      }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  ret_entry.terminator = bir::ReturnTerminator{};
  ret_function.blocks.push_back(std::move(ret_entry));

  bir::Function main_function;
  main_function.name = "main";
  main_function.return_type = bir::TypeKind::I32;

  bir::Block main_entry;
  main_entry.label = "entry";
  main_entry.insts.push_back(bir::CallInst{
      .callee = "ret",
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  main_entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};
  main_function.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(fr_s1));
  module.functions.push_back(std::move(ret_function));
  module.functions.push_back(std::move(main_function));
  return module;
}

bir::Module make_x86_multi_defined_helper_same_module_sret_i8_copyout_lane_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.globals.push_back(bir::Global{
      .name = "s1",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .initializer_elements = {bir::Value::immediate_i8('0')},
  });

  bir::Function fr_s1;
  fr_s1.name = "fr_s1";
  fr_s1.return_type = bir::TypeKind::Void;
  fr_s1.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%ret.sret",
      .size_bytes = 1,
      .align_bytes = 1,
      .is_sret = true,
  });
  fr_s1.local_slots.push_back(bir::LocalSlot{
      .name = "%t0.0",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .storage_kind = bir::LocalSlotStorageKind::LoweringScratch,
  });

  bir::Block fr_s1_entry;
  fr_s1_entry.label = "entry";
  fr_s1_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "%t0.global.aggregate.load.0"),
      .slot_name = "%t0.0",
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
          .base_name = "s1",
          .size_bytes = 1,
          .align_bytes = 1,
      },
  });
  fr_s1_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%t0.0",
      .value = bir::Value::named(bir::TypeKind::I8, "%t0.global.aggregate.load.0"),
      .align_bytes = 1,
  });
  fr_s1_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "fr_s1.ret.sret.copy.0"),
      .slot_name = "%t0.0",
      .align_bytes = 1,
  });
  fr_s1_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%t0.0",
      .value = bir::Value::named(bir::TypeKind::I8, "fr_s1.ret.sret.copy.0"),
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%ret.sret",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%ret.sret"),
          .size_bytes = 1,
          .align_bytes = 1,
      },
  });
  fr_s1_entry.terminator = bir::ReturnTerminator{};
  fr_s1.blocks.push_back(std::move(fr_s1_entry));

  bir::Function ret_function;
  ret_function.name = "ret";
  ret_function.return_type = bir::TypeKind::Void;
  ret_function.local_slots.push_back(bir::LocalSlot{
      .name = "%t0.0",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .storage_kind = bir::LocalSlotStorageKind::LoweringScratch,
  });
  ret_function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.t1.0",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
  });

  bir::Block ret_entry;
  ret_entry.label = "entry";
  ret_entry.insts.push_back(bir::CallInst{
      .callee = "fr_s1",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "%t0")},
      .arg_types = {bir::TypeKind::Ptr},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 1,
          .align_bytes = 1,
          .sret_pointer = true,
      }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  ret_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "%lv.t1.aggregate.copy.0"),
      .slot_name = "%t0.0",
      .align_bytes = 1,
  });
  ret_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.t1.0",
      .value = bir::Value::named(bir::TypeKind::I8, "%lv.t1.aggregate.copy.0"),
      .align_bytes = 1,
  });
  ret_entry.terminator = bir::ReturnTerminator{};
  ret_function.blocks.push_back(std::move(ret_entry));

  bir::Function main_function;
  main_function.name = "main";
  main_function.return_type = bir::TypeKind::I32;

  bir::Block main_entry;
  main_entry.label = "entry";
  main_entry.insts.push_back(bir::CallInst{
      .callee = "ret",
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  main_entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};
  main_function.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(fr_s1));
  module.functions.push_back(std::move(ret_function));
  module.functions.push_back(std::move(main_function));
  return module;
}

bir::Module make_x86_multi_defined_helper_same_module_sret_i8x2_copyout_lane_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.globals.push_back(bir::Global{
      .name = "s2",
      .type = bir::TypeKind::I8,
      .size_bytes = 2,
      .align_bytes = 1,
      .initializer_elements = {bir::Value::immediate_i8('1'), bir::Value::immediate_i8('2')},
  });

  bir::Function fr_s2;
  fr_s2.name = "fr_s2";
  fr_s2.return_type = bir::TypeKind::Void;
  fr_s2.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%ret.sret",
      .size_bytes = 2,
      .align_bytes = 1,
      .is_sret = true,
  });
  fr_s2.local_slots.push_back(bir::LocalSlot{
      .name = "%t0.0",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .storage_kind = bir::LocalSlotStorageKind::LoweringScratch,
  });
  fr_s2.local_slots.push_back(bir::LocalSlot{
      .name = "%t0.1",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .storage_kind = bir::LocalSlotStorageKind::LoweringScratch,
  });

  bir::Block fr_s2_entry;
  fr_s2_entry.label = "entry";
  fr_s2_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "%t0.global.aggregate.load.0"),
      .slot_name = "%t0.0",
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
          .base_name = "s2",
          .size_bytes = 1,
          .align_bytes = 1,
      },
  });
  fr_s2_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%t0.0",
      .value = bir::Value::named(bir::TypeKind::I8, "%t0.global.aggregate.load.0"),
      .align_bytes = 1,
  });
  fr_s2_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "%t0.global.aggregate.load.1"),
      .slot_name = "%t0.1",
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
          .base_name = "s2",
          .byte_offset = 1,
          .size_bytes = 1,
          .align_bytes = 1,
      },
  });
  fr_s2_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%t0.1",
      .value = bir::Value::named(bir::TypeKind::I8, "%t0.global.aggregate.load.1"),
      .align_bytes = 1,
  });
  fr_s2_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "fr_s2.ret.sret.copy.0"),
      .slot_name = "%t0.0",
      .align_bytes = 1,
  });
  fr_s2_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%t0.0",
      .value = bir::Value::named(bir::TypeKind::I8, "fr_s2.ret.sret.copy.0"),
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%ret.sret",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%ret.sret"),
          .size_bytes = 1,
          .align_bytes = 1,
      },
  });
  fr_s2_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "fr_s2.ret.sret.copy.1"),
      .slot_name = "%t0.1",
      .align_bytes = 1,
  });
  fr_s2_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%t0.1",
      .value = bir::Value::named(bir::TypeKind::I8, "fr_s2.ret.sret.copy.1"),
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%ret.sret",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%ret.sret"),
          .byte_offset = 1,
          .size_bytes = 1,
          .align_bytes = 1,
      },
  });
  fr_s2_entry.terminator = bir::ReturnTerminator{};
  fr_s2.blocks.push_back(std::move(fr_s2_entry));

  bir::Function ret_function;
  ret_function.name = "ret";
  ret_function.return_type = bir::TypeKind::Void;
  ret_function.local_slots.push_back(bir::LocalSlot{
      .name = "%t0.0",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .storage_kind = bir::LocalSlotStorageKind::LoweringScratch,
  });
  ret_function.local_slots.push_back(bir::LocalSlot{
      .name = "%t0.1",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .storage_kind = bir::LocalSlotStorageKind::LoweringScratch,
  });
  ret_function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.t2.0",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
  });
  ret_function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.t2.1",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
  });

  bir::Block ret_entry;
  ret_entry.label = "entry";
  ret_entry.insts.push_back(bir::CallInst{
      .callee = "fr_s2",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "%t0")},
      .arg_types = {bir::TypeKind::Ptr},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 2,
          .align_bytes = 1,
          .sret_pointer = true,
      }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  ret_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "%lv.t2.aggregate.copy.0"),
      .slot_name = "%t0.0",
      .align_bytes = 1,
  });
  ret_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.t2.0",
      .value = bir::Value::named(bir::TypeKind::I8, "%lv.t2.aggregate.copy.0"),
      .align_bytes = 1,
  });
  ret_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "%lv.t2.aggregate.copy.1"),
      .slot_name = "%t0.1",
      .align_bytes = 1,
  });
  ret_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.t2.1",
      .value = bir::Value::named(bir::TypeKind::I8, "%lv.t2.aggregate.copy.1"),
      .align_bytes = 1,
  });
  ret_entry.terminator = bir::ReturnTerminator{};
  ret_function.blocks.push_back(std::move(ret_entry));

  bir::Function main_function;
  main_function.name = "main";
  main_function.return_type = bir::TypeKind::I32;

  bir::Block main_entry;
  main_entry.label = "entry";
  main_entry.insts.push_back(bir::CallInst{
      .callee = "ret",
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  main_entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};
  main_function.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(fr_s2));
  module.functions.push_back(std::move(ret_function));
  module.functions.push_back(std::move(main_function));
  return module;
}

bir::Module make_x86_multi_defined_helper_same_module_sret_f32_copyout_lane_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.globals.push_back(bir::Global{
      .name = "hfa11",
      .type = bir::TypeKind::F32,
      .size_bytes = 4,
      .align_bytes = 4,
      .initializer_elements = {bir::Value::immediate_f32_bits(0x3f800000U)},
  });

  bir::Function fr_hfa11;
  fr_hfa11.name = "fr_hfa11";
  fr_hfa11.return_type = bir::TypeKind::Void;
  fr_hfa11.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%ret.sret",
      .size_bytes = 4,
      .align_bytes = 4,
      .is_sret = true,
  });
  fr_hfa11.local_slots.push_back(bir::LocalSlot{
      .name = "%t0.0",
      .type = bir::TypeKind::F32,
      .size_bytes = 4,
      .align_bytes = 4,
      .storage_kind = bir::LocalSlotStorageKind::LoweringScratch,
  });

  bir::Block fr_hfa11_entry;
  fr_hfa11_entry.label = "entry";
  fr_hfa11_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F32, "%t0.global.aggregate.load.0"),
      .slot_name = "%t0.0",
      .align_bytes = 4,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
          .base_name = "hfa11",
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  fr_hfa11_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%t0.0",
      .value = bir::Value::named(bir::TypeKind::F32, "%t0.global.aggregate.load.0"),
      .align_bytes = 4,
  });
  fr_hfa11_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F32, "fr_hfa11.ret.sret.copy.0"),
      .slot_name = "%t0.0",
      .align_bytes = 4,
  });
  fr_hfa11_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%t0.0",
      .value = bir::Value::named(bir::TypeKind::F32, "fr_hfa11.ret.sret.copy.0"),
      .align_bytes = 4,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%ret.sret",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%ret.sret"),
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  fr_hfa11_entry.terminator = bir::ReturnTerminator{};
  fr_hfa11.blocks.push_back(std::move(fr_hfa11_entry));

  bir::Function ret_function;
  ret_function.name = "ret";
  ret_function.return_type = bir::TypeKind::Void;
  ret_function.local_slots.push_back(bir::LocalSlot{
      .name = "%t0.0",
      .type = bir::TypeKind::F32,
      .size_bytes = 4,
      .align_bytes = 4,
      .storage_kind = bir::LocalSlotStorageKind::LoweringScratch,
  });
  ret_function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.t0.0",
      .type = bir::TypeKind::F32,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block ret_entry;
  ret_entry.label = "entry";
  ret_entry.insts.push_back(bir::CallInst{
      .callee = "fr_hfa11",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "%t0")},
      .arg_types = {bir::TypeKind::Ptr},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 4,
          .align_bytes = 4,
          .sret_pointer = true,
      }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  ret_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F32, "%lv.t0.aggregate.copy.0"),
      .slot_name = "%t0.0",
      .align_bytes = 4,
  });
  ret_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.t0.0",
      .value = bir::Value::named(bir::TypeKind::F32, "%lv.t0.aggregate.copy.0"),
      .align_bytes = 4,
  });
  ret_entry.terminator = bir::ReturnTerminator{};
  ret_function.blocks.push_back(std::move(ret_entry));

  bir::Function main_function;
  main_function.name = "main";
  main_function.return_type = bir::TypeKind::I32;

  bir::Block main_entry;
  main_entry.label = "entry";
  main_entry.insts.push_back(bir::CallInst{
      .callee = "ret",
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  main_entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};
  main_function.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(fr_hfa11));
  module.functions.push_back(std::move(ret_function));
  module.functions.push_back(std::move(main_function));
  return module;
}

bir::Module make_x86_multi_defined_helper_same_module_sret_f128_copyout_lane_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.globals.push_back(bir::Global{
      .name = "hfa31",
      .type = bir::TypeKind::F128,
      .size_bytes = 16,
      .align_bytes = 16,
  });

  bir::Function fr_hfa31;
  fr_hfa31.name = "fr_hfa31";
  fr_hfa31.return_type = bir::TypeKind::Void;
  fr_hfa31.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%ret.sret",
      .size_bytes = 16,
      .align_bytes = 16,
      .is_sret = true,
  });
  fr_hfa31.local_slots.push_back(bir::LocalSlot{
      .name = "%t0.0",
      .type = bir::TypeKind::F128,
      .size_bytes = 16,
      .align_bytes = 16,
      .storage_kind = bir::LocalSlotStorageKind::LoweringScratch,
  });

  bir::Block fr_hfa31_entry;
  fr_hfa31_entry.label = "entry";
  fr_hfa31_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F128, "%t0.global.aggregate.load.0"),
      .slot_name = "%t0.0",
      .align_bytes = 16,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
          .base_name = "hfa31",
          .size_bytes = 16,
          .align_bytes = 16,
      },
  });
  fr_hfa31_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%t0.0",
      .value = bir::Value::named(bir::TypeKind::F128, "%t0.global.aggregate.load.0"),
      .align_bytes = 16,
  });
  fr_hfa31_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F128, "fr_hfa31.ret.sret.copy.0"),
      .slot_name = "%t0.0",
      .align_bytes = 16,
  });
  fr_hfa31_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%t0.0",
      .value = bir::Value::named(bir::TypeKind::F128, "fr_hfa31.ret.sret.copy.0"),
      .align_bytes = 16,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%ret.sret",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%ret.sret"),
          .size_bytes = 16,
          .align_bytes = 16,
      },
  });
  fr_hfa31_entry.terminator = bir::ReturnTerminator{};
  fr_hfa31.blocks.push_back(std::move(fr_hfa31_entry));

  bir::Function ret_function;
  ret_function.name = "ret";
  ret_function.return_type = bir::TypeKind::Void;
  ret_function.local_slots.push_back(bir::LocalSlot{
      .name = "%t0.0",
      .type = bir::TypeKind::F128,
      .size_bytes = 16,
      .align_bytes = 16,
      .storage_kind = bir::LocalSlotStorageKind::LoweringScratch,
  });
  ret_function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.t0.0",
      .type = bir::TypeKind::F128,
      .size_bytes = 16,
      .align_bytes = 16,
  });

  bir::Block ret_entry;
  ret_entry.label = "entry";
  ret_entry.insts.push_back(bir::CallInst{
      .callee = "fr_hfa31",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "%t0")},
      .arg_types = {bir::TypeKind::Ptr},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 16,
          .align_bytes = 16,
          .sret_pointer = true,
      }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  ret_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F128, "%lv.t0.aggregate.copy.0"),
      .slot_name = "%t0.0",
      .align_bytes = 16,
  });
  ret_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.t0.0",
      .value = bir::Value::named(bir::TypeKind::F128, "%lv.t0.aggregate.copy.0"),
      .align_bytes = 16,
  });
  ret_entry.terminator = bir::ReturnTerminator{};
  ret_function.blocks.push_back(std::move(ret_entry));

  bir::Function main_function;
  main_function.name = "main";
  main_function.return_type = bir::TypeKind::I32;

  bir::Block main_entry;
  main_entry.label = "entry";
  main_entry.insts.push_back(bir::CallInst{
      .callee = "ret",
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  main_entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};
  main_function.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(fr_hfa31));
  module.functions.push_back(std::move(ret_function));
  module.functions.push_back(std::move(main_function));
  return module;
}

bir::Module make_x86_multi_defined_helper_same_module_local_byval_f32_call_lane_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.string_constants.push_back(bir::StringConstant{
      .name = ".str0",
      .bytes = "%.1f\n",
  });
  module.string_constants.push_back(bir::StringConstant{
      .name = ".str1",
      .bytes = "Arguments:\n",
  });
  module.globals.push_back(bir::Global{
      .name = "hfa11",
      .type = bir::TypeKind::F32,
      .size_bytes = 4,
      .align_bytes = 4,
      .initializer_elements = {bir::Value::immediate_f32_bits(0x3f800000U)},
  });

  bir::Function printf_decl;
  printf_decl.name = "printf";
  printf_decl.is_declaration = true;
  printf_decl.return_type = bir::TypeKind::I32;
  printf_decl.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p._anon_param_",
  });

  bir::Function show;
  show.name = "show";
  show.return_type = bir::TypeKind::Void;
  show.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p.a",
      .size_bytes = 4,
      .align_bytes = 4,
      .is_byval = true,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.p.a.0",
      .type = bir::TypeKind::F32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_byval_copy = true,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.a.0",
      .type = bir::TypeKind::F32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_byval_copy = true,
  });

  bir::Block show_entry;
  show_entry.label = "entry";
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F32, "%lv.param.p.a.aggregate.param.copy.0"),
      .slot_name = "%lv.param.p.a.0",
      .align_bytes = 4,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%p.a",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%p.a"),
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.p.a.0",
      .value = bir::Value::named(bir::TypeKind::F32, "%lv.param.p.a.aggregate.param.copy.0"),
      .align_bytes = 4,
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F32, "%lv.param.a.byval.copy.0"),
      .slot_name = "%lv.param.a.0",
      .align_bytes = 4,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%p.a",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%p.a"),
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.a.0",
      .value = bir::Value::named(bir::TypeKind::F32, "%lv.param.a.byval.copy.0"),
      .align_bytes = 4,
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F32, "%t2"),
      .slot_name = "%lv.param.a.0",
      .align_bytes = 4,
  });
  show_entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::FPExt,
      .result = bir::Value::named(bir::TypeKind::F64, "%t3"),
      .operand = bir::Value::named(bir::TypeKind::F32, "%t2"),
  });
  show_entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t4"),
      .callee = "printf",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@.str0"),
               bir::Value::named(bir::TypeKind::F64, "%t3")},
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::F64},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_variadic = true,
  });
  show_entry.terminator = bir::ReturnTerminator{};
  show.blocks.push_back(std::move(show_entry));

  bir::Function arg_function;
  arg_function.name = "arg";
  arg_function.return_type = bir::TypeKind::Void;
  arg_function.local_slots.push_back(bir::LocalSlot{
      .name = "%t2.0",
      .type = bir::TypeKind::F32,
      .size_bytes = 4,
      .align_bytes = 4,
      .storage_kind = bir::LocalSlotStorageKind::LoweringScratch,
  });

  bir::Block arg_entry;
  arg_entry.label = "entry";
  arg_entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t1"),
      .callee = "printf",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@.str1")},
      .arg_types = {bir::TypeKind::Ptr},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_variadic = true,
  });
  arg_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F32, "%t2.global.aggregate.load.0"),
      .slot_name = "%t2.0",
      .align_bytes = 4,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
          .base_name = "hfa11",
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  arg_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%t2.0",
      .value = bir::Value::named(bir::TypeKind::F32, "%t2.global.aggregate.load.0"),
      .align_bytes = 4,
  });
  arg_entry.insts.push_back(bir::CallInst{
      .callee = "show",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "%t2")},
      .arg_types = {bir::TypeKind::Ptr},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 4,
          .align_bytes = 4,
          .passed_on_stack = true,
          .byval_copy = true,
      }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  arg_entry.terminator = bir::ReturnTerminator{};
  arg_function.blocks.push_back(std::move(arg_entry));

  bir::Function main_function;
  main_function.name = "main";
  main_function.return_type = bir::TypeKind::I32;

  bir::Block main_entry;
  main_entry.label = "entry";
  main_entry.insts.push_back(bir::CallInst{
      .callee = "arg",
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  main_entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};
  main_function.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(show));
  module.functions.push_back(std::move(arg_function));
  module.functions.push_back(std::move(printf_decl));
  module.functions.push_back(std::move(main_function));
  return module;
}

bir::Module make_x86_multi_defined_helper_same_module_local_byval_f32_trivial_entry_module() {
  bir::Module module = make_x86_multi_defined_helper_same_module_local_byval_f32_call_lane_module();

  module.functions.erase(
      std::remove_if(module.functions.begin(),
                     module.functions.end(),
                     [](const bir::Function& function) { return function.name == "arg"; }),
      module.functions.end());

  auto main_it = std::find_if(module.functions.begin(),
                              module.functions.end(),
                              [](const bir::Function& function) { return function.name == "main"; });
  if (main_it != module.functions.end()) {
    main_it->blocks.clear();
    main_it->blocks.push_back(bir::Block{
        .label = "entry",
        .terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)},
    });
  }

  return module;
}

bir::Module make_x86_multi_defined_helper_same_module_late_stack_ptr_variadic_helper_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.string_constants.push_back(bir::StringConstant{
      .name = ".str0",
      .bytes = "%.1s %.1f %.1f %.2s %.1f %.1f %.3s\n",
  });

  bir::Function printf_decl;
  printf_decl.name = "printf";
  printf_decl.is_declaration = true;
  printf_decl.return_type = bir::TypeKind::I32;
  printf_decl.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p._anon_param_",
  });

  bir::Function show;
  show.name = "show";
  show.return_type = bir::TypeKind::Void;
  show.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p.a",
      .size_bytes = 1,
      .align_bytes = 1,
      .is_byval = true,
  });
  show.params.push_back(bir::Param{
      .type = bir::TypeKind::F32,
      .name = "%x.0",
  });
  show.params.push_back(bir::Param{
      .type = bir::TypeKind::F32,
      .name = "%x.1",
  });
  show.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p.b",
      .size_bytes = 2,
      .align_bytes = 1,
      .is_byval = true,
  });
  show.params.push_back(bir::Param{
      .type = bir::TypeKind::F32,
      .name = "%y.0",
  });
  show.params.push_back(bir::Param{
      .type = bir::TypeKind::F32,
      .name = "%y.1",
  });
  show.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p.c",
      .size_bytes = 3,
      .align_bytes = 1,
      .is_byval = true,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.a.0",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .is_byval_copy = true,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.b.0",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .is_byval_copy = true,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.b.1",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .is_byval_copy = true,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.c.0",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .is_byval_copy = true,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.c.1",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .is_byval_copy = true,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.c.2",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .is_byval_copy = true,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.x.0",
      .type = bir::TypeKind::F32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.x.4",
      .type = bir::TypeKind::F32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.y.0",
      .type = bir::TypeKind::F32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.y.4",
      .type = bir::TypeKind::F32,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block show_entry;
  show_entry.label = "entry";
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "%a.copy.0"),
      .slot_name = "%lv.param.a.0",
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%p.a",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%p.a"),
          .size_bytes = 1,
          .align_bytes = 1,
      },
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.a.0",
      .value = bir::Value::named(bir::TypeKind::I8, "%a.copy.0"),
      .align_bytes = 1,
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.x.0",
      .value = bir::Value::named(bir::TypeKind::F32, "%x.0"),
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F32, "%field0"),
      .slot_name = "%lv.param.x.0",
      .align_bytes = 4,
  });
  show_entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::FPExt,
      .result = bir::Value::named(bir::TypeKind::F64, "%wide0"),
      .operand = bir::Value::named(bir::TypeKind::F32, "%field0"),
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.x.4",
      .value = bir::Value::named(bir::TypeKind::F32, "%x.1"),
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F32, "%field1"),
      .slot_name = "%lv.param.x.4",
      .align_bytes = 4,
  });
  show_entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::FPExt,
      .result = bir::Value::named(bir::TypeKind::F64, "%wide1"),
      .operand = bir::Value::named(bir::TypeKind::F32, "%field1"),
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "%b.copy.0"),
      .slot_name = "%lv.param.b.0",
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%p.b",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%p.b"),
          .size_bytes = 2,
          .align_bytes = 1,
      },
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.b.0",
      .value = bir::Value::named(bir::TypeKind::I8, "%b.copy.0"),
      .align_bytes = 1,
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "%b.copy.1"),
      .slot_name = "%lv.param.b.1",
      .byte_offset = 1,
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%p.b",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%p.b"),
          .byte_offset = 1,
          .size_bytes = 2,
          .align_bytes = 1,
      },
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.b.1",
      .value = bir::Value::named(bir::TypeKind::I8, "%b.copy.1"),
      .align_bytes = 1,
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.y.0",
      .value = bir::Value::named(bir::TypeKind::F32, "%y.0"),
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F32, "%field2"),
      .slot_name = "%lv.param.y.0",
      .align_bytes = 4,
  });
  show_entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::FPExt,
      .result = bir::Value::named(bir::TypeKind::F64, "%wide2"),
      .operand = bir::Value::named(bir::TypeKind::F32, "%field2"),
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.y.4",
      .value = bir::Value::named(bir::TypeKind::F32, "%y.1"),
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F32, "%field3"),
      .slot_name = "%lv.param.y.4",
      .align_bytes = 4,
  });
  show_entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::FPExt,
      .result = bir::Value::named(bir::TypeKind::F64, "%wide3"),
      .operand = bir::Value::named(bir::TypeKind::F32, "%field3"),
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "%c.copy.0"),
      .slot_name = "%lv.param.c.0",
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%p.c",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%p.c"),
          .size_bytes = 3,
          .align_bytes = 1,
      },
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.c.0",
      .value = bir::Value::named(bir::TypeKind::I8, "%c.copy.0"),
      .align_bytes = 1,
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "%c.copy.1"),
      .slot_name = "%lv.param.c.1",
      .byte_offset = 1,
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%p.c",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%p.c"),
          .byte_offset = 1,
          .size_bytes = 3,
          .align_bytes = 1,
      },
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.c.1",
      .value = bir::Value::named(bir::TypeKind::I8, "%c.copy.1"),
      .align_bytes = 1,
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I8, "%c.copy.2"),
      .slot_name = "%lv.param.c.2",
      .byte_offset = 2,
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_name = "%p.c",
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "%p.c"),
          .byte_offset = 2,
          .size_bytes = 3,
          .align_bytes = 1,
      },
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.c.2",
      .value = bir::Value::named(bir::TypeKind::I8, "%c.copy.2"),
      .align_bytes = 1,
  });
  show_entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%call"),
      .callee = "printf",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@.str0"),
               bir::Value::named(bir::TypeKind::Ptr, "%lv.param.a.0"),
               bir::Value::named(bir::TypeKind::F64, "%wide0"),
               bir::Value::named(bir::TypeKind::F64, "%wide1"),
               bir::Value::named(bir::TypeKind::Ptr, "%lv.param.b.0"),
               bir::Value::named(bir::TypeKind::F64, "%wide2"),
               bir::Value::named(bir::TypeKind::F64, "%wide3"),
               bir::Value::named(bir::TypeKind::Ptr, "%lv.param.c.0")},
      .arg_types = {bir::TypeKind::Ptr,
                    bir::TypeKind::Ptr,
                    bir::TypeKind::F64,
                    bir::TypeKind::F64,
                    bir::TypeKind::Ptr,
                    bir::TypeKind::F64,
                    bir::TypeKind::F64,
                    bir::TypeKind::Ptr},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_variadic = true,
  });
  show_entry.terminator = bir::ReturnTerminator{};
  show.blocks.push_back(std::move(show_entry));

  bir::Function main_function;
  main_function.name = "main";
  main_function.return_type = bir::TypeKind::I32;
  main_function.blocks.push_back(bir::Block{
      .label = "entry",
      .terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)},
  });

  module.functions.push_back(std::move(show));
  module.functions.push_back(std::move(printf_decl));
  module.functions.push_back(std::move(main_function));
  return module;
}

bir::Module make_x86_multi_defined_float_helper_call_lane_module() {
  bir::Module module = make_x86_multi_defined_direct_call_lane_module();
  module.string_constants.push_back(bir::StringConstant{
      .name = ".str1",
      .bytes = "%.1f %.1f\n",
  });

  bir::Function show;
  show.name = "show";
  show.return_type = bir::TypeKind::Void;
  show.params.push_back(bir::Param{
      .type = bir::TypeKind::F32,
      .name = "%x.0",
  });
  show.params.push_back(bir::Param{
      .type = bir::TypeKind::F32,
      .name = "%x.1",
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.x.0",
      .type = bir::TypeKind::F32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.x.4",
      .type = bir::TypeKind::F32,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block show_entry;
  show_entry.label = "entry";
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.x.0",
      .value = bir::Value::named(bir::TypeKind::F32, "%x.0"),
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.x.4",
      .value = bir::Value::named(bir::TypeKind::F32, "%x.1"),
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F32, "%field0"),
      .slot_name = "%lv.param.x.0",
      .align_bytes = 4,
  });
  show_entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::FPExt,
      .result = bir::Value::named(bir::TypeKind::F64, "%wide0"),
      .operand = bir::Value::named(bir::TypeKind::F32, "%field0"),
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F32, "%field1"),
      .slot_name = "%lv.param.x.4",
      .align_bytes = 4,
  });
  show_entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::FPExt,
      .result = bir::Value::named(bir::TypeKind::F64, "%wide1"),
      .operand = bir::Value::named(bir::TypeKind::F32, "%field1"),
  });
  show_entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%call"),
      .callee = "printf",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@.str1"),
               bir::Value::named(bir::TypeKind::F64, "%wide0"),
               bir::Value::named(bir::TypeKind::F64, "%wide1")},
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::F64, bir::TypeKind::F64},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_variadic = true,
  });
  show_entry.terminator = bir::ReturnTerminator{};
  show.blocks.push_back(std::move(show_entry));

  module.functions.insert(module.functions.begin() + 2, std::move(show));
  return module;
}

bir::Module make_x86_multi_defined_f128_helper_call_lane_module() {
  bir::Module module = make_x86_multi_defined_direct_call_lane_module();
  module.string_constants.push_back(bir::StringConstant{
      .name = ".str1",
      .bytes = "%.1Lf\n",
  });

  bir::Function show;
  show.name = "show";
  show.return_type = bir::TypeKind::Void;
  show.params.push_back(bir::Param{
      .type = bir::TypeKind::F128,
      .name = "%x.0",
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.x.0",
      .type = bir::TypeKind::F128,
      .size_bytes = 16,
      .align_bytes = 16,
  });

  bir::Block show_entry;
  show_entry.label = "entry";
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.x.0",
      .value = bir::Value::named(bir::TypeKind::F128, "%x.0"),
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F128, "%field0"),
      .slot_name = "%lv.param.x.0",
      .align_bytes = 16,
  });
  show_entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%call"),
      .callee = "printf",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@.str1"),
               bir::Value::named(bir::TypeKind::F128, "%field0")},
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::F128},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_variadic = true,
  });
  show_entry.terminator = bir::ReturnTerminator{};
  show.blocks.push_back(std::move(show_entry));

  module.functions.insert(module.functions.begin() + 2, std::move(show));
  return module;
}

bir::Module make_x86_multi_defined_mixed_f128_helper_call_lane_module() {
  bir::Module module = make_x86_multi_defined_direct_call_lane_module();
  module.string_constants.push_back(bir::StringConstant{
      .name = ".str1",
      .bytes = "%s %.1f %.1f %s %.1Lf %.1Lf\n",
  });

  bir::Function show;
  show.name = "show";
  show.return_type = bir::TypeKind::Void;
  show.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p.0",
  });
  show.params.push_back(bir::Param{
      .type = bir::TypeKind::F32,
      .name = "%x.0",
  });
  show.params.push_back(bir::Param{
      .type = bir::TypeKind::F32,
      .name = "%x.1",
  });
  show.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p.1",
  });
  show.params.push_back(bir::Param{
      .type = bir::TypeKind::F128,
      .name = "%y.0",
  });
  show.params.push_back(bir::Param{
      .type = bir::TypeKind::F128,
      .name = "%y.1",
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.x.0",
      .type = bir::TypeKind::F32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.x.4",
      .type = bir::TypeKind::F32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.y.0",
      .type = bir::TypeKind::F128,
      .size_bytes = 16,
      .align_bytes = 16,
  });
  show.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.param.y.16",
      .type = bir::TypeKind::F128,
      .size_bytes = 16,
      .align_bytes = 16,
  });

  bir::Block show_entry;
  show_entry.label = "entry";
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.x.0",
      .value = bir::Value::named(bir::TypeKind::F32, "%x.0"),
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.x.4",
      .value = bir::Value::named(bir::TypeKind::F32, "%x.1"),
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F32, "%field0"),
      .slot_name = "%lv.param.x.0",
      .align_bytes = 4,
  });
  show_entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::FPExt,
      .result = bir::Value::named(bir::TypeKind::F64, "%wide0"),
      .operand = bir::Value::named(bir::TypeKind::F32, "%field0"),
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F32, "%field1"),
      .slot_name = "%lv.param.x.4",
      .align_bytes = 4,
  });
  show_entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::FPExt,
      .result = bir::Value::named(bir::TypeKind::F64, "%wide1"),
      .operand = bir::Value::named(bir::TypeKind::F32, "%field1"),
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.y.0",
      .value = bir::Value::named(bir::TypeKind::F128, "%y.0"),
  });
  show_entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%lv.param.y.16",
      .value = bir::Value::named(bir::TypeKind::F128, "%y.1"),
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F128, "%long0"),
      .slot_name = "%lv.param.y.0",
      .align_bytes = 16,
  });
  show_entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::F128, "%long1"),
      .slot_name = "%lv.param.y.16",
      .align_bytes = 16,
  });
  show_entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%call"),
      .callee = "printf",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@.str1"),
               bir::Value::named(bir::TypeKind::Ptr, "%p.0"),
               bir::Value::named(bir::TypeKind::F64, "%wide0"),
               bir::Value::named(bir::TypeKind::F64, "%wide1"),
               bir::Value::named(bir::TypeKind::Ptr, "%p.1"),
               bir::Value::named(bir::TypeKind::F128, "%long0"),
               bir::Value::named(bir::TypeKind::F128, "%long1")},
      .arg_types = {bir::TypeKind::Ptr,
                    bir::TypeKind::Ptr,
                    bir::TypeKind::F64,
                    bir::TypeKind::F64,
                    bir::TypeKind::Ptr,
                    bir::TypeKind::F128,
                    bir::TypeKind::F128},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .is_variadic = true,
  });
  show_entry.terminator = bir::ReturnTerminator{};
  show.blocks.push_back(std::move(show_entry));

  module.functions.insert(module.functions.begin() + 2, std::move(show));
  return module;
}

bir::Module make_x86_multi_defined_helper_same_module_variadic_stack_ptr_call_lane_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";
  module.string_constants.push_back(bir::StringConstant{
      .name = ".str0",
      .bytes = "%9s\n",
  });
  module.globals.push_back(bir::Global{
      .name = "s9",
      .type = bir::TypeKind::I8,
      .size_bytes = 1,
      .align_bytes = 1,
      .initializer_elements = {bir::Value::immediate_i8('0')},
  });
  const auto fixed_format_abi =
      c4c::backend::lir_to_bir_detail::compute_call_arg_abi(x86_target_profile(),
                                                            bir::TypeKind::Ptr);
  if (!fixed_format_abi.has_value()) {
    throw std::runtime_error(
        "missing canonical x86 pointer ABI for same-module variadic handoff test");
  }

  bir::Function myprintf;
  myprintf.name = "myprintf";
  myprintf.return_type = bir::TypeKind::Void;
  myprintf.is_variadic = true;
  myprintf.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p.format",
  });
  myprintf.blocks.push_back(bir::Block{
      .label = "entry",
      .terminator = bir::ReturnTerminator{},
  });

  bir::Function arg_function;
  arg_function.name = "arg";
  arg_function.return_type = bir::TypeKind::Void;

  bir::Block arg_entry;
  arg_entry.label = "entry";
  arg_entry.insts.push_back(bir::CallInst{
      .callee = "myprintf",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "@.str0"),
               bir::Value::immediate_i64(11),
               bir::Value::immediate_i64(22),
               bir::Value::immediate_i64(33),
               bir::Value::immediate_i64(44),
               bir::Value::named(bir::TypeKind::Ptr, "@s9"),
               bir::Value::named(bir::TypeKind::Ptr, "@s9"),
               bir::Value::named(bir::TypeKind::Ptr, "@s9"),
               bir::Value::named(bir::TypeKind::Ptr, "@s9")},
      .arg_types = {bir::TypeKind::Ptr,
                    bir::TypeKind::I64,
                    bir::TypeKind::I64,
                    bir::TypeKind::I64,
                    bir::TypeKind::I64,
                    bir::TypeKind::Ptr,
                    bir::TypeKind::Ptr,
                    bir::TypeKind::Ptr,
                    bir::TypeKind::Ptr},
      .arg_abi = {*fixed_format_abi},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
      .is_variadic = true,
  });
  arg_entry.terminator = bir::ReturnTerminator{};
  arg_function.blocks.push_back(std::move(arg_entry));

  bir::Function main_function;
  main_function.name = "main";
  main_function.return_type = bir::TypeKind::I32;

  bir::Block main_entry;
  main_entry.label = "entry";
  main_entry.insts.push_back(bir::CallInst{
      .callee = "arg",
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  main_entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};
  main_function.blocks.push_back(std::move(main_entry));

  module.functions.push_back(std::move(myprintf));
  module.functions.push_back(std::move(arg_function));
  module.functions.push_back(std::move(main_function));
  return module;
}

int check_route_outputs(const bir::Module& module,
                        const std::string& expected_asm,
                        const std::string& expected_bir_fragment,
                        const char* failure_context) {
  c4c::TargetProfile target_profile;
  const auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto prepared_bir_text = bir::print(prepared.module);

  std::string prepared_asm;
  try {
    prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  } catch (const std::exception& ex) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer rejected the prepared handoff with exception: " +
                 ex.what())
                    .c_str());
  }
  if (prepared_asm != expected_asm) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer did not emit the canonical asm")
                    .c_str());
  }

  const auto public_asm = c4c::backend::emit_x86_bir_module_entry(module, target_profile);
  if (public_asm != prepared_asm) {
    return fail((std::string(failure_context) +
                 ": explicit x86 BIR entry no longer routes through the x86 prepared-module consumer")
                    .c_str());
  }

  const auto generic_asm = c4c::backend::emit_module(
      BackendModuleInput{module}, BackendOptions{.target_profile = x86_target_profile()});
  if (generic_asm != public_asm) {
    return fail((std::string(failure_context) +
                 ": generic backend emit path no longer routes x86 BIR input through emit_x86_bir_module_entry")
                    .c_str());
  }

  if (prepared_bir_text.find(expected_bir_fragment) == std::string::npos) {
    return fail((std::string(failure_context) +
                 ": test fixture no longer prepares the expected semantic BIR shape before routing into x86")
                    .c_str());
  }

  return 0;
}

int check_route_contains_fragments(
    const bir::Module& module,
    std::initializer_list<std::string_view> asm_fragments,
    std::string_view expected_bir_fragment,
    const char* failure_context) {
  c4c::TargetProfile target_profile;
  const auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto prepared_bir_text = bir::print(prepared.module);

  std::string prepared_asm;
  try {
    prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  } catch (const std::exception& ex) {
    return fail((std::string(failure_context) +
                 ": x86 prepared-module consumer rejected the prepared handoff with exception: " +
                 ex.what())
                    .c_str());
  }
  for (const auto fragment : asm_fragments) {
    if (prepared_asm.find(fragment) == std::string::npos) {
      return fail((std::string(failure_context) +
                   ": x86 prepared-module consumer did not emit the expected helper-lane asm fragment")
                      .c_str());
    }
  }

  const auto public_asm = c4c::backend::emit_x86_bir_module_entry(module, target_profile);
  if (public_asm != prepared_asm) {
    return fail((std::string(failure_context) +
                 ": explicit x86 BIR entry no longer routes through the x86 prepared-module consumer")
                    .c_str());
  }

  const auto generic_asm = c4c::backend::emit_module(
      BackendModuleInput{module}, BackendOptions{.target_profile = x86_target_profile()});
  if (generic_asm != public_asm) {
    return fail((std::string(failure_context) +
                 ": generic backend emit path no longer routes x86 BIR input through emit_x86_bir_module_entry")
                    .c_str());
  }

  if (prepared_bir_text.find(expected_bir_fragment) == std::string::npos) {
    return fail((std::string(failure_context) +
                 ": test fixture no longer prepares the expected semantic BIR helper shape before routing into x86")
                    .c_str());
  }

  return 0;
}

std::optional<prepare::PreparedBirModule> lower_and_prepare_local_byval_home_pointer_call_module() {
  lir::LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  module.type_decls.push_back("%struct.s1 = type { [1 x i8] }");

  lir::LirFunction sink_decl;
  sink_decl.name = "sink";
  sink_decl.is_declaration = true;
  sink_decl.signature_text = "declare void @sink(ptr)";
  module.functions.push_back(std::move(sink_decl));

  lir::LirFunction function;
  function.name = "lowered_local_byval_home_pointer_call";
  function.signature_text =
      "define void @lowered_local_byval_home_pointer_call(%struct.s1 %p.a)";
  function.alloca_insts.push_back(lir::LirAllocaOp{
      .result = lir::LirOperand("%lv.param.a"),
      .type_str = "%struct.s1",
      .count = lir::LirOperand(""),
      .align = 1,
  });

  lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(lir::LirStoreOp{
      .type_str = "%struct.s1",
      .val = lir::LirOperand("%p.a"),
      .ptr = lir::LirOperand("%lv.param.a"),
  });
  entry.insts.push_back(lir::LirGepOp{
      .result = lir::LirOperand("%t2"),
      .element_type = "%struct.s1",
      .ptr = lir::LirOperand("%lv.param.a"),
      .indices = {lir::LirOperand("i64 0"), lir::LirOperand("i64 0")},
  });
  entry.insts.push_back(lir::LirCallOp{
      .result = lir::LirOperand(""),
      .return_type = "void",
      .callee = lir::LirOperand("@sink"),
      .callee_type_suffix = "(ptr)",
      .args_str = "ptr %t2",
  });
  entry.terminator = lir::LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  auto lowered = c4c::backend::try_lower_to_bir_with_options(
      module, c4c::backend::BirLoweringOptions{});
  if (!lowered.module.has_value()) {
    return std::nullopt;
  }

  return prepare::prepare_semantic_bir_module_with_options(std::move(*lowered.module),
                                                           x86_target_profile());
}

std::optional<prepare::PreparedBirModule> lower_and_prepare_byval_param_pointer_call_module() {
  lir::LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  module.type_decls.push_back("%struct.s1 = type { [1 x i8] }");

  lir::LirFunction sink_decl;
  sink_decl.name = "sink";
  sink_decl.is_declaration = true;
  sink_decl.signature_text = "declare void @sink(ptr)";
  module.functions.push_back(std::move(sink_decl));

  lir::LirFunction function;
  function.name = "lowered_byval_param_pointer_call";
  function.signature_text = "define void @lowered_byval_param_pointer_call(%struct.s1 %p.a)";

  lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(lir::LirGepOp{
      .result = lir::LirOperand("%t2"),
      .element_type = "%struct.s1",
      .ptr = lir::LirOperand("%p.a"),
      .indices = {lir::LirOperand("i64 0"), lir::LirOperand("i64 0")},
  });
  entry.insts.push_back(lir::LirCallOp{
      .result = lir::LirOperand(""),
      .return_type = "void",
      .callee = lir::LirOperand("@sink"),
      .callee_type_suffix = "(ptr)",
      .args_str = "ptr %t2",
  });
  entry.terminator = lir::LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  auto lowered = c4c::backend::try_lower_to_bir_with_options(
      module, c4c::backend::BirLoweringOptions{});
  if (!lowered.module.has_value()) {
    return std::nullopt;
  }

  return prepare::prepare_semantic_bir_module_with_options(std::move(*lowered.module),
                                                           x86_target_profile());
}

std::optional<prepare::PreparedBirModule> lower_and_prepare_indirect_aggregate_param_pointer_call_module() {
  lir::LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  module.type_decls.push_back("%struct.s17 = type { [17 x i8] }");

  lir::LirFunction sink_decl;
  sink_decl.name = "sink";
  sink_decl.is_declaration = true;
  sink_decl.signature_text = "declare void @sink(ptr)";
  module.functions.push_back(std::move(sink_decl));

  lir::LirFunction function;
  function.name = "lowered_indirect_aggregate_param_pointer_call";
  function.signature_text =
      "define void @lowered_indirect_aggregate_param_pointer_call(%struct.s17 %p.a)";

  lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(lir::LirGepOp{
      .result = lir::LirOperand("%t2"),
      .element_type = "%struct.s17",
      .ptr = lir::LirOperand("%p.a"),
      .indices = {lir::LirOperand("i64 0"), lir::LirOperand("i64 0")},
  });
  entry.insts.push_back(lir::LirCallOp{
      .result = lir::LirOperand(""),
      .return_type = "void",
      .callee = lir::LirOperand("@sink"),
      .callee_type_suffix = "(ptr)",
      .args_str = "ptr %t2",
  });
  entry.terminator = lir::LirRet{
      .value_str = std::nullopt,
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  auto lowered = c4c::backend::try_lower_to_bir_with_options(
      module, c4c::backend::BirLoweringOptions{});
  if (!lowered.module.has_value()) {
    return std::nullopt;
  }

  return prepare::prepare_semantic_bir_module_with_options(std::move(*lowered.module),
                                                           x86_target_profile());
}

prepare::PreparedValueLocationFunction* find_mutable_prepared_value_location_function(
    prepare::PreparedBirModule& prepared,
    std::string_view function_name) {
  const auto function_name_id = prepare::resolve_prepared_function_name_id(prepared.names, function_name);
  if (!function_name_id.has_value()) {
    return nullptr;
  }
  for (auto& function_locations : prepared.value_locations.functions) {
    if (function_locations.function_name == *function_name_id) {
      return &function_locations;
    }
  }
  return nullptr;
}

prepare::PreparedValueHome* find_mutable_prepared_value_home(
    prepare::PreparedBirModule& prepared,
    prepare::PreparedValueLocationFunction& function_locations,
    std::string_view value_name) {
  const auto value_name_id = prepare::resolve_prepared_value_name_id(prepared.names, value_name);
  if (!value_name_id.has_value()) {
    return nullptr;
  }
  for (auto& value_home : function_locations.value_homes) {
    if (value_home.value_name == *value_name_id) {
      return &value_home;
    }
  }
  return nullptr;
}

prepare::PreparedMoveBundle* find_mutable_prepared_move_bundle(
    prepare::PreparedValueLocationFunction& function_locations,
    prepare::PreparedMovePhase phase,
    std::size_t block_index,
    std::size_t instruction_index) {
  for (auto& move_bundle : function_locations.move_bundles) {
    if (move_bundle.phase == phase && move_bundle.block_index == block_index &&
        move_bundle.instruction_index == instruction_index) {
      return &move_bundle;
    }
  }
  return nullptr;
}

void erase_prepared_move_bundle(prepare::PreparedValueLocationFunction& function_locations,
                                prepare::PreparedMovePhase phase,
                                std::size_t block_index,
                                std::size_t instruction_index) {
  function_locations.move_bundles.erase(
      std::remove_if(function_locations.move_bundles.begin(),
                     function_locations.move_bundles.end(),
                     [&](const prepare::PreparedMoveBundle& move_bundle) {
                       return move_bundle.phase == phase &&
                              move_bundle.block_index == block_index &&
                              move_bundle.instruction_index == instruction_index;
                     }),
      function_locations.move_bundles.end());
}

bool has_prepared_call_argument_register_binding(
    const prepare::PreparedValueLocationFunction& function_locations,
    std::size_t abi_index,
    std::string_view register_name) {
  for (const auto& move_bundle : function_locations.move_bundles) {
    if (move_bundle.phase != prepare::PreparedMovePhase::BeforeCall) {
      continue;
    }
    for (const auto& move : move_bundle.moves) {
      if (move.destination_kind != prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
          move.destination_abi_index != std::optional<std::size_t>{abi_index}) {
        continue;
      }
      if (move.destination_register_name == register_name) {
        return true;
      }
    }
  }
  return false;
}

int check_route_consumes_prepared_call_move_bundle_contract() {
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(make_x86_multi_defined_direct_call_lane_module(),
                                                        x86_target_profile());
  auto* function_locations =
      find_mutable_prepared_value_location_function(prepared, "main");
  if (function_locations == nullptr) {
    return fail("bounded multi-defined call contract drift route: missing prepared value-location function");
  }

  auto* first_call_result_home =
      find_mutable_prepared_value_home(prepared, *function_locations, "%t1");
  if (first_call_result_home == nullptr) {
    return fail("bounded multi-defined call contract drift route: missing first call result home");
  }
  first_call_result_home->kind = prepare::PreparedValueHomeKind::Register;
  first_call_result_home->register_name = "r10";
  first_call_result_home->slot_id.reset();
  first_call_result_home->offset_bytes.reset();
  first_call_result_home->immediate_i32.reset();

  auto* first_printf_arg_home =
      find_mutable_prepared_value_home(prepared, *function_locations, "%t3");
  if (first_printf_arg_home == nullptr) {
    return fail("bounded multi-defined call contract drift route: missing first printf i32 argument home");
  }
  auto* first_printf_before_call = find_mutable_prepared_move_bundle(
      *function_locations, prepare::PreparedMovePhase::BeforeCall, 0, 3);
  if (first_printf_before_call == nullptr) {
    function_locations->move_bundles.push_back(prepare::PreparedMoveBundle{
        .function_name = function_locations->function_name,
        .phase = prepare::PreparedMovePhase::BeforeCall,
        .block_index = 0,
        .instruction_index = 3,
        .moves = {},
    });
    first_printf_before_call = &function_locations->move_bundles.back();
  }
  bool rewired_printf_arg = false;
  for (auto& move : first_printf_before_call->moves) {
    if (move.destination_kind != prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
        move.destination_abi_index != std::optional<std::size_t>{1}) {
      continue;
    }
    move.destination_register_name = "rdx";
    rewired_printf_arg = true;
    break;
  }
  if (!rewired_printf_arg) {
    first_printf_before_call->moves.push_back(prepare::PreparedMoveResolution{
        .from_value_id = first_printf_arg_home->value_id,
        .to_value_id = first_printf_arg_home->value_id,
        .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
        .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
        .destination_abi_index = std::size_t{1},
        .destination_register_name = std::string("rdx"),
        .block_index = 0,
        .instruction_index = 3,
        .uses_cycle_temp_source = false,
        .reason = "contract_call_arg_register_to_register",
    });
  }

  auto* first_call_after_call = find_mutable_prepared_move_bundle(
      *function_locations, prepare::PreparedMovePhase::AfterCall, 0, 0);
  if (first_call_after_call == nullptr) {
    function_locations->move_bundles.push_back(prepare::PreparedMoveBundle{
        .function_name = function_locations->function_name,
        .phase = prepare::PreparedMovePhase::AfterCall,
        .block_index = 0,
        .instruction_index = 0,
        .moves = {},
    });
    first_call_after_call = &function_locations->move_bundles.back();
  }
  bool saw_first_call_result_move = false;
  for (auto& move : first_call_after_call->moves) {
    if (move.destination_kind != prepare::PreparedMoveDestinationKind::CallResultAbi ||
        move.to_value_id != first_call_result_home->value_id) {
      continue;
    }
    move.destination_storage_kind = prepare::PreparedMoveStorageKind::Register;
    move.destination_register_name = "rax";
    saw_first_call_result_move = true;
    break;
  }
  if (!saw_first_call_result_move) {
    first_call_after_call->moves.push_back(prepare::PreparedMoveResolution{
        .from_value_id = first_call_result_home->value_id,
        .to_value_id = first_call_result_home->value_id,
        .destination_kind = prepare::PreparedMoveDestinationKind::CallResultAbi,
        .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
        .destination_abi_index = std::nullopt,
        .destination_register_name = std::string("rax"),
        .block_index = 0,
        .instruction_index = 0,
        .uses_cycle_temp_source = false,
        .reason = "contract_call_result_register_to_register",
    });
  }

  std::string prepared_asm;
  try {
    prepared_asm = c4c::backend::x86::api::emit_prepared_module(prepared);
  } catch (const std::exception& ex) {
    return fail((std::string("bounded multi-defined call contract drift route: x86 prepared-module consumer rejected the mutated prepared handoff with exception: ") +
                 ex.what())
                    .c_str());
  }
  if (prepared_asm != expected_multi_defined_direct_call_lane_contract_drift_asm()) {
    return fail("bounded multi-defined call contract drift route: x86 prepared-module consumer stopped following prepared BeforeCall/AfterCall call-lane metadata");
  }

  return 0;
}

int check_route_requires_authoritative_prepared_after_call_bundle() {
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(make_x86_multi_defined_direct_call_lane_module(),
                                                        x86_target_profile());
  auto* function_locations =
      find_mutable_prepared_value_location_function(prepared, "main");
  if (function_locations == nullptr) {
    return fail("bounded multi-defined call contract drift route: missing prepared value-location function");
  }

  auto* first_call_result_home =
      find_mutable_prepared_value_home(prepared, *function_locations, "%t1");
  if (first_call_result_home == nullptr) {
    return fail("bounded multi-defined call contract drift route: missing first call result home");
  }
  first_call_result_home->kind = prepare::PreparedValueHomeKind::Register;
  first_call_result_home->register_name = "r10";
  first_call_result_home->slot_id.reset();
  first_call_result_home->offset_bytes.reset();
  first_call_result_home->immediate_i32.reset();

  erase_prepared_move_bundle(*function_locations, prepare::PreparedMovePhase::AfterCall, 0, 0);

  try {
    static_cast<void>(c4c::backend::x86::api::emit_prepared_module(prepared));
  } catch (const std::invalid_argument& ex) {
    if (std::string(ex.what()).find("authoritative prepared call-bundle handoff") !=
        std::string::npos) {
      return 0;
    }
    return fail((std::string("bounded multi-defined call contract drift route: x86 prepared-module consumer rejected the mutated prepared handoff with the wrong exception: ") +
                 ex.what())
                    .c_str());
  } catch (const std::exception& ex) {
    return fail((std::string("bounded multi-defined call contract drift route: x86 prepared-module consumer rejected the mutated prepared handoff with the wrong exception type: ") +
                 ex.what())
                    .c_str());
  }

  return fail("bounded multi-defined call contract drift route: x86 prepared-module consumer reopened a local call-result ABI fallback when the authoritative prepared AfterCall bundle was removed");
}

int check_route_requires_authoritative_prepared_before_call_bundle() {
  auto prepared =
      prepare::prepare_semantic_bir_module_with_options(make_x86_multi_defined_direct_call_lane_module(),
                                                        x86_target_profile());
  auto* function_locations =
      find_mutable_prepared_value_location_function(prepared, "main");
  if (function_locations == nullptr) {
    return fail("bounded multi-defined call contract drift route: missing prepared value-location function");
  }

  erase_prepared_move_bundle(*function_locations, prepare::PreparedMovePhase::BeforeCall, 0, 3);

  try {
    static_cast<void>(c4c::backend::x86::api::emit_prepared_module(prepared));
  } catch (const std::invalid_argument& ex) {
    if (std::string(ex.what()).find("authoritative prepared call-bundle handoff") !=
        std::string::npos) {
      return 0;
    }
    return fail((std::string("bounded multi-defined call contract drift route: x86 prepared-module consumer rejected the mutated prepared handoff with the wrong exception: ") +
                 ex.what())
                    .c_str());
  } catch (const std::exception& ex) {
    return fail((std::string("bounded multi-defined call contract drift route: x86 prepared-module consumer rejected the mutated prepared handoff with the wrong exception type: ") +
                 ex.what())
                    .c_str());
  }

  return fail("bounded multi-defined call contract drift route: x86 prepared-module consumer reopened a local call-argument ABI fallback when the authoritative prepared BeforeCall bundle was removed");
}

int check_route_publishes_helper_same_module_byval_before_call_register_binding() {
  auto prepared = prepare::prepare_semantic_bir_module_with_options(
      make_x86_multi_defined_helper_same_module_byval_i8x2_call_lane_module(),
      x86_target_profile());
  auto* function_locations = find_mutable_prepared_value_location_function(prepared, "arg");
  if (function_locations == nullptr) {
    return fail("bounded multi-defined helper same-module byval call route: missing prepared value-location function");
  }
  const auto selected_register =
      c4c::backend::x86::select_prepared_call_argument_abi_register_if_supported(
          function_locations, 0, 0, 0);
  if (!selected_register.has_value() || *selected_register != "rdi") {
    return fail("bounded multi-defined helper same-module byval call route: prepared BeforeCall bundle did not publish the canonical x86 pointer-argument ABI register");
  }
  return 0;
}

int check_route_publishes_helper_same_module_variadic_stack_arg_before_call_bundle() {
  auto prepared = prepare::prepare_semantic_bir_module_with_options(
      make_x86_multi_defined_helper_same_module_variadic_stack_ptr_call_lane_module(),
      x86_target_profile());
  auto* function_locations = find_mutable_prepared_value_location_function(prepared, "arg");
  if (function_locations == nullptr) {
    return fail("bounded multi-defined helper same-module variadic stack-arg call route: missing prepared value-location function");
  }

  const auto arg0 =
      c4c::backend::x86::select_prepared_call_argument_abi_register_if_supported(
          function_locations, 0, 0, 0);
  const auto arg1 =
      c4c::backend::x86::select_prepared_call_argument_abi_register_if_supported(
          function_locations, 0, 0, 1);
  const auto arg2 =
      c4c::backend::x86::select_prepared_call_argument_abi_register_if_supported(
          function_locations, 0, 0, 2);
  const auto arg3 =
      c4c::backend::x86::select_prepared_call_argument_abi_register_if_supported(
          function_locations, 0, 0, 3);
  const auto arg4 =
      c4c::backend::x86::select_prepared_call_argument_abi_register_if_supported(
          function_locations, 0, 0, 4);
  const auto arg5 =
      c4c::backend::x86::select_prepared_call_argument_abi_register_if_supported(
          function_locations, 0, 0, 5);
  const auto arg6 =
      c4c::backend::x86::select_prepared_call_argument_abi_stack_offset_if_supported(
          function_locations, 0, 0, 6);
  const auto arg7 =
      c4c::backend::x86::select_prepared_call_argument_abi_stack_offset_if_supported(
          function_locations, 0, 0, 7);
  const auto arg8 =
      c4c::backend::x86::select_prepared_call_argument_abi_stack_offset_if_supported(
          function_locations, 0, 0, 8);
  if (!arg0.has_value() || *arg0 != "rdi" || !arg1.has_value() || *arg1 != "rsi" ||
      !arg2.has_value() || *arg2 != "rdx" || !arg3.has_value() || *arg3 != "rcx" ||
      !arg4.has_value() || *arg4 != "r8" || !arg5.has_value() || *arg5 != "r9" ||
      arg6 != std::optional<std::size_t>{0} || arg7 != std::optional<std::size_t>{8} ||
      arg8 != std::optional<std::size_t>{16}) {
    return fail("bounded multi-defined helper same-module variadic stack-arg call route: prepared BeforeCall bundle did not publish the canonical x86 register and stack call-argument ABI handoff");
  }
  return 0;
}

int check_route_publishes_helper_same_module_local_byval_symbol_access() {
  auto prepared = prepare::prepare_semantic_bir_module_with_options(
      make_x86_multi_defined_helper_same_module_local_byval_i8_call_lane_module(),
      x86_target_profile());
  const auto function_name_id = prepared.names.function_names.find("arg");
  if (function_name_id == c4c::kInvalidFunctionName) {
    return fail("bounded multi-defined helper same-module local byval call route: missing prepared function name for arg");
  }
  const auto* function_addressing =
      prepare::find_prepared_addressing_function(prepared.addressing, function_name_id);
  if (function_addressing == nullptr) {
    return fail("bounded multi-defined helper same-module local byval call route: missing prepared addressing function");
  }
  const auto* access = prepare::find_prepared_memory_access_by_result_name(
      prepared.names, *function_addressing, "%t2.global.aggregate.load.0");
  if (access == nullptr) {
    return fail("bounded multi-defined helper same-module local byval call route: producer did not publish the helper-side global aggregate load access");
  }
  if (access->address.base_kind != prepare::PreparedAddressBaseKind::GlobalSymbol) {
    return fail("bounded multi-defined helper same-module local byval call route: helper-side global aggregate load did not publish a global-symbol prepared address");
  }
  if (!access->address.symbol_name.has_value() ||
      prepare::prepared_link_name(prepared.names, *access->address.symbol_name) != "s1") {
    return fail("bounded multi-defined helper same-module local byval call route: helper-side global aggregate load published the wrong symbol name");
  }
  return 0;
}

int check_route_publishes_lowered_local_byval_home_pointer_owner() {
  auto prepared = lower_and_prepare_local_byval_home_pointer_call_module();
  if (!prepared.has_value()) {
    return fail("bounded multi-defined lowered local byval-home pointer-call route: lowering failed before prepared handoff inspection");
  }

  const bir::Function* module_function = nullptr;
  for (const auto& function : prepared->module.functions) {
    if (function.name == "lowered_local_byval_home_pointer_call") {
      module_function = &function;
      break;
    }
  }
  if (module_function == nullptr || module_function->blocks.size() != 1) {
    return fail("bounded multi-defined lowered local byval-home pointer-call route: missing lowered function body");
  }

  const bir::CallInst* call = nullptr;
  for (const auto& inst : module_function->blocks.front().insts) {
    const auto* candidate = std::get_if<bir::CallInst>(&inst);
    if (candidate != nullptr) {
      call = candidate;
      break;
    }
  }
  if (call == nullptr || call->callee != "sink" || call->args.size() != 1) {
    return fail("bounded multi-defined lowered local byval-home pointer-call route: missing sink call after lowering");
  }
  const auto& pointer_owner = call->args.front();
  if (pointer_owner.kind != bir::Value::Kind::Named || pointer_owner.type != bir::TypeKind::Ptr ||
      pointer_owner.name == "%t2") {
    const std::string observed_owner =
        pointer_owner.kind == bir::Value::Kind::Named ? pointer_owner.name : "<non-named>";
    return fail((std::string("bounded multi-defined lowered local byval-home pointer-call route: lowering did not publish the authoritative byval-home leaf as the pointer call owner; saw ") +
                 observed_owner)
                    .c_str());
  }

  auto* function_locations =
      find_mutable_prepared_value_location_function(*prepared, "lowered_local_byval_home_pointer_call");
  if (function_locations == nullptr) {
    return fail("bounded multi-defined lowered local byval-home pointer-call route: missing prepared value-location function");
  }
  if (find_mutable_prepared_value_home(*prepared, *function_locations, "%t2") != nullptr) {
    return fail("bounded multi-defined lowered local byval-home pointer-call route: prepared handoff still published a synthetic %t2 owner");
  }

  auto* owner_home =
      find_mutable_prepared_value_home(*prepared, *function_locations, pointer_owner.name);
  if (owner_home == nullptr) {
    return fail("bounded multi-defined lowered local byval-home pointer-call route: prepared handoff dropped the authoritative byval-home owner");
  }

  if (!has_prepared_call_argument_register_binding(*function_locations, 0, "rdi")) {
    return fail("bounded multi-defined lowered local byval-home pointer-call route: prepared BeforeCall bundle did not publish the canonical x86 pointer-argument ABI register");
  }
  return 0;
}

int check_route_publishes_lowered_byval_param_pointer_owner() {
  auto prepared = lower_and_prepare_byval_param_pointer_call_module();
  if (!prepared.has_value()) {
    return fail("bounded multi-defined lowered byval-param pointer-call route: lowering failed before prepared handoff inspection");
  }

  const bir::Function* module_function = nullptr;
  for (const auto& function : prepared->module.functions) {
    if (function.name == "lowered_byval_param_pointer_call") {
      module_function = &function;
      break;
    }
  }
  if (module_function == nullptr || module_function->blocks.size() != 1) {
    return fail("bounded multi-defined lowered byval-param pointer-call route: missing lowered function body");
  }

  const bir::CallInst* call = nullptr;
  for (const auto& inst : module_function->blocks.front().insts) {
    const auto* candidate = std::get_if<bir::CallInst>(&inst);
    if (candidate != nullptr) {
      call = candidate;
      break;
    }
  }
  if (call == nullptr || call->callee != "sink" || call->args.size() != 1) {
    return fail("bounded multi-defined lowered byval-param pointer-call route: missing sink call after lowering");
  }
  const auto& pointer_owner = call->args.front();
  if (pointer_owner.kind != bir::Value::Kind::Named || pointer_owner.type != bir::TypeKind::Ptr ||
      pointer_owner.name != "%p.a") {
    return fail("bounded multi-defined lowered byval-param pointer-call route: lowering did not publish the byval-param home as the zero-offset pointer call owner");
  }

  auto* function_locations =
      find_mutable_prepared_value_location_function(*prepared, "lowered_byval_param_pointer_call");
  if (function_locations == nullptr) {
    return fail("bounded multi-defined lowered byval-param pointer-call route: missing prepared value-location function");
  }
  if (find_mutable_prepared_value_home(*prepared, *function_locations, "%t2") != nullptr) {
    return fail("bounded multi-defined lowered byval-param pointer-call route: prepared handoff still published a synthetic %t2 owner");
  }

  auto* owner_home = find_mutable_prepared_value_home(*prepared, *function_locations, "%p.a");
  if (owner_home == nullptr) {
    return fail("bounded multi-defined lowered byval-param pointer-call route: prepared handoff dropped the byval-param home owner");
  }

  if (!has_prepared_call_argument_register_binding(*function_locations, 0, "rdi")) {
    return fail("bounded multi-defined lowered byval-param pointer-call route: prepared BeforeCall bundle did not publish the canonical x86 pointer-argument ABI register");
  }
  return 0;
}

int check_route_publishes_lowered_indirect_aggregate_param_pointer_owner() {
  auto prepared = lower_and_prepare_indirect_aggregate_param_pointer_call_module();
  if (!prepared.has_value()) {
    return fail("bounded multi-defined lowered indirect aggregate-param pointer-call route: lowering failed before prepared handoff inspection");
  }

  const bir::Function* module_function = nullptr;
  for (const auto& function : prepared->module.functions) {
    if (function.name == "lowered_indirect_aggregate_param_pointer_call") {
      module_function = &function;
      break;
    }
  }
  if (module_function == nullptr || module_function->blocks.size() != 1) {
    return fail("bounded multi-defined lowered indirect aggregate-param pointer-call route: missing lowered function body");
  }

  const bir::CallInst* call = nullptr;
  for (const auto& inst : module_function->blocks.front().insts) {
    const auto* candidate = std::get_if<bir::CallInst>(&inst);
    if (candidate != nullptr) {
      call = candidate;
      break;
    }
  }
  if (call == nullptr || call->callee != "sink" || call->args.size() != 1) {
    return fail("bounded multi-defined lowered indirect aggregate-param pointer-call route: missing sink call after lowering");
  }
  const auto& pointer_owner = call->args.front();
  if (pointer_owner.kind != bir::Value::Kind::Named || pointer_owner.type != bir::TypeKind::Ptr ||
      pointer_owner.name != "%p.a") {
    return fail("bounded multi-defined lowered indirect aggregate-param pointer-call route: lowering did not publish the incoming aggregate-param owner for the zero-offset pointer call");
  }

  auto* function_locations =
      find_mutable_prepared_value_location_function(*prepared,
                                                    "lowered_indirect_aggregate_param_pointer_call");
  if (function_locations == nullptr) {
    return fail("bounded multi-defined lowered indirect aggregate-param pointer-call route: missing prepared value-location function");
  }
  if (find_mutable_prepared_value_home(*prepared, *function_locations, "%t2") != nullptr) {
    return fail("bounded multi-defined lowered indirect aggregate-param pointer-call route: prepared handoff still published a synthetic %t2 owner");
  }

  auto* owner_home = find_mutable_prepared_value_home(*prepared, *function_locations, "%p.a");
  if (owner_home == nullptr) {
    return fail("bounded multi-defined lowered indirect aggregate-param pointer-call route: prepared handoff dropped the incoming aggregate-param owner");
  }

  if (!has_prepared_call_argument_register_binding(*function_locations, 0, "rdi")) {
    return fail("bounded multi-defined lowered indirect aggregate-param pointer-call route: prepared BeforeCall bundle did not publish the canonical x86 pointer-argument ABI register");
  }
  return 0;
}

int check_route_renders_helper_same_module_local_byval_helper_prefix() {
  auto module = make_x86_multi_defined_helper_same_module_local_byval_i8_call_lane_module();
  c4c::TargetProfile target_profile;
  const auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto prepared_arch = c4c::TargetArch::X86_64;

  std::vector<const bir::Function*> defined_functions;
  const bir::Function* entry_function = nullptr;
  for (const auto& function : prepared.module.functions) {
    if (function.is_declaration) {
      continue;
    }
    defined_functions.push_back(&function);
    if (function.name == "main") {
      entry_function = &function;
    }
  }
  if (entry_function == nullptr) {
    return fail("bounded multi-defined helper same-module local byval call route: missing main entry function");
  }

  const auto narrow_register = [&](const std::optional<std::string>& wide_register)
      -> std::optional<std::string> {
    if (!wide_register.has_value()) {
      return std::nullopt;
    }
    return c4c::backend::x86::abi::narrow_i32_register_name(*wide_register);
  };
  const auto minimal_param_register_at =
      [&](const bir::Param& param, std::size_t arg_index) -> std::optional<std::string> {
    if (param.is_varargs || param.is_sret || param.is_byval || !param.abi.has_value()) {
      return std::nullopt;
    }
    return narrow_register(
        prepare::call_arg_destination_register_name(target_profile, *param.abi, arg_index));
  };
  const auto render_asm_symbol_name = [&](std::string_view logical_name) -> std::string {
    return std::string(logical_name);
  };
  const auto render_private_data_label = [&](std::string_view pool_name) -> std::string {
    std::string label(pool_name);
    if (!label.empty() && label.front() == '@') {
      label.erase(label.begin());
    }
    while (!label.empty() && label.front() == '.') {
      label.erase(label.begin());
    }
    return ".L." + label;
  };
  const auto find_same_module_global =
      [&](std::string_view name) -> const bir::Global* {
    for (const auto& global : prepared.module.globals) {
      if (global.name == name && !global.is_extern && !global.is_thread_local) {
        return &global;
      }
    }
    return nullptr;
  };
  const auto same_module_global_scalar_size =
      [&](bir::TypeKind type) -> std::optional<std::size_t> {
    switch (type) {
      case bir::TypeKind::I8:
        return 1;
      case bir::TypeKind::I32:
        return 4;
      case bir::TypeKind::Ptr:
        return 8;
      default:
        return std::nullopt;
    }
  };
  const auto same_module_global_supports_scalar_load =
      [&](const bir::Global& global, bir::TypeKind type, std::size_t byte_offset) -> bool {
    if (!same_module_global_scalar_size(type).has_value()) {
      return false;
    }
    if (global.initializer_symbol_name.has_value()) {
      return type == bir::TypeKind::Ptr && byte_offset == 0;
    }
    if (global.initializer.has_value()) {
      const auto init_size = same_module_global_scalar_size(global.initializer->type);
      return global.initializer->kind == bir::Value::Kind::Immediate &&
             init_size.has_value() && global.initializer->type == type && byte_offset == 0;
    }
    std::size_t current_offset = 0;
    for (const auto& element : global.initializer_elements) {
      const auto element_size = same_module_global_scalar_size(element.type);
      if (!element_size.has_value()) {
        return false;
      }
      if (current_offset == byte_offset && element.type == type) {
        return true;
      }
      current_offset += *element_size;
    }
    return false;
  };
  const auto minimal_function_return_register =
      [&](const bir::Function& candidate) -> std::optional<std::string> {
    if (!candidate.return_abi.has_value()) {
      return std::nullopt;
    }
    return narrow_register(
        prepare::call_result_destination_register_name(target_profile, *candidate.return_abi));
  };
  const auto minimal_function_asm_prefix =
      [&](const bir::Function& candidate) -> std::string {
    return ".intel_syntax noprefix\n.text\n.globl " + candidate.name + "\n.type " + candidate.name +
           ", @function\n" + candidate.name + ":\n";
  };
  const auto render_trivial_defined_function_if_supported =
      [&](const bir::Function& candidate) -> std::optional<std::string> {
    return c4c::backend::x86::render_prepared_trivial_defined_function_if_supported(
        candidate, prepared_arch, minimal_function_return_register, minimal_function_asm_prefix);
  };

  const auto rendered_helpers =
      c4c::backend::x86::render_prepared_bounded_same_module_helper_prefix_if_supported(
          prepared,
          defined_functions,
          *entry_function,
          prepared_arch,
          render_trivial_defined_function_if_supported,
          minimal_function_return_register,
          minimal_function_asm_prefix,
          find_same_module_global,
          same_module_global_supports_scalar_load,
          minimal_param_register_at,
          [&](std::string_view symbol_name) {
            return render_private_data_label("@" + std::string(symbol_name));
          },
          render_asm_symbol_name);
  if (!rendered_helpers.has_value()) {
    const bir::Function* show_function = nullptr;
    const bir::Function* arg_function = nullptr;
    for (const auto* function : defined_functions) {
      if (function->name == "show") {
        show_function = function;
      } else if (function->name == "arg") {
        arg_function = function;
      }
    }
    if (show_function == nullptr || arg_function == nullptr) {
      return fail("bounded multi-defined helper same-module local byval call route: missing helper function while isolating helper-prefix rejection");
    }

    const auto render_helper_subset =
        [&](const bir::Function& helper) -> bool {
          const std::vector<const bir::Function*> helper_subset = {&helper, entry_function};
          return c4c::backend::x86::render_prepared_bounded_same_module_helper_prefix_if_supported(
                     prepared,
                     helper_subset,
                     *entry_function,
                     prepared_arch,
                     render_trivial_defined_function_if_supported,
                     minimal_function_return_register,
                     minimal_function_asm_prefix,
                     find_same_module_global,
                     same_module_global_supports_scalar_load,
                     minimal_param_register_at,
                     [&](std::string_view symbol_name) {
                       return render_private_data_label("@" + std::string(symbol_name));
                     },
                     render_asm_symbol_name)
              .has_value();
        };
    if (!render_helper_subset(*show_function)) {
      return fail("bounded multi-defined helper same-module local byval call route: helper-prefix renderer still rejects helper function show after producer-side symbol access publication");
    }
    if (!render_helper_subset(*arg_function)) {
      auto arg_only_module = module;
      auto arg_it = std::find_if(arg_only_module.functions.begin(),
                                 arg_only_module.functions.end(),
                                 [](const bir::Function& function) { return function.name == "arg"; });
      auto main_it = std::find_if(arg_only_module.functions.begin(),
                                  arg_only_module.functions.end(),
                                  [](const bir::Function& function) { return function.name == "main"; });
      if (arg_it == arg_only_module.functions.end() || main_it == arg_only_module.functions.end()) {
        return fail("bounded multi-defined helper same-module local byval call route: missing arg/main while isolating helper instruction rejection");
      }
      const auto render_arg_prefix_len =
          [&](std::size_t inst_count) -> bool {
            auto trimmed = arg_only_module;
            auto trimmed_arg_it =
                std::find_if(trimmed.functions.begin(),
                             trimmed.functions.end(),
                             [](const bir::Function& function) { return function.name == "arg"; });
            auto trimmed_main_it =
                std::find_if(trimmed.functions.begin(),
                             trimmed.functions.end(),
                             [](const bir::Function& function) { return function.name == "main"; });
            if (trimmed_arg_it == trimmed.functions.end() || trimmed_main_it == trimmed.functions.end()) {
              return false;
            }
            if (trimmed_arg_it->blocks.empty() || trimmed_arg_it->blocks.front().insts.size() < inst_count) {
              return false;
            }
            trimmed_arg_it->blocks.front().insts.resize(inst_count);
            const auto trimmed_prepared = prepare::prepare_semantic_bir_module_with_options(
                trimmed, target_profile_from_module_triple(trimmed.target_triple, target_profile));
            const bir::Function* trimmed_main = nullptr;
            const bir::Function* trimmed_arg = nullptr;
            for (const auto& function : trimmed_prepared.module.functions) {
              if (function.name == "main") {
                trimmed_main = &function;
              } else if (function.name == "arg") {
                trimmed_arg = &function;
              }
            }
            if (trimmed_main == nullptr || trimmed_arg == nullptr) {
              return false;
            }
            const std::vector<const bir::Function*> helper_subset = {trimmed_arg, trimmed_main};
            return c4c::backend::x86::render_prepared_bounded_same_module_helper_prefix_if_supported(
                       trimmed_prepared,
                       helper_subset,
                       *trimmed_main,
                       prepared_arch,
                       render_trivial_defined_function_if_supported,
                       minimal_function_return_register,
                       minimal_function_asm_prefix,
                       find_same_module_global,
                       same_module_global_supports_scalar_load,
                       minimal_param_register_at,
                       [&](std::string_view symbol_name) {
                         return render_private_data_label("@" + std::string(symbol_name));
                       },
                       render_asm_symbol_name)
                .has_value();
          };
      if (!render_arg_prefix_len(1)) {
        return fail("bounded multi-defined helper same-module local byval call route: helper-prefix renderer still rejects arg at instruction 0 (printf) after producer-side symbol access publication");
      }
      if (!render_arg_prefix_len(2)) {
        return fail("bounded multi-defined helper same-module local byval call route: helper-prefix renderer still rejects arg at instruction 1 (helper-side global aggregate load) after producer-side symbol access publication");
      }
      if (!render_arg_prefix_len(3)) {
        return fail("bounded multi-defined helper same-module local byval call route: helper-prefix renderer still rejects arg at instruction 2 (store into %t2.0) after producer-side symbol access publication");
      }
      return fail("bounded multi-defined helper same-module local byval call route: helper-prefix renderer still rejects arg at instruction 3 (same-module byval call) after producer-side symbol access publication");
    }
    return fail("bounded multi-defined helper same-module local byval call route: helper-prefix renderer still rejects the combined helper lane even though the per-helper subsets render");
  }
  if (rendered_helpers->helper_prefix.find("arg:\n") == std::string::npos ||
      rendered_helpers->helper_prefix.find("    mov BYTE PTR [rsp], al\n") ==
          std::string::npos ||
      rendered_helpers->helper_prefix.find("    lea rdi, [rsp]\n") == std::string::npos ||
      rendered_helpers->helper_prefix.find("    call show\n") == std::string::npos) {
    return fail("bounded multi-defined helper same-module local byval call route: helper-prefix renderer did not emit the canonical helper local-byval call fragments");
  }
  return 0;
}

int check_route_renders_helper_same_module_local_byval_f32_helper_prefix() {
  auto module = make_x86_multi_defined_helper_same_module_local_byval_f32_call_lane_module();
  c4c::TargetProfile target_profile;
  const auto prepared =
      prepare::prepare_semantic_bir_module_with_options(
          module, target_profile_from_module_triple(module.target_triple, target_profile));
  const auto prepared_arch = c4c::TargetArch::X86_64;

  std::vector<const bir::Function*> defined_functions;
  const bir::Function* entry_function = nullptr;
  for (const auto& function : prepared.module.functions) {
    if (function.is_declaration) {
      continue;
    }
    defined_functions.push_back(&function);
    if (function.name == "main") {
      entry_function = &function;
    }
  }
  if (entry_function == nullptr) {
    return fail("bounded multi-defined helper same-module local byval f32 call route: missing main entry function");
  }

  const auto narrow_register = [&](const std::optional<std::string>& wide_register)
      -> std::optional<std::string> {
    if (!wide_register.has_value()) {
      return std::nullopt;
    }
    return c4c::backend::x86::abi::narrow_i32_register_name(*wide_register);
  };
  const auto minimal_param_register_at =
      [&](const bir::Param& param, std::size_t arg_index) -> std::optional<std::string> {
    if (param.is_varargs || param.is_sret || param.is_byval || !param.abi.has_value()) {
      return std::nullopt;
    }
    return narrow_register(
        prepare::call_arg_destination_register_name(target_profile, *param.abi, arg_index));
  };
  const auto render_asm_symbol_name = [&](std::string_view logical_name) -> std::string {
    return std::string(logical_name);
  };
  const auto render_private_data_label = [&](std::string_view pool_name) -> std::string {
    std::string label(pool_name);
    if (!label.empty() && label.front() == '@') {
      label.erase(label.begin());
    }
    while (!label.empty() && label.front() == '.') {
      label.erase(label.begin());
    }
    return ".L." + label;
  };
  const auto find_same_module_global =
      [&](std::string_view name) -> const bir::Global* {
    for (const auto& global : prepared.module.globals) {
      if (global.name == name && !global.is_extern && !global.is_thread_local) {
        return &global;
      }
    }
    return nullptr;
  };
  const auto same_module_global_scalar_size =
      [&](bir::TypeKind type) -> std::optional<std::size_t> {
    switch (type) {
      case bir::TypeKind::I8:
        return 1;
      case bir::TypeKind::I32:
        return 4;
      case bir::TypeKind::Ptr:
        return 8;
      case bir::TypeKind::F32:
        return 4;
      case bir::TypeKind::F64:
        return 8;
      default:
        return std::nullopt;
    }
  };
  const auto same_module_global_supports_scalar_load =
      [&](const bir::Global& global, bir::TypeKind type, std::size_t byte_offset) -> bool {
    if (!same_module_global_scalar_size(type).has_value()) {
      return false;
    }
    if (global.initializer_symbol_name.has_value()) {
      return type == bir::TypeKind::Ptr && byte_offset == 0;
    }
    if (global.initializer.has_value()) {
      const auto init_size = same_module_global_scalar_size(global.initializer->type);
      return global.initializer->kind == bir::Value::Kind::Immediate &&
             init_size.has_value() && global.initializer->type == type && byte_offset == 0;
    }
    std::size_t current_offset = 0;
    for (const auto& element : global.initializer_elements) {
      const auto element_size = same_module_global_scalar_size(element.type);
      if (!element_size.has_value()) {
        return false;
      }
      if (current_offset == byte_offset && element.type == type) {
        return true;
      }
      current_offset += *element_size;
    }
    return false;
  };
  const auto minimal_function_return_register =
      [&](const bir::Function& candidate) -> std::optional<std::string> {
    if (!candidate.return_abi.has_value()) {
      return std::nullopt;
    }
    return narrow_register(
        prepare::call_result_destination_register_name(target_profile, *candidate.return_abi));
  };
  const auto minimal_function_asm_prefix =
      [&](const bir::Function& candidate) -> std::string {
    return ".intel_syntax noprefix\n.text\n.globl " + candidate.name + "\n.type " + candidate.name +
           ", @function\n" + candidate.name + ":\n";
  };
  const auto render_trivial_defined_function_if_supported =
      [&](const bir::Function& candidate) -> std::optional<std::string> {
    return c4c::backend::x86::render_prepared_trivial_defined_function_if_supported(
        candidate, prepared_arch, minimal_function_return_register, minimal_function_asm_prefix);
  };

  const auto rendered_helpers =
      c4c::backend::x86::render_prepared_bounded_same_module_helper_prefix_if_supported(
          prepared,
          defined_functions,
          *entry_function,
          prepared_arch,
          render_trivial_defined_function_if_supported,
          minimal_function_return_register,
          minimal_function_asm_prefix,
          find_same_module_global,
          same_module_global_supports_scalar_load,
          minimal_param_register_at,
          [&](std::string_view symbol_name) {
            return render_private_data_label("@" + std::string(symbol_name));
          },
          render_asm_symbol_name);
  if (!rendered_helpers.has_value()) {
    const bir::Function* show_function = nullptr;
    const bir::Function* arg_function = nullptr;
    for (const auto* function : defined_functions) {
      if (function->name == "show") {
        show_function = function;
      } else if (function->name == "arg") {
        arg_function = function;
      }
    }
    if (show_function == nullptr || arg_function == nullptr) {
      return fail("bounded multi-defined helper same-module local byval f32 call route: missing helper function while isolating helper-prefix rejection");
    }

    const auto render_helper_subset =
        [&](const bir::Function& helper) -> bool {
          const std::vector<const bir::Function*> helper_subset = {&helper, entry_function};
          return c4c::backend::x86::render_prepared_bounded_same_module_helper_prefix_if_supported(
                     prepared,
                     helper_subset,
                     *entry_function,
                     prepared_arch,
                     render_trivial_defined_function_if_supported,
                     minimal_function_return_register,
                     minimal_function_asm_prefix,
                     find_same_module_global,
                     same_module_global_supports_scalar_load,
                     minimal_param_register_at,
                     [&](std::string_view symbol_name) {
                       return render_private_data_label("@" + std::string(symbol_name));
                     },
                     render_asm_symbol_name)
              .has_value();
        };
    if (!render_helper_subset(*show_function)) {
      return fail("bounded multi-defined helper same-module local byval f32 call route: helper-prefix renderer still rejects helper function show after addressed float-load support");
    }
    if (!render_helper_subset(*arg_function)) {
      auto arg_only_module = module;
      auto arg_it = std::find_if(arg_only_module.functions.begin(),
                                 arg_only_module.functions.end(),
                                 [](const bir::Function& function) { return function.name == "arg"; });
      auto main_it = std::find_if(arg_only_module.functions.begin(),
                                  arg_only_module.functions.end(),
                                  [](const bir::Function& function) { return function.name == "main"; });
      if (arg_it == arg_only_module.functions.end() || main_it == arg_only_module.functions.end()) {
        return fail("bounded multi-defined helper same-module local byval f32 call route: missing arg/main while isolating helper instruction rejection");
      }
      const auto render_arg_prefix_len =
          [&](std::size_t inst_count) -> bool {
            auto trimmed = arg_only_module;
            auto trimmed_arg_it =
                std::find_if(trimmed.functions.begin(),
                             trimmed.functions.end(),
                             [](const bir::Function& function) { return function.name == "arg"; });
            auto trimmed_main_it =
                std::find_if(trimmed.functions.begin(),
                             trimmed.functions.end(),
                             [](const bir::Function& function) { return function.name == "main"; });
            if (trimmed_arg_it == trimmed.functions.end() || trimmed_main_it == trimmed.functions.end()) {
              return false;
            }
            if (trimmed_arg_it->blocks.empty() || trimmed_arg_it->blocks.front().insts.size() < inst_count) {
              return false;
            }
            trimmed_arg_it->blocks.front().insts.resize(inst_count);
            const auto trimmed_prepared = prepare::prepare_semantic_bir_module_with_options(
                trimmed, target_profile_from_module_triple(trimmed.target_triple, target_profile));
            const bir::Function* trimmed_main = nullptr;
            const bir::Function* trimmed_arg = nullptr;
            for (const auto& function : trimmed_prepared.module.functions) {
              if (function.name == "main") {
                trimmed_main = &function;
              } else if (function.name == "arg") {
                trimmed_arg = &function;
              }
            }
            if (trimmed_main == nullptr || trimmed_arg == nullptr) {
              return false;
            }
            const std::vector<const bir::Function*> helper_subset = {trimmed_arg, trimmed_main};
            return c4c::backend::x86::render_prepared_bounded_same_module_helper_prefix_if_supported(
                       trimmed_prepared,
                       helper_subset,
                       *trimmed_main,
                       prepared_arch,
                       render_trivial_defined_function_if_supported,
                       minimal_function_return_register,
                       minimal_function_asm_prefix,
                       find_same_module_global,
                       same_module_global_supports_scalar_load,
                       minimal_param_register_at,
                       [&](std::string_view symbol_name) {
                         return render_private_data_label("@" + std::string(symbol_name));
                       },
                       render_asm_symbol_name)
                .has_value();
          };
      if (!render_arg_prefix_len(1)) {
        return fail("bounded multi-defined helper same-module local byval f32 call route: helper-prefix renderer still rejects arg at instruction 0 (printf)");
      }
      if (!render_arg_prefix_len(2)) {
        return fail("bounded multi-defined helper same-module local byval f32 call route: helper-prefix renderer still rejects arg at instruction 1 (global float aggregate load)");
      }
      if (!render_arg_prefix_len(3)) {
        return fail("bounded multi-defined helper same-module local byval f32 call route: helper-prefix renderer still rejects arg at instruction 2 (store into %t2.0)");
      }
      return fail("bounded multi-defined helper same-module local byval f32 call route: helper-prefix renderer still rejects arg at instruction 3 (same-module byval call)");
    }
    return fail("bounded multi-defined helper same-module local byval f32 call route: helper-prefix renderer still rejects the combined helper lane even though the per-helper subsets render");
  }
  if (rendered_helpers->helper_prefix.find("show:\n") == std::string::npos ||
      rendered_helpers->helper_prefix.find("    cvtss2sd") == std::string::npos ||
      rendered_helpers->helper_prefix.find("    mov eax, 1\n") == std::string::npos ||
      rendered_helpers->helper_prefix.find("arg:\n") == std::string::npos ||
      rendered_helpers->helper_prefix.find("    lea rdi, [rsp]\n") == std::string::npos ||
      rendered_helpers->helper_prefix.find("    call show\n") == std::string::npos) {
    return fail("bounded multi-defined helper same-module local byval f32 call route: helper-prefix renderer did not emit the canonical f32 helper-call fragments");
  }
  return 0;
}

}  // namespace

int run_backend_x86_handoff_boundary_multi_defined_call_tests() {
  // Keep the bounded multi-defined same-module symbol-call lane isolated so
  // later Step 5 packets can move the remaining rejection-only boundary
  // coverage without re-threading this supported-path route.
  if (const auto status =
          check_route_outputs(make_x86_multi_defined_direct_call_lane_module(),
                              expected_minimal_multi_defined_direct_call_lane_asm(),
                              "bir.func @main() -> i32 {",
                              "bounded multi-defined-function same-module symbol-call prepared-module route");
      status != 0) {
    return status;
  }
  if (const auto status = check_route_consumes_prepared_call_move_bundle_contract(); status != 0) {
    return status;
  }
  if (const auto status = check_route_requires_authoritative_prepared_after_call_bundle();
      status != 0) {
    return status;
  }
  if (const auto status = check_route_requires_authoritative_prepared_before_call_bundle();
      status != 0) {
    return status;
  }
  if (const auto status =
          check_route_publishes_helper_same_module_byval_before_call_register_binding();
      status != 0) {
    return status;
  }
  if (const auto status =
          check_route_publishes_helper_same_module_variadic_stack_arg_before_call_bundle();
      status != 0) {
    return status;
  }
  if (const auto status = check_route_publishes_helper_same_module_local_byval_symbol_access();
      status != 0) {
    return status;
  }
  if (const auto status = check_route_publishes_lowered_local_byval_home_pointer_owner();
      status != 0) {
    return status;
  }
  if (const auto status = check_route_publishes_lowered_byval_param_pointer_owner();
      status != 0) {
    return status;
  }
  if (const auto status = check_route_publishes_lowered_indirect_aggregate_param_pointer_owner();
      status != 0) {
    return status;
  }
  if (const auto status = check_route_renders_helper_same_module_local_byval_helper_prefix();
      status != 0) {
    return status;
  }
  if (const auto status = check_route_renders_helper_same_module_local_byval_f32_helper_prefix();
      status != 0) {
    return status;
  }
  if (const auto status =
          check_route_contains_fragments(
              make_x86_multi_defined_byval_helper_call_lane_module(),
              {"show:\n",
               "    lea rdi, [rip + .L.str0]\n",
               "    call printf\n",
               "main:\n",
               "    call show\n"},
              "bir.func @show(ptr byval(size=8, align=4) %p.p) -> void {",
              "bounded multi-defined-function byval helper direct-extern prepared-module route");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_route_contains_fragments(
              make_x86_multi_defined_byval_i8_helper_copy_lane_module(),
              {"show:\n",
               "    mov BYTE PTR [rsp + 1], al\n",
               "main:\n",
               "    lea rdi, [rsp + 8]\n",
               "    call show\n"},
              "bir.func @show(ptr byval(size=1, align=1) %p.a) -> void {",
              "bounded multi-defined-function byval i8 helper copy prepared-module route");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_route_contains_fragments(
              make_x86_multi_defined_byval_i8_helper_pointer_arg_lane_module(),
              {"show:\n",
               "    lea rdi, [rip + .L.str0]\n",
               "    lea rsi, [rsp + 1]\n",
               "    call printf\n",
               "main:\n",
               "    lea rdi, [rsp + 8]\n",
               "    call show\n"},
              "bir.func @show(ptr byval(size=1, align=1) %p.a) -> void {",
              "bounded multi-defined-function byval i8 helper pointer-arg prepared-module route");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_route_contains_fragments(
              make_x86_multi_defined_byval_i8x2_helper_pointer_arg_lane_module(),
              {"show:\n",
               "    lea rdi, [rip + .L.str0]\n",
               "    lea rsi, [rsp + 2]\n",
               "    call printf\n",
               "main:\n",
               "    lea rdi, [rsp + 8]\n",
               "    call show\n"},
              "bir.func @show(ptr byval(size=2, align=1) %p.a) -> void {",
              "bounded multi-defined-function byval i8x2 helper pointer-arg prepared-module route");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_route_contains_fragments(
              make_x86_multi_defined_helper_same_module_byval_i8x2_call_lane_module(),
              {"show:\n",
               "    lea rdi, [rip + .L.str0]\n",
               "    lea rsi, [rsp + 2]\n",
               "    call printf\n",
               "arg:\n",
               "    lea rdi, [rip + g]\n",
               "    call show\n",
               "main:\n",
               "    call arg\n"},
              "bir.func @arg() -> void {",
              "bounded multi-defined-function helper same-module byval i8x2 call prepared-module route");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_route_contains_fragments(
              make_x86_multi_defined_helper_same_module_local_byval_i8_call_lane_module(),
              {"show:\n",
               "    lea rdi, [rip + .L.str0]\n",
               "    lea rsi, [rsp + 1]\n",
               "    call printf\n",
               "arg:\n",
               "    mov BYTE PTR [rsp], al\n",
               "    lea rdi, [rsp]\n",
               "    call show\n",
               "main:\n",
               "    call arg\n"},
              "bir.call void show(ptr byval(size=1, align=1) %t2)",
              "bounded multi-defined-function helper same-module local byval i8 call prepared-module route");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_route_contains_fragments(
              make_x86_multi_defined_helper_same_module_sret_i8_call_lane_module(),
              {"fr_s1:\n",
               "    mov QWORD PTR [rsp], rdi\n",
               "ret:\n",
               "    lea rdi, [rsp]\n",
               "    xor eax, eax\n",
               "    call fr_s1\n",
               "main:\n",
               "    call ret\n",
               "s1:\n"},
              "bir.call void fr_s1(ptr sret(size=1, align=1) %t0)",
              "bounded multi-defined-function helper same-module sret i8 call prepared-module route");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_route_contains_fragments(
              make_x86_multi_defined_helper_same_module_sret_i8_copyout_lane_module(),
              {"fr_s1:\n",
               "    mov QWORD PTR [rsp], rdi\n",
               "ret:\n",
               "    call fr_s1\n",
               "main:\n",
               "    call ret\n",
               "s1:\n"},
              "bir.store_local %lv.t1.0, i8 %lv.t1.aggregate.copy.0",
              "bounded multi-defined-function helper same-module sret i8 caller copyout prepared-module route");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_route_contains_fragments(
              make_x86_multi_defined_helper_same_module_sret_i8x2_copyout_lane_module(),
              {"fr_s2:\n",
               "    mov QWORD PTR [rsp], rdi\n",
               "ret:\n",
               "    call fr_s2\n",
               "    movsx eax, BYTE PTR [rsp",
               "    mov BYTE PTR [rsp",
               "main:\n",
               "    call ret\n",
               "s2:\n"},
              "bir.store_local %lv.t2.1, i8 %lv.t2.aggregate.copy.1",
              "bounded multi-defined-function helper same-module sret i8x2 caller copyout prepared-module route");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_route_contains_fragments(
              make_x86_multi_defined_helper_same_module_sret_f32_copyout_lane_module(),
              {"fr_hfa11:\n",
               "    mov QWORD PTR [rsp], rdi\n",
               "    movss ",
               "ret:\n",
               "    call fr_hfa11\n",
               "main:\n",
               "    call ret\n",
               "hfa11:\n"},
              "bir.store_local %t0.0, float fr_hfa11.ret.sret.copy.0, addr %ret.sret",
              "bounded multi-defined-function helper same-module sret f32 caller copyout prepared-module route");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_route_contains_fragments(
              make_x86_multi_defined_helper_same_module_sret_f128_copyout_lane_module(),
              {"fr_hfa31:\n",
               "    mov QWORD PTR [rsp], rdi\n",
               "    fld TBYTE PTR [rip + hfa31]\n",
               "    fstp TBYTE PTR [r",
               "ret:\n",
               "    call fr_hfa31\n",
               "    fld TBYTE PTR [rsp",
               "    fstp TBYTE PTR [rsp",
               "main:\n",
               "    call ret\n"},
              "bir.store_local %t0.0, f128 fr_hfa31.ret.sret.copy.0, addr %ret.sret",
              "bounded multi-defined-function helper same-module sret f128 caller copyout prepared-module route");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_route_contains_fragments(
              make_x86_multi_defined_helper_same_module_local_byval_f32_call_lane_module(),
              {"show:\n",
               "    movss ",
               "    cvtss2sd",
               "    mov eax, 1\n",
               "    call printf\n",
               "arg:\n",
               "    movss DWORD PTR [rsp], ",
               "    lea rdi, [rsp]\n",
               "    call show\n",
               "main:\n",
               "    call arg\n",
               "hfa11:\n",
               "    .long 1065353216\n"},
              "bir.call void show(ptr byval(size=4, align=4) %t2)",
              "bounded multi-defined-function helper same-module local byval f32 call prepared-module route");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_route_contains_fragments(
              make_x86_multi_defined_helper_same_module_local_byval_f32_trivial_entry_module(),
              {"show:\n",
               "    movss ",
               "    cvtss2sd",
               "    mov eax, 1\n",
               "    call printf\n",
               "main:\n",
               "    mov eax, 0\n",
               "    ret\n"},
              "bir.func @show(ptr byval(size=4, align=4) %p.a) -> void {",
              "bounded multi-defined-function helper same-module local byval f32 helper route with trivial entry prepared-module traversal");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_route_contains_fragments(
              make_x86_multi_defined_helper_same_module_late_stack_ptr_variadic_helper_module(),
              {"show:\n",
               "    sub rsp, 16\n",
               "    lea r10, [rsp + ",
               "    mov QWORD PTR [rsp], r10\n",
               "    mov eax, 4\n",
               "    call printf\n",
               "main:\n",
               "    mov eax, 0\n",
               "    ret\n"},
              "bir.call i32 printf(ptr @.str0, ptr %lv.param.a.0, double %wide0, double %wide1, ptr %lv.param.b.0, double %wide2, double %wide3, ptr %lv.param.c.0)",
              "bounded multi-defined-function helper same-module late stack ptr variadic helper prepared-module route");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_route_contains_fragments(
              make_x86_multi_defined_float_helper_call_lane_module(),
              {"show:\n",
               "    movss DWORD PTR [rsp +",
               "    cvtss2sd",
               "    mov eax, 2\n",
               "    call printf\n",
               "main:\n",
               "    call actual_function\n"},
              "bir.func @show(float %x.0, float %x.1) -> void {",
              "bounded multi-defined-function float helper local-materialization direct-extern prepared-module route");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_route_contains_fragments(
              make_x86_multi_defined_f128_helper_call_lane_module(),
              {"show:\n",
               "    fld TBYTE PTR [rsp +",
               "    sub rsp, 16\n",
               "    fstp TBYTE PTR [rsp]",
               "    mov eax, 0\n",
               "    call printf\n",
               "    add rsp, 16\n",
               "main:\n",
               "    call actual_function\n"},
              "bir.func @show(f128 %x.0) -> void {",
              "bounded multi-defined-function f128 helper local-materialization direct-extern prepared-module route");
      status != 0) {
    return status;
  }
  if (const auto status =
          check_route_contains_fragments(
              make_x86_multi_defined_mixed_f128_helper_call_lane_module(),
              {"show:\n",
               "    mov eax, 2\n",
               "    sub rsp, 32\n",
               "    fstp TBYTE PTR [rsp + 16]\n",
               "    call printf\n",
               "    add rsp, 32\n",
               "main:\n",
               "    call actual_function\n"},
              "bir.func @show(ptr %p.0, float %x.0, float %x.1, ptr %p.1, f128 %y.0, f128 %y.1) -> void {",
              "bounded multi-defined-function mixed helper direct-extern prepared-module route beyond six total args");
      status != 0) {
    return status;
  }

  return 0;
}
