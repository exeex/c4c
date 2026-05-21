# Semantic BIR Indirect Local-Memory Lvalue Admission Runbook

Status: Active
Source Idea: ideas/open/367_semantic_bir_indirect_local_memory_lvalue_admission.md
Supersedes: ideas/open/295_backend_regex_failure_family_inventory.md active umbrella runbook

## Purpose

Repair the semantic `lir_to_bir` admission gap for indirect local-memory
lvalues whose address comes from pointer values, then hand only advanced or
new first-bad-fact residuals back to later focused owners.

## Goal

Make pointer-derived local-memory load and store lvalues lower as real
pointer-based memory operations before AArch64 preparation.

## Core Rule

Fix semantic lowering capability. Do not claim progress through expectations,
unsupported classifications, runner behavior, timeout policy, CTest
registration, proof-log policy, AArch64 runtime changes, or filename-shaped
matching.

## Read First

- `ideas/open/367_semantic_bir_indirect_local_memory_lvalue_admission.md`
- `todo.md`
- `ideas/open/295_backend_regex_failure_family_inventory.md` latest
  deactivation note for the split context
- Current semantic and backend-route tests around `lir_to_bir` local-memory
  load/store diagnostics before editing code

## Current Targets

- `c_testsuite_aarch64_backend_src_00005_c`: store local-memory rejection for
  the pointer-to-pointer local shape `**pp = 1`.
- `c_testsuite_aarch64_backend_src_00217_c`: load local-memory rejection for
  pointer arithmetic plus casted unsigned update,
  `*(unsigned*)(data + r) += a - b`.
- Focused semantic or backend-route coverage proving indirect local-memory
  load and store lvalues use the computed pointer address.

## Non-Goals

- Do not edit AArch64 register allocation, machine printing, assembler,
  linker, runtime, runner, timeout, CTest registration, unsupported
  classifications, or external expectations for this owner.
- Do not clean up local backend-route snippet drift unless separately
  delegated.
- Do not reopen the completed `00173` pointer-derived string-load/publication
  chain unless fresh evidence shows its old first bad fact returned.
- Do not absorb composite/byval/HFA/f128 ABI, aggregate writeback, scalar
  compare/select, initializer, dynamic stack, variadic/floating, timeout, or
  runtime mismatch buckets.

## Working Model

The failing representatives stop before prepared-module or AArch64 handoff.
The likely owner is semantic BIR construction for an lvalue whose effective
address is not a direct local slot but a pointer value loaded from local state
or produced by pointer arithmetic/casts. A valid repair should admit both
loads and stores through that computed address without reducing the case to a
named source file or a single pointer depth.

## Execution Rules

- Localize the first rejecting semantic branch before changing behavior.
- Add focused coverage before or with the repair so the rule is protected
  independently of the two c-testsuite filenames.
- Preserve adjacent passing pointer-derived load behavior, especially the
  completed `00173` path.
- When a representative advances into AArch64 runtime, printer, ABI, or
  aggregate behavior, record the new first bad fact in `todo.md` instead of
  broadening this plan.

## Ordered Steps

### Step 1: Localize Indirect Local-Memory Rejection

Goal: identify the semantic `lir_to_bir` boundary that rejects the two
representatives.

Concrete actions:

- Reproduce the focused failures for `00005` and `00217` with the
  supervisor-delegated proof command.
- Inspect the semantic lowering path for the failing local-memory load and
  store diagnostics.
- Compare the failing lvalue address sources: loaded pointer value,
  pointer-to-pointer local, pointer arithmetic, and cast-derived address.
- Record the first bad branch, relevant helper names, and current diagnostic
  evidence in `todo.md`.

Completion check:

- `todo.md` names the concrete semantic owner and explains why both
  representatives share it or why the plan must split before implementation.

### Step 2: Add Focused Semantic Coverage

Goal: protect the indirect local-memory load and store shapes outside the
c-testsuite filenames.

Concrete actions:

- Add or update focused semantic/backend-route tests for an indirect store
  lvalue through a loaded pointer or pointer-to-pointer local.
- Add or update focused semantic/backend-route tests for an indirect load
  lvalue through pointer arithmetic and a cast-derived address.
- Assert the semantic BIR or backend-route contract at the memory-operation
  level, not through emitted AArch64 instruction spelling.

Completion check:

- The focused coverage fails before the semantic repair or demonstrates the
  missing contract clearly enough to guard the repair.

### Step 3: Repair Semantic Admission

Goal: lower indirect local-memory lvalues as pointer-based memory operations.

Concrete actions:

- Change the localized semantic `lir_to_bir` path so computed pointer-address
  lvalues are admitted for both load and store forms.
- Keep direct local-slot lowering intact where the address is genuinely a
  direct local object and not a loaded or computed pointer.
- Avoid special handling for `00005`, `00217`, source variable names, one cast
  spelling, one pointer depth, or one generated BIR identifier.

Completion check:

- Focused coverage passes, and the two representatives advance past semantic
  handoff or pass without expectation, runner, timeout, or AArch64 policy
  changes.

### Step 4: Validate And Reclassify Residuals

Goal: prove the focused owner and park any new first bad facts correctly.

Concrete actions:

- Run the supervisor-delegated focused proof command after the repair.
- Confirm adjacent passing pointer-derived load coverage, including the
  completed `00173` path, remains passing.
- If `00005` or `00217` advances to a new AArch64/runtime/ABI/aggregate
  residual, record that new first bad fact in `todo.md` without expanding this
  semantic plan.

Completion check:

- `todo.md` records proof results, any residual first bad facts, and whether
  the source idea is ready for close review or needs a follow-up focused
  split.
