#include "lir_adapter.hpp"

#include <cctype>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace c4c::backend {

namespace {

std::string trim(std::string text) {
  size_t start = 0;
  while (start < text.size() &&
         std::isspace(static_cast<unsigned char>(text[start]))) {
    ++start;
  }
  size_t end = text.size();
  while (end > start &&
         std::isspace(static_cast<unsigned char>(text[end - 1]))) {
    --end;
  }
  return text.substr(start, end - start);
}

std::vector<std::string> split_top_level(const std::string& text, char delim) {
  std::vector<std::string> parts;
  size_t start = 0;
  int paren_depth = 0;
  int square_depth = 0;
  int brace_depth = 0;
  int angle_depth = 0;
  for (size_t i = 0; i < text.size(); ++i) {
    switch (text[i]) {
      case '(':
        ++paren_depth;
        break;
      case ')':
        --paren_depth;
        break;
      case '[':
        ++square_depth;
        break;
      case ']':
        --square_depth;
        break;
      case '{':
        ++brace_depth;
        break;
      case '}':
        --brace_depth;
        break;
      case '<':
        ++angle_depth;
        break;
      case '>':
        --angle_depth;
        break;
      default:
        break;
    }
    if (text[i] == delim && paren_depth == 0 && square_depth == 0 &&
        brace_depth == 0 && angle_depth == 0) {
      parts.push_back(trim(text.substr(start, i - start)));
      start = i + 1;
    }
  }
  parts.push_back(trim(text.substr(start)));
  return parts;
}

[[noreturn]] void fail_unsupported(const std::string& detail) {
  throw LirAdapterError(LirAdapterErrorKind::Unsupported,
                        "minimal backend LIR adapter does not support " + detail);
}

[[noreturn]] void fail_bad_signature(const std::string& signature_text) {
  throw LirAdapterError(LirAdapterErrorKind::Malformed,
                        "minimal backend LIR adapter could not parse signature '" +
                            signature_text + "'");
}

BackendFunctionSignature parse_signature(const std::string& signature_text) {
  const std::string line = trim(signature_text);
  const size_t first_space = line.find(' ');
  const size_t at_pos = line.find('@');
  const size_t open_paren = line.find('(', at_pos == std::string::npos ? 0 : at_pos);
  const size_t close_paren = line.rfind(')');
  if (first_space == std::string::npos || at_pos == std::string::npos ||
      open_paren == std::string::npos || close_paren == std::string::npos ||
      first_space >= at_pos || open_paren >= close_paren) {
    fail_bad_signature(signature_text);
  }

  BackendFunctionSignature signature;
  signature.linkage = trim(line.substr(0, first_space));
  signature.return_type = trim(line.substr(first_space + 1, at_pos - first_space - 1));
  signature.name = line.substr(at_pos + 1, open_paren - at_pos - 1);
  if (signature.linkage.empty() || signature.return_type.empty() ||
      signature.name.empty()) {
    fail_bad_signature(signature_text);
  }

  const std::string params_text =
      trim(line.substr(open_paren + 1, close_paren - open_paren - 1));
  if (!params_text.empty()) {
    for (const std::string& part : split_top_level(params_text, ',')) {
      if (part.empty()) continue;
      if (part == "...") {
        signature.is_vararg = true;
        continue;
      }
      BackendParam param;
      const size_t last_space = part.rfind(' ');
      if (last_space == std::string::npos) {
        param.type_str = part;
      } else {
        param.type_str = trim(part.substr(0, last_space));
        param.name = trim(part.substr(last_space + 1));
      }
      if (param.type_str.empty()) fail_bad_signature(signature_text);
      signature.params.push_back(std::move(param));
    }
  }

  return signature;
}

void render_signature(std::ostringstream& out,
                      const BackendFunctionSignature& signature) {
  out << signature.linkage << " " << signature.return_type << " @"
      << signature.name << "(";
  bool first = true;
  for (const auto& param : signature.params) {
    if (!first) out << ", ";
    first = false;
    out << param.type_str;
    if (!param.name.empty()) out << " " << param.name;
  }
  if (signature.is_vararg) {
    if (!first) out << ", ";
    out << "...";
  }
  out << ")\n";
}

BackendInst adapt_inst(const c4c::codegen::lir::LirInst& inst) {
  if (const auto* bin = std::get_if<c4c::codegen::lir::LirBinOp>(&inst)) {
    BackendBinaryOpcode opcode;
    if (bin->opcode == "add") {
      opcode = BackendBinaryOpcode::Add;
    } else if (bin->opcode == "sub") {
      opcode = BackendBinaryOpcode::Sub;
    } else {
      fail_unsupported("binary opcode '" + bin->opcode + "'");
    }
    BackendBinaryInst out;
    out.opcode = opcode;
    out.result = bin->result;
    out.type_str = bin->type_str;
    out.lhs = bin->lhs;
    out.rhs = bin->rhs;
    return out;
  }

  if (const auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&inst)) {
    BackendCallInst out;
    out.result = call->result;
    out.return_type = call->return_type;
    out.callee = call->callee;
    out.callee_type_suffix = call->callee_type_suffix;
    out.args_str = call->args_str;
    return out;
  }

  fail_unsupported("non-binary/non-call instructions");
}

BackendReturn adapt_terminator(const c4c::codegen::lir::LirTerminator& terminator) {
  const auto* ret = std::get_if<c4c::codegen::lir::LirRet>(&terminator);
  if (!ret) fail_unsupported("non-return terminators");
  return BackendReturn{ret->value_str, ret->type_str};
}

std::optional<BackendFunction> adapt_single_param_add_function(
    const c4c::codegen::lir::LirFunction& function,
    const BackendFunctionSignature& signature) {
  using namespace c4c::codegen::lir;

  if (function.is_declaration || signature.linkage != "define" ||
      signature.return_type != "i32" || signature.name == "main" ||
      signature.params.size() != 1 ||
      signature.params.front().type_str != "i32" ||
      signature.params.front().name.empty() || signature.is_vararg ||
      function.blocks.size() != 1 || function.alloca_insts.size() != 2 ||
      !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* alloca =
      std::get_if<LirAllocaOp>(&function.alloca_insts.front());
  const auto* store_param =
      std::get_if<LirStoreOp>(&function.alloca_insts.back());
  if (alloca == nullptr || store_param == nullptr || alloca->type_str != "i32" ||
      alloca->result.empty() || store_param->type_str != "i32" ||
      store_param->val != signature.params.front().name ||
      store_param->ptr != alloca->result) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  const auto* ret = std::get_if<LirRet>(&block.terminator);
  if (ret == nullptr) {
    return std::nullopt;
  }
  if (block.label != "entry" || !ret->value_str.has_value() ||
      ret->type_str != "i32") {
    return std::nullopt;
  }

  const auto match_load_add_ret =
      [&](const LirLoadOp& load, const LirBinOp& add, std::string_view ret_value)
      -> std::optional<BackendFunction> {
    if (load.type_str != "i32" || load.ptr != alloca->result ||
        add.opcode != "add" || add.type_str != "i32" || add.lhs != load.result ||
        ret_value != add.result) {
      return std::nullopt;
    }

    BackendFunction out;
    out.signature = signature;
    BackendBlock out_block;
    out_block.label = block.label;
    out_block.insts.push_back(
        BackendBinaryInst{BackendBinaryOpcode::Add, add.result, add.type_str,
                          signature.params.front().name, add.rhs});
    out_block.terminator = BackendReturn{add.result, "i32"};
    out.blocks.push_back(std::move(out_block));
    return out;
  };

  if (block.insts.size() == 2) {
    const auto* load = std::get_if<LirLoadOp>(&block.insts[0]);
    const auto* add = std::get_if<LirBinOp>(&block.insts[1]);
    if (load == nullptr || add == nullptr) {
      return std::nullopt;
    }
    return match_load_add_ret(*load, *add, *ret->value_str);
  }

  if (block.insts.size() == 4) {
    const auto* load0 = std::get_if<LirLoadOp>(&block.insts[0]);
    const auto* add = std::get_if<LirBinOp>(&block.insts[1]);
    const auto* store = std::get_if<LirStoreOp>(&block.insts[2]);
    const auto* load1 = std::get_if<LirLoadOp>(&block.insts[3]);
    if (load0 == nullptr || add == nullptr || store == nullptr || load1 == nullptr ||
        store->type_str != "i32" || store->val != add->result ||
        store->ptr != alloca->result || load1->type_str != "i32" ||
        load1->ptr != alloca->result || *ret->value_str != load1->result) {
      return std::nullopt;
    }
    return match_load_add_ret(*load0, *add, store->val);
  }

  return std::nullopt;
}

std::optional<BackendFunction> adapt_local_temp_return_function(
    const c4c::codegen::lir::LirFunction& function,
    const BackendFunctionSignature& signature) {
  using namespace c4c::codegen::lir;

  if (function.is_declaration || signature.linkage != "define" ||
      signature.return_type != "i32" || signature.name != "main" ||
      !signature.params.empty() || signature.is_vararg ||
      function.blocks.size() != 1 || function.alloca_insts.empty() ||
      !function.stack_objects.empty()) {
    return std::nullopt;
  }

  std::vector<std::string> scalar_allocas;
  scalar_allocas.reserve(function.alloca_insts.size());
  for (const auto& inst : function.alloca_insts) {
    const auto* alloca = std::get_if<LirAllocaOp>(&inst);
    if (alloca == nullptr || alloca->type_str != "i32" || alloca->result.empty()) {
      return std::nullopt;
    }
    scalar_allocas.push_back(alloca->result);
  }

  const auto& block = function.blocks.front();
  const auto* ret = std::get_if<LirRet>(&block.terminator);
  if (block.label != "entry" || ret == nullptr || !ret->value_str.has_value() ||
      ret->type_str != "i32") {
    return std::nullopt;
  }

  std::unordered_map<std::string, std::string> last_store;
  if (function.alloca_insts.size() == 2 && std::holds_alternative<LirStoreOp>(function.alloca_insts[1])) {
    const auto* store = std::get_if<LirStoreOp>(&function.alloca_insts[1]);
    if (store == nullptr || store->type_str != "i32") {
      return std::nullopt;
    }
    last_store.emplace(store->ptr, store->val);
  }

  const LirLoadOp* final_load = nullptr;
  if (!block.insts.empty()) {
    final_load = std::get_if<LirLoadOp>(&block.insts.back());
  }
  if (final_load == nullptr || final_load->type_str != "i32" ||
      !ret->value_str.has_value() || *ret->value_str != final_load->result) {
    return std::nullopt;
  }

  for (std::size_t i = 0; i + 1 < block.insts.size(); ++i) {
    const auto* store = std::get_if<LirStoreOp>(&block.insts[i]);
    if (store == nullptr || store->type_str != "i32") {
      return std::nullopt;
    }
    last_store[store->ptr] = store->val;
  }

  const auto store_it = last_store.find(final_load->ptr);
  if (store_it == last_store.end()) {
    return std::nullopt;
  }

  BackendFunction out;
  out.signature = signature;
  BackendBlock out_block;
  out_block.label = block.label;
  out_block.terminator = BackendReturn{store_it->second, "i32"};
  out.blocks.push_back(std::move(out_block));
  return out;
}

std::optional<BackendFunction> adapt_local_temp_sub_return_function(
    const c4c::codegen::lir::LirFunction& function,
    const BackendFunctionSignature& signature) {
  using namespace c4c::codegen::lir;

  if (function.is_declaration || signature.linkage != "define" ||
      signature.return_type != "i32" || signature.name != "main" ||
      !signature.params.empty() || signature.is_vararg || function.blocks.size() != 1 ||
      function.alloca_insts.size() != 1 || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* alloca = std::get_if<LirAllocaOp>(&function.alloca_insts.front());
  if (alloca == nullptr || alloca->type_str != "i32" || alloca->result.empty()) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  const auto* ret = std::get_if<LirRet>(&block.terminator);
  if (block.label != "entry" || ret == nullptr || !ret->value_str.has_value() ||
      ret->type_str != "i32" || block.insts.size() != 3) {
    return std::nullopt;
  }

  const auto* store = std::get_if<LirStoreOp>(&block.insts[0]);
  const auto* load = std::get_if<LirLoadOp>(&block.insts[1]);
  const auto* sub = std::get_if<LirBinOp>(&block.insts[2]);
  if (store == nullptr || load == nullptr || sub == nullptr || store->type_str != "i32" ||
      store->ptr != alloca->result || load->type_str != "i32" ||
      load->ptr != alloca->result || sub->opcode != "sub" || sub->type_str != "i32" ||
      sub->lhs != load->result || *ret->value_str != sub->result) {
    return std::nullopt;
  }

  BackendFunction out;
  out.signature = signature;
  BackendBlock out_block;
  out_block.label = block.label;
  out_block.insts.push_back(
      BackendBinaryInst{BackendBinaryOpcode::Sub, sub->result, sub->type_str, store->val, sub->rhs});
  out_block.terminator = BackendReturn{sub->result, "i32"};
  out.blocks.push_back(std::move(out_block));
  return out;
}

std::optional<BackendFunction> adapt_local_pointer_temp_return_function(
    const c4c::codegen::lir::LirFunction& function,
    const BackendFunctionSignature& signature) {
  using namespace c4c::codegen::lir;

  if (function.is_declaration || signature.linkage != "define" ||
      signature.return_type != "i32" || signature.name != "main" ||
      !signature.params.empty() || signature.is_vararg || function.blocks.size() != 1 ||
      function.alloca_insts.size() != 2 || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* scalar_alloca = std::get_if<LirAllocaOp>(&function.alloca_insts[0]);
  const auto* ptr_alloca = std::get_if<LirAllocaOp>(&function.alloca_insts[1]);
  if (scalar_alloca == nullptr || ptr_alloca == nullptr ||
      scalar_alloca->type_str != "i32" || scalar_alloca->result.empty() ||
      ptr_alloca->type_str != "ptr" || ptr_alloca->result.empty()) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  const auto* ret = std::get_if<LirRet>(&block.terminator);
  if (block.label != "entry" || ret == nullptr || !ret->value_str.has_value() ||
      ret->type_str != "i32") {
    return std::nullopt;
  }

  std::unordered_map<std::string, std::string> scalar_values;
  std::unordered_map<std::string, std::string> pointer_targets;
  std::unordered_map<std::string, std::string> loaded_values;

  auto resolve_scalar_ptr = [&](std::string_view ptr) -> std::optional<std::string> {
    if (ptr == scalar_alloca->result) {
      return scalar_alloca->result;
    }
    const auto it = pointer_targets.find(std::string(ptr));
    if (it == pointer_targets.end()) {
      return std::nullopt;
    }
    return it->second;
  };

  for (const auto& inst : block.insts) {
    if (const auto* store = std::get_if<LirStoreOp>(&inst)) {
      if (store->type_str == "i32") {
        const auto target = resolve_scalar_ptr(store->ptr);
        if (!target.has_value()) {
          return std::nullopt;
        }
        scalar_values[*target] = store->val;
        continue;
      }

      if (store->type_str == "ptr" && store->ptr == ptr_alloca->result &&
          store->val == scalar_alloca->result) {
        pointer_targets[ptr_alloca->result] = scalar_alloca->result;
        continue;
      }

      return std::nullopt;
    }

    if (const auto* load = std::get_if<LirLoadOp>(&inst)) {
      if (load->type_str == "ptr") {
        const auto it = pointer_targets.find(load->ptr);
        if (it == pointer_targets.end()) {
          return std::nullopt;
        }
        pointer_targets[load->result] = it->second;
        continue;
      }

      if (load->type_str == "i32") {
        const auto target = resolve_scalar_ptr(load->ptr);
        if (!target.has_value()) {
          return std::nullopt;
        }
        const auto value_it = scalar_values.find(*target);
        if (value_it == scalar_values.end()) {
          return std::nullopt;
        }
        loaded_values[load->result] = value_it->second;
        continue;
      }

      return std::nullopt;
    }

    return std::nullopt;
  }

  const auto value_it = loaded_values.find(*ret->value_str);
  if (value_it == loaded_values.end()) {
    return std::nullopt;
  }

  BackendFunction out;
  out.signature = signature;
  BackendBlock out_block;
  out_block.label = block.label;
  out_block.terminator = BackendReturn{value_it->second, "i32"};
  out.blocks.push_back(std::move(out_block));
  return out;
}

std::optional<BackendFunction> adapt_local_single_arg_call_function(
    const c4c::codegen::lir::LirFunction& function,
    const BackendFunctionSignature& signature) {
  using namespace c4c::codegen::lir;

  if (function.is_declaration || signature.linkage != "define" ||
      signature.return_type != "i32" || signature.name != "main" ||
      !signature.params.empty() || signature.is_vararg ||
      function.blocks.size() != 1 || function.alloca_insts.size() != 1 ||
      !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* alloca = std::get_if<LirAllocaOp>(&function.alloca_insts.front());
  if (alloca == nullptr || alloca->type_str != "i32" || alloca->result.empty()) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  const auto* ret = std::get_if<LirRet>(&block.terminator);
  if (ret == nullptr || block.label != "entry" || !ret->value_str.has_value() ||
      ret->type_str != "i32" || block.insts.size() != 3) {
    return std::nullopt;
  }

  const auto* store = std::get_if<LirStoreOp>(&block.insts[0]);
  const auto* load = std::get_if<LirLoadOp>(&block.insts[1]);
  const auto* call = std::get_if<LirCallOp>(&block.insts[2]);
  if (store == nullptr || load == nullptr || call == nullptr ||
      store->type_str != "i32" || store->ptr != alloca->result ||
      load->type_str != "i32" ||
      load->ptr != alloca->result || call->result.empty() ||
      *ret->value_str != call->result || call->return_type != "i32" ||
      call->callee_type_suffix != "(i32)" || call->args_str != ("i32 " + load->result)) {
    return std::nullopt;
  }

  BackendFunction out;
  out.signature = signature;
  BackendBlock out_block;
  out_block.label = block.label;
  out_block.insts.push_back(BackendCallInst{call->result,
                                            call->return_type,
                                            call->callee,
                                            call->callee_type_suffix,
                                            "i32 " + store->val});
  out_block.terminator = BackendReturn{call->result, "i32"};
  out.blocks.push_back(std::move(out_block));
  return out;
}

std::optional<BackendFunction> adapt_local_two_arg_call_function(
    const c4c::codegen::lir::LirFunction& function,
    const BackendFunctionSignature& signature) {
  using namespace c4c::codegen::lir;

  if (function.is_declaration || signature.linkage != "define" ||
      signature.return_type != "i32" || signature.name != "main" ||
      !signature.params.empty() || signature.is_vararg ||
      function.blocks.size() != 1 ||
      (function.alloca_insts.size() != 1 && function.alloca_insts.size() != 2) ||
      !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* alloca = std::get_if<LirAllocaOp>(&function.alloca_insts.front());
  if (alloca == nullptr || alloca->type_str != "i32" || alloca->result.empty()) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  const auto* ret = std::get_if<LirRet>(&block.terminator);
  if (ret == nullptr || block.label != "entry" || !ret->value_str.has_value() ||
      ret->type_str != "i32") {
    return std::nullopt;
  }

  std::string normalized_args;
  std::string call_callee;
  std::string call_callee_type_suffix;
  if (function.alloca_insts.size() == 1) {
    if (block.insts.size() != 3 && block.insts.size() != 6) {
      return std::nullopt;
    }

    const auto* store = std::get_if<LirStoreOp>(&block.insts[0]);
    if (store == nullptr || store->type_str != "i32" || store->ptr != alloca->result) {
      return std::nullopt;
    }

    const c4c::codegen::lir::LirLoadOp* call_arg_load = nullptr;
    const c4c::codegen::lir::LirCallOp* call = nullptr;
    if (block.insts.size() == 3) {
      call_arg_load = std::get_if<LirLoadOp>(&block.insts[1]);
      call = std::get_if<LirCallOp>(&block.insts[2]);
      if (call_arg_load == nullptr || call == nullptr ||
          call_arg_load->type_str != "i32" || call_arg_load->ptr != alloca->result) {
        return std::nullopt;
      }
    } else {
      const auto* load_before = std::get_if<LirLoadOp>(&block.insts[1]);
      const auto* add = std::get_if<LirBinOp>(&block.insts[2]);
      const auto* store_rewrite = std::get_if<LirStoreOp>(&block.insts[3]);
      const auto* load_after = std::get_if<LirLoadOp>(&block.insts[4]);
      call = std::get_if<LirCallOp>(&block.insts[5]);
      if (load_before == nullptr || add == nullptr || store_rewrite == nullptr ||
          load_after == nullptr || call == nullptr || load_before->type_str != "i32" ||
          load_before->ptr != alloca->result || add->opcode != "add" ||
          add->type_str != "i32" || add->result.empty() ||
          store_rewrite->type_str != "i32" || store_rewrite->ptr != alloca->result ||
          store_rewrite->val != add->result || load_after->type_str != "i32" ||
          load_after->ptr != alloca->result) {
        return std::nullopt;
      }

      const bool add_zero_on_rhs = add->lhs == load_before->result && add->rhs == "0";
      const bool add_zero_on_lhs = add->lhs == "0" && add->rhs == load_before->result;
      if (!add_zero_on_rhs && !add_zero_on_lhs) {
        return std::nullopt;
      }
      call_arg_load = load_after;
    }

    if (call == nullptr || call_arg_load == nullptr || call->result.empty() ||
        *ret->value_str != call->result || call->return_type != "i32" ||
        call->callee_type_suffix != "(i32, i32)") {
      return std::nullopt;
    }

    const std::string first_local_prefix = "i32 " + call_arg_load->result + ", i32 ";
    const std::string second_local_suffix = ", i32 " + call_arg_load->result;
    if (call->args_str.size() > first_local_prefix.size() &&
        call->args_str.substr(0, first_local_prefix.size()) == first_local_prefix) {
      normalized_args =
          "i32 " + store->val + ", i32 " + call->args_str.substr(first_local_prefix.size());
    } else if (call->args_str.size() > second_local_suffix.size() &&
               call->args_str.substr(call->args_str.size() - second_local_suffix.size()) ==
                   second_local_suffix) {
      normalized_args =
          call->args_str.substr(0, call->args_str.size() - second_local_suffix.size()) +
          ", i32 " + store->val;
    } else {
      return std::nullopt;
    }
    call_callee = call->callee;
    call_callee_type_suffix = call->callee_type_suffix;
  } else {
    if (block.insts.size() != 5 && block.insts.size() != 8 && block.insts.size() != 11) {
      return std::nullopt;
    }

    const auto* alloca1 = std::get_if<LirAllocaOp>(&function.alloca_insts[1]);
    if (alloca1 == nullptr || alloca1->type_str != "i32" || alloca1->result.empty()) {
      return std::nullopt;
    }

    const auto* store0 = std::get_if<LirStoreOp>(&block.insts[0]);
    const auto* store1 = std::get_if<LirStoreOp>(&block.insts[1]);
    if (store0 == nullptr || store1 == nullptr || store0->type_str != "i32" ||
        store1->type_str != "i32" || store0->ptr != alloca->result ||
        store1->ptr != alloca1->result) {
      return std::nullopt;
    }

    const c4c::codegen::lir::LirLoadOp* call_arg_load0 = nullptr;
    const c4c::codegen::lir::LirLoadOp* call_arg_load1 = nullptr;
    const c4c::codegen::lir::LirCallOp* call = nullptr;
    const auto is_zero_rewrite = [](const LirLoadOp& load,
                                    const LirBinOp& add,
                                    const LirStoreOp& store,
                                    const std::string& slot) {
      if (load.type_str != "i32" || load.ptr != slot || add.opcode != "add" ||
          add.type_str != "i32" || add.result.empty() || store.type_str != "i32" ||
          store.ptr != slot || store.val != add.result) {
        return false;
      }
      const bool add_zero_on_rhs = add.lhs == load.result && add.rhs == "0";
      const bool add_zero_on_lhs = add.lhs == "0" && add.rhs == load.result;
      return add_zero_on_rhs || add_zero_on_lhs;
    };

    if (block.insts.size() == 5) {
      call_arg_load0 = std::get_if<LirLoadOp>(&block.insts[2]);
      call_arg_load1 = std::get_if<LirLoadOp>(&block.insts[3]);
      call = std::get_if<LirCallOp>(&block.insts[4]);
      if (call_arg_load0 == nullptr || call_arg_load1 == nullptr || call == nullptr ||
          call_arg_load0->type_str != "i32" || call_arg_load1->type_str != "i32" ||
          call_arg_load0->ptr != alloca->result || call_arg_load1->ptr != alloca1->result) {
        return std::nullopt;
      }
    } else if (block.insts.size() == 11) {
      const auto* load0_before = std::get_if<LirLoadOp>(&block.insts[2]);
      const auto* add0 = std::get_if<LirBinOp>(&block.insts[3]);
      const auto* store0_rewrite = std::get_if<LirStoreOp>(&block.insts[4]);
      const auto* load1_before = std::get_if<LirLoadOp>(&block.insts[5]);
      const auto* add1 = std::get_if<LirBinOp>(&block.insts[6]);
      const auto* store1_rewrite = std::get_if<LirStoreOp>(&block.insts[7]);
      const auto* load0_after = std::get_if<LirLoadOp>(&block.insts[8]);
      const auto* load1_after = std::get_if<LirLoadOp>(&block.insts[9]);
      call = std::get_if<LirCallOp>(&block.insts[10]);
      if (load0_before == nullptr || add0 == nullptr || store0_rewrite == nullptr ||
          load1_before == nullptr || add1 == nullptr || store1_rewrite == nullptr ||
          load0_after == nullptr || load1_after == nullptr || call == nullptr ||
          !is_zero_rewrite(*load0_before, *add0, *store0_rewrite, alloca->result) ||
          !is_zero_rewrite(*load1_before, *add1, *store1_rewrite, alloca1->result) ||
          load0_after->type_str != "i32" || load0_after->ptr != alloca->result ||
          load1_after->type_str != "i32" || load1_after->ptr != alloca1->result) {
        return std::nullopt;
      }
      call_arg_load0 = load0_after;
      call_arg_load1 = load1_after;
    } else {
      call = std::get_if<LirCallOp>(&block.insts[7]);
      if (call == nullptr) {
        return std::nullopt;
      }

      const auto is_zero_rewrite_with_reload =
          [&](const LirLoadOp& load,
              const LirBinOp& add,
              const LirStoreOp& store,
              const LirLoadOp& reload,
              const std::string& slot) {
        if (load.type_str != "i32" || load.ptr != slot || add.opcode != "add" ||
            add.type_str != "i32" || add.result.empty() || store.type_str != "i32" ||
            store.ptr != slot || store.val != add.result || reload.type_str != "i32" ||
            reload.ptr != slot) {
          return false;
        }
        return is_zero_rewrite(load, add, store, slot);
      };

      const auto* load0_before = std::get_if<LirLoadOp>(&block.insts[2]);
      const auto* add0 = std::get_if<LirBinOp>(&block.insts[3]);
      const auto* store0_rewrite = std::get_if<LirStoreOp>(&block.insts[4]);
      const auto* load0_after = std::get_if<LirLoadOp>(&block.insts[5]);
      const auto* load1_after = std::get_if<LirLoadOp>(&block.insts[6]);
      if (load0_before != nullptr && add0 != nullptr && store0_rewrite != nullptr &&
          load0_after != nullptr && load1_after != nullptr &&
          is_zero_rewrite_with_reload(*load0_before, *add0, *store0_rewrite, *load0_after,
                                      alloca->result) &&
          load1_after->type_str == "i32" && load1_after->ptr == alloca1->result) {
        call_arg_load0 = load0_after;
        call_arg_load1 = load1_after;
      } else {
        const auto* load1_before = std::get_if<LirLoadOp>(&block.insts[2]);
        const auto* add1 = std::get_if<LirBinOp>(&block.insts[3]);
        const auto* store1_rewrite = std::get_if<LirStoreOp>(&block.insts[4]);
        const auto* load0_after_second = std::get_if<LirLoadOp>(&block.insts[5]);
        const auto* load1_after_second = std::get_if<LirLoadOp>(&block.insts[6]);
        if (load1_before == nullptr || add1 == nullptr || store1_rewrite == nullptr ||
            load0_after_second == nullptr || load1_after_second == nullptr ||
            !is_zero_rewrite_with_reload(*load1_before, *add1, *store1_rewrite,
                                         *load1_after_second, alloca1->result) ||
            load0_after_second->type_str != "i32" ||
            load0_after_second->ptr != alloca->result) {
          return std::nullopt;
        }
        call_arg_load0 = load0_after_second;
        call_arg_load1 = load1_after_second;
      }

      if (call_arg_load0 == nullptr || call_arg_load1 == nullptr) {
        return std::nullopt;
      }
    }

    if (call_arg_load0 == nullptr || call_arg_load1 == nullptr || call == nullptr ||
        call->result.empty() || *ret->value_str != call->result ||
        call->return_type != "i32" || call->callee_type_suffix != "(i32, i32)" ||
        call->args_str !=
            ("i32 " + call_arg_load0->result + ", i32 " + call_arg_load1->result)) {
      return std::nullopt;
    }

    normalized_args = "i32 " + store0->val + ", i32 " + store1->val;
    call_callee = call->callee;
    call_callee_type_suffix = call->callee_type_suffix;
  }

  BackendFunction out;
  out.signature = signature;
  BackendBlock out_block;
  out_block.label = block.label;
  out_block.insts.push_back(BackendCallInst{
      *ret->value_str,
      "i32",
      call_callee,
      call_callee_type_suffix,
      normalized_args,
  });
  out_block.terminator = BackendReturn{*ret->value_str, "i32"};
  out.blocks.push_back(std::move(out_block));
  return out;
}

BackendFunction adapt_function(const c4c::codegen::lir::LirFunction& function) {
  const auto signature = parse_signature(function.signature_text);
  BackendFunction out;
  out.signature = signature;
  out.is_declaration = function.is_declaration;
  if (function.is_declaration) {
    return out;
  }

  if (const auto normalized = adapt_single_param_add_function(function, signature);
      normalized.has_value()) {
    return *normalized;
  }
  if (const auto normalized = adapt_local_temp_return_function(function, signature);
      normalized.has_value()) {
    return *normalized;
  }
  if (const auto normalized = adapt_local_temp_sub_return_function(function, signature);
      normalized.has_value()) {
    return *normalized;
  }
  if (const auto normalized = adapt_local_pointer_temp_return_function(function, signature);
      normalized.has_value()) {
    return *normalized;
  }
  if (const auto normalized = adapt_local_single_arg_call_function(function, signature);
      normalized.has_value()) {
    return *normalized;
  }
  if (const auto normalized = adapt_local_two_arg_call_function(function, signature);
      normalized.has_value()) {
    return *normalized;
  }
  if (!function.stack_objects.empty()) fail_unsupported("stack objects");
  if (!function.alloca_insts.empty()) fail_unsupported("entry allocas");
  if (function.blocks.size() != 1) fail_unsupported("multi-block functions");

  const auto& block = function.blocks.front();
  BackendBlock out_block;
  out_block.label = block.label;
  for (const auto& inst : block.insts) {
    out_block.insts.push_back(adapt_inst(inst));
  }
  out_block.terminator = adapt_terminator(block.terminator);
  out.blocks.push_back(std::move(out_block));
  return out;
}

void render_inst(std::ostringstream& out, const BackendInst& inst) {
  if (const auto* bin = std::get_if<BackendBinaryInst>(&inst)) {
    const char* opcode = nullptr;
    switch (bin->opcode) {
      case BackendBinaryOpcode::Add:
        opcode = "add";
        break;
      case BackendBinaryOpcode::Sub:
        opcode = "sub";
        break;
    }
    out << "  " << bin->result << " = " << opcode << " " << bin->type_str << " "
        << bin->lhs << ", " << bin->rhs << "\n";
    return;
  }

  const auto* call = std::get_if<BackendCallInst>(&inst);
  if (call == nullptr) return;

  out << "  ";
  if (!call->result.empty()) out << call->result << " = ";
  out << "call " << call->return_type << " ";
  if (!call->callee_type_suffix.empty()) out << call->callee_type_suffix << " ";
  out << call->callee << "(" << call->args_str << ")\n";
}

void render_function(std::ostringstream& out, const BackendFunction& function) {
  render_signature(out, function.signature);
  if (function.is_declaration) return;

  out << "{\n";
  for (const auto& block : function.blocks) {
    out << block.label << ":\n";
    for (const auto& inst : block.insts) {
      render_inst(out, inst);
    }
    if (block.terminator.value.has_value()) {
      out << "  ret " << block.terminator.type_str << " "
          << *block.terminator.value << "\n";
    } else {
      out << "  ret void\n";
    }
  }
  out << "}\n\n";
}

}  // namespace

BackendModule adapt_minimal_module(const c4c::codegen::lir::LirModule& module) {
  if (!module.globals.empty()) fail_unsupported("globals");
  if (!module.string_pool.empty()) fail_unsupported("string constants");
  if (!module.extern_decls.empty()) fail_unsupported("extern declarations");
  if (module.need_va_start || module.need_va_end || module.need_va_copy ||
      module.need_memcpy || module.need_stacksave || module.need_stackrestore ||
      module.need_abs) {
    fail_unsupported("intrinsic declarations");
  }

  BackendModule out;
  out.target_triple = module.target_triple;
  out.data_layout = module.data_layout;
  out.type_decls = module.type_decls;
  for (const auto& function : module.functions) {
    out.functions.push_back(adapt_function(function));
  }
  return out;
}

std::string render_module(const BackendModule& module) {
  std::ostringstream out;
  if (!module.data_layout.empty()) out << "target datalayout = \"" << module.data_layout << "\"\n";
  if (!module.target_triple.empty()) out << "target triple = \"" << module.target_triple << "\"\n";
  if (!module.data_layout.empty() || !module.target_triple.empty()) out << "\n";
  for (const auto& type_decl : module.type_decls) {
    out << type_decl << "\n";
  }
  if (!module.type_decls.empty()) out << "\n";

  for (const auto& function : module.functions) {
    render_function(out, function);
  }
  return out.str();
}

}  // namespace c4c::backend
