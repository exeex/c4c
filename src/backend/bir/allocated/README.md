# Allocated BIR

Status: scaffold (unimplemented).

## Owns

The E4 transaction freezes the exact resolved revision produced when D5's
subordinate `CopyResolutionTransaction` succeeds on one stable E3 candidate,
and runs the complete `Allocated` verifier profile against that revision.
Success mints
one owning `AllocatedBir` stage token, one `PreparedBir` readiness capability
bound to that token, and borrowing `MirReadyBirView` instances. All three name
the same immutable module/function revisions and the same preparation,
allocation, target-layout, and product fingerprints. `PreparedBir` contains no
graph storage, and the view never clones nodes or facts. Names are prospective
while this scaffold remains unimplemented.

## Does not own

Machine instruction selection, concrete target register spelling, frame-offset
encoding, assembler parsing, or late repair of incomplete allocation. It also
does not reinterpret constraints, recompute allocation policy, or admit a
machine-only home that was absent from the frozen BIR revision.

## Input

One private D5 copy-resolution output containing no `ParallelCopy` or
`CopyScratch` node, its exact `CopyResolutionFingerprint`, verified
abstract-register layout, exact `VerifiedPreparationBundle`, and exact current
revision projections of the constraint, E1 liveness/interference, E2
assignment, and E3 spill-state products. The constraint product is exactly one
`ProjectedConstraintSet` produced inside D5 copy resolution and keyed to the
resolved revision; E4 never reads Canonical C9 or an E3 predecessor
projection. Every other product is preserved or reprojected by its named owner
for that revision and carries the complete predecessor fingerprints. Stable
IDs, structural equality, and copied records do not establish freshness.

Specifically, before E4 begins, the enclosing `CopyResolutionTransaction` has
already invoked E1 to recompute resolved-graph liveness/interference, E2's
allocator-owned validator to install the unchanged-but-reproved assignment,
E3's spill-state validator to install the unchanged-but-reproved spill
inventory and transitions, and the existing target realizability
registry/checker to recompute its product. Each is keyed to the same resolved
`PipelineStageStamp` and `CopyResolutionFingerprint` in dependency order after
the sole `ConstraintProjectionTransaction`. E4 rejects a predecessor-keyed or
merely copied `LivenessInterferenceKey`, `AssignmentKey`, `SpillStateKey`,
`ProjectedConstraintKey`, or `TargetRealizabilityKey` product.

## Output

An `AllocatedBir`, its same-revision `PreparedBir` capability, and a borrowed
read-only `MirReadyBirView` over the closed semantic node table and
allocator-created `Spill`/`Reload` nodes plus D5-resolved single-move
`EdgeCopy` nodes. Every allocatable identity and each of its uses has one
verified legal abstract home or is covered by explicit spill residency and a
dominating assigned reload result. Every remaining non-`InlineAsm` node is
directly one-to-one realizable. The view exposes only verified typed facts and
the immutable revision/fingerprint trace needed by MIR mapping and selection.

## Verification and publication gate

The transaction reruns all applicable Raw/Canonical graph and Pseudo schema
rules, the post-D4 direct-realizability rules, and the post-D5 copy/phi-removal
rules before checking allocation. The final allocation checks prove:

- every allocatable definition, result, edge-copy role, call/inline-asm role,
  fixed-home occurrence, and ordinary use has an assignment in the admitted
  class/group/slot domain;
- assignments satisfy ties, early-clobbers, interference, aliases, reserved
  units, call clobbers, and group alignment/width; the copy-resolution record
  separately proves preservation of the former simultaneous-copy semantics;
- every spill object has one valid abstract identity and compatible value
  class, every `Spill` stores an assigned resident value at a legal point, and
  every `Reload` defines an assigned value that dominates exactly the uses it
  covers; no implicit residency transition or unresolved eviction remains;
- the target, layout, preparation, exact `ProjectedConstraintKey`,
  pseudo-schema, D4/D5,
  liveness, allocation, spill, and `CopyResolutionFingerprint` products all
  name this exact resolved revision and one transaction;
- no `ParallelCopy` or `CopyScratch` node remains, every surviving `EdgeCopy`
  is one legal single-home transfer, and the resolution record covers every
  former bundle exactly; and
- every non-`InlineAsm` node still has its verified one-to-one target mapping,
  and every required target/product binding is present, unique, and fresh.

Any non-admitted node, missing or illegal assignment, unresolved pressure,
stale or mixed fingerprint, inconsistent spill object, malformed transition,
active editor, revision change, diagnostic, cancellation, or deterministic
resource failure discards the transaction. It publishes none of
`AllocatedBir`, `PreparedBir`, `MirReadyBirView`, a partial function token, or
a reusable green report. Candidate/recheck APIs are diagnostic only and cannot
mint or repair these capabilities.

MIR accepts only `MirReadyBirView`, rechecks its revision and fingerprints,
maps each verified abstract home through the exact target layout, and selects
one machine record for each allocated pseudo node. It may choose concrete
spellings, frame offsets for verified abstract objects, and encodings, but it
cannot change assignments, add capacity spills/reloads or allocatable
temporaries, resolve or schedule copies, expand a node, reinterpret a
constraint, or conceal a missing BIR transition. An encoding constraint that
cannot honor the view fails the MIR transaction and requires a separately
reviewed upstream D4 schema/legalization change; it is never repaired while
consuming the published view.
