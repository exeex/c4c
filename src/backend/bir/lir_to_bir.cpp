#include "lir_to_bir/lowering.hpp"

#include <charconv>
#include <unordered_map>
#include <stdexcept>
#include <variant>
#include <utility>

namespace c4c::backend {

namespace {

std::string strip_global_sig(std::string_view name) {
  if (!name.empty() && name.front() == '@') {
    name.remove_prefix(1);
  }
  return std::string(name);
}

std::optional<std::int64_t> parse_hex_escaped_byte(char hi, char lo) {
  unsigned value = 0;
  const char chars[] = {hi, lo};
  for (const char ch : chars) {
    value <<= 4;
    if (ch >= '0' && ch <= '9') {
      value |= static_cast<unsigned>(ch - '0');
      continue;
    }
    if (ch >= 'a' && ch <= 'f') {
      value |= static_cast<unsigned>(ch - 'a' + 10);
      continue;
    }
    if (ch >= 'A' && ch <= 'F') {
      value |= static_cast<unsigned>(ch - 'A' + 10);
      continue;
    }
    return std::nullopt;
  }
  return static_cast<std::int64_t>(value);
}

std::string decode_llvm_c_string(std::string_view raw_bytes) {
  std::string decoded;
  decoded.reserve(raw_bytes.size());
  for (std::size_t index = 0; index < raw_bytes.size(); ++index) {
    const char ch = raw_bytes[index];
    if (ch != '\\' || index + 1 >= raw_bytes.size()) {
      decoded.push_back(ch);
      continue;
    }

    if (index + 2 < raw_bytes.size()) {
      const auto escaped = parse_hex_escaped_byte(raw_bytes[index + 1], raw_bytes[index + 2]);
      if (escaped.has_value()) {
        decoded.push_back(static_cast<char>(*escaped));
        index += 2;
        continue;
      }
    }

    const char code = raw_bytes[++index];
    switch (code) {
      case 'n':
        decoded.push_back('\n');
        break;
      case 'r':
        decoded.push_back('\r');
        break;
      case 't':
        decoded.push_back('\t');
        break;
      case '"':
        decoded.push_back('"');
        break;
      case '\\':
        decoded.push_back('\\');
        break;
      default:
        decoded.push_back('\\');
        decoded.push_back(code);
        break;
    }
  }
  return decoded;
}

struct LoweredStringConstantMetadata {
  std::vector<bir::StringConstant> ordered;
  c4c::TextTable names;
  std::unordered_map<c4c::TextId, std::string> bytes_by_name;
};

LoweredStringConstantMetadata collect_lowered_string_constants(
    const c4c::codegen::lir::LirModule& module) {
  LoweredStringConstantMetadata metadata;
  metadata.ordered.reserve(module.string_pool.size());
  metadata.bytes_by_name.reserve(module.string_pool.size());
  for (const auto& string_constant : module.string_pool) {
    if (string_constant.pool_name.empty() || string_constant.byte_length <= 0) {
      continue;
    }
    const auto stripped_name = strip_global_sig(string_constant.pool_name);
    const c4c::TextId name_id = metadata.names.intern(stripped_name);
    if (name_id == c4c::kInvalidText) {
      continue;
    }
    const auto decoded_bytes = decode_llvm_c_string(string_constant.raw_bytes);
    metadata.bytes_by_name.emplace(name_id, decoded_bytes);
    metadata.ordered.push_back(bir::StringConstant{
        .name = stripped_name,
        .bytes = decoded_bytes,
    });
  }
  return metadata;
}

bool gep_indices_are_all_zero(const c4c::codegen::lir::LirGepOp& gep) {
  for (const auto& raw_index : gep.indices) {
    const auto parsed = lir_to_bir_detail::parse_typed_operand(raw_index);
    if (!parsed.has_value() ||
        parsed->operand.kind() != c4c::codegen::lir::LirOperandKind::Immediate) {
      return false;
    }
    const auto parsed_value = lir_to_bir_detail::parse_i64(parsed->operand.str());
    if (!parsed_value.has_value() || *parsed_value != 0) {
      return false;
    }
  }
  return true;
}

using StringPointerAliasMap = std::unordered_map<std::string, c4c::TextId>;

StringPointerAliasMap collect_string_pointer_aliases(
    const c4c::codegen::lir::LirFunction& function,
    const LoweredStringConstantMetadata& string_constants) {
  StringPointerAliasMap aliases;
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      if (const auto* gep = std::get_if<c4c::codegen::lir::LirGepOp>(&inst)) {
        if (gep->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
            !gep_indices_are_all_zero(*gep)) {
          continue;
        }

        if (gep->ptr.kind() == c4c::codegen::lir::LirOperandKind::Global) {
          const auto global_name = strip_global_sig(gep->ptr.str());
          const c4c::TextId name_id = string_constants.names.find(global_name);
          if (string_constants.bytes_by_name.find(name_id) !=
              string_constants.bytes_by_name.end()) {
            aliases.emplace(gep->result.str(), name_id);
          }
          continue;
        }

        if (gep->ptr.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
          continue;
        }
        const auto alias_it = aliases.find(gep->ptr.str());
        if (alias_it != aliases.end()) {
          aliases.emplace(gep->result.str(), alias_it->second);
        }
        continue;
      }

      const auto* cast = std::get_if<c4c::codegen::lir::LirCastOp>(&inst);
      if (cast == nullptr ||
          cast->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
          cast->operand.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
          c4c::codegen::lir::trim_lir_arg_text(cast->from_type.str()) != "ptr" ||
          c4c::codegen::lir::trim_lir_arg_text(cast->to_type.str()) != "ptr") {
        continue;
      }
      const auto alias_it = aliases.find(cast->operand.str());
      if (alias_it != aliases.end()) {
        aliases.emplace(cast->result.str(), alias_it->second);
      }
    }
  }
  return aliases;
}

std::vector<const c4c::codegen::lir::LirCallOp*> collect_direct_lir_calls(
    const c4c::codegen::lir::LirFunction& function) {
  std::vector<const c4c::codegen::lir::LirCallOp*> calls;
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      const auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&inst);
      if (call == nullptr || call->callee.kind() != c4c::codegen::lir::LirOperandKind::Global) {
        continue;
      }
      calls.push_back(call);
    }
  }
  return calls;
}

std::vector<bir::CallInst*> collect_direct_bir_calls(bir::Function* function) {
  std::vector<bir::CallInst*> calls;
  for (auto& block : function->blocks) {
    for (auto& inst : block.insts) {
      auto* call = std::get_if<bir::CallInst>(&inst);
      if (call == nullptr || call->is_indirect || call->callee.empty()) {
        continue;
      }
      calls.push_back(call);
    }
  }
  return calls;
}

struct LirFunctionIdentityLookup {
  std::unordered_map<c4c::LinkNameId, const c4c::codegen::lir::LirFunction*> by_link_name_id;
  std::unordered_map<std::string, const c4c::codegen::lir::LirFunction*> fallback_by_name;
};

LirFunctionIdentityLookup build_lir_function_identity_lookup(
    const c4c::codegen::lir::LirModule& lir_module) {
  LirFunctionIdentityLookup lookup;
  lookup.by_link_name_id.reserve(lir_module.functions.size());
  lookup.fallback_by_name.reserve(lir_module.functions.size());
  for (const auto& function : lir_module.functions) {
    if (function.link_name_id != c4c::kInvalidLinkName) {
      lookup.by_link_name_id.emplace(function.link_name_id, &function);
      continue;
    }
    lookup.fallback_by_name.emplace(function.name, &function);
  }
  return lookup;
}

const c4c::codegen::lir::LirFunction* find_lir_function_for_lowered_function(
    const LirFunctionIdentityLookup& lookup,
    const bir::Function& lowered_function) {
  if (lowered_function.link_name_id != c4c::kInvalidLinkName) {
    const auto semantic_it = lookup.by_link_name_id.find(lowered_function.link_name_id);
    return semantic_it == lookup.by_link_name_id.end() ? nullptr : semantic_it->second;
  }
  const auto fallback_it = lookup.fallback_by_name.find(lowered_function.name);
  return fallback_it == lookup.fallback_by_name.end() ? nullptr : fallback_it->second;
}

void rewrite_direct_call_string_pointer_args(
    const c4c::codegen::lir::LirModule& lir_module,
    const LoweredStringConstantMetadata& string_constants,
    bir::Module* lowered_module) {
  if (lowered_module == nullptr) {
    return;
  }

  lowered_module->string_constants = string_constants.ordered;
  if (string_constants.bytes_by_name.empty()) {
    return;
  }

  const auto lir_functions = build_lir_function_identity_lookup(lir_module);

  for (auto& lowered_function : lowered_module->functions) {
    if (lowered_function.is_declaration) {
      continue;
    }

    const auto* lir_function = find_lir_function_for_lowered_function(
        lir_functions, lowered_function);
    if (lir_function == nullptr) {
      continue;
    }

    const auto string_aliases =
        collect_string_pointer_aliases(*lir_function, string_constants);
    if (string_aliases.empty()) {
      continue;
    }

    const auto lir_calls = collect_direct_lir_calls(*lir_function);
    auto lowered_calls = collect_direct_bir_calls(&lowered_function);
    if (lir_calls.size() != lowered_calls.size()) {
      continue;
    }

    for (std::size_t call_index = 0; call_index < lir_calls.size(); ++call_index) {
      const auto parsed_direct_call =
          BirFunctionLowerer::parse_direct_global_typed_call(*lir_calls[call_index]);
      if (!parsed_direct_call.has_value()) {
        continue;
      }

      auto& lowered_call = *lowered_calls[call_index];
      const auto arg_count =
          std::min(parsed_direct_call->typed_call.args.size(), lowered_call.args.size());
      for (std::size_t arg_index = 0; arg_index < arg_count; ++arg_index) {
        if (c4c::codegen::lir::trim_lir_arg_text(
                parsed_direct_call->typed_call.param_types[arg_index]) != "ptr" ||
            lowered_call.args[arg_index].type != bir::TypeKind::Ptr ||
            lowered_call.args[arg_index].kind != bir::Value::Kind::Named ||
            (!lowered_call.args[arg_index].name.empty() &&
             lowered_call.args[arg_index].name.front() == '@')) {
          continue;
        }

        const c4c::codegen::lir::LirOperand arg_operand(
            std::string(parsed_direct_call->typed_call.args[arg_index].operand));
        c4c::TextId resolved_name_id = c4c::kInvalidText;
        if (arg_operand.kind() == c4c::codegen::lir::LirOperandKind::Global) {
          const auto global_name = strip_global_sig(arg_operand.str());
          resolved_name_id = string_constants.names.find(global_name);
        } else if (arg_operand.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
          const auto alias_it = string_aliases.find(arg_operand.str());
          if (alias_it == string_aliases.end()) {
            continue;
          }
          resolved_name_id = alias_it->second;
        } else {
          continue;
        }

        if (string_constants.bytes_by_name.find(resolved_name_id) ==
            string_constants.bytes_by_name.end()) {
          continue;
        }
        const std::string_view resolved_name = string_constants.names.lookup(resolved_name_id);
        std::string rewritten_name = "@";
        rewritten_name.append(resolved_name);
        lowered_call.args[arg_index] = bir::Value::named(bir::TypeKind::Ptr, rewritten_name);
      }
    }
  }
}

}  // namespace

BirLoweringResult try_lower_to_bir_with_options(
    const c4c::codegen::lir::LirModule& module,
    const BirLoweringOptions& options) {
  auto context = make_lowering_context(module, options);
  context.note("pipeline", "begin lir_to_bir lowering pipeline");

  auto analysis = analyze_module(context);
  auto lowered = lower_module(context, analysis);
  const auto string_constants = collect_lowered_string_constants(module);
  if (lowered.has_value()) {
    rewrite_direct_call_string_pointer_args(module, string_constants, &*lowered);
  }

  context.note("pipeline", "finish lir_to_bir lowering pipeline");
  return BirLoweringResult{
      .module = std::move(lowered),
      .analysis = std::move(analysis),
      .notes = std::move(context.notes),
  };
}

std::optional<bir::Module> try_lower_to_bir(const c4c::codegen::lir::LirModule& module) {
  return try_lower_to_bir_with_options(module, BirLoweringOptions{}).module;
}

bir::Module lower_to_bir(const c4c::codegen::lir::LirModule& module) {
  auto result = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (result.module.has_value()) {
    return *result.module;
  }
  throw std::invalid_argument(
      "lir_to_bir pipeline skeleton is wired, but semantic instruction lowering is still disabled");
}

}  // namespace c4c::backend
