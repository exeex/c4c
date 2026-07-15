# D5 Out-of-SSA and Copy-Resolution Contract

Contract-Status: converged planned contract under idea 732
Implementation-Status: absent
Phase-ID: D5
Upstream: exact fully reverified D4 allocation-free Pseudo revision
Downstream: initial allocation-free D5 publication for E1; subordinate resolved candidate for E4

## Purpose

D5 is the sole required dynamic-SSA removal owner. Initial D5 replaces every
pending phi with explicit edge-local simultaneous transfers and allocation-
visible scratch reservations. After stable E3, a subordinate D5 transaction
uses assigned homes to resolve transfers into directly realizable edge copies
and stages the result privately for E4.

## Owns

Phi elimination, edge-copy plan/placement, `ParallelCopy`/`CopyScratch`
formation, total mappings, initial projection/reverification/publication, and
post-E3 copy scheduling/resolution with `CopyResolutionFingerprint`.

## Does Not Own

D5 does not alter ABI/call/target legalization, choose general assignments,
create spill decisions, change assigned homes, lay out frames, publish an E
stage, or hide copies/scratch in analysis-only products.

## Inputs

Initial: exact D4 `PseudoBir`, current projection, CFG/dominance/SSA/value-flow,
and complete phi edge identities. Subordinate closure: exact stable post-E3
candidate plus current projection, E1 liveness/interference, E2 assignments,
E3 spill state, scratch assignments, and unchanged D5 plan lineage.

## Input NodeKind/Tag Vocabulary

Initial input is exactly D4 realizable value/memory/effect/control/call/opaque
groups plus pending phi. Subordinate input additionally admits the explicit
initial-D5 `ParallelCopy`, `CopyScratch`, spill/reload, and assigned forms named
by their planning contracts. These are prospective, not production enum claims.

## Required Analyses and Products

Initial: exact CFG, dominance, dynamic SSA proof, value-flow, projection, and
terminator-derived edge occurrence identity. Subordinate: exact E1/E2/E3 and
alias-unit products for the same stable candidate; no recomputed compatible homes.

## Ordered Behavior

1. Validate exact D4 revision/products and inventory every phi/incoming edge.
2. Build a complete immutable edge-copy plan with one typed transfer per edge
   occurrence and explicit scratch requirement for each possible alias cycle.
3. Privately split edges/place `ParallelCopy` and `CopyScratch`, eliminate all
   phis through total RAUW, request fresh projection, and run full Pseudo gate.
4. Publish the initial allocation-free D5 revision for E1/E2.
5. After stable E3 only, schedule each bundle using exact assigned homes,
   replace parallel/scratch forms with directly realizable `EdgeCopy` sequence,
   reproject/reverify privately, and stage one resolved candidate for E4.

## NodeKind/Tag Lowering Matrix

| D5 input subset | Outcome | Tags retained | Tags added | Tags removed | Identity / provenance | Failure |
|---|---|---|---|---|---|---|
| pending phi | replace/delete into edge-local `D.ParallelCopy` entries and destination allocation identities | value/type/edge semantics | explicit simultaneous-copy def/use roles, pseudo admission | phi family/static SSA form/dynamic SSA requirement | phi retires after total typed RAUW; transfers fresh with exact EdgeKey provenance | incomplete incoming coverage rejects |
| possible alias/cycle component | insert `D.CopyScratch` reservation identity | type/class/group requirement | allocation-visible nonspillable scratch role | none | always fresh identity, no execution semantics | hidden/unbounded scratch rejects |
| non-phi D4 realizable node | retain | all classifications/roles/effects | `NeverSsa`/post-SSA form only where the reviewed pseudo kind requires it | dynamic SSA checkpoint requirement | preserve | mutation forbidden |
| opaque inline asm | retain with ordinary defs/uses now post-SSA | bytes/bindings/effects | post-SSA graph participation as applicable | dynamic SSA proof requirement | preserve | hidden special channel rejects |
| initial-D5 `ParallelCopy` after stable E3 | expand/schedule into typed directly realizable `D.EdgeCopy` sequence | transfer type/endpoints/edge provenance | assigned endpoint/copy realizability facts | simultaneous bundle form | parallel source retires; edge copies fresh ordered provenance | unsafe schedule rejects |
| assigned `CopyScratch` after stable E3 | consume only in proved cycle/overlap schedule, then retire | assigned scratch-home evidence | none | scratch reservation form | scratch identity retires after all uses realized | unassigned/aliased/spilled scratch rejects |
| spill/reload and other stable assigned nodes | retain unchanged during subordinate closure | identities/homes/effects/roles | none | none | preserve; D5 cannot change homes | assignment drift rejects |
| residual phi, unresolved parallel/scratch at E4 seam, unknown/illegal/omitted kind | reject | none | none | none | no publication/staging | `D5ClosureInvalid` |

## Identity and Provenance

Phi identities retire; former join-result allocation identities remain exact
destinations only through total mappings. Every transfer/scratch/copy gets a
fresh ID keyed to exact edge occurrence and role. Assigned home equality never
merges semantic identity.

## Outputs

Initial output: allocation-free D5 `PseudoBir` with zero pending phi, explicit
`ParallelCopy`/`CopyScratch`, current projection, and every introduced def/use/
clobber/scratch visible to E1/E2. Subordinate output: private resolved candidate
with only directly realizable `EdgeCopy`, exact projection, and
`CopyResolutionFingerprint`, staged for E4 without publication.

## Verification and Publication

Initial full Pseudo gate proves phi absence, exact edge coverage, simultaneous
semantics, scratch visibility/typing, post-SSA def-use, projection, and no
assignments/spills. Subordinate assigned-candidate gate proves current E1/E2/E3,
safe schedules, unchanged homes/spills, no parallel/scratch residue, and exact
reprojection. Only initial D5 publishes Pseudo; closure stages privately.

## Analysis Preservation and Invalidation

CFG splits, phi removal, copies, defs/uses, and projection invalidate CFG,
dominance, SSA, value-flow, liveness, constraints, and realizability as
applicable. Initial output recomputes for E1; subordinate output recomputes only
the facts required by E4 and cannot reuse predecessor handles.

## Failure and Diagnostics

Missing edge/incoming, duplicate destination, unsafe cycle, absent/illegal
scratch, stale E1/E2/E3, assignment drift, projection/verifier failure,
cancellation, or resource exhaustion publishes/stages nothing.

## Adjacent-Stage Contract

E1 consumes the exact initial D5 revision and must model every explicit value,
definition, use, clobber, `ParallelCopy`, and `CopyScratch`. E2 assigns scratch
normally under nonspillable/nonalias constraints. After stable E3, subordinate
D5 consumes exact assignments and hands only its private resolved candidate to
E4; it cannot bypass E1/E2/E3 or publish early.

## Implementation State

Absent. Imported phi support or legacy copy resolution is not D5 implementation.

## Proof Requirements

Prove diamonds/loops/parallel edges, phi-zero initial output, transfer
multiplicity, scratch visibility/assignment, acyclic/overlap/cycle scheduling,
stale products, projection/full gates, and E1/E4 adjacency.

## Open Questions

New copy/scratch forms require explicit prospective schema and verifier rows.
