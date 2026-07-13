# Allocated BIR

Status: scaffold (unimplemented).

## Owns

The E4 transaction freezes one stable E3 candidate and runs the complete
`Allocated` verifier profile against that exact graph revision. Success mints
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

A stable E3 candidate produced from one fully reverified D5 revision, its
verified abstract-register layout, the exact `VerifiedPreparationBundle` and
`BoundConstraintSet`, revision-bound E1 liveness/interference and E2 assignment
facts, and explicit E3 spill/reload state. Every product carries the frozen
candidate revision and its complete predecessor fingerprints.

## Output

An `AllocatedBir`, its same-revision `PreparedBir` capability, and a borrowed
read-only `MirReadyBirView` over the closed semantic node table and
allocator-created `Spill`/`Reload` nodes. Every allocatable identity and each
of its uses has one verified legal abstract home or is covered by explicit
spill residency and a dominating assigned reload result. The view exposes only
verified typed facts and the immutable revision/fingerprint trace needed by
MIR mapping and selection.

## Verification and publication gate

The transaction reruns all applicable Raw/Canonical graph and Pseudo schema
rules, the post-D4 direct-realizability rules, and the post-D5 copy/phi-removal
rules before checking allocation. The final allocation checks prove:

- every allocatable definition, result, edge-copy role, call/inline-asm role,
  fixed-home occurrence, and ordinary use has an assignment in the admitted
  class/group/slot domain;
- assignments satisfy ties, early-clobbers, interference, aliases, reserved
  units, call clobbers, group alignment/width, and simultaneous-copy rules;
- every spill object has one valid abstract identity and compatible value
  class, every `Spill` stores an assigned resident value at a legal point, and
  every `Reload` defines an assigned value that dominates exactly the uses it
  covers; no implicit residency transition or unresolved eviction remains;
- the target, layout, preparation, constraint, pseudo-schema, D4/D5,
  liveness, allocation, and spill fingerprints all name this exact revision
  and one transaction; and
- every node still has its verified one-to-one target mapping and every
  required target/product binding is present, unique, and fresh.

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
temporaries, expand a node, reinterpret a constraint, or conceal a missing BIR
transition. An encoding constraint that cannot honor the view fails the MIR
transaction and requires a separately reviewed upstream D4 schema/legalization
change; it is never repaired while consuming the published view.
