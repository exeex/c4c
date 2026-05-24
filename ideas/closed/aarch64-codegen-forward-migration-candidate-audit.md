# AArch64 Codegen Forward-Migration Candidate Audit

## Intent

Analyze the current `src/backend/mir/aarch64/codegen` subsystem and identify
which responsibilities can move earlier into `bir`, `prealloc`, or shared MIR
support so future x86 and RISC-V backend work can reuse more logic.

This is a discovery and planning idea. Its primary output should be multiple
new, smaller implementation ideas, not a broad implementation rewrite.

## Reference Model

Use `ref/claudes-c-compiler/src/backend/arm/codegen/README.md` as the baseline
for what belongs in an architecture codegen module.

The reference codegen inventory includes architecture-facing responsibilities
such as:

- core target codegen state and register facts
- prologue/epilogue and parameter store emission
- calls and return ABI emission
- memory addressing and load/store emission
- ALU, comparison, casts, float, i128, f128, atomics, intrinsics
- globals, variadic support, inline asm, asm emitter, peephole

Treat current AArch64 codegen responsibilities that do not fit this inventory
as high-probability forward-migration candidates.

## Problem Statement

Recent AArch64 codegen cleanup reduced header noise and merged a few small glue
files, but most changes stayed inside `src/backend/mir/aarch64/codegen`.

That means the larger architectural question remains: the AArch64 backend still
appears to own Prepared/MIR bridge work that may be better represented as
earlier facts or shared lowering helpers. If left target-local, x86 and RISC-V
will likely need to rediscover or duplicate the same logic.

## Audit Scope

Review all files under `src/backend/mir/aarch64/codegen`, especially the
families that are not present as first-class modules in the reference README:

- `dispatch*.cpp` / `dispatch.hpp`
- `calls_*.cpp` portions that are not target ABI emission
- `operands.cpp` and Prepared value-home decoding
- `traversal.cpp` and Prepared index construction
- `compatibility_projection.cpp`
- publication, edge-copy, producer, store-source, and value-materialization
  helpers
- any remaining code that interprets Prepared facts rather than selecting or
  printing AArch64 instructions

Also inspect the existing earlier layers before proposing moves:

- `src/backend/bir`
- `src/backend/prealloc`
- shared MIR/module code under `src/backend/mir`

## Classification

For each notable AArch64 codegen responsibility, classify it into one of these
buckets:

- `Keep target-local`: real AArch64 instruction selection, register spelling,
  target ABI emission, or target printing.
- `Move to prealloc`: target-independent or target-parameterized contract facts
  that should be computed before MIR lowering.
- `Move to BIR`: semantic normalization or IR fact recovery that should be
  expressed before preallocation.
- `Move to shared MIR`: generic Prepared-to-machine-record lowering,
  publication bookkeeping, diagnostics, indexes, or traversal utilities useful
  to x86/RISC-V.
- `Needs target hook`: mostly generic logic that requires a small target policy
  interface instead of full target-local duplication.
- `Unknown / defer`: requires more proof or adjacent target review before
  moving.

## Required Output

The runbook generated from this idea should produce:

1. A concise audit table mapping current AArch64 codegen files or helper groups
   to the classification above.
2. A comparison against the reference README inventory, explicitly calling out
   which current files are outside the reference codegen responsibility model.
3. A prioritized list of forward-migration candidates, ordered by reuse value
   for future x86/RISC-V work and implementation risk.
4. Multiple new `ideas/open/*.md` files, each scoped to one concrete migration
   slice. Each generated idea should name:
   - the current AArch64-owned responsibility
   - the proposed destination layer
   - why x86/RISC-V would benefit
   - proof needed to avoid testcase overfit

## Candidate Directions To Consider

Do not treat this list as final; use it as a starting set for the audit.

- Prepared move/publication indexing and lookup helpers that could become
  shared MIR utilities.
- Value-home and storage-encoding interpretation that could become prealloc
  contract accessors.
- Entry-formal publication planning that might belong in prealloc with target
  ABI facts as input.
- Call boundary move classification that might become shared call-lowering
  scaffolding with target-specific emission hooks.
- Edge-copy and block-entry publication bookkeeping that could move closer to
  out-of-SSA/prealloc.
- Diagnostics for missing Prepared facts that could be standardized in shared
  MIR lowering instead of repeated per target.

## Constraints

- Do not implement broad code movement as part of this audit unless a tiny
  mechanical extraction is needed to prove the classification.
- Do not move AArch64-specific register spelling, instruction encoding, ABI
  lane rules, or instruction printing into generic layers.
- Do not create generic abstractions until at least one x86 or RISC-V reuse path
  is plausible and named.
- Do not weaken tests, expectations, or backend contracts.
- Avoid testcase-shaped shortcuts. Forward migration must preserve semantic
  lowering behavior, not just current AArch64 outputs.

## Acceptance

- The audit identifies which parts of `mir/aarch64/codegen` are outside the
  reference codegen responsibility model.
- The audit produces multiple concrete follow-up ideas under `ideas/open/`.
- Each generated idea is small enough for one focused agent run.
- No active implementation plan is left half-open unless one generated idea is
  intentionally activated by the plan owner.

## Completion Note

Closed after the active runbook produced the audit table, reference-model
comparison, prioritized candidate list, and six focused follow-up ideas under
`ideas/open/`. The generated ideas cover prepared move/publication indexing,
value-home and storage interpretation, call-boundary move classification,
entry-formal publication planning, edge-copy/block-entry bookkeeping, and shared
Prepared-consumer diagnostics. No implementation changes or weakened contracts
were used as audit evidence, and no generated implementation idea was
activated.
