# No-External-Caller Formal Authority Producer

Status: Open
Type: Producer authority idea
Parent: `ideas/closed/443_closed_world_formal_pointer_authority.md`
Source Evidence: `build/agent_state/443_step4_residual_disposition/`
Owning Layer: LIR/BIR/prepared closed-world callgraph authority

## Goal

Populate a reliable no-external-caller or equivalent closed-world authority
fact for non-internal callee formals before formal pointer provenance consumes
same-module call argument sources.

## Why This Exists

Idea 443 added the formal pointer authority carrier and proved the sound
internal/static route. It also reserved
`FormalPointerAuthorityKind::NoExternalCaller`, but no producer currently
populates it. External-linkage representatives such as `930930-1::f` must
therefore remain fail-closed even when their observed same-module direct
callsites carry computed-address sources.

## In Scope

- Audit whether the current frontend/LIR/module pipeline can prove
  no-external-caller or equivalent closed-world authority for non-internal
  definitions.
- Define producer facts that distinguish accepted closed-world formals from
  external-linkage/no-proof formals.
- Populate `FormalPointerAuthorityKind::NoExternalCaller` or an equivalent
  authority only when the producer can prove it.
- Add focused producer/prepared tests for accepted no-external-caller cases and
  fail-closed external-linkage/no-proof cases.
- Preserve `InternalOnly` behavior from idea 443.

## Out Of Scope

- Publishing external-linkage formal pointer provenance from observed
  same-module callsites alone.
- Treating hidden/protected visibility, same-module wrappers, `LinkNameId`, or
  `can_elide_if_unreferenced` as no-external-caller authority without a tested
  contract.
- Weakening `prepare::prepared_pointer_value_memory_has_proven_authority`.
- Pointer-delta propagation such as `%mr_TR - 8` before base formal pointer
  authority is proven.
- RV64 target-side provenance inference.
- Accepting or modifying `test_baseline.new.log`, including its failed
  `string_authority_guard` state.

## Acceptance Criteria

- The producer has a documented and tested no-external-caller/closed-world
  authority model.
- Accepted cases populate the authority field consumed by idea 442/443
  follow-up logic.
- External-linkage/no-proof functions remain fail-closed by default.
- `930930-1::f` remains fail-closed unless the new producer authority proves no
  outside caller can provide different pointer values.
- Backend proof for implementation packets passes using canonical supervisor
  logs.

## Reviewer Reject Signals

- Reject publishing formal pointer provenance for external-linkage callees from
  observed same-module callsites alone.
- Reject testcase-shaped fixes keyed to `930930-1`, `f`, `reg1`, `mr_TR`, one
  callsite set, one visibility spelling, or one dump layout.
- Reject treating visibility/linkage convenience flags as closed-world
  authority without focused positive and negative tests.
- Reject weakening pointer-value authority checks so unknown layout or
  `UnknownCompatible` range becomes target-consumable.
- Reject baseline/log churn that accepts or rewrites `test_baseline.new.log`.

