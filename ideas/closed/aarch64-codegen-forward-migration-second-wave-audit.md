# AArch64 Codegen Forward-Migration Second-Wave Audit

## Intent

Run a second forward-migration audit over `src/backend/mir/aarch64/codegen`
after the first wave moved the obvious Prepared lookup, value-home/storage,
formal publication, call-boundary classification, edge-copy bookkeeping, and
diagnostic surfaces toward `prealloc`.

The goal is to identify the next set of target-local responsibilities that can
move earlier into `bir`, `prealloc`, or shared MIR support so future x86 and
RISC-V backend work can reuse more of the lowering pipeline.

This is a planning idea. Its main output should be multiple new, focused
`ideas/open/*.md` implementation ideas.

## Reference Model

Continue using
`ref/claudes-c-compiler/src/backend/arm/codegen/README.md` as the responsibility
model for architecture codegen.

The reference AArch64 codegen inventory covers target-facing work:

- core target codegen state and register facts
- prologue/epilogue and parameter-store emission
- calls and return ABI emission
- memory addressing and load/store emission
- ALU, comparison, casts, float, i128, f128, atomics, intrinsics
- globals, variadic support, inline asm, asm emitter, peephole

Current AArch64 responsibilities outside that inventory remain high-probability
forward-migration candidates.

## First-Wave Work Already Done

Do not regenerate ideas for these completed migration families unless the audit
finds a concrete second slice in the same area:

- prepared move/publication lookup indexing
- Prepared value-home and storage interpretation
- entry-formal publication planning
- call-boundary move classification
- edge-copy and block-entry bookkeeping
- Prepared-consumer missing-fact diagnostics

The second wave should look past those first cuts and ask what still forces
`mir/aarch64/codegen` to understand broad Prepared/MIR pipeline mechanics.

## Audit Focus

Review the remaining `dispatch*` and related AArch64 codegen files with special
attention to responsibilities that are not direct AArch64 instruction selection
or printing:

- `dispatch_value_materialization.cpp`
- `dispatch_store_sources.cpp`
- `dispatch_publication.cpp`
- `dispatch_dynamic_stack.cpp`
- `dispatch_branch_fusion.cpp`
- `dispatch_producers.cpp`
- remaining generic portions of `dispatch_calls.cpp`
- remaining generic portions of `calls_moves.cpp`
- helper declarations still collected in `dispatch.hpp`

Also inspect the current prealloc helper surfaces added by the first wave:

- `src/backend/prealloc/prepared_lookups.*`
- `src/backend/prealloc/decoded_home_storage.*`
- `src/backend/prealloc/formal_publications.*`
- call-boundary helpers in `src/backend/prealloc/call_plans.cpp` and
  `src/backend/prealloc/calls.hpp`
- block-entry publication helpers in `src/backend/prealloc/value_locations.hpp`

## Classification

Classify candidate responsibilities into these buckets:

- `Keep target-local`: AArch64 register spelling, ABI register/lane policy,
  instruction sequences, or final machine printing.
- `Move to BIR`: semantic producer or store/load normalization that should be
  represented before preallocation.
- `Move to prealloc`: target-independent or target-parameterized facts that can
  be computed from BIR plus target profile before MIR lowering.
- `Move to shared MIR`: generic Prepared-to-machine orchestration, block
  traversal, publication scheduling, or machine-record construction.
- `Needs target hook`: mostly reusable lowering/planning with small target hooks
  for register classes, scalar widths, or instruction-specific emission.
- `Defer`: plausible but too risky until x86/RISC-V has adjacent consumers.

## Candidate Directions To Evaluate

Use this list as the second-wave starting point, not as a fixed answer:

- same-block producer discovery and select-chain dependency analysis currently
  used by call arguments, branch fusion, and value publication
- scalar value publication planning for stack homes, register homes, globals,
  immediates, and pointer-base-plus-offset homes
- store-source publication planning for local/global stores, especially narrow
  store to wide load recovery and pointer store writeback
- dynamic-stack helper recognition and operand-home planning for stack save,
  stack restore, and dynamic alloca
- branch-fusion eligibility and support-instruction filtering that may split
  into generic producer facts plus target compare/branch hooks
- remaining call move ordering or republication logic that is still generic
  after first-wave call-boundary classification
- helper APIs in `dispatch.hpp` that now expose generic concepts through an
  AArch64 namespace

## Required Output

The runbook generated from this idea should produce:

1. A short map of remaining AArch64 codegen files against the reference README
   inventory, naming what is still outside the reference model.
2. A second-wave candidate table with destination layer, reuse value for
   x86/RISC-V, risk, and suggested proof.
3. Multiple new `ideas/open/*.md` files for the best candidates. Each generated
   idea should be one focused migration slice, not a bundle of unrelated moves.
4. A note explaining which tempting candidates should stay target-local for now.

## Constraints

- Do not implement broad code movement during the audit.
- Do not reopen first-wave work unless a specific remaining second slice is
  identified.
- Do not move AArch64 instruction spelling, ABI lane rules, or final assembly
  sequences into generic layers.
- Do not create shared abstractions without naming how x86 or RISC-V would use
  them.
- Do not weaken tests, expectations, or backend contracts.
- Avoid testcase-shaped moves; every proposed migration should preserve or
  clarify semantic lowering responsibility.

## Acceptance

- The audit identifies a concrete second wave of forward-migration candidates.
- The audit creates multiple follow-up ideas under `ideas/open/`.
- Each follow-up idea is small enough for one focused agent run.
- `plan.md` and `todo.md` are either closed after the audit or intentionally
  handed off to one activated follow-up idea by the plan owner.

## Closure Note

Closed after the audit run completed the required map, candidate table,
follow-up idea generation, and keep-local/defer record. The immediate lifecycle
handoff activated
`ideas/open/shared-mir-same-block-producer-select-queries.md`; the other
generated follow-up ideas remain open for later supervisor selection.
