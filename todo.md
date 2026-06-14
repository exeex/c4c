Status: Active
Source Idea Path: ideas/open/253_phase_f3_prepared_module_structural_exit_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Field Consumers And Authority Surfaces

# Current Packet

## Just Finished

Step 1 - Inventory Field Consumers And Authority Surfaces completed as an
analysis-only map for `PreparedBirModule::module`, `names`, `control_flow`,
and `store_source_publications`.

Field inventory:

- `module`
  - Route/BIR semantic fact: `src/backend/prealloc/module.hpp` owns the raw
    `bir::Module` member and `BirPreAlloc` constructors seed it from the
    semantic BIR input; `src/backend/prealloc/prealloc.cpp::BirPreAlloc::run`
    returns the prepared aggregate after legalization, out-of-SSA, regalloc,
    and contract-plan publication.
  - Blocked prepared authority: `src/backend/prealloc/legalize.cpp::run_legalize`,
    `src/backend/prealloc/out_of_ssa.cpp::run_out_of_ssa`,
    `src/backend/prealloc/stack_layout/coordinator.cpp::run_stack_layout`,
    `src/backend/prealloc/liveness.cpp::run_liveness`, and
    `src/backend/prealloc/regalloc.cpp::run_regalloc` read or mutate
    `prepared_.module` directly as the phase input/output carrier.
  - Blocked prepared authority: contract-plan publishers iterate
    `prepared.module.functions` or inspect `prepared.module.names` in
    `label_identity.cpp::publish_prepared_bir_label_identity`,
    `frame_plan.cpp::populate_frame_plan`,
    `dynamic_stack.cpp::populate_dynamic_stack_plan`,
    `call_plans.cpp::populate_call_plans`,
    `publication_plans.cpp::populate_store_source_publication_plans`,
    `variadic_entry_plans.cpp::populate_variadic_entry_plans`,
    `storage_plans.cpp::populate_storage_plans`,
    `atomics.cpp::populate_atomic_operations`,
    `intrinsics.cpp::populate_intrinsic_carriers`, and
    `inline_asm.cpp::populate_inline_asm_carriers`.
  - Target-policy surface: `call_plans.cpp::populate_call_plans` passes
    `prepared.module` into `resolve_direct_callee`,
    `build_memory_return_plan`, and `plan_call_argument_source`; stack layout,
    regalloc, atomics, publication, and label identity use `prepared.module.names`
    to preserve BIR/prepared id spelling while target-sensitive records are
    produced.
  - Retained compatibility surface: `prepared_printer.cpp::print` emits
    `bir::print(module.module)` and module emptiness checks; printer helpers
    in `prepared_printer/functions.cpp`, `prepared_printer/value_locations.cpp`,
    and `prepared_printer/select_chains.cpp` re-find BIR functions/blocks from
    the prepared aggregate for debug output.
  - Retained compatibility surface: backend contract tests directly seed,
    mutate, or assert `prepared.module`, for example
    `tests/backend/bir/backend_x86_handoff_boundary_short_circuit_test.cpp`
    and `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`;
    these are evidence surfaces, not implementation authority for this packet.

- `names`
  - Route/BIR semantic fact: `PreparedNameTables` in `module.hpp` is the shared
    id namespace used by phase producers and lookup helpers for function,
    block, value, link, slot, text, and storage-object ids.
  - Blocked prepared authority: phase and publisher code reads or interns via
    `prepared_.names` / `prepared.names` in `legalize.cpp::run_legalize`,
    `out_of_ssa.cpp::run_out_of_ssa`,
    `stack_layout/coordinator.cpp::run_stack_layout`,
    `liveness.cpp::run_liveness`, `regalloc.cpp::run_regalloc`,
    `label_identity.cpp::publish_prepared_bir_label_identity`,
    `call_plans.cpp::populate_call_plans`,
    `publication_plans.cpp::populate_store_source_publication_plans`,
    `dynamic_stack.cpp::populate_dynamic_stack_plan`,
    `variadic_entry_plans.cpp::populate_variadic_entry_plans`,
    `atomics.cpp::populate_atomic_operations`,
    `intrinsics.cpp::populate_intrinsic_carriers`, and
    `inline_asm.cpp::populate_inline_asm_carriers`.
  - Route/BIR semantic fact: lookup helpers in
    `prepared_lookups.cpp::prepared_bir_function`,
    `prepared_lookups.cpp::prepared_bir_block`,
    `prepared_lookups.cpp::prepared_bir_block_label_id`,
    `select_chain_lookups.cpp::prepared_bir_function`, and
    `select_chain_lookups.cpp::prepared_bir_block_label_id` use `names` to
    bridge prepared ids back to BIR function/block spelling.
  - Target-policy surface: call, storage, atomic, intrinsic, inline-asm, i128,
    and f128 plan builders use names to render or key target-facing records;
    `call_plans.cpp::select_prepared_call_argument_source` and related
    preserved-value/source-publication paths keep this policy-sensitive.
  - Retained compatibility surface: nearly every prepared printer helper reads
    `module.names` for stable debug text, including
    `prepared_printer/control_flow.cpp::append_prepared_control_flow`,
    `prepared_printer/calls.cpp::append_call_plans`,
    `prepared_printer/select_chains.cpp::append_store_source_publications`,
    `prepared_printer/value_locations.cpp::append_value_locations`, and
    `prepared_printer/runtime_helpers.cpp`.
  - Retained compatibility surface: tests directly use `prepared.names` for
    id lookup/interning and drift setup, for example
    `tests/backend/bir/backend_x86_handoff_boundary_*_test.cpp`,
    `tests/backend/bir/backend_prepare_structured_context_test.cpp`, and
    `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`.

- `control_flow`
  - Route/BIR semantic fact: `legalize.cpp::run_legalize` clears and populates
    `prepared_.control_flow`; `out_of_ssa.cpp::run_out_of_ssa` passes it by
    pointer while removing phi nodes and publishing join/parallel-copy facts.
  - Blocked prepared authority: `label_identity.cpp::publish_prepared_bir_label_identity`
    reads `prepared.control_flow` and mutates BIR block label ids from prepared
    labels; this keeps label identity tied to the aggregate.
  - Target-policy surface: `regalloc.cpp::run_regalloc` reads
    `prepared_.control_flow` for phi move and consumer move resolution;
    `call_plans.cpp::populate_call_plans` reads it for block labels and edge
    publication source producers; `publication_plans.cpp::populate_store_source_publication_plans`
    reads it to build edge publication source-producer lookups.
  - Route/BIR semantic fact: `prepared_lookups.cpp::make_prepared_function_lookups`
    and edge-publication helpers accept `PreparedControlFlowFunction` as the
    CFG/dominance/block-index fact that ties value locations and call plans to
    BIR blocks.
  - Retained compatibility surface: `prepared_printer/control_flow.cpp::append_prepared_control_flow`
    emits the control-flow section, and `prepared_printer/select_chains.cpp::append_select_chain_materializations`
    walks `module.control_flow.functions` to print select-chain materialization
    rows.
  - Retained compatibility surface: tests directly read/mutate
    `prepared.control_flow` for route-debug and drift contracts, for example
    `tests/backend/bir/backend_x86_handoff_boundary_short_circuit_test.cpp`,
    `tests/backend/bir/backend_x86_handoff_boundary_i32_guard_chain_test.cpp`,
    and `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`.

- `store_source_publications`
  - Route/BIR semantic fact: `publication_plans.cpp::populate_store_source_publication_plans`
    clears `prepared.store_source_publications.records`, scans store
    instructions, and appends semantic records with function/block/instruction,
    source value, destination access, source home, recovered source, byval, and
    direct-global-select-chain facts.
  - Target-policy surface: the producer depends on prepared addressing,
    stack-layout, value-location, select-chain, and control-flow lookups plus
    store-local/store-global policy; status values such as missing source value
    and missing destination access remain policy-sensitive until fail-closed
    proof is specified.
  - Retained compatibility surface: the only source-tree reader found is
    `prepared_printer/select_chains.cpp::append_store_source_publications`,
    which prints `module.store_source_publications.records` through
    `append_store_source_publication_row`.
  - Unknown/blocked: no non-printer production reader of
    `PreparedBirModule::store_source_publications` was found by targeted
    search; any future authority split needs confirmation that consumers are
    printer/tests only before deleting or wrapping the field.

## Suggested Next

Proceed to Step 2 by selecting one concrete `PreparedBirModule::module`
consumer row and mapping its semantic fact, compatibility surface, and
fail-closed proof needs without changing implementation.

## Watchouts

`module`, `names`, and `control_flow` are still broad prepared authorities:
phase mutators, contract-plan publishers, printers, and tests all read them
directly. `store_source_publications` currently appears producer-plus-printer
only, but treat that as blocked until a future packet confirms no hidden
non-source consumer or output contract.

## Proof

Analysis-only packet; no build or test proof required. Ran
`git diff --check -- todo.md`.
