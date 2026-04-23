#pragma once

#include "../x86_codegen.hpp"

namespace c4c::backend::x86 {

std::optional<PreparedI32NamedImmediateCompareSelection>
select_prepared_i32_named_immediate_compare_if_supported(
    const c4c::backend::bir::Value& lhs,
    const c4c::backend::bir::Value& rhs);

std::optional<PreparedI32NamedImmediateCompareSelection>
select_prepared_i32_named_immediate_compare_for_value_if_supported(
    const c4c::backend::bir::Value& lhs,
    const c4c::backend::bir::Value& rhs,
    std::string_view compared_value_name);

std::optional<PreparedI32NamedImmediateCompareSelection>
select_prepared_i32_named_immediate_compare_for_candidates_if_supported(
    const c4c::backend::bir::Value& lhs,
    const c4c::backend::bir::Value& rhs,
    const std::vector<std::string_view>& candidate_compare_value_names);

std::string render_prepared_i32_eax_immediate_compare_setup(std::int64_t compare_immediate);

std::optional<std::pair<std::string, std::string>> render_prepared_guard_false_branch_compare(
    const c4c::backend::bir::BinaryInst& compare,
    const std::optional<std::string_view>& current_i8_name,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations);

std::optional<std::pair<std::string, std::string>>
render_prepared_guard_false_branch_compare_from_condition(
    const c4c::backend::prepare::PreparedBranchCondition& condition,
    const std::optional<std::string_view>& current_i8_name,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations);

std::optional<ShortCircuitEntryCompareContext> build_prepared_guard_compare_context(
    const c4c::backend::prepare::PreparedBranchCondition& branch_condition,
    const std::optional<std::string_view>& current_i8_name,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations);

}  // namespace c4c::backend::x86
