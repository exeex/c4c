# AArch64 Extern Data Symbol PIC Address Formation Runbook

Status: Active
Source Idea: ideas/open/308_aarch64_extern_data_symbol_pic_address_formation.md
Supersedes: parked umbrella inventory in ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Implement the focused post-307 owner for AArch64 externally binding data-symbol
address formation.

## Goal

Make `stdout`-like externally binding data symbols use PIC-safe AArch64 address
formation while preserving legal direct local/internal symbol addressing.

## Core Rule

Progress must come from semantic symbol binding/address lowering. Do not use
filename-specific shortcuts, emitted-instruction string matching, expectation
changes, allowlists, unsupported classifications, CTest registration changes,
runner changes, timeout policy changes, or proof-log edits.

## Read First

- `ideas/open/308_aarch64_extern_data_symbol_pic_address_formation.md`
- `ideas/open/295_backend_regex_failure_family_inventory.md` post-307 split
  note
- `test_before.log` entry for `c_testsuite_aarch64_backend_src_00189_c`
- `build/c_testsuite_aarch64_backend/src/00189.c.s`
- Existing AArch64 symbol address materialization and relocation lowering code
  before editing

## Current Targets

- Focused failing test: `c_testsuite_aarch64_backend_src_00189_c`
- Generated bad form:
  `adrp x10, stdout` / `add x10, x10, :lo12:stdout` / `ldr x9, [x10]`
- Linker failure:
  `R_AARCH64_ADR_PREL_PG_HI21` against externally binding
  `stdout@@GLIBC_2.17`

## Non-Goals

- Do not repair broad runtime nonzero, runtime mismatch, crash, timeout, or
  output-storm buckets under this owner.
- Do not repair parked machine-printer/prepared-node residuals `00140`,
  `00164`, or `00214`.
- Do not repair parked semantic `lir_to_bir` admission residuals `00204` or
  `00216`.
- Do not reopen closed owners 285 through 307 unless generated-code evidence
  proves their closure boundary is wrong.
- Do not route every symbol through an over-broad fallback that hides local
  symbol-address regressions.

## Working Model

- Compiler-owned globals, function pointers, string literals, and internal
  labels can still use direct local symbol address formation when the target
  ABI permits it.
- Externally binding data objects such as libc `stdout` may be interposed or
  dynamically resolved and must not be emitted as direct local
  `R_AARCH64_ADR_PREL_PG_HI21` relocations in shared/PIC link contexts.
- The owner should find the semantic decision point for symbol binding or
  linkage and lower external data-symbol addresses through an AArch64
  PIC-safe access path.

## Execution Rules

- Keep the patch narrow to symbol binding/address formation and any tests that
  directly prove it.
- Prefer existing backend helper APIs and symbol metadata over ad hoc string
  checks.
- Preserve local/internal symbol address tests and compiler-owned globals.
- Record proof in canonical executor state and `test_after.log` as delegated
  by the supervisor.

## Ordered Steps

### Step 1: Locate Symbol Binding Address Lowering

Goal: identify the existing path that emits direct AArch64 symbol address
formation for data-symbol references.

Primary targets: AArch64 backend symbol materialization, relocation, and
machine-printing code.

Actions:

- Trace the lowered value for `stdout` in `00189.c` from frontend/HIR/BIR/LIR
  metadata into the AArch64 address-form emission path.
- Identify what metadata distinguishes local/internal symbols from externally
  binding data symbols.
- Check existing tests for symbol address, GOT/PIC, global object, and string
  literal behavior.
- Do not edit implementation until the direct `stdout` path and preservation
  cases are understood.

Completion check:

- Executor notes identify the emitting function/helper, available symbol
  metadata, and the local-symbol cases that must remain direct.

### Step 2: Implement PIC-Safe External Data-Symbol Address Formation

Goal: lower externally binding data-symbol references through a linker-safe
AArch64 sequence.

Primary target: the helper or lowering path found in Step 1.

Actions:

- Add or reuse a semantic predicate for externally binding data symbols.
- Emit a PIC-safe address sequence for external dynamic data symbols such as
  `stdout`.
- Keep local/internal/global compiler-owned symbols on their existing legal
  path unless the current metadata proves they also require the external path.
- Avoid hard-coded symbol names, c-testsuite filenames, and emitted text
  pattern checks.

Completion check:

- Generated assembly for `00189.c` no longer contains the direct non-PIC
  `adrp`/`:lo12:` relocation pair against `stdout`, and local symbol address
  behavior remains covered by narrow tests.

### Step 3: Prove Focused Behavior

Goal: validate the focused repair and reject named-test-only progress.

Actions:

- Rebuild the affected target or default preset as delegated by the
  supervisor.
- Run the focused c-testsuite proof for
  `c_testsuite_aarch64_backend_src_00189_c`.
- Run adjacent backend symbol-address tests, or add narrow tests if no
  existing coverage proves local-vs-external symbol address behavior.
- Preserve canonical `test_after.log` for executor proof if delegated.

Completion check:

- Proof shows the `00189.c` linker relocation failure is gone and no adjacent
  symbol-address behavior regressed.

### Step 4: Handoff for Review and Broader Validation

Goal: prepare the slice for supervisor review without claiming unrelated
bucket progress.

Actions:

- Summarize the semantic change, focused proof, and any residual outcome of
  `00189.c` after the relocation failure is fixed.
- List parked umbrella buckets that remain out of scope.
- Do not mark the umbrella inventory complete.

Completion check:

- `todo.md` records proof and residual notes for this owner, and the
  supervisor has enough information to choose broader backend-regex validation.
