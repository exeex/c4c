# AArch64 Calls Prepared Authority Repair Runbook

Status: Active
Source Idea: ideas/open/52_aarch64_calls_prepared_authority_repair.md
Supersedes active route: ideas/open/56_aarch64_edge_terminator_consumer_preservation_repair.md is parked open after Step 5 classified the remaining `00204` failure as variadic by-value aggregate call argument publication, not edge/terminator consumer preservation.

## Purpose

Repair duplicate AArch64 call-lowering authority so call arguments, boundary
effects, source selections, preservation facts, and results consume prepared
call plans instead of re-deriving ABI facts locally.

## Goal

Make `src/backend/mir/aarch64/codegen/calls.cpp` consume prepared call
argument publication facts for the remaining `00204` stdarg string corruption,
starting with variadic by-value aggregate payload lanes.

## Core Rule

Call lowering must not rebuild call-argument sources from raw operands,
prepared-name spelling, ABI-index scans, or recursive same-block producer
walks when prepared call/source facts should be authoritative.

## Read First

- `ideas/open/52_aarch64_calls_prepared_authority_repair.md`
- `todo.md`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- prepared call planning/query surfaces that define:
  - `PreparedCallPlan`
  - `PreparedCallArgumentPlan`
  - `PreparedCallArgumentSourceSelection`
  - `PreparedCallBoundaryEffectPlan`
  - `PreparedMoveBundle`
- parked context:
  - `ideas/open/56_aarch64_edge_terminator_consumer_preservation_repair.md`
  - `review/edge-preservation-step4-route-review.md`

## Current Targets

- Direct variadic call by-value aggregate payload publication.
- 7-byte and 9-byte aggregate argument lanes that feed `myprintf` register-save
  consumption in `00204`.
- Prepared call argument/source facts for GPR and overflow stack argument
  destinations.
- Existing focused same-feature probes:
  - `backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy`
  - `backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy`
  - `backend_codegen_route_aarch64_alu_unpublished_load_local_after_call`
  - `backend_codegen_route_aarch64_alu_unpublished_load_local_call_boundary`

## Non-Goals

- Do not change AAPCS64 register/stack staging constants, direct `bl` or
  indirect `blr` spelling, stack cleanup rules, or result-store instruction
  spelling except where needed to consume prepared facts.
- Do not repair this by reloading va_list fields in `memory.cpp`.
- Do not reopen edge source selection or scalar cast source selection from
  idea 56.
- Do not special-case `00204`, `myprintf`, `%7s`, `%9s`, specific temporary
  names, or string literal contents.
- Do not accept or commit existing dirty `memory.cpp` or
  `dispatch_edge_copies.cpp` changes as part of this lifecycle switch.
- Do not weaken expectations or mark supported tests unsupported.

## Working Model

- Idea 56 removed the stale edge-consumer/cast-source symptom, but the focused
  subset still fails only `c_testsuite_aarch64_backend_src_00204_c`.
- Step 5 classified the remaining string corruption upstream of callee
  `va_arg` consumption: generated `myprintf` consumes `%9s` via
  `gr_top + gp_offset` with the expected 16-byte progression.
- The next semantic owner is direct-call argument publication for variadic
  by-value aggregate lanes, especially how aggregate payload bytes are placed
  into GPR lanes and overflow stack slots before the call.
- The source idea's broader target is to remove duplicate call authority in
  `calls.cpp` by consuming prepared call plans and shared query facts.

## Execution Rules

- Start in `calls.cpp` and prepared call facts. Only widen into planning/query
  files if the missing fact cannot be expressed from existing prepared data.
- Prefer prepared call argument plans, source selections, boundary effects, and
  move bundles over raw BIR scans.
- Keep repairs semantic across by-value aggregate call arguments; a green
  `00204` alone is not enough.
- Preserve the focused proof ladder:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`
- Escalate to reviewer scrutiny before accepting a route that proves only the
  named tests or expands outside call argument publication.

## Steps

### Step 1: Classify by-value aggregate call lane publication

Goal: identify the prepared and emitted facts for the failing variadic
by-value aggregate argument lanes.

Primary target: `src/backend/mir/aarch64/codegen/calls.cpp`

Actions:

- Run the focused eight-test subset as the baseline for this route.
- Inspect the direct-call argument plan for the first two `myprintf` string
  calls in `00204`, especially 7-byte and 9-byte aggregate payload placement.
- Trace the emitted GPR and overflow stack argument bytes before the call.
- Compare the failing route against the passing variadic aggregate overflow
  byte-copy probe.
- Record whether the missing authority is an existing prepared call argument
  fact that `calls.cpp` ignores or a genuinely missing shared query.

Completion check:

- `todo.md` records the exact function(s), prepared fields, and emitted lane
  mismatch that own the remaining `00204` corruption, without changing
  implementation files.

### Step 2: Define the prepared call argument publication contract

Goal: choose the smallest prepared-authority contract for by-value aggregate
call argument lane publication.

Primary target: `calls.cpp`, plus prepared query headers only if required.

Actions:

- Decide whether `PreparedCallArgumentPlan`,
  `PreparedCallArgumentSourceSelection`, boundary effects, or move bundles
  already provide the needed source/destination mapping.
- If a helper is needed, define it by argument identity and ABI binding, not by
  testcase names or temporary value spelling.
- Reject local scans that reconstruct by-value aggregate lanes from raw call
  operands when prepared facts exist.

Completion check:

- `todo.md` names the selected contract and the next implementation packet,
  including owned files and proof command.

### Step 3: Repair by-value aggregate argument lane lowering

Goal: make direct variadic by-value aggregate call arguments publish the
correct payload bytes to GPR and overflow stack lanes from prepared authority.

Primary target: `src/backend/mir/aarch64/codegen/calls.cpp`

Actions:

- Consume the selected prepared call argument/source facts in the lowering path
  that materializes by-value aggregate payload lanes.
- Preserve existing ABI staging and stack cleanup behavior.
- Keep scalar, indirect-callee, and after-call result repairs out of this
  packet unless the selected helper must be shared.
- Avoid recursive same-block producer walks and raw prepared-name BIR scans as
  durable authority.

Completion check:

- The focused eight-test subset passes or `todo.md` records the next precise
  call-authority owner with the failing emitted facts.

### Step 4: Audit neighboring call authority fallbacks

Goal: remove or contain the neighboring duplicate call authority exposed by the
by-value aggregate repair.

Primary target: `calls.cpp`

Actions:

- Inspect immediate ABI binding, frame-slot argument moves, scalar
  call-argument producers, prepared-name sources, indirect callee sources, and
  after-call result publication.
- Convert only directly related fallback paths that block the current prepared
  call argument contract.
- Record unrelated duplicate call authority as future work rather than folding
  it into the current packet.

Completion check:

- No active call argument path needed for the focused subset still depends on a
  raw scan where a prepared call fact is available.

### Step 5: Validate call-route acceptance

Goal: decide whether the call/byval publication slice is coherent and whether
parked dirty context can be accepted separately.

Primary target: `todo.md`, `test_before.log`, and `test_after.log`

Actions:

- Run the focused eight-test subset.
- If the subset is green, ask supervisor/reviewer whether broader validation is
  required before accepting the calls slice.
- Keep `memory.cpp` and `dispatch_edge_copies.cpp` dirty context separate
  unless the supervisor routes a proven coherent slice for them.
- Reject expectation downgrades or any proof that depends only on `00204`.

Completion check:

- `todo.md` records a commit-ready calls slice, a precise remaining call owner,
  or a blocker that should return to plan-owner.

### Step 6: Decide source-idea completion or next call-authority packet

Goal: decide whether idea 52 is complete enough to close or should continue
with the next duplicate call-authority fallback.

Primary target: lifecycle files and supervisor-selected validation logs

Actions:

- Compare the accepted implementation against all idea 52 acceptance criteria.
- Close only if call lowering no longer uses the targeted duplicate authority
  paths and regression proof passes.
- Otherwise keep idea 52 active with a precise next packet or park it with a
  durable leftover note.

Completion check:

- The source idea is either closed with regression proof or remains open with a
  bounded next call-authority packet.
