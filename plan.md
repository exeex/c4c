# Target-Neutral Publication Plan Record Runbook

Status: Active
Source Idea: ideas/open/target-neutral-publication-plan-record.md

## Purpose

Move scalar value publication planning toward target-neutral ownership while
keeping target-specific emission policy in AArch64 codegen.

## Goal

Introduce an inspectable publication-plan record that describes which logical
value should be published to which Prepared home, then have AArch64 consume
that plan through narrow target-local lowering.

## Core Rule

Move planning facts, not AArch64 emission policy. Register spelling, memory
operand legality, ADRP/ADD construction, FP immediate materialization, and
final machine instruction construction must remain target-local.

## Read First

- `ideas/open/target-neutral-publication-plan-record.md`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- `src/backend/prealloc/`
- `src/backend/mir/query.hpp`

## Current Scope

- Publication eligibility, clobber checks, entry-publication checks, and
  Prepared-home selection for scalar values.
- A neutral record describing publication intent before target emission.
- AArch64 adapters/hooks that lower the neutral record through existing
  emission paths.
- One x86 or RISC-V proof path showing the record is reusable without AArch64
  codegen headers.

## Non-Goals

- Do not move AArch64 instruction spelling, ABI lane policy, stack-pointer
  sequences, memory operand spelling, or final machine instruction creation.
- Do not move ADRP/ADD sequences, load/store mnemonics, FP immediate handling,
  register-width spelling, or target clobber-register policy into shared code.
- Do not include store-source recovery, call-boundary republication, branch
  fusion, dynamic-stack operations, or final machine-record construction.
- Do not weaken tests or convert supported paths to unsupported.

## Working Model

The neutral publication plan should answer: what source value is being
published, which Prepared home is the destination, whether publication is
eligible, whether clobbers or entry-publication state affect the plan, and what
target hook must lower it. The target still decides how to spell registers,
materialize immediates, address memory, and emit instructions.

## Execution Rules

- Keep behavior-preserving slices small enough to prove with build plus focused
  backend coverage.
- Prefer prealloc or shared MIR records for target-independent facts; keep
  AArch64-specific operands inside AArch64 lowering.
- Preserve existing AArch64 behavior for stack homes, register homes, globals,
  immediates, and pointer-base-plus-offset homes touched by the migration.
- Add or update x86/RISC-V proof only after the neutral record is real enough
  to consume without AArch64 codegen dependencies.

## Step 1: Map Publication Planning Surfaces

Goal: identify the exact AArch64 and shared/prealloc surfaces that currently
own target-independent publication planning.

Primary targets:
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- `src/backend/prealloc/`

Actions:
- Inspect publication-to-home decisions, entry-publication checks, clobber
  checks, immediate publication, and Prepared-home selection.
- Classify each discovered fact as target-neutral planning, AArch64 emission
  policy, or mixed policy requiring a target hook.
- Record the smallest viable first extraction boundary in `todo.md`.

Completion check:
- `todo.md` lists candidate planning facts, keep-local facts, and the first
  implementation slice without changing implementation files.

## Step 2: Define Neutral Publication Plan Data

Goal: add the target-neutral record and helper surface for publication intent.

Primary targets:
- `src/backend/prealloc/`
- `src/backend/mir/query.hpp`
- `src/backend/mir/query.cpp`

Actions:
- Define a record that describes logical source, Prepared destination home,
  eligibility, clobber-sensitive facts, and required target lowering hook.
- Keep the record free of AArch64 namespaces, register spelling, memory operand
  formatting, and instruction objects.
- Add focused unit or backend coverage for the record if the repo has an
  appropriate local test seam.

Completion check:
- The record builds, is inspectable before target emission, and does not depend
  on AArch64 codegen headers.

## Step 3: Adapt AArch64 Publication Consumption

Goal: rewire AArch64 publication paths to consume the neutral plan while
preserving current emitted behavior.

Primary targets:
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`

Actions:
- Introduce a narrow AArch64 lowering adapter for the neutral plan.
- Keep stack/register/global/immediate/pointer-base-plus-offset emission in
  AArch64 codegen.
- Migrate one publication path at a time and preserve compatibility wrappers
  only where they prevent churn outside the slice.

Completion check:
- AArch64 publication behavior remains equivalent for touched paths, and the
  old target-local planning logic is reduced rather than renamed.

## Step 4: Add Cross-Target Reuse Proof

Goal: prove the neutral publication plan is reusable by another target without
inheriting AArch64 emission policy.

Primary targets:
- `tests/backend/mir/`
- x86 or RISC-V backend test or fixture surfaces

Actions:
- Add a small x86 or RISC-V compile/unit fixture or adapter that consumes the
  neutral record.
- Ensure the proof includes no AArch64 codegen headers and no AArch64-specific
  operand assumptions.

Completion check:
- The cross-target proof builds and demonstrates record consumption without
  AArch64 dependencies.

## Step 5: Validate And Close Readiness

Goal: prove the migration did not regress backend behavior and decide whether
the source idea is complete.

Actions:
- Run build proof for affected C++ targets.
- Run focused backend tests covering touched publication paths.
- Escalate to broader backend or regression-guard validation if multiple
  publication paths or shared prealloc/query surfaces changed.
- Record remaining target-policy-adjacent wrappers in `todo.md`; do not keep
  the idea open for wrappers that are intentionally out of scope.

Completion check:
- Validation is green, the x86/RISC-V reuse path exists, and the source idea's
  acceptance criteria are satisfied or explicitly handed off to a separate
  follow-up idea.
