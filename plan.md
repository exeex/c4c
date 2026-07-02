# RV64 Object Frame And Stack Helper Cleanup Runbook

Status: Active
Source Idea: ideas/open/535_rv64_object_frame_stack_helper_cleanup.md

## Purpose

Reduce RV64 object-emission coupling by extracting pure frame sizing, stack
offset, register-home lookup, basic stack load/store, and stack adjustment
helpers into the existing prepared frame helper boundary without changing
object-route behavior.

## Goal

Make the RV64 frame and stack helper ownership smaller and reviewable while
preserving stack-frame size, offset meaning, formal-entry home semantics,
diagnostics, and emitted object behavior.

## Core Rule

This is a behavior-preserving cleanup only. Do not change frame sizing,
alignment, ABI behavior, diagnostics, unsupported contracts, gcc_torture
expectations, runtime comparisons, or RV64 capability.

## Read First

- `ideas/open/535_rv64_object_frame_stack_helper_cleanup.md`
- `docs/rv64_object_emission_cleanup/structure_baseline.md`
- `docs/rv64_object_emission_cleanup/aarch64_comparison.md`
- `docs/rv64_object_emission_cleanup/staged_followups.md`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_frame_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_frame_emit.hpp`

## Current Targets

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_frame_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_frame_emit.hpp`

## Non-Goals

- Do not move call-specific byval/sret argument publication, before-return
  bundles, function traversal, or prepared instruction dispatch.
- Do not move local-memory semantics, global symbol materialization, data-object
  assembly, text module assembly, relocation handling, or ELF writing.
- Do not change frame-size, alignment, ABI, diagnostic, unsupported-contract,
  runtime, or object-byte behavior.
- Do not repair RV64 semantics or adjust tests, unsupported markers, allowlists,
  expected output, or pass/fail accounting.

## Working Model

- `object_emission.cpp` currently mixes frame math with call lowering, memory
  access, formal-entry homes, object-function traversal, and final object
  assembly.
- `prepared_frame_emit.*` is the intended narrow owner for pure frame and stack
  helper behavior.
- Early movement should only cover helpers whose dependencies are frame layout,
  stack slots, register homes, simple stack load/store emission, or stack
  adjustment.
- Helpers that require call-specific publication, local memory semantics,
  broad instruction dispatch, or function traversal stay in `object_emission.cpp`
  for later ideas.

## Execution Rules

- Start with a dependency and declaration map before moving code.
- Keep helper APIs structured around existing prepared frame facts; do not infer
  missing facts from BIR, target text, object bytes, or testcase shape.
- Prefer compatibility wrappers in `object_emission.cpp` when they keep the
  object-route boundary readable and avoid broad call-site churn.
- Preserve prepared stack-layout diagnostics and object-route observable
  behavior exactly.
- If the first pass proves no safe extraction exists, record the blocker in
  `todo.md` and stop rather than forcing a rename-only slice.

## Steps

### Step 1 - Map Frame And Stack Helper Surfaces

Goal: identify the exact frame and stack helper set that can move before code
movement.

Primary target: `object_emission.cpp` and `prepared_frame_emit.*`.

Actions:

- Use AST-backed symbol queries where practical, then targeted source reads, to
  list frame sizing, stack-slot offset, register-home lookup, stack load/store,
  and stack adjustment helpers.
- Classify each helper as pure frame/stack behavior, call-specific,
  local-memory-specific, traversal-specific, diagnostic-only, or object-module
  assembly dependent.
- Identify current consumers and complete-type/header prerequisites for the
  safe helper set.
- Record unsafe helpers whose signatures or callers depend on byval/sret calls,
  before-return bundles, local/global memory semantics, prepared instruction
  dispatch, function traversal, or final object assembly.
- Update `todo.md` with the selected Step 2 boundary and exact validation
  command for the first implementation packet.

Completion check:

- `todo.md` names the safe helper set, destination owner, compatibility wrappers
  to preserve, unsafe helpers, and validation command for Step 2.

### Step 2 - Extract Pure Frame And Stack Helpers

Goal: move only the helpers proven pure in Step 1 into the prepared frame helper
boundary.

Primary target: `prepared_frame_emit.*`.

Actions:

- Move declarations and definitions for pure frame sizing, stack-slot offset,
  register-home lookup, basic stack load/store, or simple stack adjustment
  helpers.
- Keep call-specific byval/sret publication, before-return bundles, broad
  function traversal, and prepared instruction dispatch in `object_emission.cpp`.
- Preserve prepared stack-layout diagnostics, emitted instruction bytes,
  register choices, stack offsets, and formal-entry home behavior.
- Keep compatibility wrappers where that reduces churn and preserves the
  object-route readability boundary.

Completion check:

- The build succeeds and the focused backend proof passes:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|dump_riscv64_stack_passed_parameter_home_publication|obj_runtime_rv64_large_fixed_frame_slot_access|obj_runtime_rv64_frame_slot_pointer_arg_preserves_payload)'`

### Step 3 - Probe Remaining Frame Cleanup

Goal: decide whether any remaining direct include, wrapper, or API cleanup is
safe after the helper movement.

Actions:

- Check whether any retained object-route wrappers are redundant after Step 2
  without exposing call, memory, traversal, or module dependencies through the
  prepared frame helper API.
- Remove only wrappers or includes that no longer protect ownership,
  dependency shape, or readability.
- Keep wrappers when they document object-route semantics, avoid broad
  call-site churn, or prevent dependency expansion.

Completion check:

- Either a small direct cleanup lands with the same proof as Step 2, or
  `todo.md` records why further movement is parked.

### Step 4 - Close Readiness Review

Goal: prove the idea is complete or record any remaining boundary as a separate
follow-up.

Actions:

- Review the diff against the source idea reject signals.
- Confirm stack-frame size, alignment, formal-entry home meaning, diagnostics,
  object bytes, and validation expectations were preserved.
- Confirm no call lowering, local memory semantics, broad traversal, or
  prepared instruction dispatch moved under this frame cleanup label.
- Ask the supervisor for broader backend proof if helper movement affected a
  wider object-route surface than the focused tests cover.

Completion check:

- The runbook has no remaining in-scope frame/stack helper movement,
  `todo.md` records the final proof, and closure can be evaluated against the
  source idea.
