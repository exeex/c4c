# 122 Prepared Call Argument Producer Materializability Contract

## Goal

Move the remaining target-neutral call-argument producer materializability and
publication-source routing facts out of AArch64 calls lowering and into a
shared prepared query or fact surface.

## Why This Exists

Idea 118 classified the scalar producer dispatch bridge as
`move-forward-needed`. Calls lowering already consumes idea 116 dispatch facts,
but it still shapes call-argument producer materialization, direct-global
select-chain dependencies, and missing frame-slot argument publication needs
locally. Those facts should be shared before an AArch64-local calls owner is
contracted.

## In Scope

- A target-neutral prepared query or fact describing whether a call-argument
  source producer is materializable for call lowering.
- Publication-source routing for call arguments, including direct-global
  select-chain dependencies where currently rediscovered by AArch64 calls.
- A prepared indication of missing frame-slot argument publication needs when
  that decision is target-neutral.
- AArch64 calls updates only as consumers of the new shared query/fact.
- Dump or test visibility showing the shared call-argument producer facts.

## Out Of Scope

- Reopening idea 116 edge-publication producer, current-block publication,
  join-routing, or select-chain contracts that are already closed.
- Adding AArch64-only same-block producer discovery, BIR-name matching, or
  named select-chain shortcuts.
- Moving AArch64 scalar instruction emission, register retargeting, select
  chain assembler spelling, local aggregate address payloads, or machine
  instruction wrapping into shared code.
- Broad dispatch rewrite or calls file splitting before the shared query is
  proven.
- Changing x86 or RISC-V codegen implementation.

## Likely Files

- Shared prepared/BIR prealloc query owners under `src/backend/mir/` or
  `src/backend/prealloc/`
- Existing prepared lookup or printer files that expose call argument source
  facts
- `src/backend/mir/aarch64/codegen/calls.cpp` as a consumer/proof target
- Existing backend tests covering prepared publication, select-chain
  dependencies, call argument producers, and AArch64 scalar call arguments

## Owner Boundary

Shared prepared code owns target-neutral producer materializability,
publication-source routing, direct-global select-chain dependency facts, and
missing frame-slot argument publication needs. AArch64 calls lowering owns
machine-specific scalar materialization, emitted-register updates, select-chain
assembler line construction, local aggregate address payload emission, and
dispatch-bridge machine instruction wrapping.

## Proof Route

- Characterize current call-argument producer helpers, including
  `lower_scalar_call_argument_producers`,
  `materialize_scalar_call_argument_value`,
  `find_prepared_scalar_call_argument_source_producer`,
  `materialize_direct_global_select_chain_call_argument`, and
  `materialize_missing_frame_slot_call_arguments`.
- Add a focused prepared query/fact plus printer or dump visibility for the
  target-neutral call-argument producer decision.
- Run a narrow backend/AArch64 subset covering ordinary scalar producer
  materialization, direct-global select-chain call arguments, local aggregate
  address call arguments, and missing frame-slot argument publication.
- Escalate to broader backend proof if shared dispatch or prepared lookup
  semantics change across consumers.

## Acceptance And Completion Criteria

- AArch64 calls lowering consumes a shared call-argument producer
  materializability/publication query instead of rediscovering the
  target-neutral producer route.
- Direct-global select-chain dependency and missing frame-slot argument
  publication needs are visible as prepared facts or justified as target-local
  with evidence.
- Remaining AArch64 code is limited to scalar instruction lowering, register
  retargeting, select-chain emission, aggregate address payload emission, and
  machine instruction wrapping.
- Proof covers neighboring producer routes, not only one named select-chain
  testcase.

## Reviewer Reject Signals

- The route adds same-block producer scans, BIR-name matching, direct-global
  named-case matching, or select-chain special cases in AArch64 calls.
- The route duplicates or reopens idea 116 dispatch producer/current-block
  publication/join-routing/select-chain authority instead of consuming it.
- The shared query is only a renamed wrapper around the old AArch64 discovery
  logic with the same failure mode.
- Tests are weakened, supported paths are marked unsupported, or expectations
  are rewritten to avoid producer materialization.
- The proof covers only one narrow select-chain fixture while nearby scalar,
  aggregate-address, and frame-slot publication routes remain unexamined.
- Target-specific AArch64 scalar instruction spelling or register policy moves
  into shared prepared code.
