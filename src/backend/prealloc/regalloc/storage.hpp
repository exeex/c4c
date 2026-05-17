#pragma once

#include "../regalloc.hpp"

#include <string_view>

namespace c4c::backend::prepare {

namespace regalloc_detail {

[[nodiscard]] PreparedMoveStorageKind assigned_storage_kind(const PreparedRegallocValue& value);

[[nodiscard]] bool assigned_storage_matches(const PreparedRegallocValue& lhs,
                                            const PreparedRegallocValue& rhs);

[[nodiscard]] const PreparedRegallocValue* find_regalloc_value(
    const PreparedRegallocFunction& function,
    const PreparedNameTables& names,
    std::string_view value_name);

}  // namespace regalloc_detail

}  // namespace c4c::backend::prepare
