# AArch64 Dispatch Publication Mechanical Split

Status: Active
Source Idea: ideas/open/385_aarch64_dispatch_publication_mechanical_split.md

## Purpose

Split `src/backend/mir/aarch64/codegen/dispatch_publication.cpp` into smaller
mechanical files while preserving behavior and generated assembly.

Goal: make the mixed publication responsibilities visible and keep every
publication-related implementation file under 4000 lines.

## Core Rule

Move coherent helper clusters mechanically. Do not redesign lowering, rename the
publication model broadly, weaken tests, or add testcase-shaped special cases.

## Read First

- `ideas/open/385_aarch64_dispatch_publication_mechanical_split.md`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- Nearby AArch64 codegen headers and source files only as needed for include
  and namespace conventions.
- `.codex/skills/c4c-clang-tools/SKILL.md` before each extraction packet.

## Current Targets

- Primary file:
  `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- Provisional extraction files:
  `dispatch_publication_common.cpp/hpp`,
  `dispatch_producers.cpp/hpp`,
  `dispatch_value_materialization.cpp/hpp`,
  `dispatch_edge_copies.cpp/hpp`,
  `dispatch_store_sources.cpp/hpp`
- Optional temporary central state tracking remains in
  `dispatch_publication.cpp` if dependency shape is not clean enough for the
  first pass.

## Non-Goals

- Do not redesign the dispatch architecture.
- Do not rewrite lowering behavior.
- Do not change generated assembly.
- Do not finalize publication terminology.
- Do not fold code into `calls.cpp`, `memory.cpp`, `comparison.cpp`, or other
  existing feature files.
- Do not downgrade expectations or supported-path tests.

## Working Model

Each step extracts one responsibility cluster, adds the smallest required
header surface, updates includes, builds, and runs the delegated proof subset.
Before moving a cluster, inspect function signatures and at least one caller or
callee relationship using `c4c-clang-tool-ccdb` against
`build/compile_commands.json`.

Required cluster-inspection commands:

```bash
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/dispatch_publication.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/dispatch_publication.cpp <function-name> build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/dispatch_publication.cpp <function-name> build/compile_commands.json
```

## Execution Rules

- Keep each packet small enough to review as a mechanical move.
- Preserve namespaces, linkage, helper names, and behavior unless a local
  compile requirement forces a narrowly scoped signature exposure.
- Prefer private headers for shared declarations created by the split.
- Record awkward temporary dependencies or names in `todo.md`; do not solve
  cleanup work in this idea.
- After each code-changing step, run:

```bash
cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test -j10
ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_00204_c)$' --output-on-failure
```

- Before closure, the supervisor or plan owner must ensure full-suite proof:

```bash
ctest --test-dir build -j10 --output-on-failure
```

## Steps

### Step 1: Extract Shared Publication Helpers

Goal: create `dispatch_publication_common.cpp/hpp` for low-level helpers shared
by multiple later clusters.

Primary target:
`src/backend/mir/aarch64/codegen/dispatch_publication.cpp`

Concrete actions:

- Use `c4c-clang-tool-ccdb` to inspect signatures and caller/callee shape for
  representative helpers in this cluster.
- Move these helpers when dependency shape is clean:
  `registers_alias`, `integer_bit_width`, `power_of_two_shift`,
  `find_frame_slot`, `find_stack_object`,
  `prepared_frame_slot_load_address`, `relocation_operand`,
  `register_indirect_address`, `fixed_slots_use_frame_pointer`,
  `frame_slot_address`, `scalar_view_for_type`, `gp_register_name`,
  `scalar_load_mnemonic`,
  `dispatch_publication_scalar_type_size_bytes`,
  `scalar_load_mnemonic_for_width`, `scalar_store_mnemonic`,
  `scalar_integer_width_bits`, `scalar_gp_register_view`,
  `scalar_fp_register_view`, and `immediate_integer_bits`.
- Add the minimal header declarations needed by remaining publication code and
  later extraction files.
- Update build wiring and includes according to the existing AArch64 codegen
  source layout.

Completion check:

- The narrow proof command passes.
- The move is mechanical and no lowering behavior changes.
- Any helper intentionally left behind is recorded in `todo.md` with the
  dependency reason.

### Step 2: Extract Same-Block Producer Discovery

Goal: create `dispatch_producers.cpp/hpp` for same-block producer, select-chain,
global-load, block lookup, and join-source discovery.

Concrete actions:

- Inspect signatures and at least one caller or callee for the cluster.
- Move `find_same_block_binary_producer`,
  `find_same_block_select_producer`,
  `evaluate_same_block_integer_constant`,
  `select_chain_contains_direct_global_load`,
  `find_same_block_named_producer`, `producer_instruction_index`,
  `find_load_global_target`, `load_global_symbol_label`,
  `find_bir_block`, and `is_current_block_join_parallel_copy_source`.
- Expose only the declarations needed by materialization, edge-copy, and store
  source code.

Completion check:

- The narrow proof command passes.
- Producer helpers are no longer mixed into the oversized publication file
  unless a documented temporary dependency blocks one helper.

### Step 3: Extract Value Materialization

Goal: create `dispatch_value_materialization.cpp/hpp` for prepared value and
home materialization.

Concrete actions:

- Inspect signatures and caller/callee shape for the cluster.
- Move `emit_fp_immediate_to_register`, `emit_fp_value_to_register`,
  `emit_prepared_global_symbol_load_to_register`,
  `emit_prepared_va_list_field_load_to_register`,
  `emit_prepared_pointer_value_load_to_register`,
  `emit_prepared_value_home_to_register`,
  `emit_value_publication_to_register`,
  `lower_local_slot_address_publication`,
  `lower_stack_homed_pointer_value_load_publication`,
  `lower_scalar_cast_publication_to_prepared_register`,
  `lower_scalar_fp_binary_publication_to_prepared_register`, and
  `lower_scalar_cast_publication_to_prepared_stack`.
- Keep temporary state-tracking helpers central if moving them would create a
  broad non-mechanical rewrite.

Completion check:

- The narrow proof command passes.
- The extracted file is under 4000 lines and keeps behavior unchanged.
- Any central state helpers left in `dispatch_publication.cpp` are recorded in
  `todo.md`.

### Step 4: Extract Edge Copy and Select-Chain Handling

Goal: create `dispatch_edge_copies.cpp/hpp` for edge, predecessor, join-source,
and select-chain materialization helpers.

Concrete actions:

- Inspect signatures and caller/callee shape for the cluster.
- Move `prepared_edge_select_source_is_destination_register`,
  `unique_branch_predecessor_context`, `find_edge_named_producer`,
  `prepared_memory_access`, `prepared_memory_access_matches_instruction`,
  `edge_value_publication_may_read_register_index`,
  `emit_edge_load_local_to_register`,
  `emit_edge_value_publication_to_register`,
  `lower_predecessor_join_source_publication`, `select_chain_label`,
  `emit_select_chain_value_to_register`,
  `make_select_chain_materialization_instruction`, and
  `materialize_direct_global_select_chain_call_argument`.

Completion check:

- The narrow proof command passes.
- Edge-copy and select-chain logic is separated without generated assembly
  changes.

### Step 5: Extract Store Source and Writeback Handling

Goal: create `dispatch_store_sources.cpp/hpp` for store source legalization,
pointer/store writeback helpers, and global-store publication helpers.

Concrete actions:

- Inspect signatures and caller/callee shape for the cluster.
- Move `store_local_uses_pointer_value_address`,
  `prepared_or_emitted_store_value_register`, `local_slot_reference_name`,
  `local_slot_reference_matches`, `prepared_frame_slot_object_name`,
  `prepared_load_local_frame_object_name`, `value_name_has_slot_prefix`,
  `parse_trailing_dot_offset`, `store_local_targets_logical_slot`,
  `find_latest_narrow_store_for_wide_local_load`,
  `store_local_value_is_byval_frame_slot_load`,
  `store_local_value_is_wide_load_from_narrow_local_store`,
  `store_local_value_cast_producer`,
  `store_local_value_has_select_producer`,
  `store_local_value_has_scalar_fp_binary_producer`,
  `emit_scalar_conversion_cast_to_register`,
  `lower_store_local_value_publication`,
  `lower_stack_homed_pointer_store_writeback`,
  `prepared_global_symbol_from_value_name`,
  `emit_global_symbol_address_to_register`,
  `emit_pointer_base_plus_offset_to_register`,
  `store_local_frame_slot_offset`,
  `lower_pointer_base_plus_offset_store_local_publication`,
  `is_store_global_select_snapshot_run_instruction`,
  `lower_pending_store_global_stack_value_publications`, and
  `lower_store_global_value_publication`.

Completion check:

- The narrow proof command passes.
- Store-source and writeback logic is separated without behavior changes.

### Step 6: Final Size and Dependency Pass

Goal: confirm the mechanical split meets the source idea completion criteria and
record follow-up cleanup work without doing it here.

Concrete actions:

- Measure `dispatch_publication.cpp` and all newly extracted publication files.
- Confirm every publication-related implementation file is under 4000 lines.
- Review `todo.md` for intentionally retained state helpers, awkward temporary
  dependencies, or naming issues.
- If cleanup is needed, record it as follow-up notes for the supervisor to turn
  into a separate idea; do not fold cleanup into this mechanical split.
- Run the full-suite proof required by the source idea before closure.

Completion check:

- `dispatch_publication.cpp` is below 4000 lines.
- No extracted publication-related file is over 4000 lines.
- The full-suite proof passes.
- Remaining cleanup is documented outside implementation changes for this
  idea.
