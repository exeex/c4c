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

## Immutable output and consumers

The atomic output is one immutable `BoundConstraintSet` containing typed
class/group requirements, roles, assignment-equality ties, early-clobber
exclusions, resolved abstract clobber units, and complete instruction/value
bindings. Its key contains the complete Canonical `PipelineStageStamp`, exact
`TargetFingerprint`, layout schema fingerprint, cumulative preparation-bundle
fingerprint, constraint-interpreter schema fingerprint, and a deterministic
digest of every consumed original description and ordered identity binding.

Pseudo lowering and boundary verification consume this product. E1 incorporates
its ties, early-clobber exclusions, group requirements, and resolved abstract
clobber units into revision-bound interference facts; E2 consumes the same
record as immutable legality data. Neither stage may reparse, retype, repair,
or rebind it. This stage does not classify ABI values, derive target capacity,
select calls/helpers/address strategies, allocate general values, choose
encoded machine names, or create spill/reload state.

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
