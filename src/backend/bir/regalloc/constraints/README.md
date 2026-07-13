# Register Constraints

Status: converged design contract (unimplemented).

## Sole interpretation authority

This `C9` stage is the sole constraint interpreter in BIR. It is the only
owner that parses original source constraint descriptions, types each admitted
form against `InlineAsmTargetTables` and `VerifiedTargetLayout`, and binds the
result to an `InlineAsm` instruction's ordinary ordered operands and results.
No importer, canonical pass, planner, allocation consumer, downstream builder,
or target emitter duplicates any part of that interpretation.

The initial RV64 vocabulary admits `r`, `=r`, `VR`, `VRM2`, `VRM4`, and
`VRM8`, including reviewed read/write forms, numeric ties, early-clobbers, and
explicit clobbers. `VRM1`, alternatives, named/fixed-register operands, and
unreviewed AArch64/x86 spellings fail closed. Assembly template bytes remain
opaque; mnemonics, directives, placeholders, `.insn`, and concrete names in
template text are first interpreted by the late assembler.

## Input and ordinary-value binding

The only public binding entry point is the all-module transaction:

```cpp
[[nodiscard]] Result<BoundConstraintSet, ConstraintBindingFailure>
bind_constraints(const CanonicalBir& canonical,
                 const TargetProfile& validated_target,
                 const VerifiedPreparationInput& prepared_input,
                 const VerifiedTargetLayout& layout,
                 const VerifiedPreparationBundle& preparation);
```

All five arguments must name the same complete Canonical `PipelineStageStamp`
and exact `TargetFingerprint`; `preparation` must contain the exact C3-C8
fingerprint chain and its C7 `InlineAsmTargetTables`. The API itself traverses
the Canonical module. For every `InlineAsm` it consumes the original constraint
descriptions, ordered clobber descriptions, and the containing instruction's
ordinary operand/result ordinals and stable identities directly from that
snapshot. A caller-supplied reconstructed description list or parallel value
graph is not accepted.

Parsing produces a private syntax result; typing resolves admitted spelling,
role, category/class/group, width, and target eligibility; binding attaches
those requirements to ordinary use/result identities. A read/write operand
retains a distinct incoming use and produced result. A numeric tie requires
assignment equality but never merges SSA identities. Early-clobbers become
interference exclusions, and explicit clobbers resolve to abstract alias units.
No separate inline-assembly value family is created.

## Immutable Canonical output

The atomic output is one immutable `BoundConstraintSet` containing typed
class/group requirements, roles, assignment-equality ties, early-clobber
exclusions, resolved abstract clobber units, and complete instruction/value
bindings. Its key contains the complete Canonical `PipelineStageStamp`, exact
`TargetFingerprint`, layout schema fingerprint, cumulative preparation-bundle
fingerprint, constraint-interpreter schema fingerprint, and a deterministic
digest of every consumed original description and ordered identity binding.

`BoundConstraintSet` is permanently keyed to Canonical. It is never relabeled,
copied, or treated as the constraint product for a D- or E-stage revision.
Pseudo lowering consumes it only as the immutable root binding from which the
shared projection authority below derives revision-local records. This stage
does not classify ABI values, derive target capacity, select
calls/helpers/address strategies, allocate general values, choose encoded
machine names, or create spill/reload state.

## Sole later-revision projection authority

The subordinate `ConstraintProjectionTransaction` defined here is the one
shared projection and preservation authority for every revision after
Canonical. It is infrastructure invoked inside an existing mutator
transaction, not a new A-F stage. Its only output type is an immutable
`ProjectedConstraintSet` with one `ProjectedConstraintKey`.

That key contains the C9 `BoundConstraintSet` fingerprint and its Canonical
stamp; the candidate's exact current `PipelineStageStamp`; the exact target,
layout, constraint-interpreter, pseudo-schema, and projection-schema
fingerprints; the ordered D1/D2/D4/initial-D5/E3/D5-resolution occurrence
fingerprints applicable to that candidate; and a deterministic projection
fingerprint covering the mutator's replacement/tombstone map, mutation
summary, and every current constraint-bearing instruction/value occurrence.
The product records current stable identities and ordinals, their immutable C9
source bindings where applicable, authorized pseudo requirement provenance,
old-to-current mappings, and tombstones. It neither reparses source text nor
chooses an ABI rule, target legalization, home, spill, or copy schedule.

For an unchanged occurrence, the authority may preserve the typed requirement
only after proving its identity, role, ordinal, type, and semantic requirement
unchanged. It still emits a new product and key for the new revision. A
rewritten occurrence is recomputed from the immutable C9 binding plus the
owning D1/D2/D4/D5/E3 transformation record; a new pseudo occurrence must name
the reviewed plan or schema rule authorizing its requirement; a removed
occurrence receives a tombstone and cannot remain bound. Missing mappings,
ambiguous continuations, orphan records, changed roles/types without an
authorizing rule, or incomplete coverage reject the candidate. Stable-ID
equality, structural equality, copied records, and predecessor keys never
establish freshness.

Each mutator invokes this same authority before its private output can be
verified, frozen, published, or consumed:

- D1 supplies the Canonical-to-pseudo replacement map and D1 occurrence
  fingerprint to create the first `ProjectedConstraintSet`;
- D2 supplies its call rewrite map and occurrence fingerprint before D3;
- each mutating D4 occurrence, and the final D4 gate after an empty chain,
  supplies the cumulative legalization map and exact D4 lineage;
- initial D5 supplies its join-removal, copy, scratch, and CFG mutation map
  before publishing the E1 input;
- every E3 retry supplies its spill/reload and any CFG/identity mapping before
  reverification and the next E1 attempt; and
- D5 `CopyResolutionTransaction` supplies its copy replacement and scratch
  tombstone map before the resolved candidate can enter E4.

D3, D4, D5, E1, E2, E3, and E4 accept only the
`ProjectedConstraintSet` whose key names their exact current input revision.
D1 alone may read Canonical-keyed C9 directly, and only to invoke the initial
projection. No later consumer may read C9 or a predecessor projection as its
revision-local constraint product.

Projection is failure-atomic with its enclosing mutator. It builds privately,
validates complete current-occurrence coverage and the entire key, and is
committed only with the candidate revision. Projection cancellation, stale
input, schema mismatch, invalid mapping, or validation failure aborts the
enclosing transaction and publishes no candidate, projection, tombstone,
cache entry, or partial function result. Any key ingredient or covered
occurrence change invalidates the complete product and all consumers derived
from it.

## Transaction, verification, and invalidation

`bind_constraints` parses, types, and binds all module constraints in one
private transaction.
Publication rejects unknown or target-ineligible spellings, malformed roles,
missing operand/result bindings, illegal or cyclic ties, incompatible
classes/groups, absent capacity, impossible early-clobber combinations,
unresolved clobbers, stale identities, incomplete coverage, digest mismatch,
or any Canonical/target/layout/preparation key mismatch. One error publishes
no `BoundConstraintSet`; all inputs remain unchanged.

Any change to the Canonical stage stamp, target fingerprint, layout schema,
preparation-bundle fingerprint, interpreter schema, source description,
clobber order, or operand/result identity/order invalidates the entire product.
Facts from separate transactions cannot be combined.
