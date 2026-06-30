# Closed-World Formal Pointer Authority

Status: Open
Type: Producer authority idea
Parent: `ideas/open/442_pointer_value_memory_provenance_publication.md`
Source Evidence: `build/agent_state/442_step4_residual_disposition/`
Owning Layer: LIR/BIR/prepared function-call authority before external-linkage formal pointer provenance

## Goal

Define and publish reliable closed-world, internal/private, or no-external-caller
metadata proving when same-module callsites are complete authority for a callee
formal pointer.

## Why This Exists

Idea 442 made real progress by publishing sound internal-only same-module
computed-address formal pointer provenance. Step 4 showed the remaining
`930930-1` representative is different: `930930-1::f` has external linkage, so
observed same-module direct callsites are not complete authority for its formal
pointer values. The three `%lv.param.reg1` pointer-value loads and the
`%mr_TR - 8` pointer-delta row must remain fail-closed until the producer can
prove the callee has no outside callers or otherwise has closed-world formal
authority.

## In Scope

- Audit current function/linkage/callgraph metadata available to the prepared
  producer for distinguishing internal/private/closed-world callees from
  externally callable functions.
- Define the exact authority required before same-module call argument sources
  can publish formal pointer provenance for non-`LirFunction::is_internal`
  callees.
- Add focused producer/prepared coverage for accepted closed-world formal
  authority and fail-closed external-linkage/no-proof cases.
- Preserve the completed internal-only provenance route from idea 442.
- Route pointer-delta propagation only after base formal provenance authority
  is soundly published.

## Out Of Scope

- Publishing external-linkage formal pointer provenance from observed
  same-module direct callsites alone.
- Weakening `prepare::prepared_pointer_value_memory_has_proven_authority`.
- Inferring pointer provenance, object extent, layout, or range in RV64 target
  lowering.
- Implementing pointer-delta propagation such as `%mr_TR - 8` before base
  formal authority is proven.
- Store-source/global-memory publication, direct-global return/select-chain,
  terminator/select publication, or selected global object-data work.
- Accepting or committing the rejected full-suite baseline refresh from
  `test_baseline.new.log`; that run had 3356 tests and failed
  `string_authority_guard`.

## Acceptance Criteria

- The producer has a documented authority model for when same-module callsites
  are complete for formal pointer provenance.
- Internal/private/closed-world accepted cases and external-linkage/no-proof
  rejected cases have focused tests.
- External-linkage representatives such as `930930-1::f` remain fail-closed
  unless the new authority model proves no outside caller can provide different
  pointer values.
- Any later idea 442 continuation consumes this authority instead of
  re-deriving it from testcase shape or raw callsites.
- Backend proof for implementation packets passes using the canonical logs
  selected by the supervisor.

## Reviewer Reject Signals

- Reject publishing formal pointer provenance for external-linkage callees from
  observed same-module callsites alone.
- Reject testcase-shaped fixes keyed to `930930-1`, `f`, `reg1`, `mr_TR`, one
  block, one callsite list, or one dump layout.
- Reject weakening pointer-value authority checks so unknown layout or
  `UnknownCompatible` range becomes target-consumable.
- Reject RV64 target changes that infer pointer provenance, layout, object
  extent, or range from raw BIR, value names, integer arithmetic, or object
  labels.
- Reject baseline/log churn that accepts `test_baseline.new.log` or treats its
  failed `string_authority_guard` state as a valid new baseline.

