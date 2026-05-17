Status: Active
Source Idea Path: ideas/open/262_aarch64_i128_ops_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory the i128 shard and current compiled owners

# Current Packet

## Just Finished

Step 1 inventory completed for the AArch64 i128-ops shard. No implementation
behavior was changed.

Exact i128-specific compiled owner candidates to move in later packets:

- `src/backend/mir/aarch64/codegen/instruction.cpp`
  - i128 enum/string helpers currently mixed into family-neutral instruction
    naming: `i128_transport_kind_name`,
    `prepared_i128_transport_record_error_name`,
    `i128_pair_operation_kind_name`, `i128_pair_lane_semantics_name`,
    `i128_shift_kind_name`, `i128_shift_lane_semantics_name`,
    `i128_shift_count_kind_name`, `i128_compare_signedness_name`,
    `i128_compare_high_word_semantics_name`,
    `i128_runtime_helper_boundary_kind_name`,
    `prepared_i128_pair_record_error_name`, and
    `prepared_i128_runtime_helper_record_error_name`.
  - i128 selected-node construction/status code: `i128_transport_selection_status`,
    `make_i128_transport_instruction`, `i128_pair_selection_status`,
    `make_i128_pair_operation_instruction`, `i128_shift_selection_status`,
    `make_i128_shift_instruction`, `i128_compare_selection_status`,
    `make_i128_compare_instruction`,
    `i128_runtime_helper_boundary_selection_status`, and
    `make_i128_runtime_helper_boundary_instruction`.
  - i128 prepared-record builders and helpers:
    `i128_transport_record_error`, `make_i128_lane_register_operand`,
    `make_prepared_i128_carrier_transport_record`,
    `make_prepared_i128_copy_transport_record`, `i128_pair_record_error`,
    `is_supported_i128_pair_opcode`, `i128_pair_operation_from_binary_opcode`,
    `i128_pair_lane_semantics_from_binary_opcode`,
    `make_i128_pair_operand_record`,
    `make_prepared_i128_pair_operation_record`, `i128_shift_record_error`,
    `i128_compare_record_error`, `i128_runtime_helper_record_error`,
    `is_i128_shift_opcode`, `i128_shift_kind_from_binary_opcode`,
    `i128_shift_lane_semantics_from_binary_opcode`,
    `is_supported_i128_shift_count`, `make_prepared_i128_shift_record`,
    `i128_compare_signedness_from_predicate`,
    `i128_compare_high_word_semantics_from_predicate`,
    `make_prepared_i128_compare_record`, `is_i128_div_rem_opcode`,
    `i128_runtime_helper_boundary_kind_from_prepared`,
    `make_i128_helper_operand_record`, and
    `make_prepared_i128_runtime_helper_boundary_record`.
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
  - i128 dispatch diagnostics and lookup: `append_i128_pair_diagnostic`,
    `i128_pair_error_message`, `i128_transport_error_message`,
    `i128_runtime_helper_error_message`, `is_i128_div_rem_opcode`, and
    `find_i128_runtime_helper_for_instruction`.
  - i128 lowering route bodies: `lower_i128_pair_operation_instruction` and
    `lower_i128_copy_instruction`. These currently consume prepared i128
    carriers/runtime helpers, call the prepared-record builders above, and
    return `I128PairOperationRecord`, `I128ShiftRecord`,
    `I128RuntimeHelperBoundaryRecord`, or `I128TransportRecord` machine
    instructions.
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
  - i128 helper/spelling utilities: `validate_i128_helper_move`,
    `append_i128_helper_move_line`, `pair_low_register_name`, and
    `pair_high_register_name`.
  - i128 print bodies: `print_i128_transport`, `print_i128_pair_operation`,
    `print_i128_shift`, `print_i128_compare`, and
    `print_i128_runtime_helper`.

Current compiled i128 behavior already outside the broad owners:

- `src/backend/mir/aarch64/codegen/comparison.cpp` owns structured i128 compare
  lowering helpers: `append_i128_compare_diagnostic`,
  `i128_compare_error_message`, `i128_equality_compare_condition`,
  `is_i128_relational_compare_predicate`, `i128_relational_compare_spelling`,
  and `lower_prepared_i128_compare_instruction`.
- `src/backend/mir/aarch64/codegen/memory.cpp` owns i128 transport lowering:
  `i128_transport_error_message` and `lower_i128_transport_instruction`.

Prepared/shared i128 authority to preserve and consume, not replace:

- `src/backend/prealloc/prealloc.hpp` defines the durable prepared facts:
  `PreparedI128CarrierKind`, `PreparedI128LaneRole`,
  `PreparedI128LaneCarrier`, `PreparedI128Carrier`,
  `PreparedI128CarrierFunction`, `PreparedI128Carriers`,
  `PreparedI128RuntimeHelperFamily`, `PreparedI128RuntimeHelperKind`,
  `PreparedI128RuntimeHelperResultOwnership`,
  `PreparedI128RuntimeHelperAbiTransition`,
  `PreparedI128RuntimeHelperMarshalDirection`,
  `PreparedI128RuntimeHelper`, `PreparedI128RuntimeHelperFunction`,
  `PreparedI128RuntimeHelpers`, `find_prepared_i128_carriers`,
  `find_prepared_i128_runtime_helpers`, and `find_prepared_i128_carrier`.
- `src/backend/prealloc/prealloc.cpp` builds the prepared i128 carrier and
  runtime-helper facts, including register-pair/memory-backed carrier layout,
  low/high lane bindings, div/rem helpers, float/integer conversion helper
  facts, ABI policies, marshal moves, clobber policy, and result ownership.
- `src/backend/prealloc/regalloc.cpp` also contributes prepared i128 runtime
  helper classification and fact emission for div/rem and float/integer
  conversion routes.

Source surfaces that should remain neutral or shared:

- `instruction.hpp` currently declares the i128 records/enums and instruction
  payload variants used by broad codegen. Step 2 should avoid redesigning the
  instruction hierarchy; expose only a narrow `i128_ops.hpp` API around the
  movable functions.
- `dispatch.cpp::dispatch_prepared_block` should remain the neutral per-block
  dispatcher, with only calls into the i128 owner after relocation.
- `machine_printer.cpp::print_machine_instruction_line_payloads` should remain
  the neutral variant dispatcher, with only calls into i128 spelling helpers
  after relocation.
- Prepared-data ownership remains in `src/backend/prealloc/*`; relocation
  should consume those facts rather than invent a separate i128 allocation or
  register convention.

Markdown shard content that is valid durable guidance but not all currently
compiled behavior:

- `i128_ops.md` describes the old x0/x1 accumulator-pair surface, pair
  load/store transport, unary neg/not, sign/zero high-half normalization,
  add/sub/mul/bitwise ops, variable and constant shifts, div/rem helper calls,
  float/i128 conversion helper calls, compare materialization, and result
  storage.
- Current compiled AArch64 i128 code already models structured pair transport,
  add/sub/and/or/xor, immediate shifts, comparisons, and div/rem helper
  boundaries through prepared records. Inventory did not find compiled AArch64
  owners for every legacy markdown entry such as unary neg/not, multiplication,
  variable shift printing, or float/i128 conversion emission; later packets
  should treat those as shard-reconciliation gaps, not behavior-preserving
  moves.

## Suggested Next

Step 2 should create the empty compiled owner boundary:
`src/backend/mir/aarch64/codegen/i128_ops.hpp` and
`src/backend/mir/aarch64/codegen/i128_ops.cpp`, wired into the existing build,
with declarations only for the narrow owner API needed by the exact candidates
above. Do not move behavior until that boundary builds.

## Watchouts

- Do not move or redesign the prepared authority in `src/backend/prealloc/*`;
  i128 relocation should consume `PreparedI128Carrier*` and
  `PreparedI128RuntimeHelper*` facts exactly as the current broad owners do.
- Avoid treating `i128_ops.md` as a license to add missing semantics during the
  redistribution steps. Behavior-changing gaps such as unary neg/not,
  multiplication, variable shifts, and float/i128 conversion emission need
  explicit supervisor routing if they become implementation work.
- Preserve `comparison.cpp` and `memory.cpp` as existing narrow owners unless
  the supervisor chooses to fold their i128-specific helpers into the new shard
  in a later behavior-preserving move.

## Proof

No build or tests were run. Proof was inventory-only: `c4c-clang-tool-ccdb`
symbol/signature/callee queries over `instruction.cpp`, `dispatch.cpp`,
`machine_printer.cpp`, `comparison.cpp`, and `memory.cpp`, plus targeted text
search of i128 references and a read of `i128_ops.md`. `test_after.log` was not
created because the delegated packet explicitly required no tests unless files
beyond `todo.md` changed.
