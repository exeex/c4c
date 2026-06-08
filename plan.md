# AArch64 Wide Value Owner Post-Contract Audit Runbook

Status: Active
Source Idea: ideas/open/128_aarch64_wide_value_owner_post_contract_audit.md

## Purpose

Audit `src/backend/mir/aarch64/codegen/i128_ops.cpp` and
`src/backend/mir/aarch64/codegen/f128.cpp` after recent BIR/prealloc,
call-boundary, carrier, aggregate transport, and memory-contract cleanup.
Classify whether remaining wide-value responsibilities belong to shared
BIR/prealloc contracts, AArch64 codegen consumption, or local organization.

## Goal

Produce a closure-ready audit with cluster maps, ownership tables, and precise
follow-up idea payloads only where current evidence shows shared policy is
still being rediscovered or duplicated in AArch64 codegen.

## Core Rule

This plan is analysis-only. Do not edit implementation files, tests, build
metadata, or closed idea archives while executing it.

## Read First

- `ideas/open/128_aarch64_wide_value_owner_post_contract_audit.md`
- `src/backend/mir/aarch64/codegen/i128_ops.cpp`
- `src/backend/mir/aarch64/codegen/f128.cpp`
- Overlap surfaces when needed for ownership evidence:
  - `src/backend/mir/aarch64/codegen/calls.cpp`
  - `src/backend/mir/aarch64/codegen/memory.cpp`
  - `src/backend/mir/aarch64/codegen/instruction.cpp`
  - `src/backend/mir/aarch64/machine_printer.cpp`

## Current Scope

- i128 runtime helper ownership and div/rem ABI policy.
- i128 preserved value and selected-call ownership checks.
- i128 pair transport, shift, and compare record construction.
- f128 full-width and memory-backed carrier facts.
- f128 runtime helper resource and preservation checks.
- f128 transport construction and printable-address handling.
- Overlap with calls, memory, instruction emission, and machine printing.

## Non-Goals

- Do not rewrite or split `i128_ops.cpp` or `f128.cpp` because they are large.
- Do not move AArch64 register spelling, Q-register spelling, lane/shift
  opcode spelling, or helper call assembly into shared BIR/prealloc code.
- Do not reopen closed carrier, transport, calls, memory, or ALU contracts
  without new evidence from the two wide-value files.
- Do not create vague follow-up ideas such as "clean f128" or "shrink i128".
- Do not weaken tests or expectations to make the audit easier.
- Do not start implementation work for any follow-up during this audit.

## Working Model

- Shared BIR/prealloc authority owns target-neutral carrier choice, helper
  ABI/resource policy, preservation/publication facts, memory-backed transport
  authority, and call-boundary wide-value facts.
- AArch64 codegen should validate that prepared facts are complete and then
  translate them into machine records, target register spelling, lane/shift
  opcode spelling, and runtime helper call assembly.
- Local organization helpers are acceptable when they are file-local,
  target-specific, and do not decide target-neutral lowering policy.
- Line count is not evidence of architectural drift. Evidence must identify a
  concrete ownership boundary and proof route.

## Execution Rules

- Prefer direct evidence from current code over inference from past work.
- Classify each major cluster as one of:
  - `shared-bir-prealloc-contract`
  - `aarch64-codegen-consumption`
  - `local-organization-only`
- Record routine audit notes and packet progress in `todo.md`.
- If a concrete follow-up is warranted, record its proposed filename, owner
  boundary, proof route, and reject signals in `todo.md` for supervisor or
  plan-owner lifecycle handling.
- Preserve source idea stability until closure or a delegated lifecycle action
  requires durable follow-up idea creation.

## Steps

### Step 1: Map Wide-Value Helper Clusters

Goal: Build a current cluster map for `i128_ops.cpp` and `f128.cpp` before
judging ownership boundaries.

Primary targets:

- `src/backend/mir/aarch64/codegen/i128_ops.cpp`
- `src/backend/mir/aarch64/codegen/f128.cpp`

Actions:

- Inspect top-level helpers, file-local helper groups, and public entry points.
- Group `i128_ops.cpp` clusters around helper calls, div/rem ABI, preserved
  value checks, selected-call checks, pair transport, shifts, compares, and
  machine-record construction.
- Group `f128.cpp` clusters around carrier facts, memory-backed transport,
  helper resources, preservation checks, transport construction, printable
  addresses, vector/Q-register spelling, and diagnostics.
- Note where each cluster consumes prepared facts versus choosing ownership.
- Avoid implementation edits.

Completion check:

- `todo.md` records a short cluster map for both files and flags any uncertain
  clusters that need overlap-surface inspection.

### Step 2: Audit i128 Helper And Call-Boundary Ownership

Goal: Decide whether i128 runtime helper ownership, div/rem ABI policy,
preserved value checks, and selected-call ownership checks consume prepared
facts or rediscover shared policy.

Primary targets:

- `src/backend/mir/aarch64/codegen/i128_ops.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`

Actions:

- Trace i128 runtime helper call assembly and div/rem ABI policy.
- Inspect preserved value and selected-call ownership checks.
- Separate AArch64 helper call assembly from target-neutral helper
  ABI/resource or preservation decisions.
- Record concrete evidence for every ownership classification.

Completion check:

- `todo.md` contains the i128 helper/call-boundary classification and any
  precise shared-policy gap candidate with proof route.

### Step 3: Audit i128 Pair Transport, Shift, And Compare Records

Goal: Decide whether i128 pair transport, shift lowering, and compare record
construction are AArch64-local emission or hidden shared transport policy.

Primary targets:

- `src/backend/mir/aarch64/codegen/i128_ops.cpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/machine_printer.cpp`

Actions:

- Inspect pair transport record construction and register-pair spelling.
- Inspect shift lowering and lane/shift opcode choices.
- Inspect compare record construction and printer-facing record shape.
- Keep AArch64 register, opcode, and machine-record spelling local unless the
  code proves target-neutral ownership is being chosen late.

Completion check:

- `todo.md` classifies i128 pair transport, shift, and compare clusters as
  shared contract, AArch64 consumption, or local organization only.

### Step 4: Audit f128 Carrier And Memory-Backed Facts

Goal: Decide whether f128 full-width carriers and memory-backed transport
paths consume prepared carrier facts or still own target-neutral transport
authority.

Primary targets:

- `src/backend/mir/aarch64/codegen/f128.cpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`

Actions:

- Trace full-width carrier selection and validation.
- Trace memory-backed carrier handling and publication authority.
- Compare call-boundary and memory overlap only where `f128.cpp` evidence
  shows possible duplicated ownership.
- Do not reopen closed contracts without new evidence from `f128.cpp`.

Completion check:

- `todo.md` records the f128 carrier and memory-backed classification,
  including explicit `no gap` evidence where prepared facts are consumed.

### Step 5: Audit f128 Helper Resources, Transport, And Printing

Goal: Decide whether f128 runtime helper resources, preservation checks,
transport construction, printable-address handling, and Q-register spelling are
shared policy or target-local assembly.

Primary targets:

- `src/backend/mir/aarch64/codegen/f128.cpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/machine_printer.cpp`

Actions:

- Inspect runtime helper resource and preservation diagnostics.
- Inspect transport construction and printable-address handling.
- Inspect vector/Q-register spelling and printer-facing record fields.
- Separate target-neutral helper policy from AArch64 helper invocation and
  printable machine-record details.

Completion check:

- `todo.md` records the f128 helper/transport/printing classification and any
  concrete follow-up candidate.

### Step 6: Synthesize Closure Payload

Goal: Convert audit evidence into the expected closure note payload without
starting implementation work.

Primary targets:

- `todo.md`
- `ideas/open/` only when supervisor delegates lifecycle creation of concrete
  follow-up ideas

Actions:

- Build the final cluster map for `i128_ops.cpp`.
- Build the final cluster map for `f128.cpp`.
- Build a table of remaining shared-policy rediscovery, if any.
- Build a table of target-local code that should stay in AArch64.
- For each follow-up candidate, record proposed filename, exact files, proof
  route, and reject signals.
- If no shared-policy gap remains, state why remaining size is target-local
  emission or local organization.

Completion check:

- `todo.md` has enough evidence for plan-owner closure review: both cluster
  maps, both ownership tables, explicit no-gap reasoning where applicable, and
  any precise follow-up idea payloads.
