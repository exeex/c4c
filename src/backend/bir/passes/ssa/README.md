# B4 / P04 SSA Canonicalization Pass Contract

Contract-Status: converged planned contract under idea 732
Implementation-Status: absent
Phase-ID: B4 / P04
Upstream: exact B3 `CfgCanonical` checkpoint
Downstream: exact B4 `SsaCanonical` checkpoint for B5

## Purpose

B4 is the sole phase-B owner that constructs and proves dynamic graph SSA:
one definition, dominance, use-def, and exact predecessor-qualified phi input.
The static `SsaEligible` kind classification is only an admission predicate and
never proof of these graph facts.

## Owns

SSA renaming/construction, phi insertion/normalization/deletion, complete typed
RAUW, dynamic SSA verification, and `SsaCanonical`.

## Does Not Own

B4 does not alter CFG topology, infer SSA from tags, normalize memory/
aggregates/intrinsics, bind target facts, remove SSA (D5 owns that), allocate,
or publish `CanonicalBir`.

## Inputs

One exact B3 checkpoint with cumulative properties through `CfgCanonical`,
terminator-derived CFG, stable typed definitions/uses, and phi candidates keyed
to exact predecessor edge identities.

## Input NodeKind/Tag Vocabulary

Exactly `B3.Value`, `B3.Effect`, `B3.CanonicalControl`,
`B3.PhiCandidate`, and `B3.OpaqueToken`. Every value-producing kind is queried
for stage-qualified static SSA participation, but the answer grants no dynamic
validity.

## Required Analyses and Products

Fresh exact-B3 `Cfg`, `Dominance`, and `PublicationValueFlow` products with
complete dependency fingerprints. Unknown dominance or incomplete value-flow
is failure, not a reason to publish partial SSA.

### Non-local control-transfer safety

For every call whose closed `CallEffects` says `ReturnsTwice`, and every other
registered non-local-return checkpoint, B4 derives one immutable
`NonLocalSsaBoundary` record. The record is keyed by the exact B3 revision and
names the call/checkpoint instruction, its ordinary continuation instruction,
the definitions visible immediately before the checkpoint, definitions created
or modified after it, and the values whose source semantics permit observation
after a non-local return. Equivalent registered boundaries use the same schema;
callee spelling such as `setjmp` or `longjmp` is never authority.

The record is an instruction-point visibility fact, not an edge or successor.
B3 remains the sole owner of ordinary terminator-derived topology. B4 must not
invent an exceptional predecessor, phi input, dominance relation, or synthetic
block. Instead it applies these closed rules while planning SSA construction:

- a value whose post-return observation is permitted must be represented by a
  definition available on both ordinary and non-local continuations or by an
  explicit addressable memory identity admitted for B5;
- a volatile or address-escaped source object is never promoted to
  register-only state across the boundary;
- a non-volatile automatic object whose source semantics make a modification
  indeterminate after non-local return is marked `PostReturnIndeterminate`; its
  prior SSA value cannot be silently reused as a known value;
- a definition made only after the checkpoint cannot feed a use on the
  non-local continuation; and
- ambiguity about escape, modification, volatility, continuation identity, or
  source observability rejects B4 rather than selecting ordinary single-return
  SSA assumptions.

These prospective record/diagnostic names are documentation vocabulary, not
claims of landed APIs or `NodeKind` entries.

### `asm goto` instruction-point snapshots

For every registered `asm goto` instruction/terminator pair, B4 derives exactly
one immutable `AsmGotoSsaSnapshot`. Its complete key is the exact B3 module and
function revision, function identity, instruction identity, paired terminator
identity, and exact CFG/dominance/publication dependency fingerprints. It
contains the definition stack visible immediately before the asm instruction;
the instruction's ordinary input uses, output definitions, and ordered clobber
facts; one entry for every exact post-B3 successor `EdgeKey`, classified as a
goto-label or the explicitly present fallthrough occurrence; and the exact
edge-visible definition chosen for every semantic value identity.

The definition stack is captured at the instruction point, not at block end.
Every goto-label occurrence sees only definitions available before the asm; it
cannot see an asm output or any later definition. An explicit fallthrough
occurrence sees the pre-asm stack updated by the asm's declared output
definitions, and then ordinary later instructions according to their actual
instruction order. If fallthrough is absent, no fallthrough visibility is
invented. Clobbers are recorded for later allocation consumers but are neither
SSA definitions nor permission to erase an input or output role.

Zero, one, and multiple goto-label occurrences use the same schema. Duplicate
same-destination labels retain different `EdgeKey` entries even when their
visible definition maps are equal. Destination block, predecessor block, label
spelling, rendered block name, layout position, and vector position are never
occurrence identity. A terminator with no successor is invalid at B3 and cannot
acquire a B4 snapshot.

Phi construction queries the snapshot by exact incoming occurrence. A label
incoming selects only its pre-asm-visible definition; a fallthrough incoming
selects the explicit fallthrough-visible definition. After B3 critical-edge
normalization, only the rewritten exact occurrence is authoritative; split
provenance may trace the source but cannot substitute for the live `EdgeKey`.
D5 later consumes that same exact phi-occurrence identity for edge-local copies.

These prospective product and diagnostic names are documentation vocabulary,
not claims of landed APIs or `NodeKind` entries.

## Ordered Behavior

1. Validate B3 capability and all exact analysis keys.
2. Inventory every eligible definition/use and compute dominance frontiers in
   stable block/edge/value order.
3. Derive exactly one `AsmGotoSsaSnapshot` for each registered pair from the
   immutable B3 instruction point and exact successor occurrences; assign every
   node one matrix row and reject non-eligible ordinary SSA use.
4. Build one private candidate, insert/normalize phis, rename definitions and
   uses, and delete redundant phis only through total typed mappings.
5. Recompute CFG/dominance/value-flow on the candidate, prove whole-graph SSA,
   and atomically advance B4 or discard it.
6. Derive and validate every `NonLocalSsaBoundary` against the final candidate;
   publish it only with the same exact B4 revision and SSA proof.
7. Re-derive every asm-goto snapshot against the final candidate and publish it
   only with that same exact B4 revision and whole-graph SSA proof.

## NodeKind/Tag Lowering Matrix

| Closed input subset | Outcome and closed B4 output | Tags retained | Tags added | Tags removed | Identity / provenance | Failure |
|---|---|---|---|---|---|---|
| `B3.Value` whose kind is stage-qualified `SsaEligible` | retain or rename uses into `B4.SsaValue` | value/type/semantic/effect/shape/static SSA classification | dynamic `SsaCanonical` checkpoint proof, not a node tag | none | preserve definition ID when semantics/roles/results unchanged | missing/duplicate/non-dominating definition rejects |
| value requiring merge at dominance frontier | insert canonical `B4.PhiMerge` plus renamed `B4.SsaValue` uses | incoming value types and semantic provenance | phi family, predecessor-qualified roles; dynamic proof | noncanonical transport form | every inserted phi gets fresh ID and ordered source/edge provenance | incomplete incoming edge rejects |
| `B3.PhiCandidate` already exact | retain as `B4.PhiMerge` | value/type/phi/static SSA/predecessor roles | dynamic graph SSA proof only | candidate checkpoint label | preserve only if every incoming role and result is exact | malformed or duplicate incoming rejects |
| redundant `B3.PhiCandidate` | merge/delete through one total typed replacement | selected value/type semantics | none | phi kind/admission on retired node | retire phi ID after total RAUW; retain source as provenance | live unmapped use rejects |
| ordinary value kind marked `NeverSsa` but used as SSA definition | reject | none | none | none | no publication | `SsaKindParticipationInvalid` |
| `B3.Effect` | retain as `B4.Effect` | all non-value effects/roles | none | none | preserve | treating as ordinary definition rejects |
| `B3.CanonicalControl` | retain as `B4.CanonicalControl` | control/successor/effect facts | none | none | preserve | topology mutation forbidden |
| `B3.OpaqueToken` | retain as `B4.OpaqueToken`; ordinary eligible results enter SSA normally | opaque payload/effects and ordinary roles | dynamic proof for eligible results only | none | preserve node; result definitions obey graph proof | hidden special-value channel rejects |
| unknown, illegal, omitted, or import-only kind | reject | none | none | none | no publication | `UnknownSsaForm` |

## Identity and Provenance

Inserted phis always receive fresh IDs. Renaming uses does not change producer
identity. Phi elimination retires the phi after total RAUW. Every mapping is
typed and edge-qualified; source spelling, pointer, position, or arena slot is
not identity.

## Outputs

One exact B4 checkpoint containing `B4.SsaValue`, `B4.PhiMerge`,
`B4.Effect`, `B4.CanonicalControl`, and `B4.OpaqueToken`, cumulative properties
through `SsaCanonical`, exact graph proof, revisions, and mutation summary.

## Verification and Publication

The B4 gate proves registry/stage legality, one definition for every ordinary
SSA value, complete reciprocal def-use, definition dominance, same-block order,
one typed phi incoming per exact predecessor edge including multiplicity, no
hidden special-value path, and unchanged CFG topology. Only this whole-graph
proof establishes dynamic SSA.
For every registered non-local checkpoint it additionally proves unique
instruction-point coverage, exact continuation order, no post-checkpoint-only
definition visible after non-local return, no forbidden register-only promotion,
and no ordinary CFG or phi fact fabricated from the boundary record.
For every registered asm-goto pair it proves exactly one current snapshot, one
entry per exact successor occurrence including duplicates, exact optional
fallthrough classification, complete output/input/clobber coverage, label-edge
exclusion of outputs and later definitions, explicit fallthrough output
visibility, and phi incoming equality with the keyed occurrence. Missing,
duplicate, stale, incomplete, block-end-derived, or topology-changing snapshot
data rejects the whole B4 candidate.

## Analysis Preservation and Invalidation

Definition/use/phi/operand/order changes invalidate value-flow, dominance
consumers, provenance, memory effects that observe values, liveness, and all
dependents. Any asm instruction/terminator pairing, successor occurrence,
fallthrough classification, output/input/clobber, or instruction-order change
invalidates every asm-goto snapshot and its dependents. CFG may be preserved
only after exact topology equality validation; fresh B4 products use the new
key.

## Failure and Diagnostics

Stale analysis, missing/duplicate definition, dominance violation, phi-edge
mismatch, non-eligible SSA use, unknown vocabulary, cancellation, or verifier
failure publishes no partial function, proof token, analysis, or checkpoint.

## Adjacent-Stage Contract

B3 supplies exact canonical CFG. B5 receives only the complete B4 checkpoint
and may rely on dynamic SSA because it names B4's proof, never because a kind
has `SsaEligible`. D5 later owns SSA removal.
It also receives exact-revision `NonLocalSsaBoundary` records; B5 may strengthen
their memory consequences but cannot change their value visibility or topology.

## Implementation State

Absent. Static NodeKind helpers, CFG/dominance documents, or current phi import
support do not implement dynamic B4 construction/proof.

## Proof Requirements

Prove diamonds, loops, parallel edges, unreachable policy, same-block ordering,
phi insertion/elimination, `NeverSsa` negatives, stale analyses, rollback, and
the distinction between static eligibility and dynamic proof.
Also prove returns-twice ordinary versus non-local continuation visibility,
volatile/address-escaped retention, indeterminate-value rejection, missing or
duplicate boundary coverage, stale keys, and absence of fabricated CFG edges.
Prove asm-goto zero/one/multiple label targets, explicit fallthrough
presence/absence, outputs and clobbers in every legal combination, duplicate
same-destination occurrences, critical-edge normalization, exact phi inputs,
label-edge rejection of post-asm/later definitions, fallthrough output
visibility, stale/missing/duplicate snapshots, and absence of name/text/block
identity.

## Open Questions

Any new value/merge form requires a reviewed schema admission and explicit
matrix row; no tag query may weaken the whole-graph verifier.
