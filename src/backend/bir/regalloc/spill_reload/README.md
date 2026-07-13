# Spill and Reload

Status: converged design contract (unimplemented).

## E3 sole rewrite authority

`E3` is the only ordinary capacity-repair mutator. It owns abstract spill
object identities and placement of explicit pseudo `Spill` and `Reload`
operations in BIR. It does not assign homes, choose stack displacements or
load/store encodings, lay out frames, or permit a downstream allocation
repair path.

Input is one typed E2 spill request and a private fork of the exact immutable
revision named by that request. The request must name the matching E1 key,
`ProjectedConstraintKey`, pressure point, complete conflict set, and one not-yet-spilled original
allocation identity. E3 rejects stale requests, requests synthesized without
an exhausted E2 attempt, and identities prohibited from spilling by ABI,
constraint, group, or explicit nonspillable rules.

Every D5 `CopyScratch` reservation is protected by an explicit rule that
forbids memory eviction. E3 rejects a request naming one, never rewrites a
reservation into memory residency, and
never creates replacement scratch. A scratch-home shortage or alias conflict
is an E2 allocation failure, not permission to weaken the D5 plan.

## Explicit spill state

E3 creates one deterministic abstract spill-object identity with the value's
type, size, alignment, address-space/residency class, and originating stable
identity. It never contains a finalized frame location. E3 then places:

- a `Spill` dominated by the assigned value and covering every path on which
  spill residency begins; and
- a `Reload` before each register-resident use region, with a fresh typed
  result that E2 must assign and whose lifetime is limited to that visible
  region.

Placement is CFG- and liveness-aware, including loops, call boundaries, and
edge-local copies. It preserves `ParallelCopy` atomic reads, exact `EdgeKey`
coverage, every `CopyScratch` reservation identity and simultaneous need,
terminator authority, effects, and evaluation order. E3 may split a block or
exact edge only transactionally and must update all affected identities and
graph references. It cannot resolve or schedule a copy bundle, hide a
transition in an operand, assignment table, calling convention, scratch
convention, or MIR mapping.

## Rewrite, reverify, and retry

One E3 rewrite advances the candidate revision and E3 transformation
fingerprint. It invalidates all affected CFG, dominance, def-use/value-flow,
liveness/interference, constraint projections, allocation, and target
realizability facts. Before reverification, every E3 retry invokes the sole
shared `ConstraintProjectionTransaction` with the predecessor projection, new
stamp, exact E3 occurrence fingerprint, spill/reload and CFG mutation summary,
replacement map, and tombstones. It must produce one `ProjectedConstraintSet`
keyed to the retry revision; stable IDs, structurally equal requirements, or
copied records cannot preserve freshness. The runner recomputes required structural facts and runs
the private `AssignedAllocationCandidateGate` on one frozen module. That gate
retains all cumulative graph/Pseudo rules but admits and requires the
candidate's exact assignments and explicit spill state; it cannot publish a
second `PseudoBir`. Only a green full
gate may become the immutable input to a new E1 analysis and a fresh E2
attempt; incremental checks and facts from the prior revision have no
publication authority.

Because an E3 rewrite may change edge-local liveness or pressure, it
invalidates the prior scratch interference and assignment facts even when all
reservation IDs survive. The next E1/E2 retry must re-prove the full
simultaneous-copy problem and assign every reservation again under the new
exact revision.

Progress is monotone and bounded: each green rewrite changes one previously
unspilled original identity to permanent spill residency for the transaction,
and that identity is never selected again. Reload results are allocation
obligations but cannot themselves be selected for another spill rewrite. Thus
the transaction permits at most one E3 rewrite per spill-eligible original
identity. Impossible reload pressure, an empty eligible set, repeated
identity, exceeded budget, unchanged revision, or any verifier failure aborts
instead of looping or weakening legality.

## Candidate verification and failure

The full gate proves spill-object identity/type/alignment consistency, legal
placement, dominance and path coverage, a dominating `Spill` for every
spill-resident path, and a matching `Reload` reaching every later
register-resident use. It rejects redundant or orphan transitions, uncovered
uses, stale E2 requests, recursive spills, and any implicit pressure spill.

Before candidate publication, every allocatable value is either assigned by
E2 or has this verified explicit spill state, and every `Reload` result is
assigned. A candidate is stable only when E2 returns a complete legal
assignment with no spill request and all non-spillable `CopyScratch`
reservations assigned. That exact candidate plus its current E1/E2/E3 facts
is the sole input to D5 copy resolution; E3 cannot run after resolution.
Failure, including projection failure, discards the entire private revision,
spill objects, nodes, assignments, projected product, and derived products;
the predecessor remains unchanged.

Neither copy resolution nor E4 frame-action materialization may run E3
mutation, but both revision advances invalidate the predecessor spill-state
product. Inside the enclosing `AllocatedPublicationTransaction`, after final
projection, E1 recomputation, and E2 assignment validation, E3's spill-state
owner runs a non-mutating validator over the materialized graph. It consumes
the final `PipelineStageStamp`, `CopyResolutionFingerprint`,
`FrameActionFingerprint`, exact final `ProjectedConstraintSet`, exact
`LivenessInterferenceKey`/`AssignmentKey`, and the immutable predecessor spill
object/transition inventory.
It proves that no spill object, `Spill`/`Reload` node, placement, covered use,
or residency transition changed; that every reload result retains a complete
legal assignment under current E1/E2 facts; and that copy replacement did not
introduce an implicit transition. Only then does it install one E3 spill-state
`SpillStateKey` product keyed to the final materialized revision. Stable IDs, an
unchanged inventory, or
the D5 preservation record cannot rekey it. The validator cannot add, remove,
or reposition spill state; failure rolls back the whole publication closure,
leaves predecessor products unchanged, and produces no E4 capability.

The E4 frame-action draft has already fixed spill-object regions, bases,
offsets, and displacements and materialized every required action as an
explicit one-record node. After E3 validation, the non-mutating final
`FrameRealizationPlan` covers those facts and proves every `Spill`/`Reload` and
frame action directly realizable. E3 never chooses placement or adds actions;
failure aborts atomically before `MirReadyBirView` and does not return repair.
