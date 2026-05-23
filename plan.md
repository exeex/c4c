# AArch64 Dispatch Mechanical Extraction Runbook

Status: Active
Source Idea: ideas/open/384_aarch64_dispatch_mechanical_extraction.md

## Purpose

Make `src/backend/mir/aarch64/codegen/dispatch.cpp` smaller and reviewable by
mechanically extracting helper clusters into focused implementation files while
preserving backend behavior.

Goal: reduce `dispatch.cpp` below 4000 lines without changing generated assembly,
MIR behavior, or the public dispatch API shape.

Core Rule: every code-changing step is a behavior-preserving extraction; do not
rewrite dispatch order, add backend capability, downgrade tests, or introduce
testcase-shaped shortcuts.

## Read First

- `ideas/open/384_aarch64_dispatch_mechanical_extraction.md`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.hpp`
- `CMakeLists.txt` files that enumerate AArch64 backend sources
- `.codex/skills/c4c-clang-tools/SKILL.md`

## Current Targets

- Primary implementation surface:
  `src/backend/mir/aarch64/codegen/dispatch.cpp`
- Public dispatch API:
  `src/backend/mir/aarch64/codegen/dispatch.hpp`
- Expected first-pass extraction files, adjusted only when AST-backed dependency
  inspection shows a better mechanical grouping:
  - `dispatch_dynamic_stack.cpp/hpp`
  - `dispatch_entry_formals.cpp/hpp`
  - `dispatch_diagnostics.cpp/hpp`
  - `dispatch_lookup.cpp/hpp`
  - `dispatch_branch_fusion.cpp/hpp`
  - `dispatch_publication.cpp/hpp`
  - `dispatch_calls.cpp/hpp`

## Non-Goals

- Do not change `dispatch_prepared_block` ordering or semantics.
- Do not redesign long-term ownership boundaries in this pass.
- Do not move logic into existing feature files such as `calls.cpp`,
  `memory.cpp`, `comparison.cpp`, or `returns.cpp` unless the move is strictly
  mechanical, low risk, and recorded in `todo.md` before execution.
- Do not change test expectations, generated assembly contracts, MIR behavior,
  or unsupported/supported classifications.
- Do not use this route to implement new AArch64 backend capability.

## Working Model

- Treat `dispatch.cpp` as the owner of the main block loop until helper clusters
  are separated and proven.
- Use new `dispatch_*` helper translation units as temporary mechanical
  extraction homes. A later idea may review whether any extracted pieces belong
  in existing feature files.
- Keep extracted helpers narrow and callable from the existing dispatch code.
- Prefer moving complete helper clusters over partial moves that leave hidden
  dependencies split across files.
- Record any grouping adjustment in `todo.md` before making broad ownership
  changes.

## Execution Rules

- Before each extraction step, run at least one AST-backed query from
  `.codex/skills/c4c-clang-tools`, such as:

```bash
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/dispatch.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/dispatch.cpp <function-name> build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/dispatch.cpp <function-name> build/compile_commands.json
```

- Keep each step buildable and reviewable on its own.
- Add extracted files to the normal build as CMake sources.
- Do not edit the source idea for routine execution findings; use `todo.md`
  unless durable source intent must change.
- A green narrow proof is required for every extraction step.
- Full-suite proof is required before this idea can close.

## Validation Ladder

Minimum proof for each extraction step:

```bash
cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test -j10
ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_00204_c)$' --output-on-failure
```

Close-time proof:

```bash
ctest --test-dir build -j10 --output-on-failure
```

## Steps

### Step 1: Extract Dynamic Stack and Entry Formal Helpers

Goal: establish the split style using the lowest-risk clusters named by the
source idea.

Primary target:
`src/backend/mir/aarch64/codegen/dispatch.cpp`

Concrete actions:

- Use AST-backed queries to inspect signatures, direct callees, and direct
  callers for the dynamic stack and entry formal helper clusters.
- Extract the dynamic stack cluster around:
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
- Extract the entry formal publication cluster around:
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
- Create `dispatch_dynamic_stack.cpp/hpp` and
  `dispatch_entry_formals.cpp/hpp`, or record a narrower grouping in `todo.md`
  before choosing different file names.
- Wire the new sources into the build.
- Keep all call sites behavior-preserving.

Completion check:

- `dispatch.cpp` builds with the extracted files as normal sources.
- The minimum extraction proof passes.
- `todo.md` records the AST query used and any grouping adjustment.

### Step 2: Extract Diagnostics Helpers

Goal: move diagnostic construction and formatting helpers out of
`dispatch.cpp` without changing error records or emitted text.

Concrete actions:

- Use AST-backed queries to identify diagnostic helper functions and their
  direct dependencies.
- Create `dispatch_diagnostics.cpp/hpp` if the cluster is mechanically
  separable.
- Preserve diagnostic data shapes, message text, and call-site behavior.
- Avoid mixing diagnostic extraction with semantic lowering edits.

Completion check:

- Diagnostic helpers compile from the extracted file.
- The minimum extraction proof passes.
- No expectation or diagnostic text changes are required.

### Step 3: Extract Lookup and Producer Lookup Helpers

Goal: separate value lookup, producer lookup, and related small access helpers
from the main dispatch loop.

Concrete actions:

- Use AST-backed caller/callee queries to identify lookup dependencies.
- Create `dispatch_lookup.cpp/hpp` for mechanically separable lookup helpers.
- Keep ownership and lifetime assumptions unchanged.
- Do not alter missing-value behavior or fallback diagnostics.

Completion check:

- Lookup call sites continue to compile through the extracted declarations.
- The minimum extraction proof passes.
- No behavior contract changes are introduced.

### Step 4: Extract Branch and Compare Fusion Helpers

Goal: isolate branch/compare fusion helpers while preserving current branch
selection and compare materialization behavior.

Concrete actions:

- Use AST-backed queries to identify fusion helpers and dependencies.
- Create `dispatch_branch_fusion.cpp/hpp` if mechanically separable.
- Keep fused and non-fused branch behavior identical.
- Do not change branch ordering, compare opcode selection, or MIR lowering
  contracts.

Completion check:

- Branch fusion helpers compile from the extracted file.
- The minimum extraction proof passes.
- Nearby branch/compare tests remain covered by the narrow subset.

### Step 5: Extract Publication Helpers

Goal: move value, store, and edge publication helpers into a focused
mechanical extraction file.

Concrete actions:

- Use AST-backed queries to map publication helper dependencies.
- Create `dispatch_publication.cpp/hpp`.
- Preserve publication order, notes, stores, and edge behavior.
- Do not combine publication extraction with new lowering support.

Completion check:

- Publication helpers compile from the extracted file.
- The minimum extraction proof passes.
- Existing note and dispatch tests remain green.

### Step 6: Extract Call Boundary and Indirect Call Helpers

Goal: mechanically isolate call boundary and indirect call materialization
helpers without moving them into long-term `calls.cpp` ownership yet.

Concrete actions:

- Use AST-backed queries to map call-related helper dependencies.
- Create `dispatch_calls.cpp/hpp` unless AST evidence shows a narrower
  temporary split is safer.
- Preserve call boundary, indirect call, and call materialization behavior.
- Do not add new call lowering capability.

Completion check:

- Call helpers compile from the extracted file.
- The minimum extraction proof passes.
- No call expectation changes are required.

### Step 7: Review Residual Dispatch Shape and Close Readiness

Goal: verify that the mechanical split achieved the source idea criteria and
that remaining `dispatch.cpp` content is intentionally tied to the main block
loop.

Concrete actions:

- Measure `dispatch.cpp` line count and confirm it is below 4000 lines.
- Review `dispatch.hpp` to confirm the public dispatch API remains small.
- Confirm all extracted files are normal CMake sources.
- Check the diff for behavior edits, expectation rewrites, unsupported
  downgrades, or testcase-specific shortcuts.
- Run the close-time full-suite proof before requesting source-idea closure.

Completion check:

- Source idea completion criteria are satisfied.
- Full-suite proof passes.
- Remaining follow-up ownership review is recorded as a separate idea if needed,
  not silently absorbed into this mechanical extraction route.
