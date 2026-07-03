# RV64 Inline Asm Carrier Lowering Runbook

Status: Active
Source Idea: ideas/open/571_rv64_inline_asm_carrier_lowering.md

## Purpose

Repair the RV64 object-route handling for BIR `CallInst` nodes whose callee is
`llvm.inline_asm`, so inline asm carrier calls stop falling through to the
generic unsupported-instruction fallback.

## Goal

Lower or explicitly model side-effecting RV64 inline asm carrier calls, while
preserving precise inline-asm-specific diagnostics for forms that remain
unsupported.

## Core Rule

This plan is for inline asm carriers only. Do not implement general call ABI
lowering, pointer arithmetic, select, floating-point binary, or branch-published
phi lowering here, and do not claim representative testcase progress through
expectation, unsupported-marker, allowlist, or runtime-output changes.

## Read First

- `ideas/open/571_rv64_inline_asm_carrier_lowering.md`
- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/classification.tsv`
- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/summary.md`
- per-case logs under
  `build/agent_state/570_unsupported_instruction_fragment_diagnostics/`
- `.codex/skills/c4c-clang-tools/SKILL.md` before broad C++ exploration

## Current Targets

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- nearby RV64 operand/materialization helpers used by object emission
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- representative external rows:
  - `src/20071211-1.c`
  - `src/pr51933.c`
  - `src/pr56982.c`
  - `src/pr78438.c`

## Non-Goals

- Do not add general RV64 same-module or external function call ABI lowering.
- Do not repair pointer arithmetic, select, floating-point binary, or
  branch-published phi lowering.
- Do not edit runtime comparison files, expected outputs, unsupported markers,
  or allowlists.
- Do not add filename-specific or exact-source-shape matching for the four
  representatives.
- Do not treat `src/pr78438.c` as arithmetic or shift progress until the inline
  asm carrier blocker is addressed.

## Working Model

- The 570 evidence classified four retained representatives as failing first
  on a BIR `CallInst` with `owner=none` and callee `llvm.inline_asm`.
- These cases reach BIR, prepared BIR, and MIR; the remaining blocker is in the
  RV64 object route.
- The carrier contract should handle side-effecting no-result inline asm calls
  and ordinary operands only to the extent needed for the carrier semantics.
- Unsupported inline asm constraints or forms should fail with a narrower
  inline-asm-specific diagnostic, not the old generic
  `unsupported_instruction_fragment` fallback.

## Execution Rules

- Keep `todo.md` as the mutable packet state.
- Use targeted object-emission tests as the primary proof for carrier lowering
  and diagnostic behavior.
- Use the four representative RV64 gcc_torture object-route reruns as external
  evidence after focused backend tests pass.
- Preserve existing diagnostic category stability for unsupported cases unless
  a narrower inline-asm-specific category already exists locally.
- Keep all changes semantic and carrier-oriented; reject named-case shortcuts
  and expectation rewrites as progress.

## Steps

### Step 1: Inspect Inline Asm Carrier Representation

Goal: Identify the exact BIR, prepared-BIR, and object-emission representation
for RV64 inline asm carrier calls.

Actions:
- Use `c4c-clang-tools` first for symbol and caller/callee queries around RV64
  call emission, `CallInst` handling, and inline asm detection.
- Inspect the 570 classification and per-case logs for the four carrier
  representatives.
- Record the BIR callee shape, argument/value ownership, side-effect marker,
  memory clobber representation, and symbol-address operand shape.
- Identify the first object-emission branch where `llvm.inline_asm` carriers
  should be intercepted before the generic fallback.

Completion Check:
- `todo.md` records the representation inventory, the carrier intercept point,
  and any unsupported inline asm constraint/form that must remain diagnostic
  only.

### Step 2: Add Focused Carrier Tests

Goal: Establish focused backend tests for the carrier contract before changing
lowering behavior.

Actions:
- Add or extend object-emission tests for a no-result side-effecting inline asm
  carrier with a memory clobber.
- Add or extend a test for the symbol-address operand shape seen in
  `src/pr51933.c`.
- Assert that the carrier path does not reach the generic
  `unsupported_instruction_fragment` fallback.
- Add a negative focused case if unsupported constraints/forms must still fail,
  and assert the narrower inline-asm-specific diagnostic.

Completion Check:
- The new or updated tests fail for the expected carrier/diagnostic reason
  before the implementation change, or `todo.md` records why the existing test
  harness cannot express that precondition.

### Step 3: Implement RV64 Inline Asm Carrier Object Emission

Goal: Lower the supported inline asm carrier forms through the RV64 object
route without broad call-lowering changes.

Actions:
- Add the smallest RV64 object-emission path needed to recognize
  `llvm.inline_asm` carrier `CallInst` nodes.
- Emit the inline asm body and supported operands according to the local object
  emission model.
- Preserve side-effect and memory-clobber ordering constraints represented in
  BIR/prepared BIR.
- Route unsupported constraints, result-producing inline asm, or unmodeled
  operand forms to a precise inline-asm-specific diagnostic.
- Avoid changing general call emission, unrelated operand materialization, or
  other RV64 lowering families.

Completion Check:
- `cmake --build --preset default` succeeds.
- `ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'`
  passes or any failure is recorded as a blocker in `todo.md`.

### Step 4: Rerun The Four Carrier Representatives

Goal: Produce external evidence that the inline asm carrier family no longer
falls into the generic unsupported-instruction fallback.

Actions:
- Rerun the RV64 gcc_torture object route for:
  - `src/20071211-1.c`
  - `src/pr51933.c`
  - `src/pr56982.c`
  - `src/pr78438.c`
- Save compact logs or summaries under a new
  `build/agent_state/571_rv64_inline_asm_carrier_lowering/` artifact
  directory.
- Classify each representative as lowered, still unsupported with a narrower
  inline-asm-specific diagnostic, or blocked by a later non-inline-asm owner.
- Do not edit testcase expectations, unsupported markers, allowlists, or
  runtime comparison files as part of the proof.

Completion Check:
- The artifact directory records all four reruns and proves none of the carrier
  representatives still fail first through the old generic
  `unsupported_instruction_fragment` inline asm carrier path.

### Step 5: Review And Close Readiness

Goal: Decide whether the source idea is satisfied by the carrier lowering,
focused tests, and representative evidence.

Actions:
- Confirm the source acceptance criteria are satisfied.
- Confirm focused tests cover at least the no-result memory-clobber carrier and
  the symbol-address operand shape from `src/pr51933.c`.
- Confirm unsupported forms retain precise inline-asm-specific diagnostics.
- Confirm no unrelated RV64 lowering family, expectation file, unsupported
  marker, allowlist, or runtime comparison file changed.
- Record close readiness and proof state in `todo.md`.

Completion Check:
- `todo.md` records close readiness for plan-owner evaluation and the close
  gate has a clear backend CTest plus representative-rerun validation path.
