# RV64 Prepared Emitter Decomposition Runbook

Status: Active
Source Idea: ideas/open/301_rv64_prepared_emitter_decomposition.md

## Purpose

Reshape the rv64 prepared emitter so future backend work lands in functional
owner files instead of continuing to grow
`src/backend/mir/riscv/codegen/emit.cpp`.

Goal: make the split behavior-preserving, mechanically reviewable, and backed
by rv64 runtime proof after each meaningful extraction.

Core Rule: do not add rv64 capabilities, weaken tests, or change emitted
semantics as part of this decomposition.

## Read First

- `ideas/open/301_rv64_prepared_emitter_decomposition.md`
- `src/backend/mir/aarch64/codegen/README.md`
- `src/backend/mir/riscv/codegen/emit.cpp`
- RISC-V codegen build metadata for `c4c_backend`

Use AArch64 as a layout reference, not as copy-paste implementation source.

## Current Scope

Move behavior-preserving clusters out of
`src/backend/mir/riscv/codegen/emit.cpp` into functional owner files such as:

- `prepared_module_emit.{hpp,cpp}` for public prepared-module orchestration.
- `prepared_function_emit.{hpp,cpp}` for function/block traversal, labels,
  prologue/epilogue coordination, and instruction dispatch.
- `prepared_scalar_emit.{hpp,cpp}` for scalar immediates, value movement,
  integer binary ops, casts, selects, and scalar returns.
- `prepared_call_emit.{hpp,cpp}` for direct scalar call lowering and prepared
  call-plan validation.
- `prepared_local_memory_emit.{hpp,cpp}` for frame-slot and local pointer memory
  lowering, including existing unaligned I32 bytewise helpers.
- `prepared_frame_emit.{hpp,cpp}` or a small support surface for stack-frame
  sizing, `ra` spill selection, and frame-slot offset helpers.
- `prepared_edge_publication_emit.{hpp,cpp}` for existing edge publication move
  adaptation if it can move cleanly.
- `emit.cpp` for compatibility wrappers, unavoidable public namespace glue, and
  any legacy direct-LIR tiny-add helper that still must remain.

## Non-Goals

- Do not add runtime support for globals, indirect calls, varargs, stack
  arguments, aggregate ABI, dynamic stack, or object emission.
- Do not rewrite RISC-V around AArch64 machine-node infrastructure in this
  first split.
- Do not change the qemu runner contract.
- Do not delete or semantically change the edge-publication path.
- Do not create testcase-shaped owner files, idea-number files, or a central
  catch-all records pile.

## Working Model

- Prefer owner-local helpers unless multiple owners actually need them.
- Keep extracted names boring and responsibility-oriented.
- Preserve public entrypoints and fail-closed behavior for unsupported rv64
  forms.
- Each code-moving step should be reviewable as a mechanical extraction plus
  build metadata updates.
- If a helper demands semantic redesign, stop and ask the supervisor to open a
  separate capability idea instead of changing behavior here.

## Execution Rules

- Before each extraction, inspect the cluster and identify the exact target
  owner file.
- Keep `emit.cpp` compiling at every step; do not leave a long-lived broken
  intermediate state.
- Update build metadata whenever a new `.cpp` file is introduced.
- Run the supervisor-delegated proof command exactly for each packet.
- Treat expectation downgrades, runtime test weakening, BIR/LLVM fallback
  acceptance, or named-testcase shortcuts as route failure.
- Record routine execution progress in `todo.md`; do not rewrite this runbook
  for normal packet completion.

## Step 1: Inventory Functional Clusters

Goal: map `emit.cpp` into behavior-preserving extraction targets.

Primary target: `src/backend/mir/riscv/codegen/emit.cpp`

Actions:

- Read the AArch64 codegen README and the current rv64 emitter.
- Identify frame, label, traversal, scalar, call, local-memory, edge
  publication, module entry, and compatibility-glue clusters.
- Decide which clusters must move first because other owners depend on their
  helpers.
- Record the proposed first extraction packet in `todo.md` without changing
  implementation behavior.

Completion check:

- `todo.md` names a concrete first implementation packet and proof command.
- No implementation files are changed by this inventory-only step unless the
  supervisor explicitly delegates implementation.

## Step 2: Extract Frame, Label, and Basic Support

Goal: move the support helpers needed by later owners into a narrow frame or
support surface.

Primary targets:

- `prepared_frame_emit.{hpp,cpp}` or an intentionally smaller support file
- `prepared_function_emit.{hpp,cpp}` if label/traversal ownership is separated
  at this point
- RISC-V codegen build metadata

Actions:

- Extract stack-frame sizing, alignment, `ra` spill-slot selection, frame-slot
  offset helpers, and label spelling only where ownership is clear.
- Keep shared types/helper declarations minimal.
- Leave behavior and public prepared-module entrypoints unchanged.

Completion check:

- The backend builds with the new owner file(s).
- The delegated rv64 runtime proof passes or preserves only a supervisor-owned
  documented baseline.
- `emit.cpp` no longer owns the moved support implementation.

## Step 3: Extract Scalar Returns and Scalar Operations

Goal: move scalar immediates, value movement, integer binary ops, casts,
selects, and scalar returns into a scalar owner.

Primary target: `prepared_scalar_emit.{hpp,cpp}`

Actions:

- Move existing scalar lowering helpers without expanding supported cases.
- Keep return emission behavior identical.
- Keep failure paths for unsupported scalar forms unchanged.
- Avoid testcase-specific matching.

Completion check:

- The backend builds.
- `ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure`
  passes when selected by the supervisor.
- Existing scalar behavior is covered by unchanged tests or documented baseline
  proof.

## Step 4: Extract Local Memory and Pointer Memory

Goal: move frame-slot and local pointer memory lowering into a local-memory
owner.

Primary target: `prepared_local_memory_emit.{hpp,cpp}`

Actions:

- Move local frame-slot loads/stores.
- Move pointer-width local slot load/store handling.
- Move local pointer/member-index memory accesses.
- Move existing unaligned I32 bytewise load/store helpers.
- Preserve fail-closed behavior for unsupported memory forms.

Completion check:

- The backend builds.
- The delegated rv64 runtime proof passes.
- Nearby RISC-V memory-focused tests are not weakened.

## Step 5: Extract Direct Scalar Calls

Goal: move direct scalar call lowering and call-plan validation into a call
owner.

Primary target: `prepared_call_emit.{hpp,cpp}`

Actions:

- Move existing direct scalar call lowering.
- Move argument/result register movement and validation.
- Keep unsupported indirect, vararg, stack-argument, and aggregate call forms
  fail-closed.

Completion check:

- The backend builds.
- The delegated rv64 runtime proof passes.
- No call capability is expanded under this decomposition idea.

## Step 6: Extract Edge Publication If Clean

Goal: move edge publication move adaptation into its own owner when doing so is
mechanical and behavior-preserving.

Primary target: `prepared_edge_publication_emit.{hpp,cpp}`

Actions:

- Inspect dependencies before moving the edge publication adapter.
- Extract only if the owner boundary is clear.
- Leave the adapter in `emit.cpp` temporarily if moving it would obscure
  behavior or force semantic redesign.

Completion check:

- Either the adapter lives in its owner file with green delegated proof, or
  `todo.md` records why this cluster is intentionally deferred.

## Step 7: Trim Module and Function Orchestration

Goal: reduce `emit.cpp` to public glue and push orchestration into module and
function owners.

Primary targets:

- `prepared_module_emit.{hpp,cpp}`
- `prepared_function_emit.{hpp,cpp}`
- `emit.cpp`

Actions:

- Move prepared-module text orchestration into the module owner.
- Move function/block traversal and dispatch into the function owner.
- Keep public wrappers and compatibility namespace glue in `emit.cpp` only
  where still needed.
- Ensure future scalar, call, memory, and frame work has an obvious owner other
  than `emit.cpp`.

Completion check:

- `emit.cpp` no longer owns the majority of scalar, call, local-memory, and
  frame helper implementation.
- The backend builds.
- The delegated rv64 runtime proof passes.

## Step 8: Broader Backend Validation

Goal: prove the decomposition did not break backend build/link surfaces or
RISC-V focused behavior.

Actions:

- Run the supervisor-selected broader check before closure.
- Preferred checks from the source idea:
  - `ctest --test-dir build -R 'backend_riscv|backend_codegen_route_riscv64' --output-on-failure`
  - `ctest --test-dir build -R '^backend_' --output-on-failure`
- Compare failures against documented baseline instead of rewriting
  expectations.

Completion check:

- RISC-V focused tests are monotonic.
- Broader backend validation passes or preserves only documented baseline
  failures accepted by the supervisor.
- The source idea acceptance criteria are satisfied.
