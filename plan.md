# AArch64 Variadic Va Arg Register-Save Progression Repair Runbook

Status: Active
Source Idea: ideas/open/57_aarch64_variadic_va_arg_register_save_progression_repair.md
Supersedes active route: ideas/open/52_aarch64_calls_prepared_authority_repair.md is parked open after Step 4 classified the remaining `00204` failure as callee-side aggregate `va_arg` register-save progression ordering in `variadic.cpp`, not caller-side call prepared authority.

## Purpose

Repair AArch64 aggregate `va_arg` register-save consumption so source
addresses are selected from the current `va_list` offset before the progression
field is advanced.

## Goal

Make `src/backend/mir/aarch64/codegen/variadic.cpp` consume aggregate
register-save slots using the pre-increment `gp_offset` or `fp_offset`, then
store the advanced offset after the current aggregate source has been chosen.

## Core Rule

Register-save aggregate `va_arg` lowering must not form the current source
address from the post-incremented progression value.

## Read First

- `ideas/open/57_aarch64_variadic_va_arg_register_save_progression_repair.md`
- `todo.md`
- `src/backend/mir/aarch64/codegen/variadic.cpp`
- prepared stdarg/va_arg facts that define aggregate source class,
  progression field, register-save area, and progression stride
- parked context:
  - `ideas/open/52_aarch64_calls_prepared_authority_repair.md`
  - `review/calls-step3-byval-lane-review.md`

## Current Targets

- Aggregate `va_arg` paths with `source_class=register_save_area`.
- `gp_offset` register-save consumption for direct variadic by-value aggregate
  arguments.
- Matching `fp_offset` register-save ordering if the same helper or lowering
  path owns floating-point aggregate lanes.
- Existing focused same-feature probes:
  - `backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy`
  - `backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy`
  - `backend_codegen_route_aarch64_alu_unpublished_load_local_after_call`
  - `backend_codegen_route_aarch64_alu_unpublished_load_local_call_boundary`

## Non-Goals

- Do not change caller-side call argument publication in `calls.cpp` or
  `call_plans.cpp`.
- Do not reopen edge source selection, scalar cast source selection, memory
  field publication, or dispatch edge-copy routing.
- Do not change AAPCS64 register-save constants, register numbering, overflow
  branch selection, stack cleanup, or direct-call spelling.
- Do not special-case `00204`, `myprintf`, `%7s`, `%9s`, temporary names,
  stack offsets, or string literal contents.
- Do not accept or commit existing dirty `memory.cpp` or
  `dispatch_edge_copies.cpp` changes as part of this lifecycle switch.
- Do not weaken expectations or mark supported tests unsupported.

## Working Model

- Idea 52 added the missing prepared call lane fact for direct extern variadic
  single-GPR by-value aggregate payloads.
- The focused proof still fails only
  `c_testsuite_aarch64_backend_src_00204_c`.
- Step 4 classification found that generated `myprintf` stores incoming
  variadic GPRs into the register-save area, but aggregate `va_arg`
  consumption advances `gp_offset` before computing the source address.
- For the 7-byte aggregate, the path changes `-56 -> -48`, then reads from
  `gr_top - 48`, which is the next variadic GPR slot. The correct source for
  the current aggregate is based on old `gp_offset=-56`.

## Execution Rules

- Start in `variadic.cpp`; only widen to shared stdarg planning/query code if
  the emitted lowering cannot retain the old progression value from existing
  facts.
- Keep repairs semantic across aggregate register-save `va_arg` paths; a green
  `00204` alone is not enough.
- Preserve overflow-area behavior and HFA lane homes unless direct inspection
  proves they share the same pre-increment address rule.
- Preserve the focused proof ladder:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`
- Escalate to reviewer scrutiny before accepting a route that proves only the
  named C test or alters unrelated variadic lowering.

## Steps

### Step 1: Confirm aggregate register-save progression ordering

Goal: identify the exact `variadic.cpp` lowering path that advances
`gp_offset` or `fp_offset` before selecting the aggregate register-save source
address.

Primary target: `src/backend/mir/aarch64/codegen/variadic.cpp`

Actions:

- Run or inspect the focused eight-test baseline already recorded in
  `test_after.log`.
- Trace aggregate `va_arg` plans for `source_class=register_save_area`,
  `progression_field=gp_offset` or `fp_offset`, and their
  `progression_stride`.
- Locate the emitted sequence that writes the advanced progression field and
  forms the source address for the current aggregate.
- Record whether `gp_offset` and `fp_offset` share the same ordering helper or
  need separate guarded changes.

Completion check:

- `todo.md` records the exact function/helper and ordering mismatch in
  `variadic.cpp`, plus the implementation packet boundary.

### Step 2: Repair register-save source address ordering

Goal: make aggregate register-save `va_arg` lowering form the source address
from the old progression value, then store the advanced offset.

Primary target: `src/backend/mir/aarch64/codegen/variadic.cpp`

Actions:

- Preserve the pre-increment `gp_offset` or `fp_offset` value long enough to
  compute the current register-save source address.
- Store the advanced offset using the prepared stride after the current source
  address has been selected.
- Apply the same semantic ordering to both GPR and FPR aggregate
  register-save paths when they share the bug.
- Leave overflow-area source selection, stack field publication, and unrelated
  scalar `va_arg` lowering unchanged.

Completion check:

- The focused eight-test subset passes or `todo.md` records the next precise
  variadic register-save owner with emitted facts.

### Step 3: Validate variadic route acceptance

Goal: decide whether the variadic register-save repair is coherent and whether
broader validation is needed.

Primary target: `todo.md`, `test_before.log`, and `test_after.log`

Actions:

- Run the focused eight-test subset.
- If the subset is green, record whether broader backend or full CTest
  validation is required before acceptance.
- Keep `memory.cpp`, `dispatch_edge_copies.cpp`, and parked calls context
  separate unless the supervisor routes a proven coherent slice for them.
- Reject expectation downgrades or proof that depends only on `00204`.

Completion check:

- `todo.md` records a commit-ready variadic slice, a precise remaining
  variadic owner, or a blocker that should return to plan-owner.

### Step 4: Decide source-idea completion or follow-up

Goal: decide whether this variadic source idea is complete enough to close or
should continue with another bounded register-save packet.

Primary target: lifecycle files and supervisor-selected validation logs

Actions:

- Compare the accepted implementation against the idea acceptance criteria.
- Close only if aggregate register-save consumption uses pre-increment source
  addressing and regression proof passes.
- Otherwise keep this idea active with a precise next variadic packet or park
  it with a durable leftover note.

Completion check:

- The source idea is either closed with regression proof or remains open with a
  bounded next variadic packet.
