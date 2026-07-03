# RV64 Unsupported Instruction Fragment Owner Diagnostics Runbook

Status: Active
Source Idea: ideas/open/570_rv64_unsupported_instruction_fragment_owner_diagnostics.md

## Purpose

Turn the generic RV64 object-route `unsupported_instruction_fragment` failure
into first-owner diagnostics that can split durable implementation follow-ups.

## Goal

Improve diagnostics for the nine retained representatives so each failure names
the unsupported BIR instruction, operation kind, value types, function/block
context, and any relevant prepared authority context without claiming lowering
capability progress.

## Core Rule

This is a diagnostic and evidence-enabling plan only. Do not implement the
unsupported RV64 lowering operations discovered by the improved diagnostics,
do not change testcase expectations or allowlists, and do not claim a testcase
is fixed because the diagnostic became more precise.

## Read First

- `ideas/open/570_rv64_unsupported_instruction_fragment_owner_diagnostics.md`
- `ideas/closed/549_rv64_runtime_and_no_diagnostic_triage.md`
- `build/agent_state/549_step2_first_owner_classification/classification.tsv`
- `build/agent_state/549_step2_first_owner_classification/stage_matrix.tsv`
- `build/agent_state/549_step2_first_owner_classification/classification.md`
- `.codex/skills/c4c-clang-tools/SKILL.md` before broad C++ exploration

## Current Targets

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- nearby RV64 prepared traversal/object-route diagnostic helpers if the first
  inspection proves the generic diagnostic is emitted there
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/`

## Representatives

- `src/20030408-1.c`
- `src/20000412-2.c`
- `src/20071211-1.c`
- `src/pr51933.c`
- `src/pr56982.c`
- `src/20000605-1.c`
- `src/pr78438.c`
- `src/20000622-1.c`
- `src/20000819-1.c`

## Non-Goals

- Do not implement the unsupported instruction lowerings discovered by this
  diagnostic pass.
- Do not treat the nine representatives as one opcode repair bucket.
- Do not do F128 quarantine work here.
- Do not repair prepared move-bundle classifier behavior for
  `src/20001026-1.c`.
- Do not edit runtime comparison, expected output, unsupported markers, or
  allowlists.
- Do not add named-case matching for the representative source files.

## Working Model

- The 549 evidence proves the retained representatives reach RV64 object
  lowering with BIR, prepared BIR, and MIR dumps available.
- The current generic diagnostic hides the first unsupported instruction, so
  later implementation ideas cannot safely choose an owner.
- The correct output of this plan is more precise diagnostics plus an evidence
  artifact. Follow-up implementation ideas should be split only after the new
  diagnostics identify high-confidence owner families.
- Large C++ exploration should start with `c4c-clang-tools` queries instead of
  reading all of `object_emission.cpp` by raw text.

## Execution Rules

- Keep `todo.md` as the mutable packet state.
- Use targeted object-emission tests as the primary proof for diagnostic text.
- Use representative RV64 gcc_torture single-case reruns to prove the emitted
  diagnostics are useful on external rows.
- Preserve the existing diagnostic category
  `unsupported_instruction_fragment`; enrich the message/context rather than
  hiding the bucket behind a new vague name.
- If inspection proves a required context source is unavailable without a
  broader refactor, record the blocker and split a follow-up idea instead of
  overfitting a local workaround.

## Steps

### Step 1: Inventory Diagnostic Emission Paths

Goal: Identify every current object-route path that emits the generic
`unsupported_instruction_fragment` diagnostic and the instruction/context data
available at that point.

Actions:
- Use `c4c-clang-tools` first for AST-backed queries around the diagnostic
  helpers and direct callers in RV64 object emission.
- Inspect the generic diagnostic sites in
  `src/backend/mir/riscv/codegen/object_emission.cpp` and any nearby prepared
  traversal helpers.
- Map which path handles ordinary BIR instructions versus traversal-missing or
  atomic-specific cases.
- Record which context is already available: function name, block index,
  instruction kind, result type, operand types, prepared instruction pointer,
  and prepared authority or storage context.

Completion Check:
- `todo.md` records the emission-path inventory, the first patch target, and
  any context that cannot be surfaced without a separate refactor.

### Step 2: Add Structured Unsupported-Instruction Context

Goal: Enrich the generic unsupported-instruction diagnostic with first-bad-fact
context while preserving fail-closed behavior.

Actions:
- Add the smallest helper or local formatting path needed to describe the BIR
  instruction kind/opcode, result/source types, and function/block context.
- Include prepared authority or storage context only where it is already
  available through structured state.
- Update focused object-emission tests to assert representative diagnostic
  context without depending on whole rendered module text.
- Keep the `unsupported_instruction_fragment` category stable for downstream
  scanners.

Completion Check:
- `cmake --build --preset default` succeeds.
- `ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'`
  passes or any failure is recorded as a blocker in `todo.md`.

### Step 3: Re-Run The Nine Representatives

Goal: Produce auditable external evidence that each retained representative now
emits a diagnostic specific enough for first-owner routing.

Actions:
- Re-run the nine representatives through the RV64 gcc_torture object route.
- Save logs and a compact classification table under
  `build/agent_state/570_unsupported_instruction_fragment_diagnostics/`.
- For each row, record old generic owner, new diagnostic context, and likely
  follow-up owner family.
- Screen any primary-F128 row into the existing F128 quarantine lane instead of
  ordinary-C implementation work.

Completion Check:
- The artifact directory contains rerun logs and a table covering all nine
  representatives.
- Each retained row has a diagnostic more specific than the old generic
  message, or `todo.md` records why a row needs a separate evidence-gap idea.

### Step 4: Split Follow-Up Ideas

Goal: Convert refined diagnostics into durable follow-up ideas only where
owner families are high-confidence.

Actions:
- Create `ideas/open/*.md` follow-ups for coherent owner families discovered
  by the diagnostics.
- Keep diagnostic-only, low-confidence, or mixed-owner rows in an evidence-gap
  follow-up instead of mixing implementation scopes.
- Ensure each follow-up idea names owning layer, in-scope work, out-of-scope
  work, acceptance criteria, and reviewer reject signals.

Completion Check:
- Follow-up ideas exist for high-confidence owner families, and no idea mixes
  unrelated RV64 lowering, prepared authority, BIR producer, inline-asm, call
  boundary, or runtime work.

### Step 5: Review And Close Readiness

Goal: Decide whether the source idea is satisfied by the diagnostic patch,
evidence artifact, and generated follow-ups.

Actions:
- Confirm all acceptance criteria in the source idea are satisfied.
- Confirm no implementation lowering, testcase-shaped matching, expectation
  rewrite, unsupported-marker edit, allowlist edit, or runtime-comparison
  change entered this diagnostic plan.
- Record close readiness and proof state in `todo.md`.

Completion Check:
- `todo.md` records close readiness for plan-owner evaluation and the close
  gate has a clear backend CTest or supervisor-approved validation path.
