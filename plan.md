# RV64 `va_start` Destination `va_list` Address Runbook

Status: Active
Source Idea: ideas/open/389_rv64_va_start_destination_va_list_address_publication.md
Activated from: ideas/open/389_rv64_va_start_destination_va_list_address_publication.md

## Purpose

Resolve the RV64 object-route runtime boundary exposed after `llvm.va_end.p0`
lowering was closed: `fragment_for_prepared_variadic_va_start` stores through
the prepared destination `va_list` address register before that address is
materialized or published.

## Goal

Make the prepared destination `va_list` address available before any RV64
object-emitted `va_start` store through it, or fail closed with a precise owner.

## Core Rule

Use prepared/BIR/object-emission facts to lower the semantic `va_start`
destination address route. Do not hard-code `va-arg-13.c`, function `test`,
register `s1`, object offsets, or the current failing instruction shape as the
route condition.

## Read First

- `ideas/open/389_rv64_va_start_destination_va_list_address_publication.md`
- `ideas/closed/388_rv64_object_route_variadic_va_end_boundary.md`
- `build/agent_state/388_step5_va-arg-13.case.log`
- `build/agent_state/388_step5_va-arg-13.objdump.log`
- `build/agent_state/388_step5_va-arg-13.readelf.log`
- Prepared/BIR dumps for `tests/c/external/gcc_torture/src/va-arg-13.c`
- `src/backend/mir/riscv/codegen/object_emission.cpp`

## Current Targets

- Representative: `tests/c/external/gcc_torture/src/va-arg-13.c`
- RV64 object route for `fragment_for_prepared_variadic_va_start`
- Prepared variadic helper facts for `dst_va_list_addr`
- Destination `va_list` address materialization/publication before stores
- Focused RV64 backend object-emission tests and CMake wiring as needed

## Non-Goals

- Do not reopen `llvm.va_end.p0` no-op lowering from idea 388.
- Do not reopen frame-slot-address call argument lowering from idea 386.
- Do not implement same-module memory-return/sret calls owned by idea 387.
- Do not rewrite broad aggregate `va_arg`, `va_copy`, or unrelated variadic
  helpers under this plan.
- Do not downgrade expectations, add fake initialization, or add link/runtime
  workarounds instead of repairing the prepared/object route.

## Working Model

The known representative reaches runtime and segfaults after the `va_end`
symbol boundary is gone. The first `va_start` sequence in `test` stores the
overflow-area pointer with `sd t1,0(s1)` before any preceding instruction
materializes the destination `va_list` address into `s1`. Prepared evidence
identifies the helper operand as
`dst_va_list_addr=%lv.state.8:register:reg=s1`.

The route must determine whether the object fragment itself should materialize
the destination address, or whether an earlier prepared move/address
publication fact should already have made that register valid.

## Execution Rules

- Capture prepared/BIR/object facts before selecting the lowering route.
- Add focused backend coverage before or alongside the implementation so the
  failure mode is pinned to destination-address availability before store.
- Preserve fail-closed behavior for malformed or unsupported `va_start` helper
  shapes.
- Keep later boundaries separate: record them in `todo.md` and route to an
  existing or new idea instead of silently expanding this plan.
- Use the supervisor-selected proof command for executor packets, then rerun
  `va-arg-13.c` at the representative checkpoint.

## Steps

### Step 1: Capture The Prepared `va_start` Destination Address Boundary

Goal: identify the exact prepared/BIR facts behind the current runtime
segfault.

Primary target: `va-arg-13.c` prepared/BIR/object evidence around
`fragment_for_prepared_variadic_va_start`.

Actions:

- Inspect the idea 388 Step 5 logs and current prepared/BIR dumps for the first
  `va_start` in `test`.
- Map the emitted stores through `dst_va_list_addr` to their prepared helper
  operands and destination register assignment.
- Identify the ordinary value home, selected materialization facts, and any
  prepared publication/move facts for the destination `va_list` address.
- Record the exact missing or ambiguous fact in `todo.md`.

Completion check: `todo.md` names the relevant instruction positions,
`dst_va_list_addr` operand, materialization/publication facts, and the likely
owner for making the destination address valid before store.

### Step 2: Classify The Publication Owner

Goal: decide whether the address is owned by the `va_start` fragment or by an
earlier prepared address-publication route.

Primary target: `src/backend/mir/riscv/codegen/object_emission.cpp`.

Actions:

- Inspect `fragment_for_prepared_variadic_va_start` and adjacent prepared move
  or address-materialization handling.
- Define the narrow supported facts required to materialize or rely on
  `dst_va_list_addr`.
- Define malformed or unsupported variants that must remain diagnostic/fail
  closed.
- Keep idea 386, idea 387, and idea 388 routes out of this owner decision.

Completion check: `todo.md` records the chosen lowering/publication route,
its guards, and the exact variants that should reject rather than emit unsafe
stores.

### Step 3: Add Focused Backend Coverage

Goal: prove the destination `va_list` address is available before any store
through it.

Primary target: focused RV64 backend object-emission tests/fixtures and CMake
wiring as needed.

Actions:

- Add or extend focused RV64 object-emission coverage for a supported
  `FrameSlot`/destination-address-backed `va_start` helper shape.
- Assert that address materialization/publication precedes stores through
  `dst_va_list_addr`.
- Add adjacent fail-closed coverage for malformed or unsupported helper facts.
- Avoid assertions tied to source filename, `test`, `s1`, or literal offsets
  except where the fixture itself intentionally owns the concrete expected
  encoding.

Completion check: focused coverage fails on the current unsafe route and passes
after the implementation, while unsupported adjacent variants remain rejected.

### Step 4: Implement Narrow Destination Address Publication

Goal: implement the selected RV64 object-route fix without broad variadic
helper rewrites.

Primary target: `src/backend/mir/riscv/codegen/object_emission.cpp`.

Actions:

- Use prepared selection/materialization/publication facts to make
  `dst_va_list_addr` valid before stores emitted by
  `fragment_for_prepared_variadic_va_start`.
- Emit a narrow diagnostic or unsupported route when the required facts are
  absent or ambiguous.
- Keep `llvm.va_end.p0`, frame-slot-address call argument, and same-module
  sret behavior unchanged.
- Run the delegated backend proof command and record results in `todo.md`.

Completion check: focused backend tests pass, fail-closed variants stay closed,
and the proof log records the exact command and result.

### Step 5: Rerun `va-arg-13.c` And Route The Next Boundary

Goal: verify the representative advances past the current runtime segfault or
record a narrower diagnostic owner.

Primary target: `tests/c/external/gcc_torture/src/va-arg-13.c`.

Actions:

- Rerun the representative GCC torture case with the same comparison shape
  used at the idea 388 close boundary.
- Confirm whether the c4c RV64 object route advances past the current
  segmentation fault.
- If a later boundary appears, record exact evidence and route it to an
  existing or new idea rather than expanding this plan.
- Preserve the backend proof result required by the supervisor.

Completion check: `todo.md` records the representative result, proof logs, and
either completion evidence for idea 389 or a clearly owned later boundary.
