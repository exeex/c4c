#include "lir_to_bir.hpp"
#include "call_decode.hpp"

#include <charconv>
#include <algorithm>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace c4c::backend {

namespace {

std::optional<bir::TypeKind> lower_minimal_scalar_type(const c4c::TypeSpec& type) {
  if (type.ptr_level != 0 || type.array_rank != 0) {
    return std::nullopt;
  }
  if (type.base == TB_CHAR || type.base == TB_SCHAR || type.base == TB_UCHAR) {
    return bir::TypeKind::I8;
  }
  if (type.base == TB_INT) {
    return bir::TypeKind::I32;
  }
  if (type.base == TB_LONG || type.base == TB_ULONG || type.base == TB_LONGLONG ||
      type.base == TB_ULONGLONG) {
    return bir::TypeKind::I64;
  }
  return std::nullopt;
}

std::optional<bir::TypeKind> lower_scalar_type_text(std::string_view text) {
  if (text == "i8") {
    return bir::TypeKind::I8;
  }
  if (text == "i32") {
    return bir::TypeKind::I32;
  }
  if (text == "i64") {
    return bir::TypeKind::I64;
  }
  return std::nullopt;
}

std::optional<bir::TypeKind> lower_minimal_call_arg_type_text(std::string_view text) {
  if (const auto scalar = lower_scalar_type_text(text); scalar.has_value()) {
    return scalar;
  }

  const auto trimmed = c4c::codegen::lir::trim_lir_arg_text(text);
  if (trimmed == "ptr" || (!trimmed.empty() && trimmed.back() == '*')) {
    return bir::TypeKind::I64;
  }
  return std::nullopt;
}

unsigned scalar_type_bit_width(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I8:
      return 8;
    case bir::TypeKind::I32:
      return 32;
    case bir::TypeKind::I64:
      return 64;
    case bir::TypeKind::Void:
      return 0;
  }
  return 0;
}

std::optional<std::int64_t> parse_immediate(std::string_view text) {
  std::int64_t value = 0;
  const char* begin = text.data();
  const char* end = begin + text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc() || result.ptr != end) {
    return std::nullopt;
  }
  return value;
}

bool immediate_fits_type(std::int64_t value, bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I8:
      return value >= -128 && value <= 127;
    case bir::TypeKind::I32:
      return value >= std::numeric_limits<std::int32_t>::min() &&
             value <= std::numeric_limits<std::int32_t>::max();
    case bir::TypeKind::I64:
      return true;
    case bir::TypeKind::Void:
      return false;
  }
  return false;
}

std::optional<bir::Value> lower_immediate_or_name(std::string_view value_text,
                                                  bir::TypeKind type) {
  if (value_text.empty()) {
    return std::nullopt;
  }
  if (value_text.front() == '%') {
    return bir::Value::named(type, std::string(value_text));
  }
  const auto immediate = parse_immediate(value_text);
  if (!immediate.has_value() || !immediate_fits_type(*immediate, type)) {
    return std::nullopt;
  }
  switch (type) {
    case bir::TypeKind::I8:
      return bir::Value::immediate_i8(static_cast<std::int8_t>(*immediate));
    case bir::TypeKind::I32:
      return bir::Value::immediate_i32(static_cast<std::int32_t>(*immediate));
    case bir::TypeKind::I64:
      return bir::Value::immediate_i64(*immediate);
    case bir::TypeKind::Void:
      return std::nullopt;
  }
  return std::nullopt;
}

std::string decode_llvm_byte_string(std::string_view text) {
  std::string bytes;
  bytes.reserve(text.size());
  for (std::size_t index = 0; index < text.size(); ++index) {
    if (text[index] == '\\' && index + 2 < text.size()) {
      auto hex_val = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return -1;
      };
      const int hi = hex_val(text[index + 1]);
      const int lo = hex_val(text[index + 2]);
      if (hi >= 0 && lo >= 0) {
        bytes.push_back(static_cast<char>(hi * 16 + lo));
        index += 2;
        continue;
      }
    }
    bytes.push_back(text[index]);
  }
  return bytes;
}

std::optional<bir::Module> try_lower_minimal_direct_call_module(
    const c4c::codegen::lir::LirModule& module) {
  if (!module.globals.empty() || !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }
  if (module.functions.size() != 2) {
    return std::nullopt;
  }

  const auto try_match =
      [](const c4c::codegen::lir::LirFunction& main_function,
         const c4c::codegen::lir::LirFunction& helper)
      -> std::optional<std::tuple<std::string, std::string, std::string, std::int64_t>> {
    using namespace c4c::codegen::lir;

    if (main_function.is_declaration || helper.is_declaration ||
        !backend_lir_signature_matches(
            main_function.signature_text, "define", "i32", main_function.name, {}) ||
        !backend_lir_signature_matches(helper.signature_text, "define", "i32", helper.name, {}) ||
        main_function.entry.value != 0 || helper.entry.value != 0 ||
        main_function.blocks.size() != 1 || helper.blocks.size() != 1 ||
        !main_function.alloca_insts.empty() || !helper.alloca_insts.empty() ||
        !main_function.stack_objects.empty() || !helper.stack_objects.empty()) {
      return std::nullopt;
    }

    const auto helper_return_imm =
        parse_backend_lir_zero_arg_return_imm_function(helper, std::nullopt);
    if (!helper_return_imm.has_value()) {
      return std::nullopt;
    }

    const auto& main_block = main_function.blocks.front();
    const auto* main_ret = std::get_if<LirRet>(&main_block.terminator);
    if (main_block.label != "entry" || main_block.insts.size() != 1 || main_ret == nullptr ||
        !main_ret->value_str.has_value() || main_ret->type_str != "i32") {
      return std::nullopt;
    }

    const auto* call = std::get_if<LirCallOp>(&main_block.insts.front());
    const auto callee_name =
        call == nullptr ? std::nullopt : parse_backend_zero_arg_direct_global_typed_call(*call);
    if (call == nullptr || !callee_name.has_value() || *callee_name != helper.name ||
        call->result.str().empty() || *main_ret->value_str != call->result.str()) {
      return std::nullopt;
    }

    return std::tuple<std::string, std::string, std::string, std::int64_t>{
        helper.name,
        main_function.name,
        call->result.str(),
        *helper_return_imm,
    };
  };

  auto parsed = try_match(module.functions[0], module.functions[1]);
  if (!parsed.has_value()) {
    parsed = try_match(module.functions[1], module.functions[0]);
  }
  if (!parsed.has_value()) {
    return std::nullopt;
  }

  const auto& [helper_name, main_name, call_result, return_imm] = *parsed;

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function helper;
  helper.name = helper_name;
  helper.return_type = bir::TypeKind::I32;

  bir::Block helper_entry;
  helper_entry.label = "entry";
  helper_entry.terminator.value =
      bir::Value::immediate_i32(static_cast<std::int32_t>(return_imm));
  helper.blocks.push_back(std::move(helper_entry));
  lowered.functions.push_back(std::move(helper));

  bir::Function main_function;
  main_function.name = main_name;
  main_function.return_type = bir::TypeKind::I32;

  bir::Block main_entry;
  main_entry.label = "entry";
  main_entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, call_result),
      .callee = helper_name,
      .args = {},
      .return_type_name = "i32",
  });
  main_entry.terminator.value = bir::Value::named(bir::TypeKind::I32, call_result);
  main_function.blocks.push_back(std::move(main_entry));
  lowered.functions.push_back(std::move(main_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_void_direct_call_imm_return_module(
    const c4c::codegen::lir::LirModule& module) {
  const auto parsed = parse_backend_minimal_void_direct_call_imm_return_lir_module(module);
  if (!parsed.has_value() || parsed->helper == nullptr || parsed->main_function == nullptr ||
      parsed->call == nullptr) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function helper;
  helper.name = parsed->helper->name;
  helper.return_type = bir::TypeKind::Void;

  bir::Block helper_entry;
  helper_entry.label = "entry";
  helper.blocks.push_back(std::move(helper_entry));
  lowered.functions.push_back(std::move(helper));

  bir::Function main_function;
  main_function.name = parsed->main_function->name;
  main_function.return_type = bir::TypeKind::I32;

  bir::Block main_entry;
  main_entry.label = "entry";
  main_entry.insts.push_back(bir::CallInst{
      .result = std::nullopt,
      .callee = parsed->helper->name,
      .args = {},
      .return_type_name = "void",
  });
  main_entry.terminator.value =
      bir::Value::immediate_i32(static_cast<std::int32_t>(parsed->return_imm));
  main_function.blocks.push_back(std::move(main_entry));
  lowered.functions.push_back(std::move(main_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_declared_direct_call_module(
    const c4c::codegen::lir::LirModule& module) {
  const auto parsed = parse_backend_minimal_declared_direct_call_lir_module(module);
  if (!parsed.has_value() || parsed->call == nullptr || parsed->main_function == nullptr) {
    return std::nullopt;
  }
  if (parsed->call->result.str().empty()) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  lowered.globals.reserve(module.globals.size());
  for (const auto& global : module.globals) {
    if (global.name.empty()) {
      return std::nullopt;
    }
    lowered.globals.push_back(bir::Global{
        .name = global.name,
        .type = bir::TypeKind::I64,
        .is_extern = global.is_extern_decl,
        .initializer = std::nullopt,
    });
  }

  lowered.string_constants.reserve(module.string_pool.size());
  for (const auto& string_constant : module.string_pool) {
    if (string_constant.pool_name.empty() || string_constant.byte_length < 0) {
      return std::nullopt;
    }

    std::string name = string_constant.pool_name;
    if (!name.empty() && name.front() == '@') {
      name.erase(name.begin());
    }

    lowered.string_constants.push_back(bir::StringConstant{
        .name = std::move(name),
        .bytes = decode_llvm_byte_string(string_constant.raw_bytes),
    });
  }

  bir::Function callee;
  callee.name = parsed->parsed_call.symbol_name;
  callee.return_type = bir::TypeKind::I32;
  callee.is_declaration = true;
  callee.params.reserve(parsed->parsed_call.typed_call.param_types.size());
  for (std::size_t index = 0; index < parsed->parsed_call.typed_call.param_types.size(); ++index) {
    const auto type =
        lower_minimal_call_arg_type_text(parsed->parsed_call.typed_call.param_types[index]);
    if (!type.has_value()) {
      return std::nullopt;
    }
    callee.params.push_back(bir::Param{
        .type = *type,
        .name = "%arg" + std::to_string(index),
    });
  }
  lowered.functions.push_back(std::move(callee));

  bir::Function main_function;
  main_function.name = parsed->main_function->name;
  main_function.return_type = bir::TypeKind::I32;

  bir::Block entry_block;
  entry_block.label = "entry";

  bir::CallInst call_inst;
  call_inst.result = bir::Value::named(bir::TypeKind::I32, parsed->call->result.str());
  call_inst.callee = parsed->parsed_call.symbol_name;
  call_inst.return_type_name = "i32";
  call_inst.args.reserve(parsed->args.size());
  for (const auto& arg : parsed->args) {
    switch (arg.kind) {
      case ParsedBackendExternCallArg::Kind::I32Imm:
        call_inst.args.push_back(bir::Value::immediate_i32(static_cast<std::int32_t>(arg.imm)));
        break;
      case ParsedBackendExternCallArg::Kind::I64Imm:
        call_inst.args.push_back(bir::Value::immediate_i64(arg.imm));
        break;
      case ParsedBackendExternCallArg::Kind::Ptr: {
        if (arg.operand.empty() || arg.operand.front() != '@') {
          return std::nullopt;
        }
        call_inst.args.push_back(
            bir::Value::named(bir::TypeKind::I64, arg.operand.substr(1)));
        break;
      }
    }
  }
  entry_block.insts.push_back(std::move(call_inst));

  if (parsed->return_call_result) {
    entry_block.terminator.value =
        bir::Value::named(bir::TypeKind::I32, parsed->call->result.str());
  } else {
    entry_block.terminator.value =
        bir::Value::immediate_i32(static_cast<std::int32_t>(parsed->return_imm));
  }

  main_function.blocks.push_back(std::move(entry_block));
  lowered.functions.push_back(std::move(main_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_two_arg_direct_call_module(
    const c4c::codegen::lir::LirModule& module) {
  const auto parsed = parse_backend_minimal_two_arg_direct_call_lir_module(module);
  if (!parsed.has_value() || parsed->helper == nullptr || parsed->main_function == nullptr ||
      parsed->call == nullptr || parsed->call->result.str().empty()) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function helper;
  helper.name = parsed->helper->name;
  helper.return_type = bir::TypeKind::I32;
  helper.params = {
      bir::Param{.type = bir::TypeKind::I32, .name = "%lhs"},
      bir::Param{.type = bir::TypeKind::I32, .name = "%rhs"},
  };

  bir::Block helper_entry;
  helper_entry.label = "entry";
  helper_entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "%sum"),
      .lhs = bir::Value::named(bir::TypeKind::I32, helper.params[0].name),
      .rhs = bir::Value::named(bir::TypeKind::I32, helper.params[1].name),
  });
  helper_entry.terminator.value = bir::Value::named(bir::TypeKind::I32, "%sum");
  helper.blocks.push_back(std::move(helper_entry));
  lowered.functions.push_back(std::move(helper));

  bir::Function main_function;
  main_function.name = parsed->main_function->name;
  main_function.return_type = bir::TypeKind::I32;

  bir::Block main_entry;
  main_entry.label = "entry";
  main_entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, parsed->call->result.str()),
      .callee = parsed->helper->name,
      .args = {
          bir::Value::immediate_i32(static_cast<std::int32_t>(parsed->lhs_call_arg_imm)),
          bir::Value::immediate_i32(static_cast<std::int32_t>(parsed->rhs_call_arg_imm)),
      },
      .return_type_name = "i32",
  });
  main_entry.terminator.value =
      bir::Value::named(bir::TypeKind::I32, parsed->call->result.str());
  main_function.blocks.push_back(std::move(main_entry));
  lowered.functions.push_back(std::move(main_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_direct_call_add_imm_module(
    const c4c::codegen::lir::LirModule& module) {
  const auto parsed = parse_backend_minimal_direct_call_add_imm_lir_module(module);
  if (!parsed.has_value() || parsed->helper == nullptr || parsed->main_function == nullptr ||
      parsed->call == nullptr || parsed->call->result.str().empty()) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function helper;
  helper.name = parsed->helper->name;
  helper.return_type = bir::TypeKind::I32;
  helper.params = {
      bir::Param{.type = bir::TypeKind::I32, .name = "%arg0"},
  };

  bir::Block helper_entry;
  helper_entry.label = "entry";
  helper_entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "%sum"),
      .lhs = bir::Value::named(bir::TypeKind::I32, helper.params.front().name),
      .rhs = bir::Value::immediate_i32(static_cast<std::int32_t>(parsed->add_imm)),
  });
  helper_entry.terminator.value = bir::Value::named(bir::TypeKind::I32, "%sum");
  helper.blocks.push_back(std::move(helper_entry));
  lowered.functions.push_back(std::move(helper));

  bir::Function main_function;
  main_function.name = parsed->main_function->name;
  main_function.return_type = bir::TypeKind::I32;

  bir::Block main_entry;
  main_entry.label = "entry";
  main_entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, parsed->call->result.str()),
      .callee = parsed->helper->name,
      .args = {
          bir::Value::immediate_i32(static_cast<std::int32_t>(parsed->call_arg_imm)),
      },
      .return_type_name = "i32",
  });
  main_entry.terminator.value =
      bir::Value::named(bir::TypeKind::I32, parsed->call->result.str());
  main_function.blocks.push_back(std::move(main_entry));
  lowered.functions.push_back(std::move(main_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_direct_call_identity_arg_module(
    const c4c::codegen::lir::LirModule& module) {
  const auto parsed = parse_backend_minimal_direct_call_identity_arg_lir_module(module);
  if (!parsed.has_value() || parsed->helper == nullptr || parsed->main_function == nullptr ||
      parsed->call == nullptr || parsed->call->result.str().empty()) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function helper;
  helper.name = parsed->helper->name;
  helper.return_type = bir::TypeKind::I32;
  helper.params = {
      bir::Param{.type = bir::TypeKind::I32, .name = "%arg0"},
  };

  bir::Block helper_entry;
  helper_entry.label = "entry";
  helper_entry.terminator.value =
      bir::Value::named(bir::TypeKind::I32, helper.params.front().name);
  helper.blocks.push_back(std::move(helper_entry));
  lowered.functions.push_back(std::move(helper));

  bir::Function main_function;
  main_function.name = parsed->main_function->name;
  main_function.return_type = bir::TypeKind::I32;

  bir::Block main_entry;
  main_entry.label = "entry";
  main_entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, parsed->call->result.str()),
      .callee = parsed->helper->name,
      .args = {
          bir::Value::immediate_i32(static_cast<std::int32_t>(parsed->call_arg_imm)),
      },
      .return_type_name = "i32",
  });
  main_entry.terminator.value =
      bir::Value::named(bir::TypeKind::I32, parsed->call->result.str());
  main_function.blocks.push_back(std::move(main_entry));
  lowered.functions.push_back(std::move(main_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_folded_two_arg_direct_call_module(
    const c4c::codegen::lir::LirModule& module) {
  const auto parsed = parse_backend_minimal_folded_two_arg_direct_call_lir_module(module);
  if (!parsed.has_value() || parsed->caller_function == nullptr ||
      parsed->return_imm < std::numeric_limits<std::int32_t>::min() ||
      parsed->return_imm > std::numeric_limits<std::int32_t>::max()) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function main_function;
  main_function.name = parsed->caller_function->name;
  main_function.return_type = bir::TypeKind::I32;

  bir::Block main_entry;
  main_entry.label = "entry";
  main_entry.terminator.value =
      bir::Value::immediate_i32(static_cast<std::int32_t>(parsed->return_imm));
  main_function.blocks.push_back(std::move(main_entry));
  lowered.functions.push_back(std::move(main_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_dual_identity_direct_call_sub_module(
    const c4c::codegen::lir::LirModule& module) {
  const auto parsed = parse_backend_minimal_dual_identity_direct_call_sub_lir_module(module);
  if (!parsed.has_value() || parsed->lhs_helper == nullptr || parsed->rhs_helper == nullptr ||
      parsed->main_function == nullptr || parsed->lhs_call == nullptr ||
      parsed->rhs_call == nullptr || parsed->sub == nullptr ||
      parsed->lhs_call->result.str().empty() || parsed->rhs_call->result.str().empty() ||
      parsed->sub->result.str().empty()) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  auto make_identity_helper = [](std::string_view name) {
    bir::Function helper;
    helper.name = std::string(name);
    helper.return_type = bir::TypeKind::I32;
    helper.params = {
        bir::Param{.type = bir::TypeKind::I32, .name = "%arg0"},
    };

    bir::Block entry;
    entry.label = "entry";
    entry.terminator.value = bir::Value::named(bir::TypeKind::I32, "%arg0");
    helper.blocks.push_back(std::move(entry));
    return helper;
  };

  lowered.functions.push_back(make_identity_helper(parsed->lhs_helper->name));
  lowered.functions.push_back(make_identity_helper(parsed->rhs_helper->name));

  bir::Function main_function;
  main_function.name = parsed->main_function->name;
  main_function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, parsed->lhs_call->result.str()),
      .callee = parsed->lhs_helper->name,
      .args = {bir::Value::immediate_i32(static_cast<std::int32_t>(parsed->lhs_call_arg_imm))},
      .return_type_name = "i32",
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, parsed->rhs_call->result.str()),
      .callee = parsed->rhs_helper->name,
      .args = {bir::Value::immediate_i32(static_cast<std::int32_t>(parsed->rhs_call_arg_imm))},
      .return_type_name = "i32",
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, parsed->sub->result.str()),
      .lhs = bir::Value::named(bir::TypeKind::I32, parsed->lhs_call->result.str()),
      .rhs = bir::Value::named(bir::TypeKind::I32, parsed->rhs_call->result.str()),
  });
  entry.terminator.value = bir::Value::named(bir::TypeKind::I32, parsed->sub->result.str());
  main_function.blocks.push_back(std::move(entry));
  lowered.functions.push_back(std::move(main_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_call_crossing_direct_call_module(
    const c4c::codegen::lir::LirModule& module) {
  const auto parsed = parse_backend_minimal_call_crossing_direct_call_lir_module(module);
  if (!parsed.has_value() || parsed->helper == nullptr || parsed->main_function == nullptr ||
      parsed->call == nullptr || parsed->source_add == nullptr || parsed->final_add == nullptr ||
      parsed->regalloc_source_value.empty() || parsed->call->result.str().empty() ||
      parsed->source_add->result.str().empty() || parsed->final_add->result.str().empty()) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function helper;
  helper.name = parsed->helper->name;
  helper.return_type = bir::TypeKind::I32;
  helper.params = {
      bir::Param{.type = bir::TypeKind::I32, .name = "%arg0"},
  };

  bir::Block helper_entry;
  helper_entry.label = "entry";
  helper_entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "%sum"),
      .lhs = bir::Value::named(bir::TypeKind::I32, helper.params.front().name),
      .rhs = bir::Value::immediate_i32(static_cast<std::int32_t>(parsed->helper_add_imm)),
  });
  helper_entry.terminator.value = bir::Value::named(bir::TypeKind::I32, "%sum");
  helper.blocks.push_back(std::move(helper_entry));
  lowered.functions.push_back(std::move(helper));

  bir::Function main_function;
  main_function.name = parsed->main_function->name;
  main_function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, parsed->source_add->result.str()),
      .lhs = bir::Value::immediate_i32(static_cast<std::int32_t>(parsed->source_imm)),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, parsed->call->result.str()),
      .callee = parsed->helper->name,
      .args = {
          bir::Value::named(bir::TypeKind::I32, parsed->source_add->result.str()),
      },
      .return_type_name = "i32",
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, parsed->final_add->result.str()),
      .lhs = bir::Value::named(bir::TypeKind::I32, parsed->source_add->result.str()),
      .rhs = bir::Value::named(bir::TypeKind::I32, parsed->call->result.str()),
  });
  entry.terminator.value = bir::Value::named(bir::TypeKind::I32, parsed->final_add->result.str());
  main_function.blocks.push_back(std::move(entry));
  lowered.functions.push_back(std::move(main_function));
  return lowered;
}

std::optional<bir::BinaryOpcode> lower_binary_opcode(std::string_view opcode) {
  if (opcode == "add") {
    return bir::BinaryOpcode::Add;
  }
  if (opcode == "sub") {
    return bir::BinaryOpcode::Sub;
  }
  if (opcode == "mul") {
    return bir::BinaryOpcode::Mul;
  }
  if (opcode == "and") {
    return bir::BinaryOpcode::And;
  }
  if (opcode == "or") {
    return bir::BinaryOpcode::Or;
  }
  if (opcode == "xor") {
    return bir::BinaryOpcode::Xor;
  }
  if (opcode == "shl") {
    return bir::BinaryOpcode::Shl;
  }
  if (opcode == "lshr") {
    return bir::BinaryOpcode::LShr;
  }
  if (opcode == "ashr") {
    return bir::BinaryOpcode::AShr;
  }
  if (opcode == "sdiv") {
    return bir::BinaryOpcode::SDiv;
  }
  if (opcode == "udiv") {
    return bir::BinaryOpcode::UDiv;
  }
  if (opcode == "srem") {
    return bir::BinaryOpcode::SRem;
  }
  if (opcode == "urem") {
    return bir::BinaryOpcode::URem;
  }
  if (opcode == "eq") {
    return bir::BinaryOpcode::Eq;
  }
  return std::nullopt;
}

std::optional<bir::BinaryOpcode> lower_compare_materialization_opcode(std::string_view predicate) {
  if (predicate == "eq") {
    return bir::BinaryOpcode::Eq;
  }
  if (predicate == "ne") {
    return bir::BinaryOpcode::Ne;
  }
  if (predicate == "slt") {
    return bir::BinaryOpcode::Slt;
  }
  if (predicate == "sle") {
    return bir::BinaryOpcode::Sle;
  }
  if (predicate == "sgt") {
    return bir::BinaryOpcode::Sgt;
  }
  if (predicate == "sge") {
    return bir::BinaryOpcode::Sge;
  }
  if (predicate == "ult") {
    return bir::BinaryOpcode::Ult;
  }
  if (predicate == "ule") {
    return bir::BinaryOpcode::Ule;
  }
  if (predicate == "ugt") {
    return bir::BinaryOpcode::Ugt;
  }
  if (predicate == "uge") {
    return bir::BinaryOpcode::Uge;
  }
  return std::nullopt;
}

std::optional<bir::Value> lower_lossless_immediate_cast(
    const c4c::codegen::lir::LirInst& inst) {
  const auto* cast = std::get_if<c4c::codegen::lir::LirCastOp>(&inst);
  if (cast == nullptr || cast->result.str().empty() ||
      (cast->kind != c4c::codegen::lir::LirCastKind::SExt &&
       cast->kind != c4c::codegen::lir::LirCastKind::ZExt)) {
    return std::nullopt;
  }

  const auto from_type = lower_scalar_type_text(cast->from_type.str());
  const auto to_type = lower_scalar_type_text(cast->to_type.str());
  if (!from_type.has_value() || !to_type.has_value() ||
      scalar_type_bit_width(*from_type) >= scalar_type_bit_width(*to_type)) {
    return std::nullopt;
  }

  const auto source = lower_immediate_or_name(cast->operand.str(), *from_type);
  if (!source.has_value() || source->kind != bir::Value::Kind::Immediate) {
    return std::nullopt;
  }
  if (cast->kind == c4c::codegen::lir::LirCastKind::ZExt && source->immediate < 0) {
    return std::nullopt;
  }
  if (!immediate_fits_type(source->immediate, *to_type)) {
    return std::nullopt;
  }

  switch (*to_type) {
    case bir::TypeKind::I8:
      return bir::Value::immediate_i8(static_cast<std::int8_t>(source->immediate));
    case bir::TypeKind::I32:
      return bir::Value::immediate_i32(static_cast<std::int32_t>(source->immediate));
    case bir::TypeKind::I64:
      return bir::Value::immediate_i64(source->immediate);
    case bir::TypeKind::Void:
      return std::nullopt;
  }
  return std::nullopt;
}

std::optional<bir::CastInst> lower_cast(const c4c::codegen::lir::LirInst& inst) {
  const auto* cast = std::get_if<c4c::codegen::lir::LirCastOp>(&inst);
  if (cast == nullptr || cast->result.str().empty()) {
    return std::nullopt;
  }
  const auto from_type = lower_scalar_type_text(cast->from_type.str());
  const auto to_type = lower_scalar_type_text(cast->to_type.str());
  if (!from_type.has_value() || !to_type.has_value() ||
      *from_type == *to_type) {
    return std::nullopt;
  }

  bir::CastOpcode opcode;
  if (cast->kind == c4c::codegen::lir::LirCastKind::SExt) {
    if (*to_type <= *from_type) return std::nullopt;
    opcode = bir::CastOpcode::SExt;
  } else if (cast->kind == c4c::codegen::lir::LirCastKind::ZExt) {
    if (*to_type <= *from_type) return std::nullopt;
    opcode = bir::CastOpcode::ZExt;
  } else if (cast->kind == c4c::codegen::lir::LirCastKind::Trunc) {
    if (*to_type >= *from_type) return std::nullopt;
    opcode = bir::CastOpcode::Trunc;
  } else {
    return std::nullopt;
  }

  const auto operand = lower_immediate_or_name(cast->operand.str(), *from_type);
  if (!operand.has_value()) {
    return std::nullopt;
  }

  bir::CastInst lowered;
  lowered.opcode = opcode;
  lowered.result = bir::Value::named(*to_type, cast->result.str());
  lowered.operand = *operand;
  return lowered;
}

std::optional<bir::BinaryInst> lower_binary(const c4c::codegen::lir::LirInst& inst) {
  const auto* bin = std::get_if<c4c::codegen::lir::LirBinOp>(&inst);
  if (bin == nullptr || bin->result.str().empty()) {
    return std::nullopt;
  }
  const auto type = lower_scalar_type_text(bin->type_str.str());
  if (!type.has_value()) {
    return std::nullopt;
  }
  const auto opcode = lower_binary_opcode(bin->opcode.str());
  if (!opcode.has_value()) {
    return std::nullopt;
  }

  const auto lhs = lower_immediate_or_name(bin->lhs.str(), *type);
  const auto rhs = lower_immediate_or_name(bin->rhs.str(), *type);
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }

  bir::BinaryInst lowered;
  lowered.opcode = *opcode;
  lowered.result = bir::Value::named(*type, bin->result.str());
  lowered.lhs = *lhs;
  lowered.rhs = *rhs;
  return lowered;
}

std::optional<bir::BinaryInst> lower_compare_materialization(
    const c4c::codegen::lir::LirInst& compare_inst,
    const c4c::codegen::lir::LirInst& cast_inst) {
  const auto* cmp = std::get_if<c4c::codegen::lir::LirCmpOp>(&compare_inst);
  const auto* cast = std::get_if<c4c::codegen::lir::LirCastOp>(&cast_inst);
  if (cmp == nullptr || cast == nullptr || cmp->is_float || cmp->result.str().empty() ||
      cast->result.str().empty() ||
      cast->kind != c4c::codegen::lir::LirCastKind::ZExt || cast->from_type.str() != "i1" ||
      cast->operand.str() != cmp->result.str()) {
    return std::nullopt;
  }
  const auto opcode = lower_compare_materialization_opcode(cmp->predicate.str());
  if (!opcode.has_value()) {
    return std::nullopt;
  }

  const auto type = lower_scalar_type_text(cmp->type_str.str());
  const auto widened_type = lower_scalar_type_text(cast->to_type.str());
  if (!type.has_value() || !widened_type.has_value() || *type != *widened_type) {
    return std::nullopt;
  }

  const auto lhs = lower_immediate_or_name(cmp->lhs.str(), *type);
  const auto rhs = lower_immediate_or_name(cmp->rhs.str(), *type);
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }

  bir::BinaryInst lowered;
  lowered.opcode = *opcode;
  lowered.result = bir::Value::named(*type, cast->result.str());
  lowered.lhs = *lhs;
  lowered.rhs = *rhs;
  return lowered;
}

std::optional<bir::SelectInst> lower_select_materialization(
    const c4c::codegen::lir::LirInst& compare_inst,
    const c4c::codegen::lir::LirInst& select_inst) {
  const auto* cmp = std::get_if<c4c::codegen::lir::LirCmpOp>(&compare_inst);
  const auto* select = std::get_if<c4c::codegen::lir::LirSelectOp>(&select_inst);
  if (cmp == nullptr || select == nullptr || cmp->is_float || cmp->result.str().empty() ||
      select->result.str().empty() || select->cond.str() != cmp->result.str()) {
    return std::nullopt;
  }

  const auto predicate = lower_compare_materialization_opcode(cmp->predicate.str());
  const auto type = lower_scalar_type_text(cmp->type_str.str());
  const auto selected_type = lower_scalar_type_text(select->type_str.str());
  if (!predicate.has_value() || !type.has_value() || !selected_type.has_value() ||
      *type != *selected_type) {
    return std::nullopt;
  }

  const auto lhs = lower_immediate_or_name(cmp->lhs.str(), *type);
  const auto rhs = lower_immediate_or_name(cmp->rhs.str(), *type);
  const auto true_value = lower_immediate_or_name(select->true_val.str(), *type);
  const auto false_value = lower_immediate_or_name(select->false_val.str(), *type);
  if (!lhs.has_value() || !rhs.has_value() || !true_value.has_value() ||
      !false_value.has_value()) {
    return std::nullopt;
  }

  bir::SelectInst lowered;
  lowered.predicate = *predicate;
  lowered.result = bir::Value::named(*type, select->result.str());
  lowered.lhs = *lhs;
  lowered.rhs = *rhs;
  lowered.true_value = *true_value;
  lowered.false_value = *false_value;
  return lowered;
}

std::optional<std::vector<bir::Param>> lower_function_params(
    const c4c::codegen::lir::LirFunction& lir_function) {
  std::vector<bir::Param> params;
  if (!lir_function.params.empty()) {
    if (lir_function.params.size() > 2) {
      return std::nullopt;
    }
    params.reserve(lir_function.params.size());
    for (const auto& [param_name, param_type] : lir_function.params) {
      const auto lowered_type = lower_minimal_scalar_type(param_type);
      if (!lowered_type.has_value() || param_name.empty()) {
        return std::nullopt;
      }
      params.push_back(bir::Param{*lowered_type, param_name});
    }
    return params;
  }

  const auto parsed_params =
      parse_backend_function_signature_params(lir_function.signature_text);
  if (!parsed_params.has_value() || parsed_params->size() > 2) {
    return std::nullopt;
  }

  params.reserve(parsed_params->size());
  for (const auto& param : *parsed_params) {
    const auto lowered_type = lower_scalar_type_text(param.type);
    if (param.is_varargs || !lowered_type.has_value() || param.operand.empty()) {
      return std::nullopt;
    }
    params.push_back(bir::Param{*lowered_type, param.operand});
  }
  return params;
}

struct AffineValue {
  bool uses_first_param = false;
  bool uses_second_param = false;
  std::int64_t constant = 0;
};

std::optional<AffineValue> combine_affine(const AffineValue& lhs,
                                          const AffineValue& rhs,
                                          bir::BinaryOpcode opcode) {
  if (opcode == bir::BinaryOpcode::Add) {
    if ((lhs.uses_first_param && rhs.uses_first_param) ||
        (lhs.uses_second_param && rhs.uses_second_param)) {
      return std::nullopt;
    }
    return AffineValue{lhs.uses_first_param || rhs.uses_first_param,
                       lhs.uses_second_param || rhs.uses_second_param,
                       lhs.constant + rhs.constant};
  }

  if (rhs.uses_first_param || rhs.uses_second_param) {
    return std::nullopt;
  }
  if (opcode == bir::BinaryOpcode::Sub) {
    return AffineValue{lhs.uses_first_param, lhs.uses_second_param,
                       lhs.constant - rhs.constant};
  }

  if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
      rhs.uses_second_param) {
    return std::nullopt;
  }
  if (opcode == bir::BinaryOpcode::And) {
    return AffineValue{false, false, lhs.constant & rhs.constant};
  }
  if (opcode == bir::BinaryOpcode::Or) {
    return AffineValue{false, false, lhs.constant | rhs.constant};
  }
  if (opcode == bir::BinaryOpcode::Shl) {
    if (rhs.constant < 0) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant << rhs.constant};
  }
  if (opcode == bir::BinaryOpcode::LShr) {
    if (lhs.constant < 0 || rhs.constant < 0) {
      return std::nullopt;
    }
    return AffineValue{
        false, false,
        static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs.constant) >> rhs.constant)};
  }
  if (opcode == bir::BinaryOpcode::AShr) {
    if (rhs.constant < 0) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant >> rhs.constant};
  }
  if (opcode == bir::BinaryOpcode::SDiv) {
    if (rhs.constant == 0) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant / rhs.constant};
  }
  if (opcode == bir::BinaryOpcode::UDiv) {
    if (lhs.constant < 0 || rhs.constant <= 0) {
      return std::nullopt;
    }
    return AffineValue{
        false, false,
        static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs.constant) /
                                  static_cast<std::uint64_t>(rhs.constant))};
  }
  if (opcode == bir::BinaryOpcode::SRem) {
    if (rhs.constant == 0) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant % rhs.constant};
  }
  if (opcode == bir::BinaryOpcode::URem) {
    if (lhs.constant < 0 || rhs.constant <= 0) {
      return std::nullopt;
    }
    return AffineValue{
        false, false,
        static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs.constant) %
                                  static_cast<std::uint64_t>(rhs.constant))};
  }
  if (opcode == bir::BinaryOpcode::Eq) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant == rhs.constant ? 1 : 0};
  }
  if (opcode == bir::BinaryOpcode::Ne) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant != rhs.constant ? 1 : 0};
  }
  if (opcode == bir::BinaryOpcode::Slt) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant < rhs.constant ? 1 : 0};
  }
  if (opcode == bir::BinaryOpcode::Sle) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant <= rhs.constant ? 1 : 0};
  }
  if (opcode == bir::BinaryOpcode::Sgt) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant > rhs.constant ? 1 : 0};
  }
  if (opcode == bir::BinaryOpcode::Sge) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param) {
      return std::nullopt;
    }
    return AffineValue{false, false, lhs.constant >= rhs.constant ? 1 : 0};
  }
  if (opcode == bir::BinaryOpcode::Ult) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param || lhs.constant < 0 || rhs.constant < 0) {
      return std::nullopt;
    }
    return AffineValue{
        false, false,
        static_cast<std::uint64_t>(lhs.constant) < static_cast<std::uint64_t>(rhs.constant)
            ? 1
            : 0};
  }
  if (opcode == bir::BinaryOpcode::Ule) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param || lhs.constant < 0 || rhs.constant < 0) {
      return std::nullopt;
    }
    return AffineValue{
        false, false,
        static_cast<std::uint64_t>(lhs.constant) <= static_cast<std::uint64_t>(rhs.constant)
            ? 1
            : 0};
  }
  if (opcode == bir::BinaryOpcode::Ugt) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param || lhs.constant < 0 || rhs.constant < 0) {
      return std::nullopt;
    }
    return AffineValue{
        false, false,
        static_cast<std::uint64_t>(lhs.constant) > static_cast<std::uint64_t>(rhs.constant)
            ? 1
            : 0};
  }
  if (opcode == bir::BinaryOpcode::Uge) {
    if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
        rhs.uses_second_param || lhs.constant < 0 || rhs.constant < 0) {
      return std::nullopt;
    }
    return AffineValue{
        false, false,
        static_cast<std::uint64_t>(lhs.constant) >= static_cast<std::uint64_t>(rhs.constant)
            ? 1
            : 0};
  }
  return AffineValue{false, false, lhs.constant * rhs.constant};
}

std::optional<bool> evaluate_predicate(const AffineValue& lhs,
                                       const AffineValue& rhs,
                                       bir::BinaryOpcode opcode) {
  if (lhs.uses_first_param || lhs.uses_second_param || rhs.uses_first_param ||
      rhs.uses_second_param) {
    return std::nullopt;
  }
  switch (opcode) {
    case bir::BinaryOpcode::Eq:
      return lhs.constant == rhs.constant;
    case bir::BinaryOpcode::Ne:
      return lhs.constant != rhs.constant;
    case bir::BinaryOpcode::Slt:
      return lhs.constant < rhs.constant;
    case bir::BinaryOpcode::Sle:
      return lhs.constant <= rhs.constant;
    case bir::BinaryOpcode::Sgt:
      return lhs.constant > rhs.constant;
    case bir::BinaryOpcode::Sge:
      return lhs.constant >= rhs.constant;
    case bir::BinaryOpcode::Ult:
      if (lhs.constant < 0 || rhs.constant < 0) {
        return std::nullopt;
      }
      return static_cast<std::uint64_t>(lhs.constant) <
             static_cast<std::uint64_t>(rhs.constant);
    case bir::BinaryOpcode::Ule:
      if (lhs.constant < 0 || rhs.constant < 0) {
        return std::nullopt;
      }
      return static_cast<std::uint64_t>(lhs.constant) <=
             static_cast<std::uint64_t>(rhs.constant);
    case bir::BinaryOpcode::Ugt:
      if (lhs.constant < 0 || rhs.constant < 0) {
        return std::nullopt;
      }
      return static_cast<std::uint64_t>(lhs.constant) >
             static_cast<std::uint64_t>(rhs.constant);
    case bir::BinaryOpcode::Uge:
      if (lhs.constant < 0 || rhs.constant < 0) {
        return std::nullopt;
      }
      return static_cast<std::uint64_t>(lhs.constant) >=
             static_cast<std::uint64_t>(rhs.constant);
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::AShr:
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::SRem:
    case bir::BinaryOpcode::URem:
      return std::nullopt;
  }
  return std::nullopt;
}

std::optional<std::vector<bir::BinaryInst>> lower_bounded_predecessor_chain(
    const c4c::codegen::lir::LirBlock& lir_block,
    std::string_view expected_result,
    const std::vector<bir::Param>& params,
    bir::TypeKind type) {
  std::vector<bir::BinaryInst> lowered;
  lowered.reserve(lir_block.insts.size());

  std::vector<std::string> available_names;
  available_names.reserve(params.size() + lir_block.insts.size());
  std::vector<AffineValue> affine_values;
  affine_values.reserve(params.size() + lir_block.insts.size());
  for (std::size_t index = 0; index < params.size(); ++index) {
    available_names.push_back(params[index].name);
    affine_values.push_back(AffineValue{index == 0, index == 1, 0});
  }

  auto name_is_available = [&](std::string_view name) {
    return std::find(available_names.begin(), available_names.end(), name) !=
           available_names.end();
  };
  auto operand_is_available = [&](const bir::Value& value) {
    return value.kind != bir::Value::Kind::Named || name_is_available(value.name);
  };
  auto lower_affine_value = [&](const bir::Value& value) -> std::optional<AffineValue> {
    if (value.kind == bir::Value::Kind::Immediate) {
      return AffineValue{false, false, value.immediate};
    }
    for (std::size_t index = 0; index < available_names.size(); ++index) {
      if (available_names[index] == value.name) {
        return affine_values[index];
      }
    }
    return std::nullopt;
  };

  for (const auto& inst : lir_block.insts) {
    auto binary = lower_binary(inst);
    if (!binary.has_value() || binary->result.type != type ||
        (binary->opcode != bir::BinaryOpcode::Add &&
         binary->opcode != bir::BinaryOpcode::Sub)) {
      return std::nullopt;
    }
    if (!operand_is_available(binary->lhs) || !operand_is_available(binary->rhs) ||
        name_is_available(binary->result.name)) {
      return std::nullopt;
    }
    const auto lhs = lower_affine_value(binary->lhs);
    const auto rhs = lower_affine_value(binary->rhs);
    if (!lhs.has_value() || !rhs.has_value()) {
      return std::nullopt;
    }
    const auto combined = combine_affine(*lhs, *rhs, binary->opcode);
    if (!combined.has_value()) {
      return std::nullopt;
    }
    available_names.push_back(binary->result.name);
    affine_values.push_back(*combined);
    lowered.push_back(*binary);
  }

  if (!lowered.empty() && !expected_result.empty() &&
      lowered.back().result.name != expected_result) {
    return std::nullopt;
  }
  return lowered;
}

std::optional<std::vector<bir::BinaryInst>> lower_bounded_predecessor_chain(
    const c4c::codegen::lir::LirBlock& first_block,
    const c4c::codegen::lir::LirBlock& second_block,
    std::string_view expected_result,
    const std::vector<bir::Param>& params,
    bir::TypeKind type) {
  auto lowered = lower_bounded_predecessor_chain(first_block, "", params, type);
  if (!lowered.has_value()) {
    return std::nullopt;
  }
  auto tail = lower_bounded_predecessor_chain(second_block, expected_result, params, type);
  if (!tail.has_value()) {
    return std::nullopt;
  }

  if (lowered->empty()) {
    return tail;
  }
  if (tail->empty()) {
    if (lowered->back().result.name != expected_result) {
      return std::nullopt;
    }
    return lowered;
  }

  std::vector<std::string> available_names;
  available_names.reserve(params.size() + lowered->size());
  for (const auto& param : params) {
    available_names.push_back(param.name);
  }
  for (const auto& inst : *lowered) {
    available_names.push_back(inst.result.name);
  }

  auto operand_is_available = [&](const bir::Value& value) {
    if (value.kind == bir::Value::Kind::Immediate) {
      return true;
    }
    return std::find(available_names.begin(), available_names.end(), value.name) !=
           available_names.end();
  };
  for (const auto& inst : *tail) {
    if (!operand_is_available(inst.lhs) || !operand_is_available(inst.rhs) ||
        std::find(available_names.begin(), available_names.end(), inst.result.name) !=
            available_names.end()) {
      return std::nullopt;
    }
    lowered->push_back(inst);
    available_names.push_back(inst.result.name);
  }
  return lowered;
}

std::optional<bir::Function> try_lower_conditional_return_select_function(
    const c4c::codegen::lir::LirFunction& lir_function,
    const std::vector<bir::Param>& params) {
  using namespace c4c::codegen::lir;

  if (lir_function.blocks.size() != 3) {
    return std::nullopt;
  }

  const auto& entry = lir_function.blocks[0];
  const auto& true_block = lir_function.blocks[1];
  const auto& false_block = lir_function.blocks[2];
  if (entry.label.empty() || entry.insts.size() != 3 || !true_block.insts.empty() ||
      !false_block.insts.empty()) {
    return std::nullopt;
  }

  const auto* cmp0 = std::get_if<LirCmpOp>(&entry.insts[0]);
  const auto* cast = std::get_if<LirCastOp>(&entry.insts[1]);
  const auto* cmp1 = std::get_if<LirCmpOp>(&entry.insts[2]);
  const auto* condbr = std::get_if<LirCondBr>(&entry.terminator);
  if (cmp0 == nullptr || cast == nullptr || cmp1 == nullptr || condbr == nullptr ||
      cmp0->is_float || cmp0->result.str().empty() ||
      cast->result.str().empty() || cmp1->is_float || cmp1->result.str().empty() ||
      cast->kind != LirCastKind::ZExt || cast->from_type.str() != "i1" ||
      cast->operand.str() != cmp0->result.str() || cmp1->predicate.str() != "ne" ||
      cmp1->lhs.str() != cast->result.str() || cmp1->rhs.str() != "0" ||
      condbr->cond_name != cmp1->result.str() || true_block.label != condbr->true_label ||
      false_block.label != condbr->false_label) {
    return std::nullopt;
  }

  const auto predicate = lower_compare_materialization_opcode(cmp0->predicate.str());
  const auto compare_type = lower_scalar_type_text(cmp0->type_str.str());
  const auto widened_type = lower_scalar_type_text(cast->to_type.str());
  const auto cond_type = lower_scalar_type_text(cmp1->type_str.str());
  if (!predicate.has_value() || !compare_type.has_value() || !widened_type.has_value() ||
      !cond_type.has_value() || *compare_type != *widened_type ||
      *compare_type != *cond_type) {
    return std::nullopt;
  }

  const auto* true_ret = std::get_if<LirRet>(&true_block.terminator);
  const auto* false_ret = std::get_if<LirRet>(&false_block.terminator);
  if (true_ret == nullptr || false_ret == nullptr || !true_ret->value_str.has_value() ||
      !false_ret->value_str.has_value() || true_ret->type_str != cmp0->type_str ||
      false_ret->type_str != cmp0->type_str) {
    return std::nullopt;
  }

  const auto lhs = lower_immediate_or_name(cmp0->lhs.str(), *compare_type);
  const auto rhs = lower_immediate_or_name(cmp0->rhs.str(), *compare_type);
  const auto true_value = lower_immediate_or_name(*true_ret->value_str, *compare_type);
  const auto false_value = lower_immediate_or_name(*false_ret->value_str, *compare_type);
  if (!lhs.has_value() || !rhs.has_value() || !true_value.has_value() ||
      !false_value.has_value()) {
    return std::nullopt;
  }

  auto operand_is_param_or_immediate = [&](const bir::Value& value) {
    if (value.kind == bir::Value::Kind::Immediate) {
      return true;
    }
    return std::any_of(params.begin(), params.end(), [&](const bir::Param& param) {
      return param.name == value.name;
    });
  };
  if (!operand_is_param_or_immediate(*lhs) || !operand_is_param_or_immediate(*rhs) ||
      !operand_is_param_or_immediate(*true_value) ||
      !operand_is_param_or_immediate(*false_value)) {
    return std::nullopt;
  }

  bir::Function function;
  function.name = lir_function.name;
  function.return_type = *compare_type;
  function.params = params;

  bir::Block block;
  block.label = entry.label;
  block.insts.push_back(bir::SelectInst{
      *predicate,
      bir::Value::named(*compare_type, "%t.select"),
      *lhs,
      *rhs,
      *true_value,
      *false_value,
  });
  block.terminator.value = bir::Value::named(*compare_type, "%t.select");
  function.blocks.push_back(std::move(block));
  return function;
}

std::optional<bir::Function> try_lower_conditional_phi_select_function(
    const c4c::codegen::lir::LirFunction& lir_function,
    const std::vector<bir::Param>& params) {
  using namespace c4c::codegen::lir;

  if (lir_function.blocks.size() != 4 && lir_function.blocks.size() != 6) {
    return std::nullopt;
  }

  const auto& entry = lir_function.blocks[0];
  const auto* cmp0 = entry.insts.size() > 0 ? std::get_if<LirCmpOp>(&entry.insts[0]) : nullptr;
  const auto* cast = entry.insts.size() > 1 ? std::get_if<LirCastOp>(&entry.insts[1]) : nullptr;
  const auto* cmp1 = entry.insts.size() > 2 ? std::get_if<LirCmpOp>(&entry.insts[2]) : nullptr;
  const auto* condbr = std::get_if<LirCondBr>(&entry.terminator);
  if (entry.label.empty() || entry.insts.size() != 3 || cmp0 == nullptr || cast == nullptr ||
      cmp1 == nullptr || condbr == nullptr || cmp0->is_float || cmp0->result.str().empty() ||
      cast->result.str().empty() || cmp1->is_float || cmp1->result.str().empty() ||
      cast->kind != LirCastKind::ZExt || cast->from_type.str() != "i1" ||
      cast->operand.str() != cmp0->result.str() || cmp1->predicate.str() != "ne" ||
      cmp1->lhs.str() != cast->result.str() || cmp1->rhs.str() != "0" ||
      condbr->cond_name != cmp1->result.str()) {
    return std::nullopt;
  }

  const auto predicate = lower_compare_materialization_opcode(cmp0->predicate.str());
  const auto compare_type = lower_scalar_type_text(cmp0->type_str.str());
  const auto widened_type = lower_scalar_type_text(cast->to_type.str());
  const auto cond_type = lower_scalar_type_text(cmp1->type_str.str());
  if (!predicate.has_value() || !compare_type.has_value() || !widened_type.has_value() ||
      !cond_type.has_value() || *compare_type != *widened_type ||
      *compare_type != *cond_type) {
    return std::nullopt;
  }

  const auto& true_block = lir_function.blocks[1];
  const auto& false_block = lir_function.blocks[lir_function.blocks.size() == 4 ? 2 : 3];
  const auto* true_br = std::get_if<LirBr>(&true_block.terminator);
  const auto* false_br = std::get_if<LirBr>(&false_block.terminator);
  if (true_br == nullptr || false_br == nullptr || condbr->true_label != true_block.label ||
      condbr->false_label != false_block.label) {
    return std::nullopt;
  }

  const c4c::codegen::lir::LirBlock* true_phi_pred = &true_block;
  const c4c::codegen::lir::LirBlock* false_phi_pred = &false_block;
  const c4c::codegen::lir::LirBlock* true_value_block = &true_block;
  const c4c::codegen::lir::LirBlock* false_value_block = &false_block;
  const c4c::codegen::lir::LirBlock* join_block = nullptr;
  if (lir_function.blocks.size() == 4) {
    join_block = &lir_function.blocks[3];
    if (true_br->target_label != join_block->label || false_br->target_label != join_block->label) {
      return std::nullopt;
    }
  } else {
    const auto& true_end = lir_function.blocks[2];
    const auto& false_end = lir_function.blocks[4];
    const auto* true_end_br = std::get_if<LirBr>(&true_end.terminator);
    const auto* false_end_br = std::get_if<LirBr>(&false_end.terminator);
    join_block = &lir_function.blocks[5];
    if (true_end_br == nullptr || false_end_br == nullptr ||
        true_br->target_label != true_end.label ||
        false_br->target_label != false_end.label ||
        true_end_br->target_label != join_block->label ||
        false_end_br->target_label != join_block->label) {
      return std::nullopt;
    }
    true_phi_pred = &true_end;
    false_phi_pred = &false_end;
    true_value_block = &true_block;
    false_value_block = &false_block;
  }

  const auto* phi = join_block->insts.size() > 0 ? std::get_if<LirPhiOp>(&join_block->insts[0]) : nullptr;
  const auto* ret = std::get_if<LirRet>(&join_block->terminator);
  if (phi == nullptr || ret == nullptr ||
      phi->result.str().empty() || phi->type_str.str() != cmp0->type_str.str() ||
      phi->incoming.size() != 2 || !ret->value_str.has_value() ||
      ret->type_str != cmp0->type_str ||
      phi->incoming[0].second != true_phi_pred->label ||
      phi->incoming[1].second != false_phi_pred->label) {
    return std::nullopt;
  }

  const auto lhs = lower_immediate_or_name(cmp0->lhs.str(), *compare_type);
  const auto rhs = lower_immediate_or_name(cmp0->rhs.str(), *compare_type);
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }

  const auto true_chain =
      lir_function.blocks.size() == 4
          ? lower_bounded_predecessor_chain(*true_phi_pred, phi->incoming[0].first, params,
                                            *compare_type)
          : lower_bounded_predecessor_chain(*true_value_block, *true_phi_pred,
                                            phi->incoming[0].first, params, *compare_type);
  const auto false_chain =
      lir_function.blocks.size() == 4
          ? lower_bounded_predecessor_chain(*false_phi_pred, phi->incoming[1].first, params,
                                            *compare_type)
          : lower_bounded_predecessor_chain(*false_value_block, *false_phi_pred,
                                            phi->incoming[1].first, params, *compare_type);
  if (!true_chain.has_value() || !false_chain.has_value()) {
    return std::nullopt;
  }

  auto operand_is_param_or_immediate = [&](const bir::Value& value) {
    if (value.kind == bir::Value::Kind::Immediate) {
      return true;
    }
    return std::any_of(params.begin(), params.end(), [&](const bir::Param& param) {
      return param.name == value.name;
    });
  };
  if (!operand_is_param_or_immediate(*lhs) || !operand_is_param_or_immediate(*rhs)) {
    return std::nullopt;
  }

  bir::Function function;
  function.name = lir_function.name;
  function.return_type = *compare_type;
  function.params = params;

  bir::Block block;
  block.label = entry.label;
  std::vector<std::string> available_names;
  available_names.reserve(params.size() + true_chain->size() + false_chain->size() + 1);
  for (const auto& param : params) {
    available_names.push_back(param.name);
  }
  for (const auto& inst : *true_chain) {
    block.insts.push_back(inst);
    available_names.push_back(inst.result.name);
  }
  for (const auto& inst : *false_chain) {
    block.insts.push_back(inst);
    available_names.push_back(inst.result.name);
  }

  auto lower_branch_value = [&](std::string_view value_text,
                                const std::vector<bir::BinaryInst>& chain)
      -> std::optional<bir::Value> {
    if (!chain.empty()) {
      return bir::Value::named(*compare_type, std::string(value_text));
    }
    return lower_immediate_or_name(value_text, *compare_type);
  };
  const auto true_value = lower_branch_value(phi->incoming[0].first, *true_chain);
  const auto false_value = lower_branch_value(phi->incoming[1].first, *false_chain);
  if (!true_value.has_value() || !false_value.has_value()) {
    return std::nullopt;
  }

  auto operand_is_available = [&](const bir::Value& value) {
    if (value.kind == bir::Value::Kind::Immediate) {
      return true;
    }
    return std::find(available_names.begin(), available_names.end(), value.name) !=
           available_names.end();
  };
  if (!operand_is_available(*true_value) || !operand_is_available(*false_value)) {
    return std::nullopt;
  }

  block.insts.push_back(bir::SelectInst{
      *predicate,
      bir::Value::named(*compare_type, phi->result.str()),
      *lhs,
      *rhs,
      *true_value,
      *false_value,
  });
  available_names.push_back(phi->result.str());

  for (std::size_t inst_index = 1; inst_index < join_block->insts.size(); ++inst_index) {
    auto binary = lower_binary(join_block->insts[inst_index]);
    if (!binary.has_value() || binary->result.type != *compare_type ||
        (binary->opcode != bir::BinaryOpcode::Add &&
         binary->opcode != bir::BinaryOpcode::Sub)) {
      return std::nullopt;
    }
    if (!operand_is_available(binary->lhs) || !operand_is_available(binary->rhs) ||
        std::find(available_names.begin(), available_names.end(), binary->result.name) !=
            available_names.end()) {
      return std::nullopt;
    }
    block.insts.push_back(*binary);
    available_names.push_back(binary->result.name);
  }

  const auto return_value = lower_immediate_or_name(*ret->value_str, *compare_type);
  if (!return_value.has_value()) {
    return std::nullopt;
  }
  if (return_value->kind == bir::Value::Kind::Named &&
      std::find(available_names.begin(), available_names.end(), return_value->name) ==
          available_names.end()) {
    return std::nullopt;
  }
  block.terminator.value = *return_value;
  function.blocks.push_back(std::move(block));
  return function;
}

std::optional<bir::Function> try_lower_widened_i8_add_sub_chain_function(
    const c4c::codegen::lir::LirFunction& lir_function,
    const std::vector<bir::Param>& params) {
  using namespace c4c::codegen::lir;

  if (lir_function.blocks.size() != 1) {
    return std::nullopt;
  }
  if (!std::all_of(params.begin(), params.end(), [](const bir::Param& param) {
        return param.type == bir::TypeKind::I8;
      })) {
    return std::nullopt;
  }

  const auto& lir_block = lir_function.blocks.front();
  if (lir_block.label.empty() || lir_block.insts.empty()) {
    return std::nullopt;
  }

  const auto* ret = std::get_if<LirRet>(&lir_block.terminator);
  const auto* trunc = std::get_if<LirCastOp>(&lir_block.insts.back());
  if (ret == nullptr || trunc == nullptr || !ret->value_str.has_value() ||
      trunc->result.str().empty() || *ret->value_str != trunc->result.str() ||
      ret->type_str != "i8" || trunc->kind != LirCastKind::Trunc ||
      trunc->from_type.str() != "i32" || trunc->to_type.str() != "i8") {
    return std::nullopt;
  }

  bir::Function function;
  function.name = lir_function.name;
  function.return_type = bir::TypeKind::I8;
  function.params = params;

  bir::Block block;
  block.label = lir_block.label;

  std::vector<std::string> defined_names;
  defined_names.reserve(params.size() + lir_block.insts.size());
  std::vector<bir::Value> resolved_values;
  resolved_values.reserve(params.size() + lir_block.insts.size());
  std::vector<AffineValue> affine_values;
  affine_values.reserve(params.size() + lir_block.insts.size());
  for (std::size_t index = 0; index < params.size(); ++index) {
    defined_names.push_back(params[index].name);
    resolved_values.push_back(bir::Value::named(params[index].type, params[index].name));
    affine_values.push_back(AffineValue{index == 0, index == 1, 0});
  }

  auto name_is_defined = [&](std::string_view name) {
    return std::find(defined_names.begin(), defined_names.end(), name) !=
           defined_names.end();
  };
  auto resolve_value = [&](const bir::Value& value) -> std::optional<bir::Value> {
    if (value.kind == bir::Value::Kind::Immediate) {
      return value;
    }
    for (std::size_t index = 0; index < defined_names.size(); ++index) {
      if (defined_names[index] == value.name) {
        return resolved_values[index];
      }
    }
    return std::nullopt;
  };
  auto lower_affine_value = [&](const bir::Value& value) -> std::optional<AffineValue> {
    if (value.kind == bir::Value::Kind::Immediate) {
      return AffineValue{false, false, value.immediate};
    }
    for (std::size_t index = 0; index < defined_names.size(); ++index) {
      if (defined_names[index] == value.name) {
        return affine_values[index];
      }
    }
    return std::nullopt;
  };
  auto narrow_i8_value = [&](const bir::Value& value) -> std::optional<bir::Value> {
    if (value.kind == bir::Value::Kind::Immediate) {
      if (!immediate_fits_type(value.immediate, bir::TypeKind::I8)) {
        return std::nullopt;
      }
      return bir::Value::immediate_i8(static_cast<std::int8_t>(value.immediate));
    }
    if (value.type != bir::TypeKind::I8) {
      return std::nullopt;
    }
    return value;
  };

  for (std::size_t inst_index = 0; inst_index + 1 < lir_block.insts.size(); ++inst_index) {
    if (const auto* cast = std::get_if<LirCastOp>(&lir_block.insts[inst_index]); cast != nullptr) {
      if (cast->result.str().empty() || name_is_defined(cast->result.str()) ||
          (cast->kind != LirCastKind::SExt && cast->kind != LirCastKind::ZExt) ||
          cast->from_type.str() != "i8" || cast->to_type.str() != "i32") {
        return std::nullopt;
      }
      const auto source = lower_immediate_or_name(cast->operand.str(), bir::TypeKind::I8);
      if (!source.has_value()) {
        return std::nullopt;
      }
      const auto resolved_source = resolve_value(*source);
      const auto affine_source = resolved_source.has_value()
                                     ? lower_affine_value(*resolved_source)
                                     : std::nullopt;
      if (!resolved_source.has_value() || !affine_source.has_value()) {
        return std::nullopt;
      }
      defined_names.push_back(cast->result.str());
      resolved_values.push_back(*resolved_source);
      affine_values.push_back(*affine_source);
      continue;
    }

    auto binary = lower_binary(lir_block.insts[inst_index]);
    if (!binary.has_value() || binary->result.type != bir::TypeKind::I32 ||
        (binary->opcode != bir::BinaryOpcode::Add &&
         binary->opcode != bir::BinaryOpcode::Sub &&
         binary->opcode != bir::BinaryOpcode::Mul &&
         binary->opcode != bir::BinaryOpcode::And &&
         binary->opcode != bir::BinaryOpcode::Or &&
         binary->opcode != bir::BinaryOpcode::Xor &&
         binary->opcode != bir::BinaryOpcode::Shl &&
         binary->opcode != bir::BinaryOpcode::LShr &&
         binary->opcode != bir::BinaryOpcode::AShr &&
         binary->opcode != bir::BinaryOpcode::SDiv &&
         binary->opcode != bir::BinaryOpcode::UDiv &&
         binary->opcode != bir::BinaryOpcode::SRem &&
         binary->opcode != bir::BinaryOpcode::URem) ||
        name_is_defined(binary->result.name)) {
      return std::nullopt;
    }

    const auto lhs_value = resolve_value(binary->lhs);
    const auto rhs_value = resolve_value(binary->rhs);
    if (!lhs_value.has_value() || !rhs_value.has_value()) {
      return std::nullopt;
    }
    const auto narrow_lhs = narrow_i8_value(*lhs_value);
    const auto narrow_rhs = narrow_i8_value(*rhs_value);
    if (!narrow_lhs.has_value() || !narrow_rhs.has_value()) {
      return std::nullopt;
    }

    const auto lhs_affine = lower_affine_value(*narrow_lhs);
    const auto rhs_affine = lower_affine_value(*narrow_rhs);
    if (!lhs_affine.has_value() || !rhs_affine.has_value()) {
      return std::nullopt;
    }
    if (binary->opcode == bir::BinaryOpcode::Mul &&
        (lhs_affine->uses_first_param || lhs_affine->uses_second_param ||
         rhs_affine->uses_first_param || rhs_affine->uses_second_param)) {
      return std::nullopt;
    }
    const auto combined = combine_affine(*lhs_affine, *rhs_affine, binary->opcode);
    if (!combined.has_value()) {
      return std::nullopt;
    }

    bir::BinaryInst narrowed;
    narrowed.opcode = binary->opcode;
    narrowed.result = bir::Value::named(bir::TypeKind::I8, binary->result.name);
    narrowed.lhs = *narrow_lhs;
    narrowed.rhs = *narrow_rhs;
    block.insts.push_back(narrowed);

    defined_names.push_back(binary->result.name);
    resolved_values.push_back(bir::Value::named(bir::TypeKind::I8, binary->result.name));
    affine_values.push_back(*combined);
  }

  const auto widened_return_value =
      lower_immediate_or_name(trunc->operand.str(), bir::TypeKind::I32);
  if (!widened_return_value.has_value()) {
    return std::nullopt;
  }
  const auto resolved_return_value = resolve_value(*widened_return_value);
  const auto narrow_return_value = resolved_return_value.has_value()
                                       ? narrow_i8_value(*resolved_return_value)
                                       : std::nullopt;
  if (!narrow_return_value.has_value()) {
    return std::nullopt;
  }

  block.terminator.value = *narrow_return_value;
  function.blocks.push_back(std::move(block));
  return function;
}

std::optional<bir::Function> try_lower_widened_i8_compare_return_function(
    const c4c::codegen::lir::LirFunction& lir_function,
    const std::vector<bir::Param>& params) {
  using namespace c4c::codegen::lir;

  if (!params.empty() || lir_function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& lir_block = lir_function.blocks.front();
  if (lir_block.label.empty() || lir_block.insts.size() < 3) {
    return std::nullopt;
  }

  const auto* ret = std::get_if<LirRet>(&lir_block.terminator);
  const auto* cmp = std::get_if<LirCmpOp>(&lir_block.insts[lir_block.insts.size() - 3]);
  const auto* cond_cast = std::get_if<LirCastOp>(&lir_block.insts[lir_block.insts.size() - 2]);
  const auto* trunc = std::get_if<LirCastOp>(&lir_block.insts[lir_block.insts.size() - 1]);
  if (ret == nullptr || trunc == nullptr || !ret->value_str.has_value() ||
      trunc->result.str().empty() || *ret->value_str != trunc->result.str() ||
      ret->type_str != "i8" || trunc->kind != LirCastKind::Trunc ||
      trunc->from_type.str() != "i32" || trunc->to_type.str() != "i8" ||
      cmp == nullptr || cmp->is_float || cmp->result.str().empty() ||
      cond_cast == nullptr || cond_cast->result.str().empty() ||
      cond_cast->kind != LirCastKind::ZExt || cond_cast->from_type.str() != "i1" ||
      cond_cast->to_type.str() != "i32" || cond_cast->operand.str() != cmp->result.str() ||
      trunc->operand.str() != cond_cast->result.str()) {
    return std::nullopt;
  }

  auto narrow_i8_value = [&](const bir::Value& value) -> std::optional<bir::Value> {
    if (value.kind == bir::Value::Kind::Immediate) {
      if (!immediate_fits_type(value.immediate, bir::TypeKind::I8)) {
        return std::nullopt;
      }
      return bir::Value::immediate_i8(static_cast<std::int8_t>(value.immediate));
    }
    if (value.type != bir::TypeKind::I8) {
      return std::nullopt;
    }
    return value;
  };

  std::vector<std::string> defined_names;
  defined_names.reserve(lir_block.insts.size());
  std::vector<bir::Value> resolved_values;
  resolved_values.reserve(lir_block.insts.size());

  auto name_is_defined = [&](std::string_view name) {
    return std::find(defined_names.begin(), defined_names.end(), name) !=
           defined_names.end();
  };
  auto resolve_value = [&](const bir::Value& value) -> std::optional<bir::Value> {
    if (value.kind == bir::Value::Kind::Immediate) {
      return value;
    }
    for (std::size_t index = 0; index < defined_names.size(); ++index) {
      if (defined_names[index] == value.name) {
        return resolved_values[index];
      }
    }
    return std::nullopt;
  };

  for (std::size_t inst_index = 0; inst_index + 3 < lir_block.insts.size(); ++inst_index) {
    const auto* cast = std::get_if<LirCastOp>(&lir_block.insts[inst_index]);
    if (cast == nullptr || cast->result.str().empty() || name_is_defined(cast->result.str())) {
      return std::nullopt;
    }

    if (cast->kind == LirCastKind::Trunc && cast->from_type.str() == "i32" &&
        cast->to_type.str() == "i8") {
      const auto source = lower_immediate_or_name(cast->operand.str(), bir::TypeKind::I32);
      if (!source.has_value() || source->kind != bir::Value::Kind::Immediate ||
          !immediate_fits_type(source->immediate, bir::TypeKind::I8)) {
        return std::nullopt;
      }
      defined_names.push_back(cast->result.str());
      resolved_values.push_back(
          bir::Value::immediate_i8(static_cast<std::int8_t>(source->immediate)));
      continue;
    }

    if ((cast->kind != LirCastKind::SExt && cast->kind != LirCastKind::ZExt) ||
        cast->from_type.str() != "i8" || cast->to_type.str() != "i32") {
      return std::nullopt;
    }

    const auto source = lower_immediate_or_name(cast->operand.str(), bir::TypeKind::I8);
    if (!source.has_value()) {
      return std::nullopt;
    }
    const auto resolved_source = resolve_value(*source);
    if (!resolved_source.has_value()) {
      return std::nullopt;
    }

    defined_names.push_back(cast->result.str());
    resolved_values.push_back(*resolved_source);
  }

  const auto predicate = lower_compare_materialization_opcode(cmp->predicate.str());
  if (!predicate.has_value() || cmp->type_str.str() != "i32") {
    return std::nullopt;
  }

  const auto widened_lhs = lower_immediate_or_name(cmp->lhs.str(), bir::TypeKind::I32);
  const auto widened_rhs = lower_immediate_or_name(cmp->rhs.str(), bir::TypeKind::I32);
  if (!widened_lhs.has_value() || !widened_rhs.has_value()) {
    return std::nullopt;
  }
  const auto resolved_lhs = resolve_value(*widened_lhs);
  const auto resolved_rhs = resolve_value(*widened_rhs);
  const auto narrow_lhs = resolved_lhs.has_value() ? narrow_i8_value(*resolved_lhs) : std::nullopt;
  const auto narrow_rhs = resolved_rhs.has_value() ? narrow_i8_value(*resolved_rhs) : std::nullopt;
  if (!narrow_lhs.has_value() || !narrow_rhs.has_value()) {
    return std::nullopt;
  }

  bir::Function function;
  function.name = lir_function.name;
  function.return_type = bir::TypeKind::I8;
  function.params = params;

  bir::Block block;
  block.label = lir_block.label;
  bir::BinaryInst compare;
  compare.opcode = *predicate;
  compare.result = bir::Value::named(bir::TypeKind::I8, cond_cast->result.str());
  compare.lhs = *narrow_lhs;
  compare.rhs = *narrow_rhs;
  block.insts.push_back(compare);
  block.terminator.value = bir::Value::named(bir::TypeKind::I8, compare.result.name);

  function.blocks.push_back(std::move(block));
  return function;
}

std::optional<bir::Function> try_lower_widened_i8_conditional_return_select_function(
    const c4c::codegen::lir::LirFunction& lir_function,
    const std::vector<bir::Param>& params) {
  using namespace c4c::codegen::lir;

  if (lir_function.blocks.size() != 3) {
    return std::nullopt;
  }
  if (!std::all_of(params.begin(), params.end(), [](const bir::Param& param) {
        return param.type == bir::TypeKind::I8;
      })) {
    return std::nullopt;
  }

  const auto& entry = lir_function.blocks[0];
  const auto& true_block = lir_function.blocks[1];
  const auto& false_block = lir_function.blocks[2];
  if (entry.label.empty() || entry.insts.size() < 4 || true_block.label.empty() ||
      false_block.label.empty()) {
    return std::nullopt;
  }

  const auto* cmp0 = std::get_if<LirCmpOp>(&entry.insts[entry.insts.size() - 3]);
  const auto* cond_cast = std::get_if<LirCastOp>(&entry.insts[entry.insts.size() - 2]);
  const auto* cmp1 = std::get_if<LirCmpOp>(&entry.insts[entry.insts.size() - 1]);
  const auto* condbr = std::get_if<LirCondBr>(&entry.terminator);
  if (cmp0 == nullptr || cond_cast == nullptr || cmp1 == nullptr || condbr == nullptr ||
      cmp0->is_float || cmp0->result.str().empty() || cond_cast->result.str().empty() ||
      cmp1->is_float || cmp1->result.str().empty() ||
      cond_cast->kind != LirCastKind::ZExt || cond_cast->from_type.str() != "i1" ||
      cond_cast->to_type.str() != "i32" || cond_cast->operand.str() != cmp0->result.str() ||
      cmp1->predicate.str() != "ne" || cmp1->type_str.str() != "i32" ||
      cmp1->lhs.str() != cond_cast->result.str() || cmp1->rhs.str() != "0" ||
      condbr->cond_name != cmp1->result.str() || condbr->true_label != true_block.label ||
      condbr->false_label != false_block.label) {
    return std::nullopt;
  }

  std::vector<std::string> defined_names;
  defined_names.reserve(params.size() + entry.insts.size());
  std::vector<bir::Value> resolved_values;
  resolved_values.reserve(params.size() + entry.insts.size());
  for (const auto& param : params) {
    defined_names.push_back(param.name);
    resolved_values.push_back(bir::Value::named(param.type, param.name));
  }

  auto name_is_defined = [&](std::string_view name) {
    return std::find(defined_names.begin(), defined_names.end(), name) !=
           defined_names.end();
  };
  auto resolve_value = [&](const bir::Value& value) -> std::optional<bir::Value> {
    if (value.kind == bir::Value::Kind::Immediate) {
      return value;
    }
    for (std::size_t index = 0; index < defined_names.size(); ++index) {
      if (defined_names[index] == value.name) {
        return resolved_values[index];
      }
    }
    return std::nullopt;
  };
  auto narrow_i8_value = [&](const bir::Value& value) -> std::optional<bir::Value> {
    if (value.kind == bir::Value::Kind::Immediate) {
      if (!immediate_fits_type(value.immediate, bir::TypeKind::I8)) {
        return std::nullopt;
      }
      return bir::Value::immediate_i8(static_cast<std::int8_t>(value.immediate));
    }
    if (value.type != bir::TypeKind::I8) {
      return std::nullopt;
    }
    return value;
  };

  for (std::size_t inst_index = 0; inst_index + 3 < entry.insts.size(); ++inst_index) {
    const auto* cast = std::get_if<LirCastOp>(&entry.insts[inst_index]);
    if (cast == nullptr || cast->result.str().empty() || name_is_defined(cast->result.str()) ||
        (cast->kind != LirCastKind::SExt && cast->kind != LirCastKind::ZExt) ||
        cast->from_type.str() != "i8" || cast->to_type.str() != "i32") {
      return std::nullopt;
    }

    const auto source = lower_immediate_or_name(cast->operand.str(), bir::TypeKind::I8);
    if (!source.has_value()) {
      return std::nullopt;
    }
    const auto resolved_source = resolve_value(*source);
    if (!resolved_source.has_value()) {
      return std::nullopt;
    }

    defined_names.push_back(cast->result.str());
    resolved_values.push_back(*resolved_source);
  }

  const auto predicate = lower_compare_materialization_opcode(cmp0->predicate.str());
  if (!predicate.has_value() || cmp0->type_str.str() != "i32") {
    return std::nullopt;
  }

  const auto widened_lhs = lower_immediate_or_name(cmp0->lhs.str(), bir::TypeKind::I32);
  const auto widened_rhs = lower_immediate_or_name(cmp0->rhs.str(), bir::TypeKind::I32);
  if (!widened_lhs.has_value() || !widened_rhs.has_value()) {
    return std::nullopt;
  }

  const auto resolved_lhs = resolve_value(*widened_lhs);
  const auto resolved_rhs = resolve_value(*widened_rhs);
  const auto lhs = resolved_lhs.has_value() ? narrow_i8_value(*resolved_lhs) : std::nullopt;
  const auto rhs = resolved_rhs.has_value() ? narrow_i8_value(*resolved_rhs) : std::nullopt;
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }

  auto lower_branch_value = [&](const LirBlock& block) -> std::optional<bir::Value> {
    const auto* ret = std::get_if<LirRet>(&block.terminator);
    if (ret == nullptr || !ret->value_str.has_value() || ret->type_str != "i8") {
      return std::nullopt;
    }
    if (block.insts.empty()) {
      const auto value = lower_immediate_or_name(*ret->value_str, bir::TypeKind::I8);
      if (!value.has_value()) {
        return std::nullopt;
      }
      return resolve_value(*value);
    }
    if (block.insts.size() != 1) {
      return std::nullopt;
    }

    const auto* trunc = std::get_if<LirCastOp>(&block.insts.front());
    if (trunc == nullptr || trunc->result.str().empty() || *ret->value_str != trunc->result.str() ||
        trunc->kind != LirCastKind::Trunc || trunc->from_type.str() != "i32" ||
        trunc->to_type.str() != "i8") {
      return std::nullopt;
    }

    const auto widened_value = lower_immediate_or_name(trunc->operand.str(), bir::TypeKind::I32);
    if (!widened_value.has_value()) {
      return std::nullopt;
    }
    const auto resolved_value = resolve_value(*widened_value);
    if (!resolved_value.has_value()) {
      return std::nullopt;
    }
    return narrow_i8_value(*resolved_value);
  };

  const auto true_value = lower_branch_value(true_block);
  const auto false_value = lower_branch_value(false_block);
  if (!true_value.has_value() || !false_value.has_value()) {
    return std::nullopt;
  }

  auto operand_is_param_or_immediate = [&](const bir::Value& value) {
    if (value.kind == bir::Value::Kind::Immediate) {
      return true;
    }
    return std::any_of(params.begin(), params.end(), [&](const bir::Param& param) {
      return param.name == value.name;
    });
  };
  if (!operand_is_param_or_immediate(*lhs) || !operand_is_param_or_immediate(*rhs) ||
      !operand_is_param_or_immediate(*true_value) ||
      !operand_is_param_or_immediate(*false_value)) {
    return std::nullopt;
  }

  bir::Function function;
  function.name = lir_function.name;
  function.return_type = bir::TypeKind::I8;
  function.params = params;

  bir::Block block;
  block.label = entry.label;
  block.insts.push_back(bir::SelectInst{
      *predicate,
      bir::Value::named(bir::TypeKind::I8, "%t.select"),
      *lhs,
      *rhs,
      *true_value,
      *false_value,
  });
  block.terminator.value = bir::Value::named(bir::TypeKind::I8, "%t.select");
  function.blocks.push_back(std::move(block));
  return function;
}

std::optional<bir::Function> try_lower_widened_i8_conditional_phi_select_function(
    const c4c::codegen::lir::LirFunction& lir_function,
    const std::vector<bir::Param>& params) {
  using namespace c4c::codegen::lir;

  if (lir_function.blocks.size() != 4 && lir_function.blocks.size() != 6) {
    return std::nullopt;
  }
  if (!std::all_of(params.begin(), params.end(), [](const bir::Param& param) {
        return param.type == bir::TypeKind::I8;
      })) {
    return std::nullopt;
  }

  const auto& entry = lir_function.blocks[0];
  if (entry.label.empty() || entry.insts.size() < 4) {
    return std::nullopt;
  }

  const auto* cmp0 = std::get_if<LirCmpOp>(&entry.insts[entry.insts.size() - 3]);
  const auto* cond_cast = std::get_if<LirCastOp>(&entry.insts[entry.insts.size() - 2]);
  const auto* cmp1 = std::get_if<LirCmpOp>(&entry.insts[entry.insts.size() - 1]);
  const auto* condbr = std::get_if<LirCondBr>(&entry.terminator);
  if (cmp0 == nullptr || cond_cast == nullptr || cmp1 == nullptr || condbr == nullptr ||
      cmp0->is_float || cmp0->result.str().empty() || cond_cast->result.str().empty() ||
      cmp1->is_float || cmp1->result.str().empty() ||
      cond_cast->kind != LirCastKind::ZExt || cond_cast->from_type.str() != "i1" ||
      cond_cast->to_type.str() != "i32" || cond_cast->operand.str() != cmp0->result.str() ||
      cmp1->predicate.str() != "ne" || cmp1->type_str.str() != "i32" ||
      cmp1->lhs.str() != cond_cast->result.str() || cmp1->rhs.str() != "0" ||
      condbr->cond_name != cmp1->result.str()) {
    return std::nullopt;
  }

  std::vector<std::string> defined_names;
  defined_names.reserve(params.size() + entry.insts.size());
  std::vector<bir::Value> resolved_values;
  resolved_values.reserve(params.size() + entry.insts.size());
  for (const auto& param : params) {
    defined_names.push_back(param.name);
    resolved_values.push_back(bir::Value::named(param.type, param.name));
  }

  auto name_is_defined = [&](std::string_view name) {
    return std::find(defined_names.begin(), defined_names.end(), name) !=
           defined_names.end();
  };
  auto resolve_value = [&](const bir::Value& value) -> std::optional<bir::Value> {
    if (value.kind == bir::Value::Kind::Immediate) {
      return value;
    }
    for (std::size_t index = 0; index < defined_names.size(); ++index) {
      if (defined_names[index] == value.name) {
        return resolved_values[index];
      }
    }
    return std::nullopt;
  };
  auto resolve_value_from = [&](const std::vector<std::string>& names,
                                const std::vector<bir::Value>& values,
                                const bir::Value& value) -> std::optional<bir::Value> {
    if (value.kind == bir::Value::Kind::Immediate) {
      return value;
    }
    for (std::size_t index = 0; index < names.size(); ++index) {
      if (names[index] == value.name) {
        return values[index];
      }
    }
    return std::nullopt;
  };
  auto narrow_i8_value = [&](const bir::Value& value) -> std::optional<bir::Value> {
    if (value.kind == bir::Value::Kind::Immediate) {
      if (!immediate_fits_type(value.immediate, bir::TypeKind::I8)) {
        return std::nullopt;
      }
      return bir::Value::immediate_i8(static_cast<std::int8_t>(value.immediate));
    }
    if (value.type != bir::TypeKind::I8) {
      return std::nullopt;
    }
    return value;
  };

  for (std::size_t inst_index = 0; inst_index + 3 < entry.insts.size(); ++inst_index) {
    const auto* cast = std::get_if<LirCastOp>(&entry.insts[inst_index]);
    if (cast == nullptr || cast->result.str().empty() || name_is_defined(cast->result.str()) ||
        (cast->kind != LirCastKind::SExt && cast->kind != LirCastKind::ZExt) ||
        cast->from_type.str() != "i8" || cast->to_type.str() != "i32") {
      return std::nullopt;
    }

    const auto source = lower_immediate_or_name(cast->operand.str(), bir::TypeKind::I8);
    if (!source.has_value()) {
      return std::nullopt;
    }
    const auto resolved_source = resolve_value(*source);
    if (!resolved_source.has_value()) {
      return std::nullopt;
    }

    defined_names.push_back(cast->result.str());
    resolved_values.push_back(*resolved_source);
  }

  const auto predicate = lower_compare_materialization_opcode(cmp0->predicate.str());
  if (!predicate.has_value() || cmp0->type_str.str() != "i32") {
    return std::nullopt;
  }

  const auto widened_lhs = lower_immediate_or_name(cmp0->lhs.str(), bir::TypeKind::I32);
  const auto widened_rhs = lower_immediate_or_name(cmp0->rhs.str(), bir::TypeKind::I32);
  if (!widened_lhs.has_value() || !widened_rhs.has_value()) {
    return std::nullopt;
  }
  const auto resolved_lhs = resolve_value(*widened_lhs);
  const auto resolved_rhs = resolve_value(*widened_rhs);
  const auto lhs = resolved_lhs.has_value() ? narrow_i8_value(*resolved_lhs) : std::nullopt;
  const auto rhs = resolved_rhs.has_value() ? narrow_i8_value(*resolved_rhs) : std::nullopt;
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }

  const auto& true_block = lir_function.blocks[1];
  const auto& false_block = lir_function.blocks[lir_function.blocks.size() == 4 ? 2 : 3];
  const auto* true_br = std::get_if<LirBr>(&true_block.terminator);
  const auto* false_br = std::get_if<LirBr>(&false_block.terminator);
  if (true_br == nullptr || false_br == nullptr || condbr->true_label != true_block.label ||
      condbr->false_label != false_block.label) {
    return std::nullopt;
  }

  const LirBlock* true_phi_pred = &true_block;
  const LirBlock* false_phi_pred = &false_block;
  const LirBlock* true_value_block = &true_block;
  const LirBlock* false_value_block = &false_block;
  const LirBlock* join_block = nullptr;
  if (lir_function.blocks.size() == 4) {
    join_block = &lir_function.blocks[3];
    if (true_br->target_label != join_block->label || false_br->target_label != join_block->label) {
      return std::nullopt;
    }
  } else {
    const auto& true_end = lir_function.blocks[2];
    const auto& false_end = lir_function.blocks[4];
    const auto* true_end_br = std::get_if<LirBr>(&true_end.terminator);
    const auto* false_end_br = std::get_if<LirBr>(&false_end.terminator);
    join_block = &lir_function.blocks[5];
    if (true_end_br == nullptr || false_end_br == nullptr ||
        true_br->target_label != true_end.label ||
        false_br->target_label != false_end.label ||
        true_end_br->target_label != join_block->label ||
        false_end_br->target_label != join_block->label) {
      return std::nullopt;
    }
    true_phi_pred = &true_end;
    false_phi_pred = &false_end;
  }

  const auto* phi = join_block->insts.size() > 0 ? std::get_if<LirPhiOp>(&join_block->insts[0]) : nullptr;
  const auto* ret = std::get_if<LirRet>(&join_block->terminator);
  if (phi == nullptr || ret == nullptr || phi->result.str().empty() ||
      phi->incoming.size() != 2 || !ret->value_str.has_value() || ret->type_str != "i8" ||
      phi->incoming[0].second != true_phi_pred->label ||
      phi->incoming[1].second != false_phi_pred->label) {
    return std::nullopt;
  }

  struct LoweredWidenedChain {
    std::vector<bir::BinaryInst> insts;
    bir::Value final_value;
  };
  auto lower_widened_chain = [&](const LirBlock& first_block,
                                 const LirBlock* second_block,
                                 std::string_view expected_result,
                                 bir::TypeKind expected_type)
      -> std::optional<LoweredWidenedChain> {
    std::vector<std::string> local_names = defined_names;
    std::vector<bir::Value> local_values = resolved_values;
    std::vector<bir::BinaryInst> lowered;

    auto name_is_defined_local = [&](std::string_view name) {
      return std::find(local_names.begin(), local_names.end(), name) != local_names.end();
    };
    auto handle_inst = [&](const LirInst& inst) -> bool {
      if (const auto* cast = std::get_if<LirCastOp>(&inst); cast != nullptr) {
        if (cast->result.str().empty() || name_is_defined_local(cast->result.str())) {
          return false;
        }
        if ((cast->kind == LirCastKind::SExt || cast->kind == LirCastKind::ZExt) &&
            cast->from_type.str() == "i8" && cast->to_type.str() == "i32") {
          const auto source = lower_immediate_or_name(cast->operand.str(), bir::TypeKind::I8);
          if (!source.has_value()) {
            return false;
          }
          const auto resolved_source = resolve_value_from(local_names, local_values, *source);
          if (!resolved_source.has_value()) {
            return false;
          }
          local_names.push_back(cast->result.str());
          local_values.push_back(*resolved_source);
          return true;
        }
        if (cast->kind == LirCastKind::Trunc && cast->from_type.str() == "i32" &&
            cast->to_type.str() == "i8") {
          const auto source = lower_immediate_or_name(cast->operand.str(), bir::TypeKind::I32);
          if (!source.has_value()) {
            return false;
          }
          const auto resolved_source = resolve_value_from(local_names, local_values, *source);
          const auto narrowed_source =
              resolved_source.has_value() ? narrow_i8_value(*resolved_source) : std::nullopt;
          if (!narrowed_source.has_value()) {
            return false;
          }
          local_names.push_back(cast->result.str());
          local_values.push_back(*narrowed_source);
          return true;
        }
        return false;
      }

      auto binary = lower_binary(inst);
      if (!binary.has_value() || binary->result.type != bir::TypeKind::I32 ||
          (binary->opcode != bir::BinaryOpcode::Add &&
           binary->opcode != bir::BinaryOpcode::Sub) ||
          name_is_defined_local(binary->result.name)) {
        return false;
      }
      const auto lhs_value = resolve_value_from(local_names, local_values, binary->lhs);
      const auto rhs_value = resolve_value_from(local_names, local_values, binary->rhs);
      const auto narrow_lhs = lhs_value.has_value() ? narrow_i8_value(*lhs_value) : std::nullopt;
      const auto narrow_rhs = rhs_value.has_value() ? narrow_i8_value(*rhs_value) : std::nullopt;
      if (!narrow_lhs.has_value() || !narrow_rhs.has_value()) {
        return false;
      }

      lowered.push_back(bir::BinaryInst{
          binary->opcode,
          bir::Value::named(bir::TypeKind::I8, binary->result.name),
          *narrow_lhs,
          *narrow_rhs,
      });
      local_names.push_back(binary->result.name);
      local_values.push_back(bir::Value::named(bir::TypeKind::I8, binary->result.name));
      return true;
    };

    for (const auto& inst : first_block.insts) {
      if (!handle_inst(inst)) {
        return std::nullopt;
      }
    }
    if (second_block != nullptr) {
      for (const auto& inst : second_block->insts) {
        if (!handle_inst(inst)) {
          return std::nullopt;
        }
      }
    }

    const auto final_value = lower_immediate_or_name(expected_result, expected_type);
    if (!final_value.has_value()) {
      return std::nullopt;
    }
    const auto resolved_value = resolve_value_from(local_names, local_values, *final_value);
    const auto narrow_value =
        resolved_value.has_value() ? narrow_i8_value(*resolved_value) : std::nullopt;
    if (!narrow_value.has_value()) {
      return std::nullopt;
    }
    return LoweredWidenedChain{std::move(lowered), *narrow_value};
  };

  const auto true_chain =
      lower_widened_chain(*true_value_block,
                          lir_function.blocks.size() == 4 ? nullptr : true_phi_pred,
                          phi->incoming[0].first,
                          phi->type_str.str() == "i8" ? bir::TypeKind::I8 : bir::TypeKind::I32);
  const auto false_chain =
      lower_widened_chain(*false_value_block,
                          lir_function.blocks.size() == 4 ? nullptr : false_phi_pred,
                          phi->incoming[1].first,
                          phi->type_str.str() == "i8" ? bir::TypeKind::I8 : bir::TypeKind::I32);
  if (!true_chain.has_value() || !false_chain.has_value()) {
    return std::nullopt;
  }

  auto operand_is_param_or_immediate = [&](const bir::Value& value) {
    if (value.kind == bir::Value::Kind::Immediate) {
      return true;
    }
    return std::any_of(params.begin(), params.end(), [&](const bir::Param& param) {
      return param.name == value.name;
    });
  };
  if (!operand_is_param_or_immediate(*lhs) || !operand_is_param_or_immediate(*rhs)) {
    return std::nullopt;
  }

  bir::Function function;
  function.name = lir_function.name;
  function.return_type = bir::TypeKind::I8;
  function.params = params;

  bir::Block block;
  block.label = entry.label;
  std::vector<std::string> available_names;
  available_names.reserve(params.size() + true_chain->insts.size() + false_chain->insts.size() +
                          join_block->insts.size());
  for (const auto& param : params) {
    available_names.push_back(param.name);
  }
  for (const auto& inst : true_chain->insts) {
    block.insts.push_back(inst);
    available_names.push_back(inst.result.name);
  }
  for (const auto& inst : false_chain->insts) {
    if (std::find(available_names.begin(), available_names.end(), inst.result.name) !=
        available_names.end()) {
      return std::nullopt;
    }
    block.insts.push_back(inst);
    available_names.push_back(inst.result.name);
  }

  block.insts.push_back(bir::SelectInst{
      *predicate,
      bir::Value::named(bir::TypeKind::I8, phi->result.str()),
      *lhs,
      *rhs,
      true_chain->final_value,
      false_chain->final_value,
  });
  available_names.push_back(phi->result.str());

  auto operand_is_available = [&](const bir::Value& value) {
    if (value.kind == bir::Value::Kind::Immediate) {
      return true;
    }
    return std::find(available_names.begin(), available_names.end(), value.name) !=
           available_names.end();
  };

  if (phi->type_str.str() == "i8" && join_block->insts.size() == 1 &&
      *ret->value_str == phi->result.str()) {
    block.terminator.value = bir::Value::named(bir::TypeKind::I8, phi->result.str());
    function.blocks.push_back(std::move(block));
    return function;
  }

  const auto* trunc =
      join_block->insts.empty() ? nullptr : std::get_if<LirCastOp>(&join_block->insts.back());
  if (phi->type_str.str() != "i32" || trunc == nullptr || trunc->result.str().empty() ||
      trunc->kind != LirCastKind::Trunc || trunc->from_type.str() != "i32" ||
      trunc->to_type.str() != "i8" || *ret->value_str != trunc->result.str()) {
    return std::nullopt;
  }

  for (std::size_t inst_index = 1; inst_index + 1 < join_block->insts.size(); ++inst_index) {
    auto binary = lower_binary(join_block->insts[inst_index]);
    if (!binary.has_value() || binary->result.type != bir::TypeKind::I32 ||
        (binary->opcode != bir::BinaryOpcode::Add &&
         binary->opcode != bir::BinaryOpcode::Sub)) {
      return std::nullopt;
    }

    const auto lhs_value =
        binary->lhs.kind == bir::Value::Kind::Immediate
            ? std::optional<bir::Value>(binary->lhs)
            : (binary->lhs.name == phi->result.str()
                   ? std::optional<bir::Value>(
                         bir::Value::named(bir::TypeKind::I8, phi->result.str()))
                   : std::nullopt);
    const auto rhs_value =
        binary->rhs.kind == bir::Value::Kind::Immediate
            ? std::optional<bir::Value>(binary->rhs)
            : (binary->rhs.name == phi->result.str()
                   ? std::optional<bir::Value>(
                         bir::Value::named(bir::TypeKind::I8, phi->result.str()))
                   : std::nullopt);
    const auto narrowed_lhs = lhs_value.has_value()
                                  ? narrow_i8_value(*lhs_value)
                                  : (operand_is_available(binary->lhs)
                                         ? std::optional<bir::Value>(
                                               bir::Value::named(bir::TypeKind::I8,
                                                                 binary->lhs.name))
                                         : std::nullopt);
    const auto narrowed_rhs = rhs_value.has_value()
                                  ? narrow_i8_value(*rhs_value)
                                  : (operand_is_available(binary->rhs)
                                         ? std::optional<bir::Value>(
                                               bir::Value::named(bir::TypeKind::I8,
                                                                 binary->rhs.name))
                                         : std::nullopt);
    if (!narrowed_lhs.has_value() || !narrowed_rhs.has_value() ||
        std::find(available_names.begin(), available_names.end(), binary->result.name) !=
            available_names.end()) {
      return std::nullopt;
    }
    block.insts.push_back(bir::BinaryInst{
        binary->opcode,
        bir::Value::named(bir::TypeKind::I8, binary->result.name),
        *narrowed_lhs,
        *narrowed_rhs,
    });
    available_names.push_back(binary->result.name);
  }

  const auto widened_return_value =
      lower_immediate_or_name(trunc->operand.str(), bir::TypeKind::I32);
  if (!widened_return_value.has_value()) {
    return std::nullopt;
  }
  bir::Value return_value;
  if (widened_return_value->kind == bir::Value::Kind::Immediate) {
    const auto narrowed_return = narrow_i8_value(*widened_return_value);
    if (!narrowed_return.has_value()) {
      return std::nullopt;
    }
    return_value = *narrowed_return;
  } else if (operand_is_available(bir::Value::named(bir::TypeKind::I8, widened_return_value->name))) {
    return_value = bir::Value::named(bir::TypeKind::I8, widened_return_value->name);
  } else {
    return std::nullopt;
  }

  block.terminator.value = return_value;
  function.blocks.push_back(std::move(block));
  return function;
}

}  // namespace

std::optional<bir::Module> try_lower_to_bir(const c4c::codegen::lir::LirModule& module) {
  if (const auto lowered = try_lower_minimal_direct_call_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered = try_lower_minimal_void_direct_call_imm_return_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered = try_lower_minimal_declared_direct_call_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered = try_lower_minimal_two_arg_direct_call_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered = try_lower_minimal_direct_call_add_imm_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered = try_lower_minimal_direct_call_identity_arg_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered = try_lower_minimal_folded_two_arg_direct_call_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered = try_lower_minimal_dual_identity_direct_call_sub_module(module);
      lowered.has_value()) {
    return lowered;
  }
  if (const auto lowered = try_lower_minimal_call_crossing_direct_call_module(module);
      lowered.has_value()) {
    return lowered;
  }

  if (!module.globals.empty() || !module.string_pool.empty() || !module.extern_decls.empty() ||
      module.functions.size() != 1) {
    return std::nullopt;
  }

  const auto& lir_function = module.functions.front();
  if (lir_function.is_declaration || !lir_function.alloca_insts.empty() ||
      lir_function.params.size() > 2) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  const auto params = lower_function_params(lir_function);
  if (!params.has_value()) {
    return std::nullopt;
  }

  if (const auto select_function =
          try_lower_conditional_return_select_function(lir_function, *params);
      select_function.has_value()) {
    lowered.functions.push_back(*select_function);
    return lowered;
  }
  if (const auto select_function =
          try_lower_conditional_phi_select_function(lir_function, *params);
      select_function.has_value()) {
    lowered.functions.push_back(*select_function);
    return lowered;
  }
  if (const auto widened_i8_select_function =
          try_lower_widened_i8_conditional_phi_select_function(lir_function, *params);
      widened_i8_select_function.has_value()) {
    lowered.functions.push_back(*widened_i8_select_function);
    return lowered;
  }
  if (const auto widened_i8_select_function =
          try_lower_widened_i8_conditional_return_select_function(lir_function, *params);
      widened_i8_select_function.has_value()) {
    lowered.functions.push_back(*widened_i8_select_function);
    return lowered;
  }
  if (const auto widened_i8_function =
          try_lower_widened_i8_compare_return_function(lir_function, *params);
      widened_i8_function.has_value()) {
    lowered.functions.push_back(*widened_i8_function);
    return lowered;
  }
  if (const auto widened_i8_function =
          try_lower_widened_i8_add_sub_chain_function(lir_function, *params);
      widened_i8_function.has_value()) {
    lowered.functions.push_back(*widened_i8_function);
    return lowered;
  }

  if (lir_function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& lir_block = lir_function.blocks.front();
  if (lir_block.label.empty()) {
    return std::nullopt;
  }

  const auto* ret = std::get_if<c4c::codegen::lir::LirRet>(&lir_block.terminator);
  if (ret == nullptr || !ret->value_str.has_value()) {
    return std::nullopt;
  }
  const auto return_type = lower_scalar_type_text(ret->type_str);
  if (!return_type.has_value()) {
    return std::nullopt;
  }

  bir::Function function;
  function.name = lir_function.name;
  function.return_type = *return_type;
  function.params = *params;

  bir::Block block;
  block.label = lir_block.label;
  std::vector<std::string> defined_names;
  defined_names.reserve(function.params.size() + lir_block.insts.size());
  std::vector<bir::Value> resolved_values;
  resolved_values.reserve(function.params.size() + lir_block.insts.size());
  for (const auto& param : function.params) {
    defined_names.push_back(param.name);
    resolved_values.push_back(bir::Value::named(param.type, param.name));
  }
  std::vector<AffineValue> affine_values;
  affine_values.reserve(function.params.size() + lir_block.insts.size());
  for (std::size_t index = 0; index < function.params.size(); ++index) {
    affine_values.push_back(
        AffineValue{index == 0, index == 1, 0});
  }
  auto resolve_value = [&](const bir::Value& value) -> std::optional<bir::Value> {
    if (value.kind == bir::Value::Kind::Immediate) {
      return value;
    }
    for (std::size_t index = 0; index < defined_names.size(); ++index) {
      if (defined_names[index] == value.name) {
        return resolved_values[index];
      }
    }
    return std::nullopt;
  };
  for (std::size_t inst_index = 0; inst_index < lir_block.insts.size(); ++inst_index) {
    auto name_is_defined = [&](std::string_view name) {
      return std::find(defined_names.begin(), defined_names.end(), name) !=
             defined_names.end();
    };
    auto operand_is_available = [&](const bir::Value& value) {
      return value.kind != bir::Value::Kind::Named || name_is_defined(value.name);
    };
    auto lower_affine_value = [&](const bir::Value& value) -> std::optional<AffineValue> {
      if (value.kind == bir::Value::Kind::Immediate) {
        return AffineValue{false, false, value.immediate};
      }
      for (std::size_t index = 0; index < defined_names.size(); ++index) {
        if (defined_names[index] == value.name) {
          return affine_values[index];
        }
      }
      return std::nullopt;
    };

    if (const auto* cast = std::get_if<c4c::codegen::lir::LirCastOp>(&lir_block.insts[inst_index]);
        cast != nullptr) {
      const auto immediate_cast = lower_lossless_immediate_cast(lir_block.insts[inst_index]);
      if (immediate_cast.has_value() && !name_is_defined(cast->result.str())) {
        defined_names.push_back(cast->result.str());
        resolved_values.push_back(*immediate_cast);
        affine_values.push_back(AffineValue{false, false, immediate_cast->immediate});
        continue;
      }
      auto cast_inst = lower_cast(lir_block.insts[inst_index]);
      if (cast_inst.has_value() && !name_is_defined(cast->result.str())) {
        const auto resolved_operand = resolve_value(cast_inst->operand);
        if (!resolved_operand.has_value()) {
          return std::nullopt;
        }
        cast_inst->operand = *resolved_operand;
        const auto operand_affine = lower_affine_value(cast_inst->operand);
        if (!operand_affine.has_value()) {
          return std::nullopt;
        }
        defined_names.push_back(cast->result.str());
        resolved_values.push_back(bir::Value::named(cast_inst->result.type, cast->result.str()));
        affine_values.push_back(*operand_affine);
        block.insts.push_back(*cast_inst);
        continue;
      }
      return std::nullopt;
    }

    auto binary = [&]() -> std::optional<bir::BinaryInst> {
      if (inst_index + 1 < lir_block.insts.size()) {
        auto lowered_compare = lower_compare_materialization(
            lir_block.insts[inst_index], lir_block.insts[inst_index + 1]);
        if (lowered_compare.has_value()) {
          ++inst_index;
          return lowered_compare;
        }
      }
      return lower_binary(lir_block.insts[inst_index]);
    }();
    if (binary.has_value()) {
      const auto lhs_value = resolve_value(binary->lhs);
      const auto rhs_value = resolve_value(binary->rhs);
      if (!lhs_value.has_value() || !rhs_value.has_value() ||
          name_is_defined(binary->result.name)) {
        return std::nullopt;
      }
      binary->lhs = *lhs_value;
      binary->rhs = *rhs_value;
      const auto lhs = lower_affine_value(binary->lhs);
      const auto rhs = lower_affine_value(binary->rhs);
      if (!lhs.has_value() || !rhs.has_value()) {
        return std::nullopt;
      }
      const auto combined = combine_affine(*lhs, *rhs, binary->opcode);
      if (!combined.has_value()) {
        return std::nullopt;
      }
      defined_names.push_back(binary->result.name);
      resolved_values.push_back(bir::Value::named(binary->result.type, binary->result.name));
      affine_values.push_back(*combined);
      block.insts.push_back(*binary);
      continue;
    }

    if (inst_index + 1 >= lir_block.insts.size()) {
      return std::nullopt;
    }
    auto select = lower_select_materialization(
        lir_block.insts[inst_index], lir_block.insts[inst_index + 1]);
    if (!select.has_value()) {
      return std::nullopt;
    }
    ++inst_index;
    const auto lhs_value = resolve_value(select->lhs);
    const auto rhs_value = resolve_value(select->rhs);
    const auto true_value_resolved = resolve_value(select->true_value);
    const auto false_value_resolved = resolve_value(select->false_value);
    if (!lhs_value.has_value() || !rhs_value.has_value() ||
        !true_value_resolved.has_value() || !false_value_resolved.has_value() ||
        name_is_defined(select->result.name)) {
      return std::nullopt;
    }
    select->lhs = *lhs_value;
    select->rhs = *rhs_value;
    select->true_value = *true_value_resolved;
    select->false_value = *false_value_resolved;
    const auto lhs = lower_affine_value(select->lhs);
    const auto rhs = lower_affine_value(select->rhs);
    const auto true_value = lower_affine_value(select->true_value);
    const auto false_value = lower_affine_value(select->false_value);
    if (!lhs.has_value() || !rhs.has_value() || !true_value.has_value() ||
        !false_value.has_value()) {
      return std::nullopt;
    }
    const auto predicate = evaluate_predicate(*lhs, *rhs, select->predicate);
    if (!predicate.has_value()) {
      return std::nullopt;
    }
    defined_names.push_back(select->result.name);
    resolved_values.push_back(bir::Value::named(select->result.type, select->result.name));
    affine_values.push_back(*predicate ? *true_value : *false_value);
    block.insts.push_back(*select);
  }

  auto return_value = lower_immediate_or_name(*ret->value_str, function.return_type);
  if (!return_value.has_value()) {
    return std::nullopt;
  }
  const auto resolved_return_value = resolve_value(*return_value);
  if (!resolved_return_value.has_value()) {
    return std::nullopt;
  }
  block.terminator.value = *resolved_return_value;

  function.blocks.push_back(std::move(block));
  lowered.functions.push_back(std::move(function));
  return lowered;
}

bir::Module lower_to_bir(const c4c::codegen::lir::LirModule& module) {
  auto lowered = try_lower_to_bir(module);
  if (!lowered.has_value()) {
    throw std::invalid_argument(
        "bir scaffold lowering currently supports only straight-line single-block i8/i32/i64 return-immediate/add-sub slices, sext/zext/trunc casts, constant-only mul/and/or/shl/lshr/ashr/sdiv/udiv/srem/urem/eq/ne/slt/sle/sgt/sge/ult/ule/ugt/uge materialization slices, bounded compare-fed integer select materialization, bounded compare-fed phi joins with empty or add/sub-only predecessor arms including join-local add/sub chains after the fused select, plus bounded one- and two-parameter affine chains over those scalar types");
  }
  return *lowered;
}

}  // namespace c4c::backend
