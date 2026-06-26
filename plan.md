# RV64 Object Route Scalar Compare Trunc Runbook

Status: Active
Source Idea: ideas/open/375_rv64_object_route_scalar_compare_trunc_lowering.md

## Purpose

Lower prepared scalar compare-result fragments that feed integer truncation in
the RV64 object route.

## Goal

Admit the first semantic RV64 object-emission path for a prepared scalar
compare result that is truncated into an integer destination, using explicit
prepared value-home, width, predicate, and destination facts.

## Core Rule

Lower compare/trunc semantics from prepared metadata. Do not special-case
`src/20000217-1.c`, hard-code predicates, constants, value ids, frame slots,
or destination registers, and do not reopen frame-slot address, local-memory,
or call-argument routes that already have separate owners.

## Read First

- `ideas/open/375_rv64_object_route_scalar_compare_trunc_lowering.md`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `build/agent_state/372_step2_20000217-1.showbug-semantic-bir.txt` when present
- `build/agent_state/372_step2_20000217-1.showbug-prepared-bir.txt` when present

## Current Targets

- Primary representative:
  - `src/20000217-1.c`
- Focused backend coverage:
  - `backend_riscv_object_emission`
  - `backend_prepare_frame_stack_call_contract`
  - `backend_prepared_printer`

## Non-Goals

- Frame-slot address call-argument materialization; idea 372 closed that route.
- Pointer-value local-memory loads and stores; idea 368 closed that route.
- Frame-slot payload-value call arguments; idea 373 closed that route.
- Non-register parameter homes; use
  `ideas/open/374_rv64_object_route_non_register_param_homes.md`.
- Aggregate `va_arg`, byval aggregate homes, terminator lowering, or CFG
  reconstruction.
- Expectation downgrades, allowlist filtering, unsupported-test weakening, or
  source-shape shortcuts.

## Working Model

The known residual is a prepared scalar comparison whose boolean result is
represented as an integer truncation before publication. Existing object-route
compare support covers narrower `eq`/`ne` shapes, while this route must audit
and admit the first safe wider predicate/trunc form.

The implementation should consume only explicit prepared facts:

- compare predicate and operand homes
- source and destination integer widths
- trunc source identity and destination value home
- selected register or stack publication target
- supported scalar integer classes and ABI-safe materialization sequence

Unsupported neighbors should remain fail-closed with precise diagnostics:
unsupported predicates, unsupported widths, missing or dynamic value homes,
non-integer destinations, ambiguous trunc sources, and compare/trunc shapes not
proved by focused coverage.

## Execution Rules

- Keep implementation changes focused in RV64 object emission and focused
  backend tests.
- Preserve existing `eq`/`ne` compare-result behavior.
- Add fail-closed tests beside the admitted positive fixture.
- Put representative and audit logs under `build/agent_state/`.
- Use `test_after.log` only for the supervisor-delegated proof command.
- Do not edit the source idea unless lifecycle state or durable intent changes.

## Steps

### Step 1: Audit Scalar Compare Trunc Facts

Goal: identify the prepared compare and trunc metadata needed for the
representative and focused fixtures.

Actions:

- Inspect semantic and prepared BIR for `src/20000217-1.c`, especially
  `showbug`.
- Confirm the exact compare predicate, operand homes, trunc source, result
  width, and publication destination for the first supportable form.
- Compare the observed shape against existing `eq`/`ne` object-route support.
- Record required metadata, missing facts, and out-of-scope neighbors in
  `todo.md`.

Completion check:

- `todo.md` names the concrete prepared compare/trunc shape for the
  implementation packet and lists unsupported adjacent shapes that must remain
  fail-closed.

### Step 2: Admit Scalar Compare Trunc Lowering

Goal: implement the first supportable RV64 lowering path for prepared scalar
compare results feeding integer truncation.

Actions:

- Lower the audited compare predicate and trunc destination using prepared
  value-home, width, and destination facts.
- Preserve existing `eq`/`ne` compare behavior.
- Keep unsupported predicates, widths, homes, non-integer destinations, and
  ambiguous trunc sources fail-closed with precise diagnostics.
- Add focused positive and negative tests in
  `tests/backend/mir/backend_riscv_object_emission_test.cpp`.

Completion check:

- Focused tests prove the admitted compare/trunc path and fail-closed
  unsupported neighbors.
- The delegated proof command passes:

```sh
cmake --build build --target c4cll backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test &&
ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure > test_after.log
```

### Step 3: Rerun Representative

Goal: verify semantic progress on the listed source representative.

Actions:

- Rerun `src/20000217-1.c` through the RV64 gcc torture backend runner with a
  temporary allowlist.
- Store runner and case logs under `build/agent_state/`.
- Record whether the case passes, advances, or remains blocked by an existing
  out-of-scope owner.

Completion check:

- `todo.md` records the representative result, next boundary if any, and proof
  artifact paths.

### Step 4: Closure or Handoff Decision

Goal: decide whether idea 375 is complete or needs a narrowed follow-up.

Actions:

- Compare focused test coverage and representative evidence against the source
  idea acceptance criteria.
- If complete, request lifecycle close.
- If a distinct residual owner appears, route it to an existing or new
  `ideas/open/` child before closing or deactivating.

Completion check:

- The plan owner can either close idea 375 with regression proof or identify
  the exact remaining owner without broadening this runbook.
