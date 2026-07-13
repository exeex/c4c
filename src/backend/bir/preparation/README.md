# Immutable Preparation Planners

Status: converged design contract (unimplemented).

## Boundary and exact order

`CanonicalBir` means exactly the immutable, target-independent B7 / P07
revision published by B8. Preparation begins only after the [external C1
target-profile boundary](../../../target_profile/README.md) validates one
`TargetProfile` and `verify_preparation_input` returns a borrowing
`VerifiedPreparationInput` over that exact `CanonicalBir`/target pair. The
capability carries the complete Canonical `PipelineStageStamp`—module epoch,
module revision, and ordered function-revision digest—and the exact
`TargetFingerprint`. It does not create a new BIR revision or contain a target
layout, ABI classification, plan, parsed constraint, assignment, or any other
prepared fact.

The C1 gate promises only that the B8 capability and selected target are fresh
and mutually bound and that every preparation-facing semantic input is
present, typed, and stably identified: function and call signatures and calling
conventions, variadic boundaries and promotions, address/object/relocation
descriptors and required provenance, original inline-assembly descriptions
with complete ordinary operand/result identities, and helper-eligible semantic
operation descriptors. It rechecks the applicable Canonical and forbidden-fact
rules without mutation. Target layout, eligibility, placement, helper-route,
and constraint-meaning decisions remain exclusively C2-C9 and may still fail
closed. A verified target layout carrying the same stamp and target fingerprint
is required after C2.

The dependency order is fixed and serial:

```text
C1 VerifiedPreparationInput + C2 VerifiedTargetLayout
  -> C3 AbiPlan
  -> C4 CallPlan
  -> C5 VariadicPlan
  -> C6 AddressPlan
  -> C7 InlineAsmTargetTables
  -> C8 RuntimeHelperPlan
  -> VerifiedPreparationBundle
  -> C9 register-constraint interpretation
```

This order is a fact dependency chain, not a sequence of BIR revisions. Each
planner sees all immutable predecessor products named above, and no planner may
skip, recreate, or silently weaken a predecessor. The `runtime_helpers` step is
after `inline_asm`, matching the normative root order; the documentation review
order does not alter execution order.

## Common product key and ownership

Every planner output is immutable and carries one `PreparationKey` containing
the complete Canonical `PipelineStageStamp`, exact `TargetFingerprint`, exact
target-layout schema fingerprint, planner schema fingerprint, and the ordered
fingerprints of all predecessor products. Module revision alone, architecture
alone, or structural similarity never establishes freshness.

Preparation owns target-aware classification and planning: ABI eligibility,
call and variadic requirements, address strategies, admitted inline-assembly
vocabulary/context tables, and runtime-helper interfaces. It never chooses a
general register assignment, performs pressure eviction or spill/reload,
creates concrete frame offsets, selects machine opcodes, or copies the
instruction graph. Canonical storage is read-only throughout this phase.

Each fact has one producer and declared consumers:

| Product | Sole producer | Immediate consumer | Later consumers |
|---|---|---|---|
| `VerifiedPreparationInput` | [verifier-only binding gate](../verify/README.md) over the [external C1 validated target](../../../target_profile/README.md) | target layout (`C2`) | all C3-C9 products as the Canonical/target binding prerequisite |
| `VerifiedTargetLayout` | target layout (`C2`) | ABI (`C3`) | all later planners, constraint stage, allocator, boundary verification |
| `AbiPlan` | ABI (`C3`) | calls (`C4`) | variadic, helpers, call lowering, verification |
| `CallPlan` | calls (`C4`) | variadic (`C5`) | helpers, call lowering, allocation |
| `VariadicPlan` | variadic (`C5`) | address (`C6`) | D2 call lowering; later boundaries consume the resulting pseudo/revision lineage, not a late frame-planning authority |
| `AddressPlan` | address (`C6`) | inline-asm tables (`C7`) | D1 pseudo lowering and D4 target legalization; F1 consumes only the resulting directly realizable nodes |
| `InlineAsmTargetTables` | inline-asm preparation (`C7`) | runtime helpers (`C8`) | register constraints (`C9`) |
| `RuntimeHelperPlan` | runtime helpers (`C8`) | bundle publication | pseudo and shared call lowering |
| `VerifiedPreparationBundle` | preparation publication gate after `C8` | register constraints (`C9`) | pseudo lowering, allocation, verification |
| `BoundConstraintSet` | `bind_constraints` (`C9`) | D1's initial `ConstraintProjectionTransaction` | no later direct consumer; its fingerprint remains lineage inside each exact-revision `ProjectedConstraintSet` |

The register-constraint stage is the only interpreter of source constraint
descriptions. Preparation supplies data tables; it does not produce typed
operand/result bindings, ties, early-clobber exclusions, or resolved clobber
units.

## Atomic publication and invalidation

The C3-C8 orchestration transaction accumulates private immutable candidates and
publishes the bundle only after all six planners succeed and all product keys
agree. A missing product, stale Canonical stamp, target/layout mismatch,
predecessor-fingerprint mismatch, incomplete identity reference, planner
diagnostic, or cancellation publishes no bundle. Already published inputs are
unchanged.

Any new Canonical stamp, target fingerprint, target-profile or layout schema,
planner schema, original preparation-facing identity/description, or
predecessor fingerprint invalidates the affected product and every transitive
successor through C9. Products from different transactions cannot be spliced
together.

`VerifiedPreparationBundle` is not `PreparedBir` and contains no instruction
graph. E4 mints `PreparedBir` only as a readiness capability bound to the exact
immutable revision owned by `AllocatedBir`; `MirReadyBirView` is a read-only
borrow of that same revision. Neither contains or copies graph storage, and a
bundle fingerprint is accepted there only when every predecessor and the
target/layout key match the frozen allocated candidate.

Legacy coverage includes `prepared_fact_boundary.hpp`,
`prepared_object_traversal.*`, `prepared_lookups.*`, and `lookup_agreement.*`.
Their useful behavior becomes typed immutable plan APIs; traversal coordinates
and agreement side tables do not survive as authorities.
