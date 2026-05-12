#include "lowering.hpp"

#include <algorithm>

namespace c4c::backend {

using lir_to_bir_detail::compute_aggregate_type_layout;
using lir_to_bir_detail::lookup_backend_aggregate_type_layout_result;
using lir_to_bir_detail::lookup_backend_aggregate_type_ref_layout_result;
using lir_to_bir_detail::lower_integer_type;
using lir_to_bir_detail::type_size_bytes;

namespace {

std::optional<std::string> parse_byval_pointee_type(std::string_view type_text) {
  constexpr std::string_view kPrefix = "ptr byval(";

  auto trimmed = c4c::codegen::lir::trim_lir_arg_text(type_text);
  if (trimmed.size() <= kPrefix.size() || trimmed.substr(0, kPrefix.size()) != kPrefix) {
    return std::nullopt;
  }

  const auto body = trimmed.substr(kPrefix.size());
  int paren_depth = 1;
  int bracket_depth = 0;
  int brace_depth = 0;
  int angle_depth = 0;
  for (std::size_t index = 0; index < body.size(); ++index) {
    switch (body[index]) {
      case '(':
        if (bracket_depth == 0 && brace_depth == 0 && angle_depth == 0) {
          ++paren_depth;
        }
        break;
      case ')':
        if (bracket_depth == 0 && brace_depth == 0 && angle_depth == 0) {
          --paren_depth;
          if (paren_depth == 0) {
            const auto pointee =
                c4c::codegen::lir::trim_lir_arg_text(body.substr(0, index));
            if (pointee.empty()) {
              return std::nullopt;
            }
            return std::string(pointee);
          }
        }
        break;
      case '[':
        ++bracket_depth;
        break;
      case ']':
        if (bracket_depth > 0) {
          --bracket_depth;
        }
        break;
      case '{':
        ++brace_depth;
        break;
      case '}':
        if (brace_depth > 0) {
          --brace_depth;
        }
        break;
      case '<':
        ++angle_depth;
        break;
      case '>':
        if (angle_depth > 0) {
          --angle_depth;
        }
        break;
      default:
        break;
    }
  }

  return std::nullopt;
}

std::string normalize_aggregate_param_type(std::string_view type_text) {
  if (const auto byval_pointee = parse_byval_pointee_type(type_text); byval_pointee.has_value()) {
    return *byval_pointee;
  }
  return std::string(c4c::codegen::lir::trim_lir_arg_text(type_text));
}

BirFunctionLowerer::AggregateTypeLayout selected_aggregate_type_layout(
    std::string_view type_text,
    const BirFunctionLowerer::TypeDeclMap& type_decls,
    const lir_to_bir_detail::BackendStructuredLayoutTable& structured_layouts) {
  // Step 4 no-id compatibility bridge: aggregate.cpp still threads rendered
  // type text through local aggregate slot declaration, leaf discovery, and
  // byval copy helpers. Structured layouts remain authoritative when the text
  // names one, while metadata-bearing byval params must use the LirTypeRef
  // lookup path below and fail closed there. Remove this bridge once local
  // aggregate slot state and byval copy state carry structured type refs.
  return lookup_backend_aggregate_type_layout_result(type_text, type_decls, structured_layouts)
      .layout;
}

std::optional<BirFunctionLowerer::AggregateTypeLayout> selected_aggregate_type_ref_layout(
    const c4c::codegen::lir::LirTypeRef& type_ref,
    const BirFunctionLowerer::TypeDeclMap& type_decls,
    const lir_to_bir_detail::BackendStructuredLayoutTable& structured_layouts) {
  const auto normalized_type = normalize_aggregate_param_type(type_ref.str());
  auto normalized_ref =
      type_ref.has_struct_name_id()
          ? c4c::codegen::lir::LirTypeRef::struct_type(normalized_type,
                                                       type_ref.struct_name_id())
          : c4c::codegen::lir::LirTypeRef(normalized_type);
  const auto lookup =
      lookup_backend_aggregate_type_ref_layout_result(normalized_ref,
                                                      type_decls,
                                                      structured_layouts);
  const auto& layout = lookup.layout;
  if (!lookup.used_structured_layout ||
      (layout.kind != BirFunctionLowerer::AggregateTypeLayout::Kind::Struct &&
       layout.kind != BirFunctionLowerer::AggregateTypeLayout::Kind::Array) ||
      layout.size_bytes == 0 || layout.align_bytes == 0) {
    return std::nullopt;
  }
  return layout;
}

}  // namespace

std::string BirFunctionLowerer::aggregate_param_slot_base(std::string_view param_name) {
  std::string sanitized(param_name);
  if (!sanitized.empty() && sanitized.front() == '%') {
    sanitized.erase(sanitized.begin());
  }
  if (sanitized.empty()) {
    sanitized = "arg";
  }
  return "%lv.param." + sanitized;
}

std::optional<BirFunctionLowerer::AggregateTypeLayout> BirFunctionLowerer::lower_byval_aggregate_layout(
    std::string_view text,
    const TypeDeclMap& type_decls) {
  return lower_byval_aggregate_layout(text, type_decls, nullptr);
}

std::optional<BirFunctionLowerer::AggregateTypeLayout> BirFunctionLowerer::lower_byval_aggregate_layout(
    std::string_view text,
    const TypeDeclMap& type_decls,
    const lir_to_bir_detail::BackendStructuredLayoutTable* structured_layouts) {
  const std::string normalized_type = normalize_aggregate_param_type(text);
  // Step 4 no-id compatibility bridge: this helper is the shared rendered-text
  // byval/aggregate layout entrypoint for callers that do not yet carry
  // LirTypeRef/StructNameId metadata. Structured-layout callers still resolve
  // through selected_aggregate_type_layout(), while metadata-bearing byval
  // params must use selected_aggregate_type_ref_layout() and fail closed there.
  // Remove this raw-text entrypoint once all aggregate/byval lowering sites
  // thread structured type identity or an explicit no-id legacy marker.
  auto layout = structured_layouts != nullptr
                    ? selected_aggregate_type_layout(normalized_type,
                                                     type_decls,
                                                     *structured_layouts)
                    : compute_aggregate_type_layout(normalized_type, type_decls);
  if ((layout.kind != AggregateTypeLayout::Kind::Struct &&
       layout.kind != AggregateTypeLayout::Kind::Array) ||
      layout.size_bytes == 0 || layout.align_bytes == 0) {
    return std::nullopt;
  }
  return layout;
}

std::vector<std::pair<std::size_t, std::string>> BirFunctionLowerer::collect_sorted_leaf_slots(
    const LocalAggregateSlots& aggregate_slots) const {
  const auto layout =
      selected_aggregate_type_layout(aggregate_slots.type_text,
                                     type_decls_,
                                     structured_layouts_);
  if ((layout.kind != AggregateTypeLayout::Kind::Struct &&
       layout.kind != AggregateTypeLayout::Kind::Array) ||
      layout.size_bytes == 0 || layout.align_bytes == 0) {
    return {};
  }

  const auto begin_offset = aggregate_slots.base_byte_offset;
  const auto end_offset = begin_offset + layout.size_bytes;
  std::vector<std::pair<std::size_t, std::string>> leaves;
  leaves.reserve(aggregate_slots.leaf_slots.size());
  for (const auto& [byte_offset, slot_name] : aggregate_slots.leaf_slots) {
    if (byte_offset < begin_offset || byte_offset >= end_offset) {
      continue;
    }
    leaves.push_back({byte_offset - begin_offset, slot_name});
  }
  std::sort(leaves.begin(),
            leaves.end(),
            [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });
  return leaves;
}

BirFunctionLowerer::AggregateParamMap BirFunctionLowerer::collect_aggregate_params() const {
  AggregateParamMap aggregate_params;
  const bool structured_params_available = has_structured_signature_params(function_);
  // Generated LIR carries structured signature params. The text parser remains
  // only for legacy hand-built LIR fixtures that do not populate those fields.
  const auto parsed_params = structured_params_available
                                 ? structured_signature_params(function_)
                                 : parse_function_signature_params(function_.signature_text);
  if (!parsed_params.has_value()) {
    return aggregate_params;
  }

  const bool use_declared_names =
      !structured_params_available && !function_.params.empty() &&
      function_.params.size() == parsed_params->size();
  const auto limit = use_declared_names ? function_.params.size() : parsed_params->size();
  for (std::size_t index = 0; index < limit; ++index) {
    const auto& parsed_param = (*parsed_params)[index];
    if (parsed_param.is_varargs) {
      return {};
    }
    const auto normalized_type = normalize_aggregate_param_type(parsed_param.type);
    if (lower_integer_type(normalized_type).has_value()) {
      continue;
    }
    std::string name =
        use_declared_names ? function_.params[index].first : std::string(parsed_param.operand);
    if (name.empty()) {
      name = parsed_param.operand;
    }
    if (name.empty()) {
      return {};
    }
    const bool is_explicit_byval_param = parsed_param.is_byval;
    const bool type_ref_spells_byval =
        structured_params_available &&
        parse_byval_pointee_type(function_.signature_param_type_refs[index].str()).has_value();
    if (structured_params_available && type_ref_spells_byval && !is_explicit_byval_param) {
      aggregate_params.emplace(std::move(name),
                               AggregateParamInfo{
                                   .type_text = normalized_type,
                                   .layout = AggregateTypeLayout{},
                               });
      return aggregate_params;
    }
    const bool use_structured_byval_layout =
        is_explicit_byval_param &&
        function_.signature_param_type_refs[index].has_struct_name_id();
    const auto layout =
        structured_params_available && use_structured_byval_layout
            ? selected_aggregate_type_ref_layout(function_.signature_param_type_refs[index],
                                                type_decls_,
                                                structured_layouts_)
            : [&]() -> std::optional<AggregateTypeLayout> {
                // Step 4 no-id compatibility bridge: aggregate parameter
                // collection owns legacy byval params whose parsed signature
                // position has no StructNameId-bearing LirTypeRef. The
                // limitation is that layout is still selected from rendered
                // normalized type text. Remove this once every byval parameter
                // path carries structured signature type refs or an explicit
                // no-id legacy marker.
                return lower_byval_aggregate_layout(normalized_type,
                                                    type_decls_,
                                                    &structured_layouts_);
              }();
    if (!layout.has_value()) {
      if (structured_params_available && use_structured_byval_layout) {
        aggregate_params.emplace(std::move(name),
                                 AggregateParamInfo{
                                     .type_text = normalized_type,
                                     .layout = AggregateTypeLayout{},
                                 });
        return aggregate_params;
      }
      return {};
    }

    aggregate_params.emplace(std::move(name),
                             AggregateParamInfo{
                                 .type_text = normalized_type,
                                 .layout = *layout,
                             });
  }
  return aggregate_params;
}

bool BirFunctionLowerer::append_local_aggregate_scalar_slots(std::string_view type_text,
                                                             std::string_view slot_prefix,
                                                             std::size_t byte_offset,
                                                             std::size_t align_bytes,
                                                             LocalAggregateSlots* aggregate_slots) {
  const auto layout = selected_aggregate_type_layout(type_text,
                                                     type_decls_,
                                                     structured_layouts_);
  if (layout.kind == AggregateTypeLayout::Kind::Invalid ||
      layout.size_bytes == 0 || layout.align_bytes == 0) {
    return false;
  }

  switch (layout.kind) {
    case AggregateTypeLayout::Kind::Scalar: {
      const std::string slot_name =
          std::string(slot_prefix) + "." + std::to_string(byte_offset);
      local_slot_types_.emplace(slot_name, layout.scalar_type);
      local_pointer_slots_.emplace(slot_name, slot_name);
      local_aggregate_field_slots_.insert(slot_name);
      aggregate_slots->leaf_slots.emplace(byte_offset, slot_name);
      lowered_function_.local_slots.push_back(bir::LocalSlot{
          .name = slot_name,
          .type = layout.scalar_type,
          .align_bytes = align_bytes > 0 ? align_bytes : layout.align_bytes,
      });
      return true;
    }
    case AggregateTypeLayout::Kind::Array: {
      const auto element_layout =
          selected_aggregate_type_layout(layout.element_type_text,
                                         type_decls_,
                                         structured_layouts_);
      if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
          element_layout.size_bytes == 0) {
        return false;
      }
      for (std::size_t index = 0; index < layout.array_count; ++index) {
        if (!append_local_aggregate_scalar_slots(layout.element_type_text,
                                                 slot_prefix,
                                                 byte_offset + index * element_layout.size_bytes,
                                                 align_bytes,
                                                 aggregate_slots)) {
          return false;
        }
      }
      return true;
    }
    case AggregateTypeLayout::Kind::Struct:
      for (const auto& field : layout.fields) {
        if (!append_local_aggregate_scalar_slots(field.type_text,
                                                 slot_prefix,
                                                 byte_offset + field.byte_offset,
                                                 align_bytes,
                                                 aggregate_slots)) {
          return false;
        }
      }
      return true;
    default:
      return false;
  }
}

bool BirFunctionLowerer::declare_local_aggregate_slots(std::string_view type_text,
                                                       std::string_view slot_name,
                                                       std::size_t align_bytes) {
  const auto aggregate_layout = selected_aggregate_type_layout(type_text,
                                                               type_decls_,
                                                               structured_layouts_);
  if (aggregate_layout.kind != AggregateTypeLayout::Kind::Struct &&
      aggregate_layout.kind != AggregateTypeLayout::Kind::Array) {
    return false;
  }

  LocalAggregateSlots aggregate_slots{
      .storage_type_text = std::string(c4c::codegen::lir::trim_lir_arg_text(type_text)),
      .type_text = std::string(c4c::codegen::lir::trim_lir_arg_text(type_text)),
      .base_byte_offset = 0,
  };
  if (!append_local_aggregate_scalar_slots(type_text,
                                           slot_name,
                                           0,
                                           align_bytes,
                                           &aggregate_slots)) {
    return false;
  }
  local_aggregate_slots_.emplace(std::string(slot_name), std::move(aggregate_slots));
  return true;
}

bool BirFunctionLowerer::append_local_aggregate_copy_from_slots(
    const LocalAggregateSlots& source_slots,
    const LocalAggregateSlots& target_slots,
    std::string_view temp_prefix,
    std::vector<bir::Inst>* lowered_insts) const {
  // Step 4 no-id compatibility bridge: local aggregate copy lowering owns
  // source/target LocalAggregateSlots that retain rendered type text only. The
  // limitation is that copy-size validation cannot compare original
  // LirTypeRef/StructNameId metadata for either aggregate value. Remove this
  // once LocalAggregateSlots carry structured type identity through aggregate
  // copy planning.
  const auto source_layout =
      lower_byval_aggregate_layout(source_slots.type_text, type_decls_, &structured_layouts_);
  const auto target_layout =
      lower_byval_aggregate_layout(target_slots.type_text, type_decls_, &structured_layouts_);
  if (!source_layout.has_value() || !target_layout.has_value() ||
      source_layout->size_bytes != target_layout->size_bytes) {
    return false;
  }

  const auto target_leaves = collect_sorted_leaf_slots(target_slots);
  for (const auto& [byte_offset, target_slot_name] : target_leaves) {
    const auto source_slot_it =
        source_slots.leaf_slots.find(source_slots.base_byte_offset + byte_offset);
    if (source_slot_it == source_slots.leaf_slots.end()) {
      return false;
    }
    const auto slot_type_it = local_slot_types_.find(target_slot_name);
    const auto source_slot_type_it = local_slot_types_.find(source_slot_it->second);
    if (slot_type_it == local_slot_types_.end() ||
        source_slot_type_it == local_slot_types_.end() ||
        source_slot_type_it->second != slot_type_it->second) {
      return false;
    }
    const std::string temp_name =
        std::string(temp_prefix) + "." + std::to_string(byte_offset);
    lowered_insts->push_back(bir::LoadLocalInst{
        .result = bir::Value::named(slot_type_it->second, temp_name),
        .slot_name = source_slot_it->second,
    });
    lowered_insts->push_back(bir::StoreLocalInst{
        .slot_name = target_slot_name,
        .value = bir::Value::named(slot_type_it->second, temp_name),
    });
  }
  return true;
}

bool BirFunctionLowerer::append_local_aggregate_copy_to_pointer(
    const LocalAggregateSlots& source_slots,
    const bir::Value& target_pointer,
    std::size_t target_align_bytes,
    std::string_view temp_prefix,
    std::vector<bir::Inst>* lowered_insts) const {
  const auto source_leaves = collect_sorted_leaf_slots(source_slots);
  for (const auto& [byte_offset, source_slot_name] : source_leaves) {
    const auto slot_type_it = local_slot_types_.find(source_slot_name);
    if (slot_type_it == local_slot_types_.end()) {
      return false;
    }
    const auto slot_size = type_size_bytes(slot_type_it->second);
    if (slot_size == 0) {
      return false;
    }
    const std::string temp_name =
        std::string(temp_prefix) + "." + std::to_string(byte_offset);
    lowered_insts->push_back(bir::LoadLocalInst{
        .result = bir::Value::named(slot_type_it->second, temp_name),
        .slot_name = source_slot_name,
    });
    lowered_insts->push_back(bir::StoreLocalInst{
        .slot_name = source_slot_name,
        .value = bir::Value::named(slot_type_it->second, temp_name),
        .address = bir::MemoryAddress{
            .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
            .base_value = target_pointer,
            .byte_offset = static_cast<std::int64_t>(byte_offset),
            .size_bytes = slot_size,
            .align_bytes = std::max(slot_size, target_align_bytes),
        },
    });
  }
  return true;
}

bool BirFunctionLowerer::materialize_aggregate_param_aliases(std::vector<bir::Inst>* lowered_insts) {
  for (const auto& [param_name, info] : aggregate_params_) {
    if ((info.layout.kind != AggregateTypeLayout::Kind::Struct &&
         info.layout.kind != AggregateTypeLayout::Kind::Array) ||
        info.layout.size_bytes == 0 || info.layout.align_bytes == 0) {
      return false;
    }
    const auto slot_base = aggregate_param_slot_base(param_name);
    if (!declare_local_aggregate_slots(info.type_text, slot_base, info.layout.align_bytes)) {
      return false;
    }

    const auto aggregate_it = local_aggregate_slots_.find(slot_base);
    if (aggregate_it == local_aggregate_slots_.end()) {
      return false;
    }

    aggregate_value_aliases_[param_name] = slot_base;

    const auto leaves = collect_sorted_leaf_slots(aggregate_it->second);
    for (const auto& [byte_offset, slot_name] : leaves) {
      const auto slot_type_it = local_slot_types_.find(slot_name);
      if (slot_type_it == local_slot_types_.end()) {
        return false;
      }
      const auto slot_size = type_size_bytes(slot_type_it->second);
      if (slot_size == 0) {
        return false;
      }
      const std::string temp_name =
          slot_base + ".aggregate.param.copy." + std::to_string(byte_offset);
      lowered_insts->push_back(bir::LoadLocalInst{
          .result = bir::Value::named(slot_type_it->second, temp_name),
          .slot_name = slot_name,
          .address =
              bir::MemoryAddress{
                  .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
                  .base_value = bir::Value::named(bir::TypeKind::Ptr, param_name),
                  .byte_offset = static_cast<std::int64_t>(byte_offset),
                  .size_bytes = slot_size,
                  .align_bytes = std::max(slot_size, info.layout.align_bytes),
              },
      });
      lowered_insts->push_back(bir::StoreLocalInst{
          .slot_name = slot_name,
          .value = bir::Value::named(slot_type_it->second, temp_name),
      });
    }
  }
  return true;
}

}  // namespace c4c::backend
