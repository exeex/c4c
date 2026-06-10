#include "private.hpp"

#include "../publication_plans.hpp"
#include "../select_chain_lookups.hpp"

#include <optional>
#include <sstream>
#include <string_view>
#include <variant>

namespace c4c::backend::prepare {

namespace {

std::string_view maybe_function_name(const PreparedNameTables& names, FunctionNameId id) {
  if (id == kInvalidFunctionName) {
    return "<none>";
  }
  return prepared_function_name(names, id);
}

std::string_view maybe_block_label(const PreparedNameTables& names, BlockLabelId id) {
  if (id == kInvalidBlockLabel) {
    return "<none>";
  }
  return prepared_block_label(names, id);
}

std::string_view maybe_value_name(const PreparedNameTables& names, ValueNameId id) {
  if (id == kInvalidValueName) {
    return "<none>";
  }
  return prepared_value_name(names, id);
}

[[nodiscard]] const bir::Function* find_bir_function(const PreparedBirModule& module,
                                                     FunctionNameId function_name) {
  const std::string_view name = maybe_function_name(module.names, function_name);
  if (name.empty() || name == "<none>") {
    return nullptr;
  }
  for (const auto& function : module.module.functions) {
    if (function.name == name) {
      return &function;
    }
  }
  return nullptr;
}

[[nodiscard]] std::optional<bir::Value> instruction_result(const bir::Inst& inst) {
  if (const auto* load = std::get_if<bir::LoadLocalInst>(&inst); load != nullptr) {
    return load->result;
  }
  if (const auto* load = std::get_if<bir::LoadGlobalInst>(&inst); load != nullptr) {
    return load->result;
  }
  if (const auto* cast = std::get_if<bir::CastInst>(&inst); cast != nullptr) {
    return cast->result;
  }
  if (const auto* binary = std::get_if<bir::BinaryInst>(&inst); binary != nullptr) {
    return binary->result;
  }
  if (const auto* select = std::get_if<bir::SelectInst>(&inst); select != nullptr) {
    return select->result;
  }
  return std::nullopt;
}

[[nodiscard]] bool should_print_select_chain_materialization(
    const PreparedScalarSelectChainMaterialization& materialization) {
  return materialization.available &&
         (materialization.root_is_select ||
          materialization.direct_global_dependency.contains_direct_global_load);
}

void append_select_chain_row(
    std::ostringstream& out,
    const PreparedBirModule& module,
    const PreparedEdgePublicationSourceProducerLookups& source_producers,
    FunctionNameId function_name,
    BlockLabelId block_label,
    const PreparedScalarSelectChainMaterialization& materialization) {
  out << "  select_chain function=" << maybe_function_name(module.names, function_name)
      << " block=" << maybe_block_label(module.names, block_label)
      << " value=" << maybe_value_name(module.names, materialization.root_value_name)
      << " root_is_select=" << (materialization.root_is_select ? "yes" : "no")
      << " root_inst=" << *materialization.root_instruction_index;
  if (materialization.direct_global_dependency.contains_direct_global_load) {
    out << " direct_global_select_chain=yes"
        << " direct_global_root_is_select="
        << (materialization.direct_global_dependency.root_is_select ? "yes" : "no")
        << " direct_global_root_inst="
        << *materialization.direct_global_dependency.root_instruction_index;
  }
  const auto* producer =
      find_indexed_prepared_edge_publication_source_producer(
          &source_producers, materialization.root_value_name);
  if (producer != nullptr) {
    out << " source_producer="
        << prepared_edge_publication_source_producer_kind_name(producer->kind);
    if (producer->block_label != kInvalidBlockLabel) {
      out << " source_producer_block="
          << maybe_block_label(module.names, producer->block_label);
    }
    out << " source_producer_inst=" << producer->instruction_index;
  }
  out << "\n";
}

const char* yes_no(bool value) {
  return value ? "yes" : "no";
}

void append_optional_index(std::ostringstream& out,
                           std::string_view label,
                           std::optional<std::size_t> value) {
  if (value.has_value()) {
    out << " " << label << "=" << *value;
  }
}

void append_store_source_publication_row(
    std::ostringstream& out,
    const PreparedBirModule& module,
    const PreparedStoreSourcePublicationRecord& record) {
  const auto& plan = record.plan;
  out << "  store_source function="
      << maybe_function_name(module.names, record.function_name)
      << " block=" << maybe_block_label(module.names, record.block_label)
      << " inst=" << record.instruction_index
      << " source=" << maybe_value_name(module.names, plan.source_value_name)
      << " status=" << prepared_store_source_publication_status_name(plan.status)
      << " intent=" << prepared_store_source_publication_intent_name(plan.intent)
      << " source_producer="
      << prepared_edge_publication_source_producer_kind_name(plan.source_producer_kind);
  if (plan.source_producer_block_label.has_value()) {
    out << " source_producer_block="
        << maybe_block_label(module.names, *plan.source_producer_block_label);
  }
  append_optional_index(out,
                        "source_producer_inst",
                        plan.source_producer_instruction_index);
  out << " source_load_local=" << yes_no(plan.source_load_local != nullptr)
      << " source_load_global=" << yes_no(plan.source_load_global != nullptr)
      << " source_cast=" << yes_no(plan.source_cast != nullptr)
      << " source_binary=" << yes_no(plan.source_binary != nullptr)
      << " source_select=" << yes_no(plan.source_select != nullptr)
      << " direct_global_select_chain="
      << yes_no(plan.direct_global_select_chain_source)
      << " direct_global_root_is_select="
      << yes_no(plan.direct_global_select_chain_root_is_select);
  append_optional_index(out,
                        "direct_global_root_inst",
                        plan.direct_global_select_chain_root_instruction_index);
  out << "\n";
}

}  // namespace

void append_store_source_publications(std::ostringstream& out,
                                      const PreparedBirModule& module) {
  out << "--- prepared-store-source-publications ---\n";
  for (const auto& record : module.store_source_publications.records) {
    append_store_source_publication_row(out, module, record);
  }
}

void append_select_chain_materializations(std::ostringstream& out,
                                          const PreparedBirModule& module) {
  out << "--- prepared-select-chain-materializations ---\n";
  for (const auto& function_cf : module.control_flow.functions) {
    const auto* function = find_bir_function(module, function_cf.function_name);
    if (function == nullptr) {
      continue;
    }
    const auto source_producers =
        make_prepared_edge_publication_source_producer_lookups(module, function_cf);
    for (const auto& cf_block : function_cf.blocks) {
      if (cf_block.block_label == kInvalidBlockLabel) {
        continue;
      }
      const auto* block =
          find_block_in_function(*function, maybe_block_label(module.names, cf_block.block_label));
      if (block == nullptr) {
        continue;
      }
      for (std::size_t inst_index = 0; inst_index < block->insts.size(); ++inst_index) {
        const auto result = instruction_result(block->insts[inst_index]);
        if (!result.has_value()) {
          continue;
        }
        const auto materialization =
            find_prepared_scalar_select_chain_materialization(module.names,
                                                              &source_producers,
                                                              cf_block.block_label,
                                                              block,
                                                              *result,
                                                              inst_index + 1U);
        if (!materialization.root_instruction_index.has_value() ||
            !should_print_select_chain_materialization(materialization)) {
          continue;
        }
        append_select_chain_row(out,
                                module,
                                source_producers,
                                function_cf.function_name,
                                cf_block.block_label,
                                materialization);
      }
    }
  }
}

}  // namespace c4c::backend::prepare
