# AArch64 Codegen Forward-Migration Second-Wave Audit Runbook

Status: Active
Source Idea: ideas/open/aarch64-codegen-forward-migration-second-wave-audit.md

## Purpose

Audit the remaining `src/backend/mir/aarch64/codegen` responsibilities after
the first forward-migration wave and turn the best second-wave opportunities
into focused follow-up ideas.

## Goal

Identify target-local codegen responsibilities that can move earlier into
`bir`, `prealloc`, or shared MIR support without implementing broad movement in
this run.

## Core Rule

This is an audit and planning run. Do not implement migration slices, weaken
tests, or propose testcase-shaped moves as capability progress.

## Read First

- `ideas/open/aarch64-codegen-forward-migration-second-wave-audit.md`
- `ref/claudes-c-compiler/src/backend/arm/codegen/README.md`
- First-wave helper surfaces:
  - `src/backend/prealloc/prepared_lookups.*`
  - `src/backend/prealloc/decoded_home_storage.*`
  - `src/backend/prealloc/formal_publications.*`
  - `src/backend/prealloc/call_plans.cpp`
  - `src/backend/prealloc/calls.hpp`
  - `src/backend/prealloc/value_locations.hpp`

## Current Targets

Inspect the remaining AArch64 codegen surfaces, especially:

- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_store_sources.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_dynamic_stack.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_branch_fusion.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- remaining generic portions of `dispatch_calls.cpp`
- remaining generic portions of `calls_moves.cpp`
- helper declarations still collected in `dispatch.hpp`

## Non-Goals

- Do not implement broad code movement during this audit.
- Do not regenerate first-wave ideas unless a concrete remaining second slice is
  visible in the same area.
- Do not move AArch64 instruction spelling, ABI lane rules, final assembly
  sequences, or target-specific instruction selection into generic layers.
- Do not create shared abstractions without naming how x86 or RISC-V would use
  them.
- Do not edit implementation files except for read-only inspection commands.

## Working Model

Classify each candidate responsibility into one of these buckets:

- `Keep target-local`: AArch64 register spelling, ABI register/lane policy,
  instruction sequences, or final machine printing.
- `Move to BIR`: semantic producer or store/load normalization that should be
  represented before preallocation.
- `Move to prealloc`: target-independent or target-parameterized facts computed
  from BIR plus target profile before MIR lowering.
- `Move to shared MIR`: generic Prepared-to-machine orchestration, block
  traversal, publication scheduling, or machine-record construction.
- `Needs target hook`: reusable planning with small target hooks for register
  classes, scalar widths, or instruction-specific emission.
- `Defer`: plausible but too risky until x86/RISC-V has adjacent consumers.

## Execution Rules

- Prefer concise audit artifacts and focused follow-up ideas over a large
  speculative design note.
- Every proposed migration must name the reuse value for x86 or RISC-V.
- Every generated follow-up idea must be small enough for one focused agent run.
- Every generated follow-up idea must include concrete reviewer reject signals.
- Keep tempting target-local work explicitly listed as target-local or deferred.
- Use build or compile checks only if audit edits touch executable repo files;
  lifecycle-only or idea-only edits do not require code validation.

## Ordered Steps

### Step 1: Map Remaining Codegen Surfaces

Goal: Create a compact inventory of the remaining AArch64 codegen files against
the ARM reference README responsibility model.

Concrete actions:

- Inspect the reference README inventory.
- Inspect the target files listed under `Current Targets`.
- Mark which responsibilities are direct AArch64 instruction selection or
  printing.
- Mark which responsibilities are still broad Prepared/MIR pipeline mechanics.
- Record the map in `todo.md` or in an audit note before generating follow-up
  ideas.

Completion check:

- There is a short file-to-responsibility map naming what still sits outside the
  reference target-codegen model.

### Step 2: Classify Second-Wave Candidates

Goal: Convert the inventory into a candidate table with destination layer,
reuse value, risk, and suggested proof.

Concrete actions:

- Evaluate same-block producer discovery and select-chain dependency analysis.
- Evaluate scalar value publication planning for stack homes, register homes,
  globals, immediates, and pointer-base-plus-offset homes.
- Evaluate store-source publication planning, including narrow-store to
  wide-load recovery and pointer-store writeback.
- Evaluate dynamic-stack helper recognition and operand-home planning.
- Evaluate branch-fusion eligibility and support-instruction filtering.
- Evaluate remaining generic call move ordering or republication logic.
- Evaluate generic helper APIs still exposed through `dispatch.hpp`.

Completion check:

- There is a candidate table with bucket, destination layer, x86/RISC-V reuse
  value, risk, and proof expectation for each viable candidate.

### Step 3: Generate Focused Follow-Up Ideas

Goal: Create multiple new `ideas/open/*.md` implementation ideas for the best
second-wave candidates.

Concrete actions:

- Select only candidates with clear ownership, reuse value, and a focused proof
  path.
- Write one `ideas/open/*.md` file per migration slice.
- Include intent, scope, non-goals, acceptance, and `## Reviewer Reject Signals`
  in each new idea.
- Keep the current source idea stable unless durable intent actually changes.

Completion check:

- Multiple focused follow-up ideas exist under `ideas/open/`, each traceable to
  this audit and each small enough for one execution run.

### Step 4: Record Keep-Local And Deferred Decisions

Goal: Prevent future routes from treating target-local or too-risky candidates
as implied migration work.

Concrete actions:

- List tempting candidates that should stay target-local for now.
- List candidates deferred until x86/RISC-V has adjacent consumers or proof.
- Explain the target-specific or risk reason for each decision.

Completion check:

- The audit records a clear keep-local/defer note for non-selected candidates.

### Step 5: Lifecycle Handoff

Goal: Leave canonical lifecycle state ready for the next supervisor decision.

Concrete actions:

- If the audit source idea is satisfied, report that the runbook can be closed
  or intentionally handed off to one generated follow-up idea.
- If a follow-up should become active immediately, ask the supervisor to route a
  plan-owner lifecycle switch.
- Keep `todo.md` aligned with this runbook until lifecycle state changes.

Completion check:

- The supervisor has enough information to close this audit or activate one
  generated follow-up idea without re-reading the entire audit history.
