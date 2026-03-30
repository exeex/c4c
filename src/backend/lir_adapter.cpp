#include "lir_adapter.hpp"

#include "ir_printer.hpp"

#include "../codegen/lir/call_args.hpp"

#include <charconv>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace c4c::backend {

namespace {

using c4c::codegen::lir::LirBinOp;
using c4c::codegen::lir::LirBinaryOpcode;
using c4c::codegen::lir::LirBinaryOpcodeRef;
using c4c::codegen::lir::LirCmpOp;
using c4c::codegen::lir::LirCmpPredicate;
using c4c::codegen::lir::LirCallOp;

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

std::optional<std::int64_t> parse_i64(std::string_view text) {
  std::int64_t value = 0;
  const char* begin = text.data();
  const char* end = begin + text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc() || result.ptr != end) {
    return std::nullopt;
  }
  return value;
}

const c4c::codegen::lir::LirGlobal* find_lir_global(
    const c4c::codegen::lir::LirModule& module,
    std::string_view name) {
  for (const auto& global : module.globals) {
    if (global.name == name) {
      return &global;
    }
  }
  return nullptr;
}

const c4c::codegen::lir::LirStringConst* find_lir_string_constant(
    const c4c::codegen::lir::LirModule& module,
    std::string_view pool_name) {
  for (const auto& string_const : module.string_pool) {
    if (string_const.pool_name == pool_name) {
      return &string_const;
    }
  }
  return nullptr;
}

std::optional<std::vector<std::string>> infer_extern_param_types(
    const c4c::codegen::lir::LirModule& module,
    const c4c::codegen::lir::LirExternDecl& decl) {
  std::optional<std::vector<std::string>> inferred;
  const std::string callee = "@" + decl.name;

  for (const auto& function : module.functions) {
    for (const auto& block : function.blocks) {
      for (const auto& inst : block.insts) {
        const auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&inst);
        if (call == nullptr || call->callee.str() != callee) {
          continue;
        }

        const auto parsed_call = parse_backend_typed_call(*call);
        if (!parsed_call.has_value()) {
          continue;
        }

        std::vector<std::string> param_types;
        param_types.reserve(parsed_call->param_types.size());
        for (const auto type : parsed_call->param_types) {
          param_types.push_back(std::string(type));
        }
        if (!inferred.has_value()) {
          inferred = std::move(param_types);
          continue;
        }
        if (*inferred != param_types) {
          throw LirAdapterError(
              LirAdapterErrorKind::Unsupported,
              "minimal backend LIR adapter does not support extern declarations with inconsistent typed call surfaces");
        }
      }
    }
  }

  return inferred;
}

BackendFunction adapt_extern_decl(const c4c::codegen::lir::LirModule& module,
                                  const c4c::codegen::lir::LirExternDecl& decl) {
  BackendFunction out;
  out.is_declaration = true;
  out.signature.linkage = "declare";
  out.signature.return_type = decl.return_type_str;
  out.signature.name = decl.name;

  if (out.signature.return_type.empty() || out.signature.name.empty()) {
    throw LirAdapterError(LirAdapterErrorKind::Malformed,
                          "minimal backend LIR adapter could not adapt extern declaration");
  }

  const auto param_types = infer_extern_param_types(module, decl);
  if (param_types.has_value()) {
    out.signature.params.reserve(param_types->size());
    for (const auto& type_str : *param_types) {
      out.signature.params.push_back(BackendParam{type_str, {}});
    }
  }

  return out;
}

BackendGlobal adapt_global(const c4c::codegen::lir::LirGlobal& global) {
  return BackendGlobal{
      global.name,
      global.linkage_vis,
      global.qualifier,
      global.llvm_type,
      global.init_text,
      global.align_bytes,
      global.is_extern_decl,
  };
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

std::optional<BackendBinaryOpcode> adapt_binary_opcode(
    const LirBinaryOpcodeRef& opcode_ref) {
  switch (opcode_ref.typed().value_or(static_cast<LirBinaryOpcode>(0xff))) {
    case LirBinaryOpcode::Add: return BackendBinaryOpcode::Add;
    case LirBinaryOpcode::Sub: return BackendBinaryOpcode::Sub;
    case LirBinaryOpcode::Mul: return BackendBinaryOpcode::Mul;
    case LirBinaryOpcode::SDiv: return BackendBinaryOpcode::SDiv;
    case LirBinaryOpcode::SRem: return BackendBinaryOpcode::SRem;
    default: return std::nullopt;
  }
}

bool has_binary_opcode(const LirBinOp& bin, LirBinaryOpcode opcode) {
  return bin.opcode.typed() == opcode;
}

bool has_cmp_predicate(const LirCmpOp& cmp, LirCmpPredicate predicate) {
  return cmp.predicate.typed() == predicate;
}

std::optional<BackendComparePredicate> adapt_compare_predicate(
    const LirCmpPredicate predicate) {
  switch (predicate) {
    case LirCmpPredicate::Slt: return BackendComparePredicate::Slt;
    case LirCmpPredicate::Sle: return BackendComparePredicate::Sle;
    case LirCmpPredicate::Sgt: return BackendComparePredicate::Sgt;
    case LirCmpPredicate::Sge: return BackendComparePredicate::Sge;
    case LirCmpPredicate::Eq: return BackendComparePredicate::Eq;
    case LirCmpPredicate::Ne: return BackendComparePredicate::Ne;
    case LirCmpPredicate::Ult: return BackendComparePredicate::Ult;
    case LirCmpPredicate::Ule: return BackendComparePredicate::Ule;
    case LirCmpPredicate::Ugt: return BackendComparePredicate::Ugt;
    case LirCmpPredicate::Uge: return BackendComparePredicate::Uge;
  }
  return std::nullopt;
}

std::optional<std::vector<BackendBinaryInst>> adapt_conditional_phi_join_predecessor_compute_chain(
    const c4c::codegen::lir::LirBlock& block,
    std::string_view expected_result) {
  if (block.insts.empty()) {
    return std::vector<BackendBinaryInst>{};
  }
  if (block.insts.size() > 2) {
    return std::nullopt;
  }

  std::vector<BackendBinaryInst> out;
  out.reserve(block.insts.size());

  const auto* first = std::get_if<LirBinOp>(&block.insts.front());
  if (first == nullptr || first->type_str != "i32" || !parse_i64(first->lhs).has_value() ||
      !parse_i64(first->rhs).has_value()) {
    return std::nullopt;
  }

  const auto opcode = adapt_binary_opcode(first->opcode);
  if (!opcode.has_value() ||
      (*opcode != BackendBinaryOpcode::Add && *opcode != BackendBinaryOpcode::Sub)) {
    return std::nullopt;
  }

  out.push_back(BackendBinaryInst{
      *opcode,
      first->result,
      first->type_str,
      first->lhs,
      first->rhs,
  });

  if (block.insts.size() == 1) {
    if (first->result != expected_result) {
      return std::nullopt;
    }
    return out;
  }

  const auto* second = std::get_if<LirBinOp>(&block.insts[1]);
  if (second == nullptr || second->type_str != "i32" || second->result != expected_result ||
      second->lhs != first->result || !parse_i64(second->rhs).has_value()) {
    return std::nullopt;
  }

  const auto second_opcode = adapt_binary_opcode(second->opcode);
  if (!second_opcode.has_value() ||
      (*second_opcode != BackendBinaryOpcode::Add &&
       *second_opcode != BackendBinaryOpcode::Sub)) {
    return std::nullopt;
  }

  out.push_back(BackendBinaryInst{
      *second_opcode,
      second->result,
      second->type_str,
      second->lhs,
      second->rhs,
  });
  return out;
}

std::vector<std::string> own_backend_call_param_types(
    const c4c::codegen::lir::ParsedLirTypedCallView& parsed) {
  std::vector<std::string> param_types;
  param_types.reserve(parsed.param_types.size());
  for (const auto type : parsed.param_types) {
    param_types.push_back(std::string(type));
  }
  return param_types;
}

BackendCallInst make_backend_call_inst(std::string result,
                                       std::string return_type,
                                       std::string callee,
                                       const c4c::codegen::lir::ParsedLirTypedCallView& parsed,
                                       bool render_callee_type_suffix) {
  return BackendCallInst{
      std::move(result),
      std::move(return_type),
      std::move(callee),
      own_backend_call_param_types(parsed),
      c4c::codegen::lir::own_lir_typed_call_args(parsed),
      render_callee_type_suffix,
  };
}

BackendInst adapt_inst(const c4c::codegen::lir::LirInst& inst) {
  if (const auto* bin = std::get_if<c4c::codegen::lir::LirBinOp>(&inst)) {
    const auto opcode = adapt_binary_opcode(bin->opcode);
    if (!opcode.has_value()) {
      fail_unsupported("binary opcode '" + bin->opcode + "'");
    }
    BackendBinaryInst out;
    out.opcode = *opcode;
    out.result = bin->result;
    out.type_str = bin->type_str;
    out.lhs = bin->lhs;
    out.rhs = bin->rhs;
    return out;
  }

  if (const auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&inst)) {
    const auto parsed_call = parse_backend_typed_call(*call);
    if (!parsed_call.has_value()) {
      throw LirAdapterError(
          LirAdapterErrorKind::Malformed,
          "minimal backend LIR adapter could not parse typed call operands for '" +
              std::string(call->callee.str()) + "'");
    }
    return make_backend_call_inst(call->result.str(),
                                  call->return_type.str(),
                                  call->callee.str(),
                                  *parsed_call,
                                  !c4c::codegen::lir::trim_lir_arg_text(call->callee_type_suffix).empty());
  }

  fail_unsupported("non-binary/non-call instructions");
}

std::optional<BackendFunction> adapt_string_literal_char_function(
    const c4c::codegen::lir::LirFunction& function,
    const c4c::codegen::lir::LirModule& module,
    const BackendFunctionSignature& signature) {
  using namespace c4c::codegen::lir;

  if (function.is_declaration || signature.linkage != "define" ||
      signature.return_type != "i32" || signature.name != "main" ||
      !signature.params.empty() || signature.is_vararg || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty() ||
      !module.globals.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry" || block.insts.size() != 5) {
    return std::nullopt;
  }

  const auto* base_gep = std::get_if<LirGepOp>(&block.insts[0]);
  const auto* index_cast = std::get_if<LirCastOp>(&block.insts[1]);
  const auto* byte_gep = std::get_if<LirGepOp>(&block.insts[2]);
  const auto* load = std::get_if<LirLoadOp>(&block.insts[3]);
  const auto* extend = std::get_if<LirCastOp>(&block.insts[4]);
  const auto* ret = std::get_if<LirRet>(&block.terminator);
  if (base_gep == nullptr || index_cast == nullptr || byte_gep == nullptr ||
      load == nullptr || extend == nullptr || ret == nullptr ||
      !ret->value_str.has_value()) {
    return std::nullopt;
  }

  const auto* string_const = find_lir_string_constant(module, base_gep->ptr);
  if (string_const == nullptr) {
    return std::nullopt;
  }
  const std::string string_array_type =
      "[" + std::to_string(string_const->byte_length) + " x i8]";
  if (base_gep->element_type != string_array_type ||
      base_gep->indices.size() != 2 || base_gep->indices[0] != "i64 0" ||
      base_gep->indices[1] != "i64 0") {
    return std::nullopt;
  }

  if (index_cast->kind != LirCastKind::SExt || index_cast->from_type != "i32" ||
      index_cast->to_type != "i64") {
    return std::nullopt;
  }
  const auto byte_index = parse_i64(index_cast->operand);
  if (!byte_index.has_value() || *byte_index < 0 ||
      *byte_index >= static_cast<std::int64_t>(string_const->byte_length)) {
    return std::nullopt;
  }

  if (byte_gep->element_type != "i8" || byte_gep->ptr != base_gep->result ||
      byte_gep->indices.size() != 1 ||
      byte_gep->indices[0] != ("i64 " + index_cast->result) ||
      load->type_str != "i8" || load->ptr != byte_gep->result) {
    return std::nullopt;
  }

  if ((extend->kind != LirCastKind::SExt && extend->kind != LirCastKind::ZExt) ||
      extend->from_type != "i8" || extend->operand != load->result ||
      extend->to_type != "i32" || *ret->value_str != extend->result ||
      ret->type_str != "i32") {
    return std::nullopt;
  }

  BackendFunction out;
  out.signature = signature;
  BackendBlock out_block;
  out_block.label = block.label;
  out_block.insts.push_back(BackendLoadInst{
      extend->result,
      "i32",
      "i8",
      BackendAddress{string_const->pool_name.substr(1), *byte_index},
      extend->kind == LirCastKind::SExt ? BackendLoadExtension::SignExtend
                                        : BackendLoadExtension::ZeroExtend,
  });
  out_block.terminator = BackendReturn{*ret->value_str, "i32"};
  out.blocks.push_back(std::move(out_block));
  return out;
}

BackendTerminator adapt_terminator(const c4c::codegen::lir::LirTerminator& terminator) {
  const auto* ret = std::get_if<c4c::codegen::lir::LirRet>(&terminator);
  if (!ret) fail_unsupported("non-return terminators");
  return BackendReturn{ret->value_str, ret->type_str};
}

std::optional<BackendFunction> adapt_conditional_return_function(
    const c4c::codegen::lir::LirFunction& function,
    const BackendFunctionSignature& signature) {
  using namespace c4c::codegen::lir;

  if (function.is_declaration || signature.linkage != "define" ||
      signature.return_type != "i32" || signature.name != "main" ||
      !signature.params.empty() || signature.is_vararg || function.entry.value != 0 ||
      function.blocks.size() != 3 || !function.alloca_insts.empty() ||
      !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks[0];
  const auto& true_block = function.blocks[1];
  const auto& false_block = function.blocks[2];
  if (entry.label != "entry" || entry.insts.size() != 3 || !true_block.insts.empty() ||
      !false_block.insts.empty()) {
    return std::nullopt;
  }

  const auto* cmp0 = std::get_if<LirCmpOp>(&entry.insts[0]);
  const auto* cast = std::get_if<LirCastOp>(&entry.insts[1]);
  const auto* cmp1 = std::get_if<LirCmpOp>(&entry.insts[2]);
  const auto* condbr = std::get_if<LirCondBr>(&entry.terminator);
  const auto cmp0_predicate = cmp0 == nullptr ? std::nullopt : cmp0->predicate.typed();
  const auto cmp1_predicate = cmp1 == nullptr ? std::nullopt : cmp1->predicate.typed();
  if (cmp0 == nullptr || cast == nullptr || cmp1 == nullptr || condbr == nullptr ||
      cmp0->is_float || !cmp0_predicate.has_value() ||
      !adapt_compare_predicate(*cmp0_predicate).has_value() || cmp0->type_str != "i32" ||
      cast->kind != LirCastKind::ZExt || cast->from_type != "i1" ||
      cast->operand != cmp0->result || cast->to_type != "i32" || cmp1->is_float ||
      cmp1_predicate != LirCmpPredicate::Ne || cmp1->type_str != "i32" ||
      cmp1->lhs != cast->result || cmp1->rhs != "0" || condbr->cond_name != cmp1->result ||
      true_block.label != condbr->true_label || false_block.label != condbr->false_label) {
    return std::nullopt;
  }

  const auto* true_ret = std::get_if<LirRet>(&true_block.terminator);
  const auto* false_ret = std::get_if<LirRet>(&false_block.terminator);
  if (true_ret == nullptr || false_ret == nullptr || !true_ret->value_str.has_value() ||
      !false_ret->value_str.has_value() || true_ret->type_str != "i32" ||
      false_ret->type_str != "i32") {
    return std::nullopt;
  }

  if (!parse_i64(cmp0->lhs).has_value() || !parse_i64(cmp0->rhs).has_value() ||
      !parse_i64(*true_ret->value_str).has_value() ||
      !parse_i64(*false_ret->value_str).has_value()) {
    return std::nullopt;
  }

  BackendFunction out;
  out.signature = signature;

  BackendBlock out_entry;
  out_entry.label = entry.label;
  out_entry.insts.push_back(BackendCompareInst{
      *adapt_compare_predicate(*cmp0_predicate),
      cmp0->result,
      "i32",
      cmp0->lhs,
      cmp0->rhs,
  });
  out_entry.terminator =
      BackendTerminator::make_cond_branch(cmp0->result, true_block.label, false_block.label);
  out.blocks.push_back(std::move(out_entry));

  BackendBlock out_true;
  out_true.label = true_block.label;
  out_true.terminator = BackendReturn{*true_ret->value_str, "i32"};
  out.blocks.push_back(std::move(out_true));

  BackendBlock out_false;
  out_false.label = false_block.label;
  out_false.terminator = BackendReturn{*false_ret->value_str, "i32"};
  out.blocks.push_back(std::move(out_false));

  return out;
}

std::optional<BackendFunction> adapt_conditional_phi_join_function(
    const c4c::codegen::lir::LirFunction& function,
    const BackendFunctionSignature& signature) {
  using namespace c4c::codegen::lir;

  if (function.is_declaration || signature.linkage != "define" ||
      signature.return_type != "i32" || signature.name != "main" ||
      !signature.params.empty() || signature.is_vararg || function.entry.value != 0 ||
      function.blocks.size() != 4 || !function.alloca_insts.empty() ||
      !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks[0];
  const auto& true_block = function.blocks[1];
  const auto& false_block = function.blocks[2];
  const auto& join_block = function.blocks[3];
  if (entry.label != "entry" || entry.insts.size() != 3 ||
      true_block.insts.size() > 2 || false_block.insts.size() > 2 ||
      (join_block.insts.size() != 1 && join_block.insts.size() != 2)) {
    return std::nullopt;
  }

  const auto* cmp0 = std::get_if<LirCmpOp>(&entry.insts[0]);
  const auto* cast = std::get_if<LirCastOp>(&entry.insts[1]);
  const auto* cmp1 = std::get_if<LirCmpOp>(&entry.insts[2]);
  const auto* condbr = std::get_if<LirCondBr>(&entry.terminator);
  const auto cmp0_predicate = cmp0 == nullptr ? std::nullopt : cmp0->predicate.typed();
  const auto cmp1_predicate = cmp1 == nullptr ? std::nullopt : cmp1->predicate.typed();
  if (cmp0 == nullptr || cast == nullptr || cmp1 == nullptr || condbr == nullptr ||
      cmp0->is_float || !cmp0_predicate.has_value() ||
      !adapt_compare_predicate(*cmp0_predicate).has_value() || cmp0->type_str != "i32" ||
      cast->kind != LirCastKind::ZExt || cast->from_type != "i1" ||
      cast->operand != cmp0->result || cast->to_type != "i32" || cmp1->is_float ||
      cmp1_predicate != LirCmpPredicate::Ne || cmp1->type_str != "i32" ||
      cmp1->lhs != cast->result || cmp1->rhs != "0" || condbr->cond_name != cmp1->result ||
      true_block.label != condbr->true_label || false_block.label != condbr->false_label) {
    return std::nullopt;
  }

  const auto* true_br = std::get_if<LirBr>(&true_block.terminator);
  const auto* false_br = std::get_if<LirBr>(&false_block.terminator);
  const auto* phi = std::get_if<LirPhiOp>(&join_block.insts.front());
  const auto* ret = std::get_if<LirRet>(&join_block.terminator);
  if (true_br == nullptr || false_br == nullptr || phi == nullptr || ret == nullptr ||
      true_br->target_label != join_block.label ||
      false_br->target_label != join_block.label || phi->type_str != "i32" ||
      phi->incoming.size() != 2 || !ret->value_str.has_value() ||
      ret->type_str != "i32") {
    return std::nullopt;
  }

  if (phi->incoming[0].second != true_block.label ||
      phi->incoming[1].second != false_block.label) {
    return std::nullopt;
  }

  const auto true_compute =
      adapt_conditional_phi_join_predecessor_compute_chain(true_block, phi->incoming[0].first);
  const auto false_compute =
      adapt_conditional_phi_join_predecessor_compute_chain(false_block, phi->incoming[1].first);
  if ((!true_block.insts.empty() && !true_compute.has_value()) ||
      (!false_block.insts.empty() && !false_compute.has_value())) {
    return std::nullopt;
  }
  if (true_block.insts.empty() && !parse_i64(phi->incoming[0].first).has_value()) {
    return std::nullopt;
  }
  if (false_block.insts.empty() && !parse_i64(phi->incoming[1].first).has_value()) {
    return std::nullopt;
  }

  std::optional<BackendBinaryInst> join_compute;
  std::string return_value = "%t.join";
  if (join_block.insts.size() == 2) {
    const auto* add = std::get_if<LirBinOp>(&join_block.insts[1]);
    if (add == nullptr || !has_binary_opcode(*add, LirBinaryOpcode::Add) ||
        add->type_str != "i32" || add->lhs != phi->result ||
        !parse_i64(add->rhs).has_value() || *ret->value_str != add->result) {
      return std::nullopt;
    }

    join_compute = BackendBinaryInst{
        BackendBinaryOpcode::Add,
        add->result,
        add->type_str,
        "%t.join",
        add->rhs,
    };
    return_value = add->result;
  }

  BackendFunction out;
  out.signature = signature;

  BackendBlock out_entry;
  out_entry.label = entry.label;
  out_entry.insts.push_back(BackendCompareInst{
      *adapt_compare_predicate(*cmp0_predicate),
      cmp0->result,
      "i32",
      cmp0->lhs,
      cmp0->rhs,
  });
  out_entry.terminator =
      BackendTerminator::make_cond_branch(cmp0->result, true_block.label, false_block.label);
  out.blocks.push_back(std::move(out_entry));

  BackendBlock out_true;
  out_true.label = true_block.label;
  if (true_compute.has_value()) {
    for (const auto& inst : *true_compute) {
      out_true.insts.push_back(inst);
    }
  }
  out_true.terminator = BackendTerminator::make_branch(join_block.label);
  out.blocks.push_back(std::move(out_true));

  BackendBlock out_false;
  out_false.label = false_block.label;
  if (false_compute.has_value()) {
    for (const auto& inst : *false_compute) {
      out_false.insts.push_back(inst);
    }
  }
  out_false.terminator = BackendTerminator::make_branch(join_block.label);
  out.blocks.push_back(std::move(out_false));

  BackendBlock out_join;
  out_join.label = join_block.label;
  out_join.insts.push_back(BackendPhiInst{
      "%t.join",
      "i32",
      {
          BackendPhiIncoming{phi->incoming[0].first, true_block.label},
          BackendPhiIncoming{phi->incoming[1].first, false_block.label},
      },
  });
  if (join_compute.has_value()) {
    out_join.insts.push_back(*join_compute);
  }
  out_join.terminator = BackendReturn{return_value, "i32"};
  out.blocks.push_back(std::move(out_join));

  return out;
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
        !has_binary_opcode(add, LirBinaryOpcode::Add) || add.type_str != "i32" ||
        add.lhs != load.result ||
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
      load->ptr != alloca->result || !has_binary_opcode(*sub, LirBinaryOpcode::Sub) ||
      sub->type_str != "i32" ||
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

std::optional<BackendFunction> adapt_local_temp_arithmetic_return_function(
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
      ret->type_str != "i32" || block.insts.size() < 5) {
    return std::nullopt;
  }

  const auto parse_const_i32 = [&](const std::string& value) -> std::optional<std::int64_t> {
    try {
      std::size_t pos = 0;
      const auto parsed = std::stoll(value, &pos, 10);
      if (pos != value.size()) {
        return std::nullopt;
      }
      return parsed;
    } catch (...) {
      return std::nullopt;
    }
  };

  std::optional<std::int64_t> slot_value;
  std::unordered_map<std::string, std::int64_t> values;
  std::string last_result;

  auto get_value = [&](const std::string& value) -> std::optional<std::int64_t> {
    if (const auto imm = parse_const_i32(value); imm.has_value()) {
      return imm;
    }
    const auto it = values.find(value);
    if (it == values.end()) {
      return std::nullopt;
    }
    return it->second;
  };

  for (const auto& inst : block.insts) {
    if (const auto* store = std::get_if<LirStoreOp>(&inst)) {
      if (store->type_str != "i32" || store->ptr != alloca->result) {
        return std::nullopt;
      }
      slot_value = get_value(store->val);
      if (!slot_value.has_value()) {
        return std::nullopt;
      }
      last_result.clear();
      continue;
    }

    if (const auto* load = std::get_if<LirLoadOp>(&inst)) {
      if (load->type_str != "i32" || load->ptr != alloca->result || !slot_value.has_value()) {
        return std::nullopt;
      }
      values[load->result] = *slot_value;
      last_result = load->result;
      continue;
    }

    const auto* bin = std::get_if<LirBinOp>(&inst);
    if (bin == nullptr || bin->type_str != "i32" || bin->lhs != last_result) {
      return std::nullopt;
    }

    const auto lhs = get_value(bin->lhs);
    const auto rhs = get_value(bin->rhs);
    if (!lhs.has_value() || !rhs.has_value()) {
      return std::nullopt;
    }

    std::optional<std::int64_t> result;
    if (has_binary_opcode(*bin, LirBinaryOpcode::Mul)) {
      result = *lhs * *rhs;
    } else if (has_binary_opcode(*bin, LirBinaryOpcode::SDiv)) {
      if (*rhs == 0) {
        return std::nullopt;
      }
      result = *lhs / *rhs;
    } else if (has_binary_opcode(*bin, LirBinaryOpcode::SRem)) {
      if (*rhs == 0) {
        return std::nullopt;
      }
      result = *lhs % *rhs;
    } else if (has_binary_opcode(*bin, LirBinaryOpcode::Sub)) {
      result = *lhs - *rhs;
    } else if (has_binary_opcode(*bin, LirBinaryOpcode::Add)) {
      result = *lhs + *rhs;
    } else {
      return std::nullopt;
    }

    values[bin->result] = *result;
    last_result = bin->result;
  }

  const auto return_value = get_value(*ret->value_str);
  if (!return_value.has_value()) {
    return std::nullopt;
  }

  BackendFunction out;
  out.signature = signature;
  BackendBlock out_block;
  out_block.label = block.label;
  out_block.terminator = BackendReturn{std::to_string(*return_value), "i32"};
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

std::optional<BackendFunction> adapt_double_indirect_local_pointer_conditional_return_function(
    const c4c::codegen::lir::LirFunction& function,
    const BackendFunctionSignature& signature) {
  using namespace c4c::codegen::lir;

  if (function.is_declaration || signature.linkage != "define" ||
      signature.return_type != "i32" || signature.name != "main" ||
      !signature.params.empty() || signature.is_vararg || function.blocks.empty() ||
      function.alloca_insts.size() != 3 || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* scalar_alloca = std::get_if<LirAllocaOp>(&function.alloca_insts[0]);
  const auto* ptr_alloca = std::get_if<LirAllocaOp>(&function.alloca_insts[1]);
  const auto* ptrptr_alloca = std::get_if<LirAllocaOp>(&function.alloca_insts[2]);
  if (scalar_alloca == nullptr || ptr_alloca == nullptr || ptrptr_alloca == nullptr ||
      scalar_alloca->type_str != "i32" || ptr_alloca->type_str != "ptr" ||
      ptrptr_alloca->type_str != "ptr" || scalar_alloca->result.empty() ||
      ptr_alloca->result.empty() || ptrptr_alloca->result.empty()) {
    return std::nullopt;
  }

  std::unordered_map<std::string, const LirBlock*> blocks_by_label;
  for (const auto& block : function.blocks) {
    blocks_by_label.emplace(block.label, &block);
  }

  std::unordered_map<std::string, std::string> scalar_values;
  std::unordered_map<std::string, std::string> pointer_values;
  std::unordered_map<std::string, std::string> integer_values;
  std::unordered_map<std::string, bool> predicate_values;

  auto resolve_pointer_rvalue = [&](std::string_view value) -> std::optional<std::string> {
    const std::string key(value);
    if (key == scalar_alloca->result || key == ptr_alloca->result ||
        key == ptrptr_alloca->result) {
      return key;
    }
    const auto it = pointer_values.find(key);
    if (it == pointer_values.end()) {
      return std::nullopt;
    }
    return it->second;
  };

  auto read_pointer_cell = [&](std::string_view ptr) -> std::optional<std::string> {
    const auto it = pointer_values.find(std::string(ptr));
    if (it == pointer_values.end()) {
      return std::nullopt;
    }
    return it->second;
  };

  auto resolve_scalar_target = [&](const auto& self,
                                   std::string_view ptr) -> std::optional<std::string> {
    const std::string key(ptr);
    if (key == scalar_alloca->result) {
      return key;
    }
    const auto pointee = read_pointer_cell(ptr);
    if (!pointee.has_value()) {
      return std::nullopt;
    }
    if (*pointee == scalar_alloca->result) {
      return *pointee;
    }
    return self(self, *pointee);
  };

  auto resolve_integer_value = [&](std::string_view value) -> std::optional<std::string> {
    if (parse_i64(value).has_value()) {
      return std::string(value);
    }
    const auto it = integer_values.find(std::string(value));
    if (it == integer_values.end()) {
      return std::nullopt;
    }
    return it->second;
  };

  std::string current_label = function.blocks.front().label;
  for (std::size_t steps = 0; steps < function.blocks.size() * 2; ++steps) {
    const auto block_it = blocks_by_label.find(current_label);
    if (block_it == blocks_by_label.end()) {
      return std::nullopt;
    }
    const auto& block = *block_it->second;

    for (const auto& inst : block.insts) {
      if (const auto* store = std::get_if<LirStoreOp>(&inst)) {
        if (store->type_str == "i32") {
          const auto target = resolve_scalar_target(resolve_scalar_target, store->ptr);
          if (!target.has_value()) {
            return std::nullopt;
          }
          scalar_values[*target] = store->val;
          continue;
        }

        if (store->type_str == "ptr") {
          const auto stored_pointer = resolve_pointer_rvalue(store->val);
          if (!stored_pointer.has_value()) {
            return std::nullopt;
          }
          pointer_values[store->ptr] = *stored_pointer;
          continue;
        }

        return std::nullopt;
      }

      if (const auto* load = std::get_if<LirLoadOp>(&inst)) {
        if (load->type_str == "ptr") {
          const auto pointee = read_pointer_cell(load->ptr);
          if (!pointee.has_value()) {
            return std::nullopt;
          }
          pointer_values[load->result] = *pointee;
          continue;
        }

        if (load->type_str == "i32") {
          const auto target = resolve_scalar_target(resolve_scalar_target, load->ptr);
          if (!target.has_value()) {
            return std::nullopt;
          }
          const auto value_it = scalar_values.find(*target);
          if (value_it == scalar_values.end()) {
            return std::nullopt;
          }
          integer_values[load->result] = value_it->second;
          continue;
        }

        return std::nullopt;
      }

      if (const auto* cmp = std::get_if<LirCmpOp>(&inst)) {
        if (cmp->is_float || !has_cmp_predicate(*cmp, LirCmpPredicate::Ne) ||
            cmp->type_str != "i32") {
          return std::nullopt;
        }
        const auto lhs = resolve_integer_value(cmp->lhs);
        const auto rhs = resolve_integer_value(cmp->rhs);
        if (!lhs.has_value() || !rhs.has_value()) {
          return std::nullopt;
        }
        const auto lhs_imm = parse_i64(*lhs);
        const auto rhs_imm = parse_i64(*rhs);
        if (!lhs_imm.has_value() || !rhs_imm.has_value()) {
          return std::nullopt;
        }
        predicate_values[cmp->result] = *lhs_imm != *rhs_imm;
        continue;
      }

      return std::nullopt;
    }

    if (const auto* ret = std::get_if<LirRet>(&block.terminator)) {
      if (!ret->value_str.has_value() || ret->type_str != "i32") {
        return std::nullopt;
      }
      const auto resolved = resolve_integer_value(*ret->value_str);
      if (!resolved.has_value()) {
        return std::nullopt;
      }

      BackendFunction out;
      out.signature = signature;
      BackendBlock out_block;
      out_block.label = "entry";
      out_block.terminator = BackendReturn{*resolved, "i32"};
      out.blocks.push_back(std::move(out_block));
      return out;
    }

    if (const auto* br = std::get_if<LirBr>(&block.terminator)) {
      current_label = br->target_label;
      continue;
    }

    const auto* condbr = std::get_if<LirCondBr>(&block.terminator);
    if (condbr == nullptr) {
      return std::nullopt;
    }
    const auto pred_it = predicate_values.find(condbr->cond_name);
    if (pred_it == predicate_values.end()) {
      return std::nullopt;
    }
    current_label = pred_it->second ? condbr->true_label : condbr->false_label;
  }

  return std::nullopt;
}

std::optional<BackendFunction> adapt_goto_only_constant_return_function(
    const c4c::codegen::lir::LirFunction& function,
    const BackendFunctionSignature& signature) {
  using namespace c4c::codegen::lir;

  if (function.is_declaration || signature.linkage != "define" ||
      signature.return_type != "i32" || signature.name != "main" ||
      !signature.params.empty() || signature.is_vararg || function.blocks.empty() ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  std::unordered_map<std::string, const LirBlock*> blocks_by_label;
  for (const auto& block : function.blocks) {
    if (!block.insts.empty()) {
      return std::nullopt;
    }
    blocks_by_label.emplace(block.label, &block);
  }

  std::unordered_map<std::string, bool> visited;
  std::string current_label = function.blocks.front().label;
  for (std::size_t steps = 0; steps < function.blocks.size(); ++steps) {
    if (visited.find(current_label) != visited.end()) {
      return std::nullopt;
    }
    visited.emplace(current_label, true);

    const auto block_it = blocks_by_label.find(current_label);
    if (block_it == blocks_by_label.end()) {
      return std::nullopt;
    }
    const auto& block = *block_it->second;

    if (const auto* ret = std::get_if<LirRet>(&block.terminator)) {
      if (!ret->value_str.has_value() || ret->type_str != "i32" ||
          !parse_i64(*ret->value_str).has_value()) {
        return std::nullopt;
      }

      BackendFunction out;
      out.signature = signature;
      BackendBlock out_block;
      out_block.label = "entry";
      out_block.terminator = BackendReturn{*ret->value_str, "i32"};
      out.blocks.push_back(std::move(out_block));
      return out;
    }

    const auto* br = std::get_if<LirBr>(&block.terminator);
    if (br == nullptr) {
      return std::nullopt;
    }
    current_label = br->target_label;
  }

  return std::nullopt;
}

std::optional<BackendFunction> adapt_single_local_countdown_loop_function(
    const c4c::codegen::lir::LirFunction& function,
    const BackendFunctionSignature& signature) {
  using namespace c4c::codegen::lir;

  if (function.is_declaration || signature.linkage != "define" ||
      signature.return_type != "i32" || signature.name != "main" ||
      !signature.params.empty() || signature.is_vararg || function.blocks.empty() ||
      function.alloca_insts.size() != 1 || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* scalar_alloca = std::get_if<LirAllocaOp>(&function.alloca_insts.front());
  if (scalar_alloca == nullptr || scalar_alloca->type_str != "i32" ||
      scalar_alloca->result.empty()) {
    return std::nullopt;
  }

  std::size_t static_store_count = 0;
  std::size_t static_cmp_count = 0;
  std::size_t static_sub_count = 0;
  std::size_t static_ret_count = 0;
  std::optional<std::int64_t> initial_value;

  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      if (const auto* store = std::get_if<LirStoreOp>(&inst)) {
        if (store->type_str != "i32" || store->ptr != scalar_alloca->result) {
          return std::nullopt;
        }
        ++static_store_count;
        if (block.label == "entry") {
          if (initial_value.has_value()) {
            return std::nullopt;
          }
          initial_value = parse_i64(store->val);
          if (!initial_value.has_value()) {
            return std::nullopt;
          }
        }
        continue;
      }

      if (const auto* load = std::get_if<LirLoadOp>(&inst)) {
        if (load->type_str != "i32" || load->ptr != scalar_alloca->result) {
          return std::nullopt;
        }
        continue;
      }

      if (const auto* bin = std::get_if<LirBinOp>(&inst)) {
        if (!has_binary_opcode(*bin, LirBinaryOpcode::Sub) || bin->type_str != "i32" ||
            bin->rhs != "1") {
          return std::nullopt;
        }
        ++static_sub_count;
        continue;
      }

      if (const auto* cmp = std::get_if<LirCmpOp>(&inst)) {
        if (cmp->is_float || !has_cmp_predicate(*cmp, LirCmpPredicate::Ne) ||
            cmp->type_str != "i32") {
          return std::nullopt;
        }
        ++static_cmp_count;
        continue;
      }

      return std::nullopt;
    }

    if (std::holds_alternative<LirRet>(block.terminator)) {
      ++static_ret_count;
      continue;
    }
    if (std::holds_alternative<LirBr>(block.terminator) ||
        std::holds_alternative<LirCondBr>(block.terminator)) {
      continue;
    }
    return std::nullopt;
  }

  if (!initial_value.has_value() || *initial_value < 0 || static_store_count != 2 ||
      static_cmp_count != 1 || static_sub_count != 1 || static_ret_count != 1) {
    return std::nullopt;
  }

  std::unordered_map<std::string, const LirBlock*> blocks_by_label;
  for (const auto& block : function.blocks) {
    blocks_by_label.emplace(block.label, &block);
  }

  std::int64_t scalar_value = *initial_value;
  std::unordered_map<std::string, std::int64_t> integer_values;
  std::unordered_map<std::string, bool> predicate_values;
  std::string current_label = function.blocks.front().label;
  const std::size_t max_steps =
      (static_cast<std::size_t>(*initial_value) + 1) * (function.blocks.size() + 1);

  auto resolve_integer_value = [&](std::string_view value) -> std::optional<std::int64_t> {
    if (const auto imm = parse_i64(value); imm.has_value()) {
      return *imm;
    }
    const auto it = integer_values.find(std::string(value));
    if (it == integer_values.end()) {
      return std::nullopt;
    }
    return it->second;
  };

  for (std::size_t step = 0; step < max_steps; ++step) {
    integer_values.clear();
    predicate_values.clear();

    const auto block_it = blocks_by_label.find(current_label);
    if (block_it == blocks_by_label.end()) {
      return std::nullopt;
    }
    const auto& block = *block_it->second;

    for (const auto& inst : block.insts) {
      if (const auto* store = std::get_if<LirStoreOp>(&inst)) {
        const auto value = resolve_integer_value(store->val);
        if (!value.has_value()) {
          return std::nullopt;
        }
        scalar_value = *value;
        continue;
      }

      if (const auto* load = std::get_if<LirLoadOp>(&inst)) {
        integer_values[load->result] = scalar_value;
        continue;
      }

      if (const auto* bin = std::get_if<LirBinOp>(&inst)) {
        const auto lhs = resolve_integer_value(bin->lhs);
        const auto rhs = resolve_integer_value(bin->rhs);
        if (!lhs.has_value() || !rhs.has_value()) {
          return std::nullopt;
        }
        integer_values[bin->result] = *lhs - *rhs;
        continue;
      }

      const auto* cmp = std::get_if<LirCmpOp>(&inst);
      if (cmp == nullptr) {
        return std::nullopt;
      }
      const auto lhs = resolve_integer_value(cmp->lhs);
      const auto rhs = resolve_integer_value(cmp->rhs);
      if (!lhs.has_value() || !rhs.has_value()) {
        return std::nullopt;
      }
      predicate_values[cmp->result] = *lhs != *rhs;
    }

    if (const auto* ret = std::get_if<LirRet>(&block.terminator)) {
      if (!ret->value_str.has_value() || ret->type_str != "i32") {
        return std::nullopt;
      }
      const auto resolved = resolve_integer_value(*ret->value_str);
      if (!resolved.has_value() || *resolved != 0) {
        return std::nullopt;
      }

      BackendFunction out;
      out.signature = signature;
      BackendBlock out_block;
      out_block.label = "entry";
      out_block.terminator = BackendReturn{"0", "i32"};
      out.blocks.push_back(std::move(out_block));
      return out;
    }

    if (const auto* br = std::get_if<LirBr>(&block.terminator)) {
      current_label = br->target_label;
      continue;
    }

    const auto* condbr = std::get_if<LirCondBr>(&block.terminator);
    if (condbr == nullptr) {
      return std::nullopt;
    }
    const auto pred_it = predicate_values.find(condbr->cond_name);
    if (pred_it == predicate_values.end()) {
      return std::nullopt;
    }
    current_label = pred_it->second ? condbr->true_label : condbr->false_label;
  }

  return std::nullopt;
}

std::optional<BackendFunction> adapt_countdown_while_loop_function(
    const c4c::codegen::lir::LirFunction& function,
    const BackendFunctionSignature& signature) {
  using namespace c4c::codegen::lir;

  if (function.is_declaration || signature.linkage != "define" ||
      signature.return_type != "i32" || signature.name != "main" ||
      !signature.params.empty() || signature.is_vararg || function.blocks.size() != 4 ||
      function.alloca_insts.size() != 1 || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* scalar_alloca = std::get_if<LirAllocaOp>(&function.alloca_insts.front());
  if (scalar_alloca == nullptr || scalar_alloca->type_str != "i32" ||
      scalar_alloca->result.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks[0];
  const auto& loop = function.blocks[1];
  const auto& body = function.blocks[2];
  const auto& exit = function.blocks[3];

  const auto* entry_store =
      entry.insts.size() == 1 ? std::get_if<LirStoreOp>(&entry.insts.front()) : nullptr;
  const auto* entry_br = std::get_if<LirBr>(&entry.terminator);
  if (entry.label != "entry" || entry_store == nullptr || entry_br == nullptr ||
      entry_store->type_str != "i32" || entry_store->ptr != scalar_alloca->result ||
      entry_br->target_label != loop.label) {
    return std::nullopt;
  }
  const auto initial_value = parse_i64(entry_store->val);
  if (!initial_value.has_value() || *initial_value < 0) {
    return std::nullopt;
  }

  const auto* loop_load =
      loop.insts.size() == 2 ? std::get_if<LirLoadOp>(&loop.insts[0]) : nullptr;
  const auto* loop_cmp =
      loop.insts.size() == 2 ? std::get_if<LirCmpOp>(&loop.insts[1]) : nullptr;
  const auto* loop_condbr = std::get_if<LirCondBr>(&loop.terminator);
  const auto loop_predicate = loop_cmp == nullptr ? std::nullopt : loop_cmp->predicate.typed();
  if (loop_load == nullptr || loop_cmp == nullptr || loop_condbr == nullptr ||
      loop_load->type_str != "i32" || loop_load->ptr != scalar_alloca->result ||
      loop_cmp->is_float || loop_predicate != LirCmpPredicate::Ne ||
      loop_cmp->type_str != "i32" || loop_cmp->lhs != loop_load->result ||
      loop_cmp->rhs != "0" || loop_condbr->cond_name != loop_cmp->result ||
      loop_condbr->true_label != body.label || loop_condbr->false_label != exit.label) {
    return std::nullopt;
  }

  const auto* body_load =
      body.insts.size() == 3 ? std::get_if<LirLoadOp>(&body.insts[0]) : nullptr;
  const auto* body_sub =
      body.insts.size() == 3 ? std::get_if<LirBinOp>(&body.insts[1]) : nullptr;
  const auto* body_store =
      body.insts.size() == 3 ? std::get_if<LirStoreOp>(&body.insts[2]) : nullptr;
  const auto* body_br = std::get_if<LirBr>(&body.terminator);
  if (body_load == nullptr || body_sub == nullptr || body_store == nullptr ||
      body_br == nullptr || body_load->type_str != "i32" ||
      body_load->ptr != scalar_alloca->result ||
      !has_binary_opcode(*body_sub, LirBinaryOpcode::Sub) || body_sub->type_str != "i32" ||
      body_sub->lhs != body_load->result || body_sub->rhs != "1" ||
      body_store->type_str != "i32" || body_store->ptr != scalar_alloca->result ||
      body_store->val != body_sub->result || body_br->target_label != loop.label) {
    return std::nullopt;
  }

  const auto* exit_load =
      exit.insts.size() == 1 ? std::get_if<LirLoadOp>(&exit.insts.front()) : nullptr;
  const auto* exit_ret = std::get_if<LirRet>(&exit.terminator);
  if (exit_load == nullptr || exit_ret == nullptr || exit_load->type_str != "i32" ||
      exit_load->ptr != scalar_alloca->result || !exit_ret->value_str.has_value() ||
      *exit_ret->value_str != exit_load->result || exit_ret->type_str != "i32") {
    return std::nullopt;
  }

  BackendFunction out;
  out.signature = signature;

  BackendBlock out_entry;
  out_entry.label = entry.label;
  out_entry.terminator = BackendTerminator::make_branch(loop.label);

  BackendBlock out_loop;
  out_loop.label = loop.label;
  out_loop.insts.push_back(BackendPhiInst{
      "%t.iter",
      "i32",
      {
          BackendPhiIncoming{entry_store->val, entry.label},
          BackendPhiIncoming{"%t.dec", body.label},
      },
  });
  out_loop.insts.push_back(BackendCompareInst{
      BackendComparePredicate::Ne,
      "%t.keep_going",
      "i32",
      "%t.iter",
      "0",
  });
  out_loop.terminator =
      BackendTerminator::make_cond_branch("%t.keep_going", body.label, exit.label);

  BackendBlock out_body;
  out_body.label = body.label;
  out_body.insts.push_back(BackendBinaryInst{
      BackendBinaryOpcode::Sub,
      "%t.dec",
      "i32",
      "%t.iter",
      "1",
  });
  out_body.terminator = BackendTerminator::make_branch(loop.label);

  BackendBlock out_exit;
  out_exit.label = exit.label;
  out_exit.terminator = BackendReturn{"%t.iter", "i32"};

  out.blocks.push_back(std::move(out_entry));
  out.blocks.push_back(std::move(out_loop));
  out.blocks.push_back(std::move(out_body));
  out.blocks.push_back(std::move(out_exit));
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
      *ret->value_str != call->result || call->return_type != "i32") {
    return std::nullopt;
  }

  const auto parsed_call = parse_backend_typed_call(*call);
  const auto call_operand = parse_backend_single_typed_call_operand(*call, "i32");
  if (!parsed_call.has_value() || !call_operand.has_value() || *call_operand != load->result) {
    return std::nullopt;
  }

  BackendFunction out;
  out.signature = signature;
  BackendBlock out_block;
  out_block.label = block.label;
  auto normalized_call = make_backend_call_inst(call->result.str(),
                                                call->return_type.str(),
                                                call->callee.str(),
                                                *parsed_call,
                                                true);
  normalized_call.args.front().operand = store->val.str();
  out_block.insts.push_back(std::move(normalized_call));
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

  std::optional<BackendCallInst> normalized_call;
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
          load_before->ptr != alloca->result ||
          !has_binary_opcode(*add, LirBinaryOpcode::Add) || add->type_str != "i32" ||
          add->result.empty() ||
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
        *ret->value_str != call->result || call->return_type != "i32") {
      return std::nullopt;
    }

    const auto parsed_call = parse_backend_typed_call(*call);
    const auto call_operands = parse_backend_two_typed_call_operands(*call, "i32", "i32");
    if (!parsed_call.has_value() || !call_operands.has_value()) {
      return std::nullopt;
    }

    normalized_call = make_backend_call_inst(call->result.str(),
                                             call->return_type.str(),
                                             call->callee.str(),
                                             *parsed_call,
                                             true);
    bool replaced_local = false;
    for (auto& arg : normalized_call->args) {
      if (arg.operand == call_arg_load->result) {
        arg.operand = store->val.str();
        replaced_local = true;
      }
    }
    if (!replaced_local) {
      return std::nullopt;
    }
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
      if (load.type_str != "i32" || load.ptr != slot ||
          !has_binary_opcode(add, LirBinaryOpcode::Add) ||
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
        if (load.type_str != "i32" || load.ptr != slot ||
            !has_binary_opcode(add, LirBinaryOpcode::Add) ||
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
        call->return_type != "i32") {
      return std::nullopt;
    }

    const auto parsed_call = parse_backend_typed_call(*call);
    const auto call_operands = parse_backend_two_typed_call_operands(*call, "i32", "i32");
    if (!parsed_call.has_value() || !call_operands.has_value() ||
        call_operands->first != call_arg_load0->result ||
        call_operands->second != call_arg_load1->result) {
      return std::nullopt;
    }

    normalized_call = make_backend_call_inst(call->result.str(),
                                             call->return_type.str(),
                                             call->callee.str(),
                                             *parsed_call,
                                             true);
    normalized_call->args[0].operand = store0->val.str();
    normalized_call->args[1].operand = store1->val.str();
  }

  BackendFunction out;
  out.signature = signature;
  BackendBlock out_block;
  out_block.label = block.label;
  if (!normalized_call.has_value()) {
    return std::nullopt;
  }
  out_block.insts.push_back(std::move(*normalized_call));
  out_block.terminator = BackendReturn{*ret->value_str, "i32"};
  out.blocks.push_back(std::move(out_block));
  return out;
}

std::optional<BackendFunction> adapt_direct_global_load_function(
    const c4c::codegen::lir::LirFunction& function,
    const c4c::codegen::lir::LirModule& module,
    const BackendFunctionSignature& signature) {
  using namespace c4c::codegen::lir;

  if (function.is_declaration || signature.linkage != "define" ||
      signature.return_type != "i32" || signature.name != "main" ||
      !signature.params.empty() || signature.is_vararg || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  const auto* ret = std::get_if<LirRet>(&block.terminator);
  const auto* load = block.insts.size() == 1
                         ? std::get_if<LirLoadOp>(&block.insts.front())
                         : nullptr;
  const std::string load_ptr = load == nullptr ? std::string{} : load->ptr.str();
  if (block.label != "entry" || load == nullptr || ret == nullptr ||
      load->type_str != "i32" || !ret->value_str.has_value() ||
      *ret->value_str != load->result || ret->type_str != "i32" ||
      load_ptr.size() < 2 || load_ptr.front() != '@') {
    return std::nullopt;
  }

  const auto* global = find_lir_global(module, std::string_view(load_ptr).substr(1));
  if (global == nullptr || global->llvm_type != "i32") {
    return std::nullopt;
  }

  BackendFunction out;
  out.signature = signature;
  BackendBlock out_block;
  out_block.label = block.label;
  out_block.insts.push_back(
      BackendLoadInst{
          load->result,
          load->type_str,
          load->type_str,
          BackendAddress{global->name, 0},
      });
  out_block.terminator = BackendReturn{*ret->value_str, "i32"};
  out.blocks.push_back(std::move(out_block));
  return out;
}

std::optional<BackendFunction> adapt_direct_global_store_reload_function(
    const c4c::codegen::lir::LirFunction& function,
    const c4c::codegen::lir::LirModule& module,
    const BackendFunctionSignature& signature) {
  using namespace c4c::codegen::lir;

  if (function.is_declaration || signature.linkage != "define" ||
      signature.return_type != "i32" || signature.name != "main" ||
      !signature.params.empty() || signature.is_vararg || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry" || block.insts.size() != 2) {
    return std::nullopt;
  }

  const auto* store = std::get_if<LirStoreOp>(&block.insts[0]);
  const auto* load = std::get_if<LirLoadOp>(&block.insts[1]);
  const auto* ret = std::get_if<LirRet>(&block.terminator);
  if (store == nullptr || load == nullptr || ret == nullptr ||
      store->type_str != "i32" || load->type_str != "i32" ||
      !ret->value_str.has_value() || *ret->value_str != load->result ||
      ret->type_str != "i32") {
    return std::nullopt;
  }

  const std::string store_ptr = store->ptr.str();
  if (store_ptr.size() < 2 || store_ptr.front() != '@' || load->ptr != store_ptr) {
    return std::nullopt;
  }

  const auto* global = find_lir_global(module, std::string_view(store_ptr).substr(1));
  if (global == nullptr || global->is_extern_decl || global->llvm_type != "i32") {
    return std::nullopt;
  }

  BackendFunction out;
  out.signature = signature;
  BackendBlock out_block;
  out_block.label = block.label;
  out_block.insts.push_back(BackendStoreInst{
      store->type_str,
      store->val,
      BackendAddress{global->name, 0},
  });
  out_block.insts.push_back(BackendLoadInst{
      load->result,
      load->type_str,
      load->type_str,
      BackendAddress{global->name, 0},
  });
  out_block.terminator = BackendReturn{*ret->value_str, "i32"};
  out.blocks.push_back(std::move(out_block));
  return out;
}

std::optional<BackendFunction> adapt_global_int_pointer_roundtrip_function(
    const c4c::codegen::lir::LirFunction& function,
    const c4c::codegen::lir::LirModule& module,
    const BackendFunctionSignature& signature) {
  using namespace c4c::codegen::lir;

  if (function.is_declaration || signature.linkage != "define" ||
      signature.return_type != "i32" || signature.name != "main" ||
      !signature.params.empty() || signature.is_vararg || function.blocks.size() != 1 ||
      function.alloca_insts.size() != 2 || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto* addr_alloca = std::get_if<LirAllocaOp>(&function.alloca_insts[0]);
  const auto* ptr_alloca = std::get_if<LirAllocaOp>(&function.alloca_insts[1]);
  if (addr_alloca == nullptr || ptr_alloca == nullptr ||
      addr_alloca->result != "%lv.addr" || addr_alloca->type_str != "i64" ||
      ptr_alloca->result != "%lv.p" || ptr_alloca->type_str != "ptr") {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry" || block.insts.size() != 7) {
    return std::nullopt;
  }

  const auto* ptrtoint = std::get_if<LirCastOp>(&block.insts[0]);
  const auto* store_addr = std::get_if<LirStoreOp>(&block.insts[1]);
  const auto* reload_addr = std::get_if<LirLoadOp>(&block.insts[2]);
  const auto* inttoptr = std::get_if<LirCastOp>(&block.insts[3]);
  const auto* store_ptr = std::get_if<LirStoreOp>(&block.insts[4]);
  const auto* reload_ptr = std::get_if<LirLoadOp>(&block.insts[5]);
  const auto* final_load = std::get_if<LirLoadOp>(&block.insts[6]);
  const auto* ret = std::get_if<LirRet>(&block.terminator);
  if (ptrtoint == nullptr || store_addr == nullptr || reload_addr == nullptr ||
      inttoptr == nullptr || store_ptr == nullptr || reload_ptr == nullptr ||
      final_load == nullptr || ret == nullptr || !ret->value_str.has_value() ||
      *ret->value_str != final_load->result || ret->type_str != "i32") {
    return std::nullopt;
  }

  const std::string global_ptr = ptrtoint->operand;
  if (ptrtoint->kind != LirCastKind::PtrToInt || ptrtoint->from_type != "ptr" ||
      ptrtoint->to_type != "i64" || global_ptr.size() < 2 || global_ptr.front() != '@') {
    return std::nullopt;
  }

  const auto* global = find_lir_global(module, std::string_view(global_ptr).substr(1));
  if (global == nullptr || global->llvm_type != "i32" || global->is_extern_decl ||
      final_load->type_str != "i32") {
    return std::nullopt;
  }

  if (store_addr->type_str != "i64" || store_addr->val != ptrtoint->result ||
      store_addr->ptr != addr_alloca->result || reload_addr->type_str != "i64" ||
      reload_addr->ptr != addr_alloca->result || inttoptr->kind != LirCastKind::IntToPtr ||
      inttoptr->from_type != "i64" || inttoptr->operand != reload_addr->result ||
      inttoptr->to_type != "ptr" || store_ptr->type_str != "ptr" ||
      store_ptr->val != inttoptr->result || store_ptr->ptr != ptr_alloca->result ||
      reload_ptr->type_str != "ptr" || reload_ptr->ptr != ptr_alloca->result ||
      final_load->ptr != reload_ptr->result) {
    return std::nullopt;
  }

  BackendFunction out;
  out.signature = signature;
  BackendBlock out_block;
  out_block.label = block.label;
  out_block.insts.push_back(
      BackendLoadInst{
          final_load->result,
          final_load->type_str,
          final_load->type_str,
          BackendAddress{global->name, 0},
      });
  out_block.terminator = BackendReturn{*ret->value_str, "i32"};
  out.blocks.push_back(std::move(out_block));
  return out;
}

std::optional<BackendFunction> adapt_indexed_global_array_load_function(
    const c4c::codegen::lir::LirFunction& function,
    const c4c::codegen::lir::LirModule& module,
    const BackendFunctionSignature& signature) {
  using namespace c4c::codegen::lir;

  if (function.is_declaration || signature.linkage != "define" ||
      signature.return_type != "i32" || signature.name != "main" ||
      !signature.params.empty() || signature.is_vararg || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry" || block.insts.size() != 4) {
    return std::nullopt;
  }

  const auto* base_gep = std::get_if<LirGepOp>(&block.insts[0]);
  const auto* index_cast = std::get_if<LirCastOp>(&block.insts[1]);
  const auto* elem_gep = std::get_if<LirGepOp>(&block.insts[2]);
  const auto* load = std::get_if<LirLoadOp>(&block.insts[3]);
  const auto* ret = std::get_if<LirRet>(&block.terminator);
  if (base_gep == nullptr || index_cast == nullptr || elem_gep == nullptr ||
      load == nullptr || ret == nullptr || !ret->value_str.has_value() ||
      *ret->value_str != load->result || ret->type_str != "i32") {
    return std::nullopt;
  }

  const std::string base_ptr = base_gep->ptr.str();
  if (base_ptr.size() < 2 || base_ptr.front() != '@' ||
      base_gep->indices.size() != 2 || base_gep->indices[0] != "i64 0" ||
      base_gep->indices[1] != "i64 0" || index_cast->kind != LirCastKind::SExt ||
      index_cast->from_type != "i32" || index_cast->to_type != "i64" ||
      elem_gep->ptr != base_gep->result || elem_gep->element_type != "i32" ||
      elem_gep->indices.size() != 1 ||
      elem_gep->indices[0] != ("i64 " + index_cast->result) ||
      load->type_str != "i32" || load->ptr != elem_gep->result) {
    return std::nullopt;
  }

  const auto* global = find_lir_global(module, std::string_view(base_ptr).substr(1));
  if (global == nullptr || global->llvm_type.size() < 7 ||
      global->llvm_type.substr(global->llvm_type.size() - 7) != " x i32]") {
    return std::nullopt;
  }

  const auto element_index = parse_i64(index_cast->operand);
  if (!element_index.has_value() || *element_index < 0) {
    return std::nullopt;
  }

  BackendFunction out;
  out.signature = signature;
  BackendBlock out_block;
  out_block.label = block.label;
  out_block.insts.push_back(BackendLoadInst{
      load->result,
      load->type_str,
      load->type_str,
      BackendAddress{global->name, *element_index * 4},
  });
  out_block.terminator = BackendReturn{*ret->value_str, "i32"};
  out.blocks.push_back(std::move(out_block));
  return out;
}

std::optional<BackendFunction> adapt_global_char_pointer_diff_function(
    const c4c::codegen::lir::LirFunction& function,
    const c4c::codegen::lir::LirModule& module,
    const BackendFunctionSignature& signature) {
  using namespace c4c::codegen::lir;

  if (function.is_declaration || signature.linkage != "define" ||
      signature.return_type != "i32" || signature.name != "main" ||
      !signature.params.empty() || signature.is_vararg || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  if (module.globals.size() != 1 || !module.string_pool.empty() ||
      !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (global.is_internal || global.is_const || global.is_extern_decl ||
      global.linkage_vis != "" || global.qualifier != "global ") {
    return std::nullopt;
  }

  const std::string array_prefix = "[";
  const std::string array_suffix = " x i8]";
  if (global.llvm_type.size() <= array_prefix.size() + array_suffix.size() ||
      global.llvm_type.substr(0, array_prefix.size()) != array_prefix ||
      global.llvm_type.substr(global.llvm_type.size() - array_suffix.size()) !=
          array_suffix) {
    return std::nullopt;
  }
  const auto global_size_text = global.llvm_type.substr(
      array_prefix.size(),
      global.llvm_type.size() - array_prefix.size() - array_suffix.size());
  const auto global_size = parse_i64(global_size_text);
  if (!global_size.has_value() || *global_size < 2) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry" || block.insts.size() != 12) {
    return std::nullopt;
  }

  const auto* base_gep1 = std::get_if<LirGepOp>(&block.insts[0]);
  const auto* index1 = std::get_if<LirCastOp>(&block.insts[1]);
  const auto* byte_gep1 = std::get_if<LirGepOp>(&block.insts[2]);
  const auto* base_gep0 = std::get_if<LirGepOp>(&block.insts[3]);
  const auto* index0 = std::get_if<LirCastOp>(&block.insts[4]);
  const auto* byte_gep0 = std::get_if<LirGepOp>(&block.insts[5]);
  const auto* ptrtoint1 = std::get_if<LirCastOp>(&block.insts[6]);
  const auto* ptrtoint0 = std::get_if<LirCastOp>(&block.insts[7]);
  const auto* diff = std::get_if<LirBinOp>(&block.insts[8]);
  const auto* expected_diff = std::get_if<LirCastOp>(&block.insts[9]);
  const auto* cmp = std::get_if<LirCmpOp>(&block.insts[10]);
  const auto* extend = std::get_if<LirCastOp>(&block.insts[11]);
  const auto* ret = std::get_if<LirRet>(&block.terminator);
  if (base_gep1 == nullptr || index1 == nullptr || byte_gep1 == nullptr ||
      base_gep0 == nullptr || index0 == nullptr || byte_gep0 == nullptr ||
      ptrtoint1 == nullptr || ptrtoint0 == nullptr || diff == nullptr ||
      expected_diff == nullptr || cmp == nullptr || extend == nullptr ||
      ret == nullptr || !ret->value_str.has_value()) {
    return std::nullopt;
  }

  const std::string global_ptr = "@" + global.name;
  if (base_gep1->element_type != global.llvm_type || base_gep1->ptr != global_ptr ||
      base_gep1->indices.size() != 2 || base_gep1->indices[0] != "i64 0" ||
      base_gep1->indices[1] != "i64 0" || base_gep0->element_type != global.llvm_type ||
      base_gep0->ptr != global_ptr || base_gep0->indices.size() != 2 ||
      base_gep0->indices[0] != "i64 0" || base_gep0->indices[1] != "i64 0") {
    return std::nullopt;
  }

  const auto byte_offset = parse_i64(index1->operand);
  const auto zero_offset = parse_i64(index0->operand);
  const auto expected_value = parse_i64(expected_diff->operand);
  if (!byte_offset.has_value() || !zero_offset.has_value() ||
      !expected_value.has_value() || *byte_offset <= 0 || *zero_offset != 0 ||
      *byte_offset >= *global_size || *expected_value != *byte_offset) {
    return std::nullopt;
  }

  if (index1->kind != LirCastKind::SExt || index1->from_type != "i32" ||
      index1->to_type != "i64" || index0->kind != LirCastKind::SExt ||
      index0->from_type != "i32" || index0->to_type != "i64" ||
      byte_gep1->element_type != "i8" || byte_gep1->ptr != base_gep1->result ||
      byte_gep1->indices.size() != 1 ||
      byte_gep1->indices[0] != ("i64 " + index1->result) ||
      byte_gep0->element_type != "i8" || byte_gep0->ptr != base_gep0->result ||
      byte_gep0->indices.size() != 1 ||
      byte_gep0->indices[0] != ("i64 " + index0->result) ||
      ptrtoint1->kind != LirCastKind::PtrToInt || ptrtoint1->from_type != "ptr" ||
      ptrtoint1->operand != byte_gep1->result || ptrtoint1->to_type != "i64" ||
      ptrtoint0->kind != LirCastKind::PtrToInt || ptrtoint0->from_type != "ptr" ||
      ptrtoint0->operand != byte_gep0->result || ptrtoint0->to_type != "i64") {
    return std::nullopt;
  }

  if (diff->opcode.typed() != LirBinaryOpcode::Sub || diff->type_str != "i64" ||
      diff->lhs != ptrtoint1->result || diff->rhs != ptrtoint0->result ||
      expected_diff->kind != LirCastKind::SExt || expected_diff->from_type != "i32" ||
      expected_diff->to_type != "i64" ||
      cmp->predicate.typed() != LirCmpPredicate::Eq || cmp->is_float ||
      cmp->type_str != "i64" || cmp->lhs != diff->result ||
      cmp->rhs != expected_diff->result || extend->kind != LirCastKind::ZExt ||
      extend->from_type != "i1" || extend->operand != cmp->result ||
      extend->to_type != "i32" || *ret->value_str != extend->result ||
      ret->type_str != "i32") {
    return std::nullopt;
  }

  BackendFunction out;
  out.signature = signature;
  BackendBlock out_block;
  out_block.label = block.label;
  out_block.insts.push_back(BackendPtrDiffEqInst{
      extend->result,
      "i32",
      BackendAddress{global.name, *byte_offset},
      BackendAddress{global.name, 0},
      1,
      *expected_value,
  });
  out_block.terminator = BackendReturn{*ret->value_str, "i32"};
  out.blocks.push_back(std::move(out_block));
  return out;
}

std::optional<BackendFunction> adapt_global_int_pointer_diff_function(
    const c4c::codegen::lir::LirFunction& function,
    const c4c::codegen::lir::LirModule& module,
    const BackendFunctionSignature& signature) {
  using namespace c4c::codegen::lir;

  if (function.is_declaration || signature.linkage != "define" ||
      signature.return_type != "i32" || signature.name != "main" ||
      !signature.params.empty() || signature.is_vararg || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  if (module.globals.size() != 1 || !module.string_pool.empty() ||
      !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto& global = module.globals.front();
  if (global.is_internal || global.is_const || global.is_extern_decl ||
      global.linkage_vis != "" || global.qualifier != "global ") {
    return std::nullopt;
  }

  const std::string array_prefix = "[";
  const std::string array_suffix = " x i32]";
  if (global.llvm_type.size() <= array_prefix.size() + array_suffix.size() ||
      global.llvm_type.substr(0, array_prefix.size()) != array_prefix ||
      global.llvm_type.substr(global.llvm_type.size() - array_suffix.size()) !=
          array_suffix) {
    return std::nullopt;
  }
  const auto global_size_text = global.llvm_type.substr(
      array_prefix.size(),
      global.llvm_type.size() - array_prefix.size() - array_suffix.size());
  const auto global_size = parse_i64(global_size_text);
  if (!global_size.has_value() || *global_size < 2) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry" || block.insts.size() != 13) {
    return std::nullopt;
  }

  const auto* base_gep1 = std::get_if<LirGepOp>(&block.insts[0]);
  const auto* index1 = std::get_if<LirCastOp>(&block.insts[1]);
  const auto* elem_gep1 = std::get_if<LirGepOp>(&block.insts[2]);
  const auto* base_gep0 = std::get_if<LirGepOp>(&block.insts[3]);
  const auto* index0 = std::get_if<LirCastOp>(&block.insts[4]);
  const auto* elem_gep0 = std::get_if<LirGepOp>(&block.insts[5]);
  const auto* ptrtoint1 = std::get_if<LirCastOp>(&block.insts[6]);
  const auto* ptrtoint0 = std::get_if<LirCastOp>(&block.insts[7]);
  const auto* diff = std::get_if<LirBinOp>(&block.insts[8]);
  const auto* scaled_diff = std::get_if<LirBinOp>(&block.insts[9]);
  const auto* expected_diff = std::get_if<LirCastOp>(&block.insts[10]);
  const auto* cmp = std::get_if<LirCmpOp>(&block.insts[11]);
  const auto* extend = std::get_if<LirCastOp>(&block.insts[12]);
  const auto* ret = std::get_if<LirRet>(&block.terminator);
  if (base_gep1 == nullptr || index1 == nullptr || elem_gep1 == nullptr ||
      base_gep0 == nullptr || index0 == nullptr || elem_gep0 == nullptr ||
      ptrtoint1 == nullptr || ptrtoint0 == nullptr || diff == nullptr ||
      scaled_diff == nullptr || expected_diff == nullptr || cmp == nullptr ||
      extend == nullptr || ret == nullptr || !ret->value_str.has_value()) {
    return std::nullopt;
  }

  const std::string global_ptr = "@" + global.name;
  if (base_gep1->element_type != global.llvm_type || base_gep1->ptr != global_ptr ||
      base_gep1->indices.size() != 2 || base_gep1->indices[0] != "i64 0" ||
      base_gep1->indices[1] != "i64 0" || base_gep0->element_type != global.llvm_type ||
      base_gep0->ptr != global_ptr || base_gep0->indices.size() != 2 ||
      base_gep0->indices[0] != "i64 0" || base_gep0->indices[1] != "i64 0") {
    return std::nullopt;
  }

  const auto element_index = parse_i64(index1->operand);
  const auto zero_index = parse_i64(index0->operand);
  const auto element_size = parse_i64(scaled_diff->rhs);
  const auto expected_value = parse_i64(expected_diff->operand);
  if (!element_index.has_value() || !zero_index.has_value() ||
      !element_size.has_value() || !expected_value.has_value() ||
      *element_index <= 0 || *zero_index != 0 || *element_size <= 0 ||
      *expected_value != *element_index) {
    return std::nullopt;
  }

  if (index1->kind != LirCastKind::SExt || index1->from_type != "i32" ||
      index1->to_type != "i64" || index0->kind != LirCastKind::SExt ||
      index0->from_type != "i32" || index0->to_type != "i64" ||
      elem_gep1->element_type != "i32" || elem_gep1->ptr != base_gep1->result ||
      elem_gep1->indices.size() != 1 ||
      elem_gep1->indices[0] != ("i64 " + index1->result) ||
      elem_gep0->element_type != "i32" || elem_gep0->ptr != base_gep0->result ||
      elem_gep0->indices.size() != 1 ||
      elem_gep0->indices[0] != ("i64 " + index0->result) ||
      ptrtoint1->kind != LirCastKind::PtrToInt || ptrtoint1->from_type != "ptr" ||
      ptrtoint1->operand != elem_gep1->result || ptrtoint1->to_type != "i64" ||
      ptrtoint0->kind != LirCastKind::PtrToInt || ptrtoint0->from_type != "ptr" ||
      ptrtoint0->operand != elem_gep0->result || ptrtoint0->to_type != "i64") {
    return std::nullopt;
  }

  if (diff->opcode.typed() != LirBinaryOpcode::Sub || diff->type_str != "i64" ||
      diff->lhs != ptrtoint1->result || diff->rhs != ptrtoint0->result ||
      scaled_diff->opcode.typed() != LirBinaryOpcode::SDiv ||
      scaled_diff->type_str != "i64" || scaled_diff->lhs != diff->result ||
      expected_diff->kind != LirCastKind::SExt || expected_diff->from_type != "i32" ||
      expected_diff->to_type != "i64" ||
      cmp->predicate.typed() != LirCmpPredicate::Eq || cmp->is_float ||
      cmp->type_str != "i64" || cmp->lhs != scaled_diff->result ||
      cmp->rhs != expected_diff->result || extend->kind != LirCastKind::ZExt ||
      extend->from_type != "i1" || extend->operand != cmp->result ||
      extend->to_type != "i32" || *ret->value_str != extend->result ||
      ret->type_str != "i32") {
    return std::nullopt;
  }

  BackendFunction out;
  out.signature = signature;
  BackendBlock out_block;
  out_block.label = block.label;
  out_block.insts.push_back(BackendPtrDiffEqInst{
      extend->result,
      "i32",
      BackendAddress{global.name, *element_index * *element_size},
      BackendAddress{global.name, 0},
      *element_size,
      *expected_value,
  });
  out_block.terminator = BackendReturn{*ret->value_str, "i32"};
  out.blocks.push_back(std::move(out_block));
  return out;
}

BackendFunction adapt_function(const c4c::codegen::lir::LirFunction& function,
                               const c4c::codegen::lir::LirModule& module) {
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
  if (const auto normalized = adapt_local_temp_arithmetic_return_function(function, signature);
      normalized.has_value()) {
    return *normalized;
  }
  if (const auto normalized = adapt_local_pointer_temp_return_function(function, signature);
      normalized.has_value()) {
    return *normalized;
  }
  if (const auto normalized = adapt_conditional_return_function(function, signature);
      normalized.has_value()) {
    return *normalized;
  }
  if (const auto normalized = adapt_conditional_phi_join_function(function, signature);
      normalized.has_value()) {
    return *normalized;
  }
  if (const auto normalized =
          adapt_double_indirect_local_pointer_conditional_return_function(function, signature);
      normalized.has_value()) {
    return *normalized;
  }
  if (const auto normalized = adapt_goto_only_constant_return_function(function, signature);
      normalized.has_value()) {
    return *normalized;
  }
  if (const auto normalized = adapt_countdown_while_loop_function(function, signature);
      normalized.has_value()) {
    return *normalized;
  }
  if (const auto normalized = adapt_single_local_countdown_loop_function(function, signature);
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
  if (const auto normalized = adapt_string_literal_char_function(function, module, signature);
      normalized.has_value()) {
    return *normalized;
  }
  if (const auto normalized = adapt_direct_global_load_function(function, module, signature);
      normalized.has_value()) {
    return *normalized;
  }
  if (const auto normalized =
          adapt_direct_global_store_reload_function(function, module, signature);
      normalized.has_value()) {
    return *normalized;
  }
  if (const auto normalized =
          adapt_global_int_pointer_roundtrip_function(function, module, signature);
      normalized.has_value()) {
    return *normalized;
  }
  if (const auto normalized =
          adapt_global_char_pointer_diff_function(function, module, signature);
      normalized.has_value()) {
    return *normalized;
  }
  if (const auto normalized =
          adapt_global_int_pointer_diff_function(function, module, signature);
      normalized.has_value()) {
    return *normalized;
  }
  if (const auto normalized =
          adapt_indexed_global_array_load_function(function, module, signature);
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

}  // namespace

BackendModule adapt_minimal_module(const c4c::codegen::lir::LirModule& module) {
  if (module.need_va_start || module.need_va_end || module.need_va_copy ||
      module.need_memcpy || module.need_stacksave || module.need_stackrestore ||
      module.need_abs) {
    fail_unsupported("intrinsic declarations");
  }

  BackendModule out;
  out.target_triple = module.target_triple;
  out.data_layout = module.data_layout;
  out.type_decls = module.type_decls;
  for (const auto& global : module.globals) {
    out.globals.push_back(adapt_global(global));
  }
  for (const auto& string_const : module.string_pool) {
    if (string_const.pool_name.empty() || string_const.pool_name.front() != '@') {
      fail_unsupported("string constants without a global-style pool name");
    }
    out.string_constants.push_back(BackendStringConstant{
        string_const.pool_name.substr(1),
        string_const.raw_bytes,
        static_cast<std::size_t>(string_const.byte_length),
    });
  }
  for (const auto& decl : module.extern_decls) {
    out.functions.push_back(adapt_extern_decl(module, decl));
  }
  for (const auto& function : module.functions) {
    out.functions.push_back(adapt_function(function, module));
  }
  return out;
}

std::string render_module(const BackendModule& module) {
  return print_backend_ir(module);
}

std::optional<c4c::codegen::lir::ParsedLirDirectGlobalTypedCallView>
parse_backend_direct_global_typed_call(const c4c::codegen::lir::LirCallOp& call) {
  return c4c::codegen::lir::parse_lir_direct_global_typed_call(call);
}

std::optional<ParsedBackendTypedCallView> parse_backend_typed_call(
    const BackendCallInst& call) {
  return c4c::codegen::lir::borrow_lir_typed_call(call.param_types, call.args);
}

std::optional<ParsedBackendDirectGlobalTypedCallView> parse_backend_direct_global_typed_call(
    const BackendCallInst& call) {
  const auto symbol_name = c4c::codegen::lir::parse_lir_direct_global_callee(call.callee);
  if (!symbol_name.has_value()) {
    return std::nullopt;
  }
  const auto typed_call = parse_backend_typed_call(call);
  if (!typed_call.has_value()) {
    return std::nullopt;
  }
  return ParsedBackendDirectGlobalTypedCallView{*symbol_name, std::move(*typed_call)};
}

}  // namespace c4c::backend
