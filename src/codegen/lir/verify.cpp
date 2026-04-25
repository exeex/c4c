#include "ir.hpp"
#include "call_args_ops.hpp"

#include <sstream>
#include <unordered_map>

namespace c4c::codegen::lir {

namespace {

[[noreturn]] void fail_verify(std::string_view field, const std::string& detail) {
  throw LirVerifyError(LirVerifyErrorKind::Malformed,
                       std::string(field) + ": " + detail);
}

std::string operand_kind_name(LirOperandKind kind) {
  switch (kind) {
    case LirOperandKind::SsaValue: return "ssa";
    case LirOperandKind::Global: return "global";
    case LirOperandKind::Label: return "label";
    case LirOperandKind::Immediate: return "immediate";
    case LirOperandKind::SpecialToken: return "special-token";
    case LirOperandKind::RawText: return "raw-text";
  }
  return "unknown";
}

bool operand_kind_allowed(LirOperandKind kind,
                          std::initializer_list<LirOperandKind> allowed_kinds) {
  for (const auto allowed : allowed_kinds) {
    if (allowed == kind) return true;
  }
  if (kind == LirOperandKind::RawText) {
    for (const auto allowed : allowed_kinds) {
      if (allowed == LirOperandKind::Immediate) return true;
      if (allowed == LirOperandKind::Global) return true;
    }
  }
  if (kind == LirOperandKind::SpecialToken) {
    for (const auto allowed : allowed_kinds) {
      if (allowed == LirOperandKind::Global) return true;
    }
  }
  if (kind == LirOperandKind::Immediate) {
    for (const auto allowed : allowed_kinds) {
      if (allowed == LirOperandKind::Global) return true;
    }
  }
  return false;
}

std::optional<std::string> type_ref_mismatch_detail(const LirTypeRef& type) {
  const auto classified_kind = type.empty() ? LirTypeKind::RawText
                                            : LirTypeRef(type.str()).kind();
  if (type.kind() != classified_kind) {
    if (type.has_struct_name_id() && type.kind() == LirTypeKind::Struct) {
      return std::nullopt;
    }
    std::ostringstream detail;
    detail << "typed kind disagrees with text '" << type.str() << "'";
    return detail.str();
  }

  if (type.kind() == LirTypeKind::Integer) {
    const auto typed_width = type.integer_bit_width();
    const auto text_width = LirTypeRef(type.str()).integer_bit_width();
    if (typed_width != text_width) {
      std::ostringstream detail;
      detail << "typed integer width ";
      if (typed_width.has_value()) {
        detail << *typed_width;
      } else {
        detail << "<missing>";
      }
      detail << " disagrees with text '" << type.str() << "'";
      return detail.str();
    }
  }

  return std::nullopt;
}

std::optional<std::string> type_ref_struct_name_mismatch_detail(
    const StructNameTable& struct_names,
    const LirTypeRef& type) {
  if (!type.has_struct_name_id()) return std::nullopt;

  const std::string_view rendered_name =
      struct_names.spelling(type.struct_name_id());
  if (rendered_name.empty()) {
    return "StructNameId mirror must resolve to a struct name";
  }

  if (rendered_name != type.str()) {
    std::ostringstream detail;
    detail << "StructNameId mirror '" << rendered_name
           << "' disagrees with text '" << type.str() << "'";
    return detail.str();
  }

  return std::nullopt;
}

const std::string& require_module_type_ref(const LirModule& mod,
                                           const LirTypeRef& type,
                                           std::string_view field,
                                           bool allow_void = false) {
  const std::string& rendered = require_type_ref(type, field, allow_void);
  if (const auto mismatch =
          type_ref_struct_name_mismatch_detail(mod.struct_names, type);
      mismatch.has_value()) {
    fail_verify(field, *mismatch);
  }
  return rendered;
}

void verify_result_operand(const LirOperand& operand, std::string_view field) {
  require_operand_kind(operand, field, {LirOperandKind::SsaValue});
}

void verify_value_operand(const LirOperand& operand, std::string_view field) {
  require_operand_kind(operand, field,
                       {LirOperandKind::SsaValue,
                        LirOperandKind::Global,
                        LirOperandKind::Immediate,
                        LirOperandKind::SpecialToken});
}

void verify_pointer_operand(const LirOperand& operand, std::string_view field) {
  require_operand_kind(operand, field,
                       {LirOperandKind::SsaValue, LirOperandKind::Global});
}

void verify_optional_count_operand(const LirOperand& operand,
                                   std::string_view field) {
  require_operand_kind(operand, field,
                       {LirOperandKind::SsaValue, LirOperandKind::Immediate},
                       true);
}

void verify_inst(const LirModule& mod, const LirInst& inst) {
  if (const auto* op = std::get_if<LirMemcpyOp>(&inst)) {
    verify_pointer_operand(op->dst, "LirMemcpyOp.dst");
    verify_pointer_operand(op->src, "LirMemcpyOp.src");
    verify_value_operand(op->size, "LirMemcpyOp.size");
    return;
  }
  if (const auto* op = std::get_if<LirMemsetOp>(&inst)) {
    verify_pointer_operand(op->dst, "LirMemsetOp.dst");
    verify_value_operand(op->byte_val, "LirMemsetOp.byte_val");
    verify_value_operand(op->size, "LirMemsetOp.size");
    return;
  }
  if (const auto* op = std::get_if<LirVaStartOp>(&inst)) {
    verify_pointer_operand(op->ap_ptr, "LirVaStartOp.ap_ptr");
    return;
  }
  if (const auto* op = std::get_if<LirVaEndOp>(&inst)) {
    verify_pointer_operand(op->ap_ptr, "LirVaEndOp.ap_ptr");
    return;
  }
  if (const auto* op = std::get_if<LirVaCopyOp>(&inst)) {
    verify_pointer_operand(op->dst_ptr, "LirVaCopyOp.dst_ptr");
    verify_pointer_operand(op->src_ptr, "LirVaCopyOp.src_ptr");
    return;
  }
  if (const auto* op = std::get_if<LirStackSaveOp>(&inst)) {
    verify_result_operand(op->result, "LirStackSaveOp.result");
    return;
  }
  if (const auto* op = std::get_if<LirStackRestoreOp>(&inst)) {
    verify_pointer_operand(op->saved_ptr, "LirStackRestoreOp.saved_ptr");
    return;
  }
  if (const auto* op = std::get_if<LirAbsOp>(&inst)) {
    verify_result_operand(op->result, "LirAbsOp.result");
    verify_value_operand(op->arg, "LirAbsOp.arg");
    require_module_type_ref(mod, op->int_type, "LirAbsOp.int_type");
    return;
  }
  if (const auto* op = std::get_if<LirIndirectBrOp>(&inst)) {
    verify_pointer_operand(op->addr, "LirIndirectBrOp.addr");
    if (op->targets.empty()) {
      fail_verify("LirIndirectBrOp.targets", "must not be empty");
    }
    return;
  }
  if (const auto* op = std::get_if<LirExtractValueOp>(&inst)) {
    verify_result_operand(op->result, "LirExtractValueOp.result");
    require_module_type_ref(mod, op->agg_type, "LirExtractValueOp.agg_type");
    verify_value_operand(op->agg, "LirExtractValueOp.agg");
    return;
  }
  if (const auto* op = std::get_if<LirInsertValueOp>(&inst)) {
    verify_result_operand(op->result, "LirInsertValueOp.result");
    require_module_type_ref(mod, op->agg_type, "LirInsertValueOp.agg_type");
    verify_value_operand(op->agg, "LirInsertValueOp.agg");
    require_module_type_ref(mod, op->elem_type, "LirInsertValueOp.elem_type");
    verify_value_operand(op->elem, "LirInsertValueOp.elem");
    return;
  }
  if (const auto* op = std::get_if<LirLoadOp>(&inst)) {
    verify_result_operand(op->result, "LirLoadOp.result");
    require_module_type_ref(mod, op->type_str, "LirLoadOp.type_str", true);
    verify_pointer_operand(op->ptr, "LirLoadOp.ptr");
    return;
  }
  if (const auto* op = std::get_if<LirStoreOp>(&inst)) {
    require_module_type_ref(mod, op->type_str, "LirStoreOp.type_str", true);
    verify_value_operand(op->val, "LirStoreOp.val");
    verify_pointer_operand(op->ptr, "LirStoreOp.ptr");
    return;
  }
  if (const auto* op = std::get_if<LirCastOp>(&inst)) {
    verify_result_operand(op->result, "LirCastOp.result");
    require_module_type_ref(mod, op->from_type, "LirCastOp.from_type");
    verify_value_operand(op->operand, "LirCastOp.operand");
    require_module_type_ref(mod, op->to_type, "LirCastOp.to_type");
    return;
  }
  if (const auto* op = std::get_if<LirGepOp>(&inst)) {
    verify_result_operand(op->result, "LirGepOp.result");
    require_module_type_ref(mod, op->element_type, "LirGepOp.element_type");
    verify_pointer_operand(op->ptr, "LirGepOp.ptr");
    return;
  }
  if (const auto* op = std::get_if<LirCallOp>(&inst)) {
    require_operand_kind(op->result, "LirCallOp.result",
                         {LirOperandKind::SsaValue}, true);
    require_module_type_ref(mod, op->return_type, "LirCallOp.return_type", true);
    const StructNameId known_return_name_id = mod.struct_names.find(op->return_type.str());
    if (known_return_name_id != kInvalidStructName &&
        mod.find_struct_decl(known_return_name_id) &&
        op->return_type.struct_name_id() != known_return_name_id) {
      fail_verify("LirCallOp.return_type",
                  "known struct return type must carry matching StructNameId");
    }
    verify_pointer_operand(op->callee, "LirCallOp.callee");
    if (!op->arg_type_refs.empty()) {
      const auto parsed = parse_lir_typed_call_or_infer_params(*op);
      if (!parsed.has_value()) {
        fail_verify("LirCallOp.arg_type_refs",
                    "cannot validate argument mirrors against malformed call arguments");
      }
      if (op->arg_type_refs.size() != parsed->args.size()) {
        std::ostringstream detail;
        detail << "has " << op->arg_type_refs.size()
               << " argument mirrors but call text has " << parsed->args.size()
               << " arguments";
        fail_verify("LirCallOp.arg_type_refs", detail.str());
      }
      for (size_t index = 0; index < op->arg_type_refs.size(); ++index) {
        const std::string& shadow = require_module_type_ref(
            mod, op->arg_type_refs[index], "LirCallOp.arg_type_refs");
        if (shadow != parsed->args[index].type) {
          std::ostringstream detail;
          detail << "argument " << index << " mirror does not match call text; shadow '"
                 << shadow << "', call argument type '" << parsed->args[index].type << "'";
          fail_verify("LirCallOp.arg_type_refs", detail.str());
        }
        const StructNameId known_arg_name_id = mod.struct_names.find(shadow);
        if (known_arg_name_id != kInvalidStructName &&
            mod.find_struct_decl(known_arg_name_id) &&
            op->arg_type_refs[index].struct_name_id() != known_arg_name_id) {
          fail_verify("LirCallOp.arg_type_refs",
                      "known struct argument type must carry matching StructNameId");
        }
      }
    }
    if (op->result.empty() && op->return_type != "void") {
      fail_verify("LirCallOp.result",
                  "must hold an SSA result for non-void calls");
    }
    if (!op->result.empty() && op->return_type == "void") {
      fail_verify("LirCallOp.return_type",
                  "void calls must not carry a result operand");
    }
    return;
  }
  if (const auto* op = std::get_if<LirBinOp>(&inst)) {
    verify_result_operand(op->result, "LirBinOp.result");
    (void)render_binary_opcode(op->opcode, "LirBinOp.opcode");
    require_module_type_ref(mod, op->type_str, "LirBinOp.type_str", true);
    verify_value_operand(op->lhs, "LirBinOp.lhs");
    if (op->rhs.empty()) {
      if (op->opcode.typed() != LirBinaryOpcode::FNeg) {
        fail_verify("LirBinOp.rhs",
                    "must not be empty for non-unary binary instructions");
      }
    } else {
      verify_value_operand(op->rhs, "LirBinOp.rhs");
    }
    return;
  }
  if (const auto* op = std::get_if<LirCmpOp>(&inst)) {
    verify_result_operand(op->result, "LirCmpOp.result");
    (void)render_cmp_predicate(op->predicate, "LirCmpOp.predicate");
    require_module_type_ref(mod, op->type_str, "LirCmpOp.type_str");
    verify_value_operand(op->lhs, "LirCmpOp.lhs");
    verify_value_operand(op->rhs, "LirCmpOp.rhs");
    return;
  }
  if (const auto* op = std::get_if<LirPhiOp>(&inst)) {
    verify_result_operand(op->result, "LirPhiOp.result");
    require_module_type_ref(mod, op->type_str, "LirPhiOp.type_str");
    if (op->incoming.empty()) {
      fail_verify("LirPhiOp.incoming", "must not be empty");
    }
    return;
  }
  if (const auto* op = std::get_if<LirSelectOp>(&inst)) {
    verify_result_operand(op->result, "LirSelectOp.result");
    require_module_type_ref(mod, op->type_str, "LirSelectOp.type_str");
    verify_value_operand(op->cond, "LirSelectOp.cond");
    verify_value_operand(op->true_val, "LirSelectOp.true_val");
    verify_value_operand(op->false_val, "LirSelectOp.false_val");
    return;
  }
  if (const auto* op = std::get_if<LirInsertElementOp>(&inst)) {
    verify_result_operand(op->result, "LirInsertElementOp.result");
    require_module_type_ref(mod, op->vec_type, "LirInsertElementOp.vec_type");
    verify_value_operand(op->vec, "LirInsertElementOp.vec");
    require_module_type_ref(mod, op->elem_type, "LirInsertElementOp.elem_type");
    verify_value_operand(op->elem, "LirInsertElementOp.elem");
    verify_value_operand(op->index, "LirInsertElementOp.index");
    return;
  }
  if (const auto* op = std::get_if<LirExtractElementOp>(&inst)) {
    verify_result_operand(op->result, "LirExtractElementOp.result");
    require_module_type_ref(mod, op->vec_type, "LirExtractElementOp.vec_type");
    verify_value_operand(op->vec, "LirExtractElementOp.vec");
    require_module_type_ref(mod, op->index_type, "LirExtractElementOp.index_type");
    verify_value_operand(op->index, "LirExtractElementOp.index");
    return;
  }
  if (const auto* op = std::get_if<LirShuffleVectorOp>(&inst)) {
    verify_result_operand(op->result, "LirShuffleVectorOp.result");
    require_module_type_ref(mod, op->vec_type, "LirShuffleVectorOp.vec_type");
    verify_value_operand(op->vec1, "LirShuffleVectorOp.vec1");
    verify_value_operand(op->vec2, "LirShuffleVectorOp.vec2");
    require_module_type_ref(mod, op->mask_type, "LirShuffleVectorOp.mask_type");
    verify_value_operand(op->mask, "LirShuffleVectorOp.mask");
    return;
  }
  if (const auto* op = std::get_if<LirVaArgOp>(&inst)) {
    verify_result_operand(op->result, "LirVaArgOp.result");
    verify_pointer_operand(op->ap_ptr, "LirVaArgOp.ap_ptr");
    require_module_type_ref(mod, op->type_str, "LirVaArgOp.type_str");
    return;
  }
  if (const auto* op = std::get_if<LirAllocaOp>(&inst)) {
    verify_result_operand(op->result, "LirAllocaOp.result");
    require_module_type_ref(mod, op->type_str, "LirAllocaOp.type_str");
    verify_optional_count_operand(op->count, "LirAllocaOp.count");
    return;
  }
  if (const auto* op = std::get_if<LirInlineAsmOp>(&inst)) {
    require_operand_kind(op->result, "LirInlineAsmOp.result",
                         {LirOperandKind::SsaValue}, true);
    require_module_type_ref(mod, op->ret_type, "LirInlineAsmOp.ret_type", true);
    if (op->result.empty() && op->ret_type != "void") {
      fail_verify("LirInlineAsmOp.result",
                  "must hold an SSA result for non-void inline asm");
    }
    if (!op->result.empty() && op->ret_type == "void") {
      fail_verify("LirInlineAsmOp.ret_type",
                  "void inline asm must not carry a result operand");
    }
    return;
  }
}

void verify_terminator(const LirTerminator& terminator) {
  if (const auto* cbr = std::get_if<LirCondBr>(&terminator)) {
    const LirOperand cond(cbr->cond_name);
    require_operand_kind(cond, "LirCondBr.cond_name",
                         {LirOperandKind::SsaValue,
                          LirOperandKind::Immediate,
                          LirOperandKind::SpecialToken});
    return;
  }
  if (const auto* ret = std::get_if<LirRet>(&terminator)) {
    if (!ret->value_str.has_value() && !ret->type_str.empty() &&
        ret->type_str != "void") {
      fail_verify("LirRet.type_str",
                  "must be void when the return has no value");
    }
    if (ret->value_str.has_value() && ret->type_str.empty()) {
      fail_verify("LirRet.type_str",
                  "must not be empty when the return has a value");
    }
    return;
  }
  if (const auto* sw = std::get_if<LirSwitch>(&terminator)) {
    if (sw->selector_name.empty() || sw->selector_type.empty()) {
      fail_verify("LirSwitch", "must carry both selector name and selector type");
    }
    return;
  }
}

std::string_view legacy_type_decl_name(std::string_view decl) {
  constexpr std::string_view marker = " = type ";
  const std::size_t marker_pos = decl.find(marker);
  if (marker_pos == std::string_view::npos) return {};
  return decl.substr(0, marker_pos);
}

void verify_struct_decl_shadows(const LirModule& mod) {
  std::unordered_map<std::string_view, std::string_view> legacy_by_name;
  for (const auto& decl : mod.type_decls) {
    const std::string_view decl_view(decl);
    const std::string_view name = legacy_type_decl_name(decl_view);
    if (name.empty()) continue;
    legacy_by_name.emplace(name, decl_view);
  }

  for (const auto& decl : mod.struct_decls) {
    const std::string_view name = mod.struct_names.spelling(decl.name_id);
    if (name.empty()) {
      fail_verify("LirStructDecl.name_id", "must resolve to a struct name");
    }

    const auto legacy_it = legacy_by_name.find(name);
    if (legacy_it == legacy_by_name.end()) {
      fail_verify("LirStructDecl.shadow",
                  "missing legacy type_decls line for '" + std::string(name) + "'");
    }

    for (const auto& field : decl.fields) {
      require_module_type_ref(mod, field.type, "LirStructDecl.fields.type");
    }

    const std::string shadow = render_struct_decl_llvm(mod, decl);
    if (shadow != legacy_it->second) {
      std::ostringstream detail;
      detail << "structured declaration for '" << name
             << "' does not match legacy type_decls line; shadow '" << shadow
             << "', legacy '" << legacy_it->second << "'";
      fail_verify("LirStructDecl.shadow", detail.str());
    }
  }
}

void verify_extern_decl_shadows(const LirModule& mod) {
  for (const auto& decl : mod.extern_decls) {
    if (!decl.return_type.empty()) {
      const std::string& shadow =
          require_module_type_ref(mod, decl.return_type, "LirExternDecl.return_type", true);
      if (shadow != decl.return_type_str) {
        std::ostringstream detail;
        detail << "return_type does not match return_type_str; shadow '" << shadow
               << "', return_type_str '" << decl.return_type_str << "'";
        fail_verify("LirExternDecl.return_type", detail.str());
      }
      const StructNameId known_struct_name_id = mod.struct_names.find(decl.return_type_str);
      if (known_struct_name_id != kInvalidStructName &&
          mod.find_struct_decl(known_struct_name_id) &&
          decl.return_type.struct_name_id() != known_struct_name_id) {
        fail_verify("LirExternDecl.return_type",
                    "known struct return type must carry matching StructNameId");
      }
    }
  }
}

void verify_global_type_ref_shadows(const LirModule& mod) {
  for (const auto& global : mod.globals) {
    if (!global.llvm_type_ref.has_value()) continue;
    const std::string& shadow =
        require_module_type_ref(mod, *global.llvm_type_ref, "LirGlobal.llvm_type_ref");
    if (shadow != global.llvm_type) {
      std::ostringstream detail;
      detail << "structured type mirror for global '" << global.name
             << "' does not match llvm_type; shadow '" << shadow
             << "', llvm_type '" << global.llvm_type << "'";
      fail_verify("LirGlobal.llvm_type_ref", detail.str());
    }
  }
}

std::string_view function_signature_line(const LirFunction& fn) {
  std::string_view signature = fn.signature_text;
  while (!signature.empty()) {
    const size_t line_end = signature.find('\n');
    const std::string_view line =
        line_end == std::string_view::npos ? signature : signature.substr(0, line_end);
    if (line.rfind("define ", 0) == 0 || line.rfind("declare ", 0) == 0) {
      return line;
    }
    if (line_end == std::string_view::npos) break;
    signature.remove_prefix(line_end + 1);
  }
  return {};
}

std::string_view trim_signature_token(std::string_view value) {
  while (!value.empty() && value.front() == ' ') value.remove_prefix(1);
  while (!value.empty() && value.back() == ' ') value.remove_suffix(1);
  return value;
}

bool consume_signature_token(std::string_view& value, std::string_view token) {
  if (value.rfind(token, 0) != 0) return false;
  if (value.size() > token.size() && value[token.size()] != ' ') return false;
  value.remove_prefix(token.size());
  value = trim_signature_token(value);
  return true;
}

std::string_view signature_return_type_text(std::string_view line) {
  const size_t params_begin = line.find('(');
  if (params_begin == std::string_view::npos) return {};
  std::string_view prefix = trim_signature_token(line.substr(0, params_begin));

  if (consume_signature_token(prefix, "define")) {
    consume_signature_token(prefix, "internal");
    consume_signature_token(prefix, "weak");
  } else if (consume_signature_token(prefix, "declare")) {
    consume_signature_token(prefix, "extern_weak");
  } else {
    return {};
  }
  consume_signature_token(prefix, "hidden");
  consume_signature_token(prefix, "protected");

  const size_t symbol_pos = prefix.rfind(" @");
  if (symbol_pos == std::string_view::npos) return {};
  return trim_signature_token(prefix.substr(0, symbol_pos));
}

std::vector<std::string_view> signature_param_texts(std::string_view line) {
  const size_t params_begin = line.find('(');
  if (params_begin == std::string_view::npos) return {};

  int depth = 0;
  size_t params_end = std::string_view::npos;
  for (size_t index = params_begin; index < line.size(); ++index) {
    if (line[index] == '(') {
      ++depth;
    } else if (line[index] == ')') {
      --depth;
      if (depth == 0) {
        params_end = index;
        break;
      }
    }
  }
  if (params_end == std::string_view::npos || params_end <= params_begin + 1) return {};

  std::vector<std::string_view> params;
  std::string_view text = line.substr(params_begin + 1, params_end - params_begin - 1);
  int paren_depth = 0;
  int brace_depth = 0;
  int bracket_depth = 0;
  size_t start = 0;
  for (size_t index = 0; index <= text.size(); ++index) {
    if (index == text.size() ||
        (text[index] == ',' && paren_depth == 0 && brace_depth == 0 &&
         bracket_depth == 0)) {
      params.push_back(trim_signature_token(text.substr(start, index - start)));
      start = index + 1;
      continue;
    }
    if (text[index] == '(') ++paren_depth;
    if (text[index] == ')') --paren_depth;
    if (text[index] == '{') ++brace_depth;
    if (text[index] == '}') --brace_depth;
    if (text[index] == '[') ++bracket_depth;
    if (text[index] == ']') --bracket_depth;
  }
  return params;
}

void verify_function_signature_type_ref_shadows(const LirModule& mod) {
  for (const auto& fn : mod.functions) {
    const std::string_view line = function_signature_line(fn);
    if (line.empty()) {
      fail_verify("LirFunction.signature_text", "missing define/declare signature line");
    }

    if (fn.signature_return_type_ref.has_value()) {
      const std::string& shadow = require_module_type_ref(
          mod, *fn.signature_return_type_ref, "LirFunction.signature_return_type_ref", true);
      if (signature_return_type_text(line) != shadow) {
        std::ostringstream detail;
        detail << "return type mirror for function '" << fn.name
               << "' does not match signature_text; shadow '" << shadow
               << "', signature return '" << signature_return_type_text(line) << "'";
        fail_verify("LirFunction.signature_return_type_ref", detail.str());
      }
    }

    const auto params = signature_param_texts(line);
    if (!fn.signature_param_type_refs.empty() &&
        fn.signature_param_type_refs.size() > params.size()) {
      std::ostringstream detail;
      detail << "function '" << fn.name << "' has "
             << fn.signature_param_type_refs.size()
             << " parameter mirrors but only " << params.size()
             << " signature parameters";
      fail_verify("LirFunction.signature_param_type_refs", detail.str());
    }

    for (size_t index = 0; index < fn.signature_param_type_refs.size(); ++index) {
      const std::string& shadow = require_module_type_ref(
          mod, fn.signature_param_type_refs[index], "LirFunction.signature_param_type_refs");
      const std::string_view param = params[index];
      const bool exact_match = param == shadow;
      const bool named_match =
          param.size() > shadow.size() && param.substr(0, shadow.size()) == shadow &&
          param[shadow.size()] == ' ';
      if (!exact_match && !named_match) {
        std::ostringstream detail;
        detail << "parameter " << index << " mirror for function '" << fn.name
               << "' does not match signature_text; shadow '" << shadow
               << "', signature parameter '" << param << "'";
        fail_verify("LirFunction.signature_param_type_refs", detail.str());
      }
    }
  }
}

std::string join_layout_fields(const std::vector<std::string>& fields) {
  std::ostringstream out;
  out << "[";
  for (size_t index = 0; index < fields.size(); ++index) {
    if (index != 0) out << ", ";
    out << fields[index];
  }
  out << "]";
  return out.str();
}

void verify_structured_layout_observations(const LirModule& mod) {
  for (const auto& observation : mod.structured_layout_observations) {
    if (observation.site.empty()) {
      fail_verify("LirStructuredLayoutObservation.site", "must not be empty");
    }
    if (observation.type_name.empty()) {
      fail_verify("LirStructuredLayoutObservation.type_name", "must not be empty");
    }
    if (observation.name_id == kInvalidStructName) {
      fail_verify("LirStructuredLayoutObservation.name_id", "must be valid");
    }

    const std::string_view rendered_name = mod.struct_names.spelling(observation.name_id);
    if (rendered_name.empty()) {
      fail_verify("LirStructuredLayoutObservation.name_id",
                  "must resolve to a struct name");
    }
    if (rendered_name != observation.type_name) {
      std::ostringstream detail;
      detail << "StructNameId mirror '" << rendered_name
             << "' disagrees with observed type '" << observation.type_name << "'";
      fail_verify("LirStructuredLayoutObservation.type_name", detail.str());
    }

    if (!observation.parity_checked || observation.parity_matches) continue;

    std::ostringstream detail;
    detail << "structured layout mismatch at " << observation.site << " for "
           << observation.type_name << "; legacy size "
           << observation.legacy_size_bytes << ", legacy align "
           << observation.legacy_align_bytes << ", legacy fields "
           << join_layout_fields(observation.legacy_field_types)
           << ", structured fields "
           << join_layout_fields(observation.structured_field_types);
    fail_verify("LirStructuredLayoutObservation.parity", detail.str());
  }
}

}  // namespace

const std::string& require_operand_kind(
    const LirOperand& operand,
    std::string_view field,
    std::initializer_list<LirOperandKind> allowed_kinds,
    bool allow_empty) {
  if (operand.empty()) {
    if (allow_empty) return operand.str();
    fail_verify(field, "must not be empty");
  }
  if (!operand_kind_allowed(operand.kind(), allowed_kinds)) {
    std::ostringstream detail;
    detail << "expected operand kind mismatch for '" << operand.str()
           << "'; got " << operand_kind_name(operand.kind());
    fail_verify(field, detail.str());
  }
  return operand.str();
}

const std::string& require_type_ref(const LirTypeRef& type,
                                    std::string_view field,
                                    bool allow_void) {
  if (type.empty()) fail_verify(field, "must not be empty");
  if (const auto mismatch = type_ref_mismatch_detail(type); mismatch.has_value()) {
    fail_verify(field, *mismatch);
  }
  if (!allow_void && type.kind() == LirTypeKind::Void) {
    fail_verify(field, "void is not valid here");
  }
  return type.str();
}

std::string_view render_binary_opcode(const LirBinaryOpcodeRef& opcode,
                                      std::string_view field) {
  const auto typed = opcode.typed();
  if (!typed.has_value()) {
    fail_verify(field, "unknown typed binary opcode '" + opcode.str() + "'");
  }
  return opcode.str();
}

std::string_view render_cmp_predicate(const LirCmpPredicateRef& predicate,
                                      std::string_view field) {
  const auto typed = predicate.typed();
  if (!typed.has_value()) {
    fail_verify(field,
                "unknown typed comparison predicate '" + predicate.str() + "'");
  }
  return predicate.str();
}

void verify_module(const LirModule& mod) {
  verify_struct_decl_shadows(mod);
  verify_structured_layout_observations(mod);
  verify_extern_decl_shadows(mod);
  verify_global_type_ref_shadows(mod);
  verify_function_signature_type_ref_shadows(mod);
  for (const auto& function : mod.functions) {
    for (const auto& inst : function.alloca_insts) verify_inst(mod, inst);
    for (const auto& block : function.blocks) {
      for (const auto& inst : block.insts) verify_inst(mod, inst);
      verify_terminator(block.terminator);
    }
  }
}

}  // namespace c4c::codegen::lir
