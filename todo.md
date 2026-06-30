Status: Active
Source Idea Path: ideas/open/445_closed_world_no_external_caller_metadata_source.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Selected Source Contract Or Rejection

# Current Packet

## Just Finished

Completed Step 2: defined the selected-source contract and rejected all current
metadata sources for non-internal `NoExternalCaller`.

Selected-source decision:

- No current compiler-owned metadata source is accepted for
  `FormalPointerAuthorityKind::NoExternalCaller` on non-internal definitions.
- Static/internal linkage remains the accepted baseline for
  `FormalPointerAuthorityKind::InternalOnly` only.
- `FormalPointerAuthorityKind::NoExternalCaller` should remain unpopulated in
  the current compiler model.
- External-linkage definitions such as `930930-1::f` remain `Unknown` and
  fail-closed.

Future accepted source contract:

- Producer-owned explicit closed-world/no-external-caller fact.
- Semantic function identity for the exact definition.
- Definition-status proof; declarations and unknown externals are false by
  default.
- Complete caller-set coverage.
- Function-address escape coverage.
- Indirect-call target exclusion or coverage.
- Tested visibility/linker semantics if visibility or binding is part of the
  source.
- Positive and negative tests proving both publication and fail-closed
  behavior.

Rejected current signals:

- Observed same-module direct calls or computed-address call sources.
- Visibility spelling, `LinkNameId`, `can_elide_if_unreferenced`, local
  reachability, absent extern declarations, prepared wrapper shape, prepared
  value homes, or testcase shape.
- Current function-address references or indirect-call markers without a
  complete escape and target-set proof.

Artifact:

- `build/agent_state/445_step2_source_contract_rejection/rejection.md`

## Suggested Next

Plan-owner disposition for idea 445.

Recommendation: close this plan as source rejection documentation unless the
project wants to split a new source idea. A future implementation should begin
from a new compiler-owned whole-program/LTO-style mode or a tested
linker/visibility contract; do not proceed to Step 3 implementation in this
plan without that source.

## Watchouts

- Do not infer authority from observed same-module direct calls, visibility
  spelling, `LinkNameId`, `can_elide_if_unreferenced`, absent extern
  declarations, or testcase shape.
- Account for function-address escape and indirect-call target exclusion before
  accepting any metadata source.
- Keep `930930-1::f` fail-closed unless a real source proves no outside caller
  can provide different pointer values.
- Do not weaken `prepare::prepared_pointer_value_memory_has_proven_authority`.
- Do not accept or modify `test_baseline.new.log`.

## Proof

Step 2 contract/rejection-only check:

```sh
git diff --check
```
