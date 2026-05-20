# AArch64 Variadic HFA And Aggregate Output Residual Runbook

Status: Active
Source Idea: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Reactivated after: ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md

## Purpose

Continue the AArch64 `00204.c` representative after the byval aggregate
call-argument publication route repaired the `%9s` payload lane fault. The new
first bad fact is a later HFA/aggregate output mismatch, not remaining byval
lane publication.

## Goal

Classify and repair the generated-code owner that corrupts the HFA/aggregate
output section now that prior byval, stdarg cursor, fixed-formal, and local
publication faults have advanced.

## Core Rule

Repair a general AArch64 HFA/floating or aggregate output capability. Do not
special-case `00204.c`, one HFA shape, one aggregate size, one float literal,
one output line, one register, one stack offset, or one emitted instruction
sequence.

## Read First

- `ideas/open/326_aarch64_variadic_hfa_floating_residual.md`
- `todo.md`
- generated AArch64 artifacts for `00204.c` under
  `build/c_testsuite_aarch64_backend/src/`
- existing focused `test_after.log`, when present

## Current Targets

- Representative external case:
  - `c_testsuite_aarch64_backend_src_00204_c`
- Current first bad fact:
  - expected `33.1,33.3 34.1,34.4 34.1,34.4 34.1,34.4`
  - actual `33.1,33.3 34.1,34.4 34.2,34.1 34.2,0.0`
- Current owner classification:
  - the prior partial upper-lane byval aggregate publication fault is fixed
  - the first bad fact has advanced into HFA/aggregate output
  - byval `stdarg:` payload publication is a guardrail, not the active owner

## Non-Goals

- Do not reopen byval aggregate call-argument lane publication unless fresh
  generated-code evidence again shows prepared byval bytes failing to reach
  their AAPCS64 call lanes.
- Do not reopen stdarg cursor progression, `va_start` overflow cursor setup,
  non-HFA aggregate `va_arg` byte copies, fixed-formal entry publication,
  local/value-home publication, frame/formal publication, or prior byval
  rounded placement without direct generated-code evidence.
- Do not change semantic admission, runners, timeout policy, expectations,
  unsupported classifications, proof-log policy, or CTest registration.

## Working Model

The representative now gets past the byval string payloads and reaches a later
floating/aggregate output block. The mismatch swaps or drops floating members
around the `33.*` / `34.*` output, so the next packet should localize whether
the fault is HFA argument transport, aggregate/floating return publication,
`va_arg` source selection or progression, HFA lane materialization, or
ordinary call setup for the observing output call.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Start from generated-code evidence for the new first bad fact before editing
  implementation.
- Preserve the repaired byval aggregate lane publication and payload helper
  coverage from idea 328.
- Prefer focused backend coverage for the localized HFA/floating or aggregate
  owner before relying on the external c-testsuite representative.
- If `00204.c` advances past this residual, record the next first bad fact in
  `todo.md` and return it to lifecycle classification if it belongs to another
  owner.

## Ordered Steps

### Step 1: Localize The Advanced HFA/Aggregate Output Mismatch

Goal: identify the first generated-code owner for the `33.*` / `34.*`
floating or aggregate output corruption.

Primary targets:

- generated AArch64 assembly and dumps for `00204.c`
- AArch64 HFA/floating argument, return, `va_arg`, and output call lowering
  paths implicated by the generated evidence

Actions:

- Compare expected and actual runtime output around the advanced first bad
  fact and map the values back to source-level HFA or aggregate calls.
- Inspect prepared BIR and generated assembly for the producing call, any
  selected aggregate or HFA temporary, and the observing output call.
- Determine whether source values are wrong before call lowering, lost during
  HFA/floating lane materialization, read from stale homes, or clobbered during
  output call setup.
- Record the concrete owner, source value, prepared home, ABI lane or stack
  slot, and emitted instruction sequence in `todo.md`.

Completion check:

- `todo.md` names the first owner with generated-code evidence and separates
  it from byval lane publication, stdarg cursor handling, and prior local or
  fixed-formal publication owners.

### Step 2: Repair The Classified Owner

Goal: repair the localized AArch64 HFA/floating or aggregate output path
generally.

Primary target: the backend lowering files identified by Step 1.

Actions:

- Apply the narrow semantic repair for the classified owner.
- Preserve existing guardrails for byval aggregate lane publication,
  non-HFA aggregate `va_arg`, `va_start`, fixed-formal entry publication,
  local/value-home publication, and frame/formal publication.
- Avoid testcase-shaped matching on the representative source, output line,
  value literals, registers, or stack offsets.

Completion check:

- Generated code for the localized owner publishes or consumes the expected
  HFA/floating or aggregate values through the correct AAPCS64 lanes or homes.

### Step 3: Add Focused Coverage

Goal: make the repaired owner observable in local backend tests.

Primary target: focused AArch64 backend runtime or dump coverage matching the
classified owner.

Actions:

- Add or extend focused coverage that fails without the repair and passes with
  it.
- Include guardrails for the adjacent repaired byval payload lane publication
  and relevant HFA/floating or aggregate paths.
- Keep the test semantic; do not assert only one representative output string
  when a lower-level owner can be observed directly.

Completion check:

- Focused coverage proves the repaired owner and adjacent guardrails.

### Step 4: Validate Representative And Classify Residuals

Goal: prove the repair on focused guardrails and the `00204.c`
representative, then hand off any new first bad fact.

Primary target: supervisor-selected focused proof scope including
`c_testsuite_aarch64_backend_src_00204_c`.

Actions:

- Run the delegated focused proof command and record results in `todo.md`.
- Confirm the `33.*` / `34.*` mismatch is gone.
- Confirm byval aggregate lane publication, stdarg cursor, non-HFA aggregate
  `va_arg`, fixed-formal, local/value-home, and frame/formal guardrails remain
  stable.
- If `00204.c` still fails, classify whether the next first bad fact remains
  in this source idea or belongs to another distinct initiative.

Completion check:

- `todo.md` records fresh proof. The localized HFA/floating or aggregate
  output fault is gone, prior-owner guardrails remain stable, and any
  remaining blocker is explicitly localized for the next lifecycle decision.
