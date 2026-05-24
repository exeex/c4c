Status: Active
Source Idea Path: ideas/open/prepared-move-publication-indexing-prealloc.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Map Current AArch64 Prepared Indexing

# Current Packet

## Just Finished

Step 1 in `plan.md` is complete: mapped current AArch64-owned Prepared lookup
construction and query sites for call plans, address materialization, move
bundles, and value homes.

Construction sites inspected:

- Call-plan lookup: `src/backend/mir/aarch64/codegen/traversal.cpp`
  `make_prepared_call_plan_indexes` builds
  `module::PreparedCallPlanIndexes` from
  `PreparedCallPlansFunction::calls` plus
  `PreparedControlFlowFunction::blocks`. It indexes calls by
  `(block_index, instruction_index)`, records prior preserved values by
  `PreparedValueId`, computes first stack-preserved values per call, and uses
  Prepared control-flow dominance over block labels to decide reachability.
- Address-materialization lookup:
  `src/backend/mir/aarch64/codegen/traversal.cpp`
  `make_prepared_address_materialization_indexes` builds
  `module::PreparedAddressMaterializationIndexes` from
  `PreparedBirModule::addressing` via `find_prepared_addressing`, keyed by
  `PreparedAddressMaterialization::block_label`.
- Move-bundle lookup: `src/backend/mir/aarch64/codegen/traversal.cpp`
  `make_prepared_move_bundle_indexes` builds
  `module::PreparedMoveBundleIndexes` from
  `PreparedValueLocationFunction::move_bundles`, keyed by
  `(PreparedMovePhase, block_index, instruction_index)`.
- Value-home lookup: `src/backend/mir/aarch64/codegen/traversal.cpp`
  `make_prepared_value_home_indexes` builds
  `module::PreparedValueHomeIndexes` from
  `PreparedValueLocationFunction::value_homes`, keyed by
  `PreparedValueId` and by durable `ValueNameId`.
- Publication site: `lower_prepared_functions` in
  `src/backend/mir/aarch64/codegen/traversal.cpp` constructs all four indexes
  per prepared function and stores pointers on
  `module::FunctionLoweringContext`; the storage types live in
  `src/backend/mir/aarch64/module/module.hpp`.

Query sites inspected:

- Call plans: `src/backend/mir/aarch64/codegen/calls.cpp`
  `find_prepared_call_plan` uses `call_plan_indexes->calls_by_position` before
  falling back to a scan of `FunctionLoweringContext::call_plans`; callers
  include `require_prepared_call_plan` and call dispatch/lowering in
  `dispatch_calls.cpp`, `dispatch.cpp`, `globals.cpp`, and variadic lowering.
- Call preservation side indexes:
  `src/backend/mir/aarch64/codegen/calls_preservation.cpp`
  `find_latest_prior_preserved_value_by_position`,
  `find_prior_preserved_value_by_dominating_position`, and related helpers
  query `prior_preserved_by_value`; `dispatch_calls.cpp`
  `first_stack_preserved_values_for_call` queries
  `first_stack_preserved_by_call_index`.
- Address materialization:
  `src/backend/mir/aarch64/codegen/globals.cpp`
  `make_block_address_materialization_index` queries
  `address_materialization_indexes->materializations_by_block`, then builds a
  block-local instruction map for lowering in `lower_address_materializations`.
  Direct record construction still uses Prepared names, value locations,
  storage plans, and addressing facts.
- Move bundles: `src/backend/mir/aarch64/codegen/calls_preservation.cpp`
  `find_move_bundle` queries `move_bundle_indexes->bundles_by_position`; main
  consumers are `calls_moves.cpp` `lower_before_call_moves`,
  `lower_after_call_moves`, `lower_before_return_moves`, and
  `lower_value_moves`.
- Value homes: `src/backend/mir/aarch64/codegen/dispatch_lookup.cpp`
  `find_value_home` and `prepared_value_id` query the index through
  `src/backend/prealloc/value_locations.hpp`
  `find_indexed_prepared_value_home` and `find_indexed_prepared_value_id`;
  consumers span dispatch, calls, memory, globals, returns, ALU, branch
  fusion, edge copies, publications, inline asm, intrinsics, and variadic
  lowering.

Target-independent lookup construction to move later:

- Position keys, per-function index objects, pointer maps into Prepared facts,
  value-id/name maps, block-label materialization grouping, move-bundle
  phase/position grouping, and call preserved-value reachability summaries are
  Prepared/prealloc consumer facts and do not encode AArch64 instruction
  emission.

Target-local AArch64 policy/emission to keep in AArch64:

- Register spelling/parsing and views in `calls_common.cpp`, `operands.cpp`,
  `globals.cpp`, and `memory.cpp`.
- AAPCS64 call/argument/result lowering, byval handling, stack-preservation
  publication, and call-boundary instruction records in `calls*.cpp`.
- Address-materialization MIR kind selection, target symbol/TLS/GOT policy
  checks, page/low12 instruction records, and printing in `globals.cpp` and
  `machine_printer.cpp`.
- Actual move emission from Prepared move records to AArch64
  `CallBoundaryMoveInstructionRecord`, register operands, memory operands, and
  instruction nodes in `calls_moves.cpp`.
- Assembly printing, frame-pointer/base addressing legality, ABI lane policy,
  and target diagnostics remain target-local.

Existing coverage located:

- Call-plan lookup and call-boundary behavior:
  `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`,
  `tests/backend/mir/backend_aarch64_return_lowering_test.cpp`,
  `tests/backend/mir/backend_aarch64_call_boundary_owner_test.cpp`,
  `tests/backend/mir/backend_aarch64_target_instruction_records_test.cpp`,
  and `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`.
- Address-materialization lookup and records:
  `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`,
  `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`, and
  `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`.
- Move-bundle lookup and lowering phases:
  `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`,
  `tests/backend/mir/backend_aarch64_return_lowering_test.cpp`,
  `tests/backend/mir/backend_aarch64_call_boundary_owner_test.cpp`,
  `tests/backend/mir/backend_aarch64_target_instruction_records_test.cpp`,
  and `tests/backend/mir/backend_aarch64_prepared_scalar_alu_records_test.cpp`.
- Value-home lookup and operand/use sites:
  `tests/backend/mir/backend_aarch64_operand_resolution_test.cpp`,
  `tests/backend/mir/backend_aarch64_prepared_scalar_alu_records_test.cpp`,
  `tests/backend/mir/backend_aarch64_prepared_scalar_cast_records_test.cpp`,
  `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`,
  `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp`,
  `tests/backend/mir/backend_aarch64_prepared_branch_records_test.cpp`, and
  `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`.

## Suggested Next

Proceed to Step 2: add the target-neutral read-only Prepared lookup helper in
`src/backend/prealloc` or a tightly adjacent prepared-consumer support module,
starting with record/accessor shapes equivalent to the four AArch64 index
families above.

## Watchouts

Keep the helper limited to Prepared/prealloc facts and lookup shape. Do not move
register spelling, AAPCS64 policy, address-materialization MIR kind selection,
GOT/TLS/global policy checks, frame-pointer/addressing legality, diagnostics,
or assembly printing out of AArch64. `src/backend/prealloc/value_locations.hpp`
already has target-independent indexed value-home lookup templates; reuse that
style rather than wrapping AArch64 map names in a new helper.

## Proof

Documentation/audit-only packet. No build or ctest was required because no
implementation files, headers, tests, snapshots, or generated outputs were
modified; proof is this recorded source audit in `todo.md`.
