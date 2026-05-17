#include "spill_reload.hpp"

#include "classification.hpp"
#include "intervals.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

namespace c4c::backend::prepare::regalloc_detail {

void append_spill_reload_ops(const PreparedLivenessFunction& liveness_function,
                             const std::vector<std::optional<std::size_t>>& spill_points,
                             PreparedRegallocFunction& regalloc_function) {
  for (std::size_t value_index = 0; value_index < regalloc_function.values.size(); ++value_index) {
    const auto spill_point = spill_points[value_index];
    const auto& value = regalloc_function.values[value_index];
    if (!spill_point.has_value() || !value.assigned_stack_slot.has_value()) {
      continue;
    }
    const auto& published_register = value.assigned_register.has_value()
                                         ? value.assigned_register
                                         : value.spill_register_authority;

    if (const auto spill_location = locate_program_point(liveness_function, *spill_point);
        spill_location.has_value()) {
      regalloc_function.spill_reload_ops.push_back(PreparedSpillReloadOp{
          .value_id = value.value_id,
          .op_kind = PreparedSpillReloadOpKind::Spill,
          .block_index = spill_location->block_index,
          .instruction_index = spill_location->instruction_index,
          .register_bank = register_bank_from_class(value.register_class),
          .register_name = published_register.has_value()
                               ? std::optional<std::string>{published_register->register_name}
                               : std::nullopt,
          .contiguous_width = published_register.has_value()
                                  ? published_register->contiguous_width
                                  : std::max<std::size_t>(value.register_group_width, 1),
          .occupied_register_names = published_register.has_value()
                                         ? published_register->occupied_register_names
                                         : std::vector<std::string>{},
          .slot_id = value.assigned_stack_slot.has_value()
                         ? std::optional<PreparedFrameSlotId>{value.assigned_stack_slot->slot_id}
                         : std::nullopt,
          .stack_offset_bytes = value.assigned_stack_slot.has_value()
                                    ? std::optional<std::size_t>{
                                          value.assigned_stack_slot->offset_bytes}
                                    : std::nullopt,
      });
    }

    std::optional<std::size_t> last_reload_point;
    for (const std::size_t use_point : liveness_function.values[value_index].use_points) {
      if (use_point <= *spill_point || last_reload_point == use_point) {
        continue;
      }
      if (const auto reload_location = locate_program_point(liveness_function, use_point);
          reload_location.has_value()) {
        regalloc_function.spill_reload_ops.push_back(PreparedSpillReloadOp{
            .value_id = value.value_id,
            .op_kind = PreparedSpillReloadOpKind::Reload,
            .block_index = reload_location->block_index,
            .instruction_index = reload_location->instruction_index,
            .register_bank = register_bank_from_class(value.register_class),
            .register_name = published_register.has_value()
                                 ? std::optional<std::string>{published_register->register_name}
                                 : std::nullopt,
            .contiguous_width = published_register.has_value()
                                    ? published_register->contiguous_width
                                    : std::max<std::size_t>(value.register_group_width, 1),
            .occupied_register_names = published_register.has_value()
                                           ? published_register->occupied_register_names
                                           : std::vector<std::string>{},
            .slot_id = value.assigned_stack_slot.has_value()
                           ? std::optional<PreparedFrameSlotId>{value.assigned_stack_slot->slot_id}
                           : std::nullopt,
            .stack_offset_bytes = value.assigned_stack_slot.has_value()
                                      ? std::optional<std::size_t>{
                                            value.assigned_stack_slot->offset_bytes}
                                      : std::nullopt,
        });
        last_reload_point = use_point;
      }
    }
  }
}

}  // namespace c4c::backend::prepare::regalloc_detail
