# 384 AArch64 Dispatch Mechanical Extraction

## Context

`src/backend/mir/aarch64/codegen/dispatch.cpp` is currently over 10k lines while
`dispatch.hpp` exposes only the block-context constructor and prepared-block
dispatch entrypoint. The implementation has accumulated many unrelated private
helpers inside one translation unit:

- dynamic stack lowering
- entry formal publication
- value lookup and producer lookup
- branch/compare fusion
- value/store publication
- edge publication
- call boundary and indirect call materialization
- diagnostics

The current full-suite baseline is green, so this idea is intentionally about
mechanical extraction first. Do not redesign ownership boundaries in this pass.
After the code is split into smaller files, review the resulting shape and then
decide whether some pieces should move to existing files such as `calls.cpp`,
`memory.cpp`, `comparison.cpp`, or `returns.cpp`.

## Goal

Split `dispatch.cpp` into smaller implementation files while preserving behavior.
The immediate goal is to make the current responsibilities visible and reviewable,
not to finalize the long-term ownership model.

## Non-Goals

- Do not rewrite the dispatch order in `dispatch_prepared_block`.
- Do not change generated assembly or MIR behavior.
- Do not fold logic into `calls.cpp` or other existing feature files yet unless
  the move is strictly mechanical and low risk.
- Do not use testcase-specific matching or expectation rewrites.
- Do not use this idea to add new backend capability.

## Required Tooling

Executor packets for this idea should use `.codex/skills/c4c-clang-tools`.
`dispatch.cpp` is too large to split by visual inspection alone.

Before each extraction packet, use at least one AST-backed query such as:

```bash
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/dispatch.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/dispatch.cpp <function-name> build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/dispatch.cpp <function-name> build/compile_commands.json
```

Use these queries to identify clusters and direct dependencies before moving code.

## Extraction Strategy

Start with low-coupling clusters. Keep `dispatch.cpp` as the owner of the main
block loop until the helper clusters are separated and proven.

Suggested first-pass files:

- `dispatch_diagnostics.cpp/hpp`
- `dispatch_dynamic_stack.cpp/hpp`
- `dispatch_entry_formals.cpp/hpp`
- `dispatch_lookup.cpp/hpp`
- `dispatch_branch_fusion.cpp/hpp`
- `dispatch_publication.cpp/hpp`
- `dispatch_calls.cpp/hpp`

These names are provisional. If extraction shows a better grouping, record it in
`todo.md` during execution before making larger ownership changes.

## First Packet Preference

Start with `dispatch_dynamic_stack.cpp/hpp` and `dispatch_entry_formals.cpp/hpp`.
Those clusters are relatively self-contained and are useful probes for the split
style:

- dynamic stack cluster around:
  - `is_dynamic_alloca_helper`
  - `dynamic_stack_helper_kind`
  - `find_dynamic_stack_op`
  - `find_dynamic_stack_value_home`
  - `dynamic_stack_register_home_name`
  - `dynamic_stack_stack_home_address`
  - `dynamic_stack_home_requires_stable_frame_pointer`
  - `dynamic_stack_element_size_bytes`
  - `make_dynamic_stack_rejection_record`
  - `make_dynamic_stack_assembler_instruction`
  - `make_dynamic_stack_failure`
  - `lower_dynamic_stack_save`
  - `lower_dynamic_alloca`
  - `lower_dynamic_stack_restore`
  - `lower_dynamic_stack_helper_call`

- entry formal publication cluster around:
  - `entry_formal_register_view`
  - `entry_formal_load_opcode`
  - `entry_formal_store_opcode`
  - `entry_formal_same_aarch64_register_bank`
  - `entry_formal_aarch64_register_slot_count`
  - `entry_formal_aarch64_register_slot_start`
  - `entry_formal_abi_register_index`
  - `entry_formal_source_register`
  - `entry_formal_store_lines`
  - `entry_formal_load_lines`
  - `entry_formal_stack_argument_size_bytes`
  - `entry_formal_stack_argument_alignment_bytes`
  - `entry_formal_uses_incoming_stack`
  - `entry_formal_incoming_stack_offset`
  - `function_has_call`
  - `entry_formal_frame_size_bytes`
  - `entry_formal_fixed_home_base_register`
  - `append_entry_formal_byte_store`
  - `append_entry_formal_byte_load`
  - `entry_formal_byval_aggregate_store_lines`
  - `entry_formal_byval_aggregate_stack_source_publication_lines`
  - `entry_formal_destination_register`
  - `record_entry_formal_register_home`
  - `entry_formal_stack_source_publication_lines`
  - `entry_formal_register_move_lines`
  - `make_entry_formal_publication_instruction`
  - `lower_entry_formal_publications`

## Proof

Each extraction packet must at minimum run:

```bash
cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test -j10
ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_00204_c)$' --output-on-failure
```

Broader proof is required before closing this idea:

```bash
ctest --test-dir build -j10 --output-on-failure
```

## Completion Criteria

- `dispatch.cpp` is below 4000 lines.
- The extracted files compile as normal CMake sources.
- The public dispatch API remains small and clear.
- No behavior-changing edits are mixed into the mechanical extraction commits.
- Full-suite baseline remains green.

## Closure Note

Closed after the mechanical extraction reduced `dispatch.cpp` to 2745 lines,
kept `dispatch.hpp` at a small 26-line public API, and wired the extracted
`dispatch_*.cpp` files as normal CMake sources. No tests or expectations were
changed as part of the extraction route. The close-time full-suite evidence in
`test_baseline.log` reports 3381/3381 tests passing.
