# B3 / P03 CFG Canonicalization Pass Contract

Contract-Status: converged planned contract under idea 732
Implementation-Status: absent
Phase-ID: B3 / P03
Upstream: exact B2 `ScalarsCanonical` checkpoint
Downstream: exact B3 `CfgCanonical` checkpoint for B4

## Purpose

B3 makes terminators, successor slots, blocks, edges, reachability, and
critical-edge policy canonical before dynamic SSA construction.

## Owns

Terminator/edge normalization, unreachable-block disposition, critical-edge
splitting required by B4, deterministic block order, and `CfgCanonical`.

## Does Not Own

B3 does not change scalar meaning, construct phi/SSA, normalize memory or
later families, choose target facts, allocate, or publish `CanonicalBir`.

## Inputs

One immutable B2 checkpoint carrying cumulative Raw/B1/B2 properties, exact
revisions/stamp, stable typed IDs, terminator-owned successors, and no later
stage facts.

## Input NodeKind/Tag Vocabulary

Exactly `B2.ScalarValue`, `B2.CompareValue`, `B2.ConvertValue`,
`B2.SelectValue` (collectively `B2.Value`), `B2.LegalEffect`,
`B2.RawControl`, `B2.RawPhiMerge`, and `B2.OpaqueToken`.

## Required Analyses and Products

One exact-B2 `Cfg` product built only from terminators and stable block IDs.
The pass may independently validate its proposed successor relation; cached
predecessor maps or layout adjacency are not authority.

### `asm goto` topology

An `asm goto` is represented by one structured inline-assembly instruction
immediately followed by its owning terminator. B3 alone canonicalizes the
terminator's ordered successor occurrences:

- zero, one, or multiple ordered goto-label slots are valid;
- one typed fallthrough slot is either explicitly present or explicitly
  absent; block layout never supplies an implicit fallthrough;
- two label slots that name the same destination remain different occurrences;
- output and clobber roles belong to the paired instruction and never create,
  remove, or identify a successor; and
- a form with neither a label occurrence nor fallthrough is rejected rather
  than reinterpreted as ordinary inline assembly.

Each live occurrence is identified only by its terminator-derived `EdgeKey`.
When B3 splits or otherwise normalizes an occurrence, it records a total
old-occurrence-to-new-occurrence provenance mapping and rewrites every existing
edge reference in the same private candidate. Critical-edge normalization may
insert a block and successor, but it cannot merge duplicate occurrences or
infer value visibility. B4 consumes only the resulting exact B3 occurrences.

## Ordered Behavior

1. Validate B2 capability and exact CFG key.
2. Derive successor slots/edge multiplicity from terminators and assign every
   control member one matrix row.
3. Validate every `asm goto` instruction/terminator pair, its explicit optional
   fallthrough, and its zero-or-more ordered label slots; then plan reachability
   removal, terminator replacement, and required edge splits in deterministic
   order.
4. Build one private candidate, rewrite phi-edge references together with CFG
   edits, and derive the mutation summary.
5. Recompute candidate CFG, verify cumulative rules, and atomically advance B3
   or discard the whole candidate.

## NodeKind/Tag Lowering Matrix

| Closed input subset | Outcome and closed B3 output | Tags retained | Tags added | Tags removed | Identity / provenance | Failure |
|---|---|---|---|---|---|---|
| `B2.Value` | retain as `B3.Value` | value/semantic/effect/shape facts | none | none | preserve | any value rewrite forbidden |
| `B2.LegalEffect` | retain as `B3.Effect` | effect/control/shape facts | none | none | preserve | any effect rewrite forbidden |
| canonical return/unreachable control | retain as `B3.CanonicalControl` | control/terminator/successor facts | canonical-CFG checkpoint fact only | none | preserve | malformed role rejects |
| noncanonical jump/conditional/switch/indirect control | replace with exact `B3.CanonicalControl`; split edges when required | branch semantics, condition and ordered successor intent | canonical successor-role shape | Raw/noncanonical control shape | replacement/splits use fresh IDs/blocks and ordered provenance unless all identity conditions hold | missing target or role rejects |
| unreachable block contents | delete only after proving no live externally required value/effect/control obligation, otherwise reject | none on deletion | none | retired node/block admission | retire IDs with diagnostic tombstones | live obligation is `CfgUnreachableLive` |
| `B2.RawPhiMerge` unaffected by topology | retain as `B3.PhiCandidate` with exact rewritten edge keys | value/type/static SSA eligibility and incoming semantics | canonical predecessor-edge association | obsolete edge association | preserve node only if operand roles remain exact; otherwise replace | incomplete edge mapping rejects |
| `B2.OpaqueToken` | retain as `B3.OpaqueToken` | opaque payload/ordinary roles/effects | none | none | preserve | control hidden in payload rejects |
| unknown, illegal, omitted, or import-only kind | reject | none | none | none | no publication | `UnknownCfgForm` |

## Identity and Provenance

New split blocks, terminators, and any role-changed phi candidates get fresh
IDs. A source edge is provenance only, not identity. Deletion requires complete
use/effect closure; block order or label spelling never identifies an edge.

## Outputs

One exact B3 checkpoint with `B3.Value`, `B3.Effect`,
`B3.CanonicalControl`, `B3.PhiCandidate`, and `B3.OpaqueToken`, cumulative
properties through `CfgCanonical`, and a complete mutation summary.

## Verification and Publication

The candidate verifier re-derives CFG solely from terminators; proves one
terminator per defined block, exact successor slots/multiplicity, reachability
policy, split-edge/phi reference coherence, stable ownership/def-use, and no
target/later-stage vocabulary. For `asm goto` it additionally proves exact
instruction/terminator pairing, explicit fallthrough presence or absence,
ordered label-slot coverage including duplicates, a nonempty total successor
set, and total edge-reference rewriting through normalization. It does not
inspect an SSA snapshot or infer visibility. Only a complete green candidate
advances.

## Analysis Preservation and Invalidation

Any block, terminator, edge, order, or phi-edge rewrite invalidates CFG,
dominance, SSA, provenance, publication/value-flow, and dependent analyses.
Fresh B3 CFG and later dominance use the new exact key; no old handle retags.

## Failure and Diagnostics

Stale CFG, invalid successor, incomplete split, live unreachable obligation,
unknown vocabulary, resource/cancellation, or verifier failure publishes no
checkpoint, partial block set, or analysis.

## Adjacent-Stage Contract

B2 supplies scalar-normal input. B4 accepts only this exact B3 checkpoint and
fresh B3 CFG/dominance facts; it cannot repair a malformed CFG.

## Implementation State

Absent. Core terminator storage and CFG analysis design are prerequisites, not
this pass implementation.

## Proof Requirements

Prove every control shape and negative neighbor, parallel edges, loops,
unreachable/live cases, exact phi edge updates, rollback, invalidation, and no
implicit control pass-through.

## Open Questions

Any new terminator shape requires a closed successor-role rule and matrix row.
