# Immutable Preparation Planners

Status: converged design contract (unimplemented).

## Boundary and exact order

Preparation begins only after the prepared-input gate returns a borrowing
`VerifiedPreparationInput` over one immutable `CanonicalBir`. That capability
carries the complete Canonical `PipelineStageStamp`—module epoch, module
revision, and ordered function-revision digest—and one exact
`TargetFingerprint`. A verified target layout with the same target fingerprint
is also required.

The dependency order is fixed and serial:

```text
VerifiedPreparationInput + VerifiedTargetLayout
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
| `VerifiedTargetLayout` | target layout (`C2`) | ABI (`C3`) | all later planners, constraint stage, allocator, boundary verification |
| `AbiPlan` | ABI (`C3`) | calls (`C4`) | variadic, helpers, call lowering, verification |
| `CallPlan` | calls (`C4`) | variadic (`C5`) | helpers, call lowering, allocation |
| `VariadicPlan` | variadic (`C5`) | address (`C6`) | call lowering and frame planning |
| `AddressPlan` | address (`C6`) | inline-asm tables (`C7`) | pseudo lowering and MIR selection |
| `InlineAsmTargetTables` | inline-asm preparation (`C7`) | runtime helpers (`C8`) | register constraints (`C9`) |
| `RuntimeHelperPlan` | runtime helpers (`C8`) | bundle publication | pseudo and shared call lowering |
| `VerifiedPreparationBundle` | preparation publication gate | register constraints (`C9`) | pseudo lowering, allocation, verification |

The register-constraint stage is the only interpreter of source constraint
descriptions. Preparation supplies data tables; it does not produce typed
operand/result bindings, ties, early-clobber exclusions, or resolved clobber
units.

## Atomic publication and invalidation

The orchestration transaction accumulates private immutable candidates and
publishes the bundle only after all six planners succeed and all product keys
agree. A missing product, stale Canonical stamp, target/layout mismatch,
predecessor-fingerprint mismatch, incomplete identity reference, planner
diagnostic, or cancellation publishes no bundle. Already published inputs are
unchanged.

Any new Canonical stamp, target fingerprint, layout schema, planner schema, or
predecessor fingerprint invalidates the affected product and every transitive
successor. Products from different transactions cannot be spliced together.

`VerifiedPreparationBundle` is not `PreparedBir` and contains no instruction
graph. `PreparedBir` remains reserved for the later verified allocated
revision; `MirReadyBirView` borrows that same later revision.

Legacy coverage includes `prepared_fact_boundary.hpp`,
`prepared_object_traversal.*`, `prepared_lookups.*`, and `lookup_agreement.*`.
Their useful behavior becomes typed immutable plan APIs; traversal coordinates
and agreement side tables do not survive as authorities.
