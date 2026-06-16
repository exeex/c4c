# AArch64 00204 stdarg Semantic BIR Local-Memory Admission Runbook

Status: Active
Source Idea: ideas/open/286_aarch64_00204_stdarg_semantic_bir_local_memory_admission.md

## Purpose

Repair the AArch64-target semantic BIR path so the `00204.c` stdarg fixture can
lower through `myprintf` instead of failing in the load local-memory semantic
family.

## Goal

Admit the real AArch64 `va_arg(ap)` aggregate local-memory load shape through
semantic BIR and preserve the existing x86 00204 dump behavior.

## Core Rule

Fix the semantic local-memory / aggregate `va_arg` lowering capability. Do not
special-case `00204.c`, `myprintf`, `movi`, HFA type names, expected dump text,
or CTest labels.

## Read First

- `ideas/open/286_aarch64_00204_stdarg_semantic_bir_local_memory_admission.md`
- `src/backend/bir/lir_to_bir/memory/coordinator.cpp`
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`
- `src/backend/bir/lir_to_bir/memory/addressing.cpp`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/prealloc/variadic.hpp`
- `src/backend/prealloc/variadic_entry_plans.cpp`
- `tests/backend/bir/CMakeLists.txt`
- Existing focused backend tests around semantic local-memory and AArch64
  variadic helper plans.

## Current Targets

- AArch64-target `myprintf` semantic BIR lowering for the `va_arg(ap)`
  aggregate loads in `tests/c/external/c-testsuite/src/00204.c`.
- CTest targets:
  - `backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold`
  - `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`
- Corresponding x86 00204 semantic/prepared dump coverage.

## Non-Goals

- Broad AArch64 ABI rewrites outside the rejected local-memory load shape.
- Prepared-only recovery or printed-dump text recovery as the source of truth.
- Expectation-only changes for #134 or #136.
- Downgrading supported dump tests to unsupported or weaker contracts.
- Removing or weakening x86 00204 dump coverage.

## Working Model

- Step 2 moved the AArch64 target tests past the earlier `myprintf` load
  local-memory failure.
- The current exposed failure is later in the same semantic route:
  `semantic lir_to_bir function 'stdarg' failed in semantic call family
  'direct-call semantic family'`.
- The remaining repair should reuse structured call, type/layout, local-slot,
  pointer, aggregate, and prepared variadic carriers already present in the
  semantic path.

## Execution Rules

- Keep each step narrow enough for a build plus targeted CTest proof.
- Record routine progress, blockers, and proof in `todo.md`, not in this
  runbook or the source idea.
- Add or adjust tests only to prove the semantic lowering capability; do not
  make expectation rewrites the main source of progress.
- A green target test is not sufficient if nearby same-feature aggregate
  `va_arg` or local-memory shapes remain unexamined.
- If the investigation proves the active route is too broad or wrong, stop and
  ask the supervisor for plan-owner review instead of expanding scope in code.

## Ordered Steps

### Step 1: Capture the Failing AArch64 Local-Memory Shape

Goal: identify the exact semantic LIR/BIR admission shape that fails for the
AArch64-target `00204.c` `myprintf` aggregate `va_arg` loads.

Primary target: diagnostics and focused dumps around semantic lowering for
`myprintf`.

Actions:

- Reproduce the failing AArch64 dump tests and capture the first bad function,
  block, instruction kind, local slot, pointer source, and aggregate layout
  facts.
- Compare the failing AArch64 shape with the passing x86 00204 semantic and
  prepared dump routes.
- Inspect the structured carriers already available in local slots, addressing,
  calling, and prepared variadic helper plans before choosing an implementation
  surface.
- Record the selected repair surface and exact proof command in `todo.md`.

Completion check:

- `todo.md` names the failing load/addressing shape, the semantic helper or
  owner file to repair, and the narrow CTest command the executor will use for
  the first implementation packet.

### Step 2: Repair Semantic Admission for the Aggregate `va_arg` Load Shape

Goal: lower the rejected AArch64 aggregate local-memory shape using general
semantic carriers.

Primary target: the smallest semantic BIR local-memory/addressing/calling
surface that owns the rejected shape.

Actions:

- Implement the admission rule in the semantic path, not in prepared
  publication or dump text.
- Reuse existing structured layout, local-slot, pointer, aggregate alias, or
  variadic access-plan facts where available.
- Preserve existing diagnostics for truly unsupported local-memory loads.
- Avoid any branch keyed to fixture names, function names, dump snippets, or
  specific HFA struct spellings.

Completion check:

- The repaired path lowers the previously rejected shape and the narrow
  AArch64 target tests no longer fail in the load local-memory semantic family.
- The diff contains no named-case shortcut for `00204.c`, `myprintf`, `movi`,
  or HFA fixture names.

### Step 3: Repair the Exposed `stdarg` Direct-Call Blocker

Goal: continue the same AArch64 00204 semantic route after the local-memory
repair by admitting the newly exposed `stdarg` direct-call semantic family
shape.

Primary target: the smallest semantic BIR call-lowering surface that owns the
first unsupported direct call in `stdarg`.

Actions:

- Reproduce the current AArch64 target failures and capture the first rejected
  call inside `stdarg`, including callee, argument value shapes, aggregate or
  variadic carriers, and target ABI facts needed by the call family.
- Compare the rejected AArch64 call shape with the corresponding x86 00204
  semantic route before choosing an implementation surface.
- Implement only a general semantic call-lowering admission rule needed for the
  exposed shape; do not broaden into unrelated AArch64 ABI rewrites.
- Preserve the Step 2 local-memory repair and keep the same narrow AArch64
  publication tests as the proof command unless the supervisor delegates a
  stricter subset.

Completion check:

- The narrow AArch64 target tests no longer fail in `stdarg` /
  `direct-call semantic family`.
- The diff contains no named-case shortcut for `00204.c`, `stdarg`,
  `myprintf`, `movi`, or HFA fixture names.

### Step 4: Add Focused Capability Coverage

Goal: prove the semantic capability independently of the named `00204.c`
fixture.

Primary target: focused backend unit or dump assertion near existing semantic
local-memory, aggregate, or variadic tests.

Actions:

- Add a focused test that exercises the admitted aggregate local-memory shape
  without depending on the `00204.c` function name.
- Keep assertions structural: local-memory admission, aggregate payload shape,
  variadic access carrier, or lowered BIR operation facts.
- Preserve the existing x86 00204 semantic/prepared dump tests.

Completion check:

- The focused test fails before the semantic repair and passes after it.
- The test would reject a fixture-name-only or expectation-only fix.

### Step 5: Validate AArch64 and x86 Publication Behavior

Goal: prove the fix satisfies the source idea without regressing existing 00204
behavior.

Primary target: targeted backend CTest coverage plus any supervisor-requested
broader check.

Actions:

- Run:
  `ctest --test-dir build -R '^(backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$' --output-on-failure`
- Run the corresponding x86 00204 semantic/prepared dump tests.
- Run the focused capability test from Step 4.
- If multiple semantic local-memory owners changed, ask the supervisor to
  choose a broader backend or full regression command.

Completion check:

- AArch64 target tests, corresponding x86 00204 tests, and focused capability
  coverage are green.
- No supported test contract was weakened, skipped, or converted to a weaker
  expectation.
