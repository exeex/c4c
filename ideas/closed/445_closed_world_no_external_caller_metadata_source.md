# Closed-World No-External-Caller Metadata Source

Status: Closed
Type: Metadata producer source idea
Parent: `ideas/closed/444_no_external_caller_formal_authority_producer.md`
Source Evidence:
- `build/agent_state/444_step1_no_external_caller_audit/audit.md`
- `build/agent_state/444_step2_no_external_caller_contract/contract.md`
Owning Layer: Frontend/LIR/module closed-world metadata before prepared formal pointer authority
Closed Evidence:
- `build/agent_state/445_step1_metadata_source_classification/classification.md`
- `build/agent_state/445_step2_source_contract_rejection/rejection.md`

## Goal

Introduce or identify a real closed-world/no-external-caller metadata source
that can safely feed `FormalPointerAuthorityKind::NoExternalCaller` for
non-internal definitions.

## Why This Exists

Idea 444 defined the `NoExternalCaller` producer contract and proved the
current pipeline cannot populate it soundly. The missing prerequisite is not in
the formal pointer consumer: the compiler needs a durable function-level source
of authority, such as a tested closed-world mode, whole-program analysis,
linker/visibility contract, complete caller-set guarantee, function-address
escape summary, and indirect-call target exclusion.

Until such a source exists, external-linkage functions like `930930-1::f` must
remain `FormalPointerAuthorityKind::Unknown`, and idea 442 must not publish
external-linkage formal pointer provenance.

## In Scope

- Decide which compiler-owned source can legitimately assert
  no-external-caller or equivalent closed-world authority for non-internal
  definitions.
- Define required metadata for function identity, definition status, complete
  caller-set coverage, address escape, indirect-call target exclusion, and any
  visibility/linker contract.
- Add focused positive and negative coverage for the selected metadata source.
- Feed `FormalPointerAuthorityKind::NoExternalCaller` only after the source is
  explicit and tested.
- Preserve `InternalOnly` behavior from idea 443 and the false-by-default
  contract from idea 444.

## Out Of Scope

- Publishing formal pointer provenance from observed same-module direct calls
  alone.
- Treating hidden/protected visibility, `LinkNameId`,
  `can_elide_if_unreferenced`, or absent extern declarations as authority
  without a tested contract.
- Weakening `prepare::prepared_pointer_value_memory_has_proven_authority`.
- Pointer-delta propagation such as `%mr_TR - 8` before base formal authority
  is proven.
- RV64 target-side provenance inference.
- Accepting or modifying `test_baseline.new.log`.

## Acceptance Criteria

- A concrete metadata source is selected or the route is explicitly rejected as
  unavailable in the current compiler model.
- Accepted metadata has focused tests proving positive no-external-caller cases
  and negative external-linkage/no-proof/address-escape/indirect-call cases.
- `FormalPointerAuthorityKind::NoExternalCaller` is populated only from the
  selected metadata source.
- External-linkage representatives such as `930930-1::f` remain fail-closed
  unless the metadata source proves no outside caller can provide different
  pointer values.
- Any later idea 442 continuation consumes the authority field rather than raw
  callsites or testcase shape.

## Completion Notes

- Step 1 classified current frontend, HIR, LIR, module, linker/visibility,
  callgraph, and whole-program-mode surfaces.
- Step 2 rejected every current non-internal metadata source for
  `FormalPointerAuthorityKind::NoExternalCaller`.
- Static/internal linkage remains accepted only for `InternalOnly`.
- `NoExternalCaller` remains unpopulated, and `930930-1::f` remains `Unknown`
  and fail-closed.
- A future accepted source would need producer-owned closed-world authority,
  semantic identity, definition-status proof, complete caller-set coverage,
  function-address escape coverage, indirect-call target exclusion/coverage,
  and tested visibility/linker semantics if used.
- No implementation packet is available in the current compiler model.
- Close gate used canonical `test_before.log` and `test_after.log`; both logs
  report 327 passed, 0 failed, and the non-decreasing regression guard passed.

## Future Source Candidates

These are not active follow-up ideas yet because no source has been selected:

- compiler-owned whole-program/LTO-style mode with complete caller and escape
  semantics;
- tested linker/visibility contract proving no outside caller can invoke
  selected non-internal definitions.

## Reviewer Reject Signals

- Reject testcase-shaped fixes keyed to `930930-1`, `f`, `reg1`, `mr_TR`, one
  callsite set, one visibility spelling, or one dump layout.
- Reject metadata that claims closed-world authority without accounting for
  function-address escape and indirect-call target exclusion.
- Reject treating visibility/linkage convenience flags as sufficient without
  focused positive and negative tests.
- Reject weakening pointer-value authority checks so unknown layout or
  `UnknownCompatible` range becomes target-consumable.
- Reject baseline/log churn that accepts or rewrites `test_baseline.new.log`.

