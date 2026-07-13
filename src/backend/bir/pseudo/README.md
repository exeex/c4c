# Pseudo Instruction Schema

Status: converged design contract (unimplemented).

## Purpose and closure

This directory owns the closed, target-aware but machine-independent
instruction schema used from `D1` through allocated publication. Pseudo BIR
continues to use the core BIR module, function, block, instruction, value,
terminator, def-use, and ordinary SSA ownership graph; it is a new immutable
revision, not a parallel IR.

An instruction is admitted only when its bounded variant and descriptor appear
in the table below. Every descriptor fixes operand/result roles and types,
effects, trapping behavior, abstract requirement sites, permitted stage
interval, and direct-realizability obligation. There is no open-ended opcode,
string-named operation, semantic-leftover carrier, compatibility payload, or
extension namespace.

## Closed admitted instruction table

| Variant family | Exact admitted purpose | First owner | Stage legality |
|---|---|---|---|
| `Copy`, `Select`, `IntegerAlu`, `FloatAlu`, `Compare`, `Convert` | typed scalar/value movement and arithmetic descriptors | `D1` | `D1` onward; must be directly realizable after `D4` |
| `Address`, `Load`, `Store`, `Atomic`, `Fence` | typed target-aware address and memory operations with explicit effects | `D1` | `D1` onward; must be directly realizable after `D4` |
| `AggregatePiece`, `Vector`, `Intrinsic` | bounded piece/lane/portable-feature operations retained for reviewed legalization | `D1` | `D1` through `D4`; each instance is eliminated or directly realizable after `D4` |
| `GenericCall` | ordinary or runtime-helper call awaiting the shared ABI transport rewrite | `D1` | private D1 candidate only; forbidden at `D3` publication |
| `AbiArgMove`, `AbiArgStore`, `AbiCall`, `AbiResultMove`, `AbiPreserve`, `AbiRestore` | explicit shared call transport with abstract ABI-slot and stack-object requirements | [D2 shared call lowering](../passes/call_lowering/README.md) | `D2` onward; must be directly realizable after `D4` |
| `InlineAsm` | one opaque template plus ordinary ordered uses/results and the exact current `ProjectedConstraintSet` record | `D1` preserves/binds | `D1` onward; one BIR node maps to one opaque MIR record |
| `ParallelCopy`, `EdgeCopy`, `CopyScratch` | typed edge-local assignments replacing phi/block-argument transport; `ParallelCopy` reads all sources before simultaneously writing its unique destinations, `EdgeCopy` is one directly realizable move, and `CopyScratch` is an explicit allocation-only reservation identity | `D5` | `ParallelCopy` and `CopyScratch` are intermediate from initial D5 publication through the post-E3 D5 resolution closure and forbidden at E4; only resolved `EdgeCopy` survives to E4/MIR, checked against exact originating `EdgeKey` provenance |
| `Spill`, `Reload` | explicit capacity-repair transitions using abstract spill-object identity | `E3` | allocation retry candidate onward; forbidden in D-stage publication |

Core terminators remain the sole CFG-successor authority. The admitted
terminator set is `Return`, `Jump`, `CondJump`, `Switch`, `IndirectJump`,
`AsmGoto`, and `Unreachable`, with the core ordered `SuccessorSlot`/`EdgeKey`
schema. A pseudo instruction cannot carry a hidden successor.

The family name is not a wildcard. Each family has a versioned finite
descriptor registry; adding an operation, descriptor alternative, stage
interval, or operand role changes the pseudo-schema fingerprint and requires
architecture review. Target support tables may reject a descriptor but cannot
add one.

## InlineAsm remains ordinary BIR

`InlineAsm` retains one instruction ID, ordinary ordered input uses, ordinary
ordered result definitions, declared effects, ordered clobbers, and any paired
`AsmGoto` label slots. Its original template and constraint-description bytes
remain unchanged. Its `ProjectedConstraintSet` record for the exact current revision must cover every
operand/result ordinal exactly once as required, preserve distinct read/write
SSA identities, and carry valid ties, early-clobber exclusions, abstract alias
units, and the exact source-description digest.

No pseudo stage reparses text, guesses placeholder roles, creates an asm-only
value graph, or derives implicit clobbers from the opaque template. A malformed
or stale binding is rejection, never repair.

## Revision and stage keys

Every pseudo candidate and publication carries one `PseudoStageKey`:

```text
exact current PipelineStageStamp
parent Canonical PipelineStageStamp
TargetFingerprint
target-layout schema fingerprint
VerifiedPreparationBundle fingerprint
Canonical BoundConstraintSet fingerprint
exact current ProjectedConstraintKey and projection fingerprint
pseudo-schema fingerprint
ordered D1/D2/D4/initial-D5/E3/D5-resolution transformation fingerprints
applicable to this stage
```

The exact current stamp includes module epoch, module revision, and the ordered
function-revision digest. Products keyed only by module revision, target name,
or semantic equality are stale. Every mutator invokes the subordinate shared
`ConstraintProjectionTransaction` before verification, and only its
`ProjectedConstraintSet` keyed to the new stamp is current. A mutation always creates a new exact stamp;
unchanged entities retain their identities, while new entities receive newly
reserved IDs
and removed ones become tombstones. Stable IDs never allow a product from the
parent revision to be reused without an explicit preservation proof.

## Stage invariants and ownership

- D1 eliminates all Canonical semantic instruction alternatives and admits
  only the table above; D2 eliminates every `GenericCall` before D3.
- D3 permits unassigned allocatable values. Requirements, ties, clobbers, and
  abstract ABI slots constrain later allocation but are not assignments.
- D4 eliminates every remaining semantic one-to-many requirement. After its
  full gate, every non-`InlineAsm` node present at D4 is directly realizable as
  one machine instruction without introducing a new allocatable value, use,
  definition, or CFG edge. D5's explicitly bounded copy intermediates are the
  only later exception and must be resolved in BIR before E4.
- D5 alone adds copy pseudos for out-of-SSA. E3 alone adds `Spill`/`Reload`.
- D5 copy destinations are explicit assignment roles for stable virtual
  allocation identities, not new SSA definitions. A `ParallelCopy` has
  canonical destination order, unique destinations, typed sources, and atomic
  read-before-write semantics. Each entry in a multi-entry potential-alias
  component has a typed, edge-local `CopyScratch` identity created at initial
  D5 publication, made live at E1, and assigned a legal home at E2 that aliases
  neither a transferred home nor another simultaneously needed scratch. This
  bounded worst-case reservation set is explicit pre-E4 BIR allocation state,
  never an implicit MIR temporary.
- An `EdgeKey` on a D5 copy is provenance checked against terminator-derived
  topology and the D5 placement plan. It is not a stored successor. Every copy
  executes in a source-, destination-, or freshly split edge-local region that
  denotes exactly that successor-slot occurrence.
- After the final stable E3 candidate, D5's `CopyResolutionTransaction` uses
  the exact assignments and scratch reservations to replace every
  `ParallelCopy` with an ordered sequence of directly realizable `EdgeCopy`
  nodes. A resolved `EdgeCopy` may use the stable `CopyScratch` identity as an
  assigned endpoint, but the allocation-only `CopyScratch` node itself is
  tombstoned. The transaction creates no identity, allocation, spill/reload,
  or MIR work and fails atomically when a legal sequence cannot be proved.
- That revision advance invalidates predecessor E1, E2, E3 spill-state,
  projected-constraint, and realizability products. In the same transaction,
  after the sole `ConstraintProjectionTransaction`, E1 recomputes resolved
  liveness/interference, E2 validates and installs the unchanged legal
  assignments without reallocating, E3 validates and installs unchanged
  explicit spill state without mutation, and the existing target registry
  recomputes realizability. Each product names the exact resolved
  `PipelineStageStamp` and `CopyResolutionFingerprint`; stable IDs and the
  preservation record never rekey products, and any owner failure rolls back
  the entire candidate before E4.
- The schema never stores machine-register identities, machine instruction
  encodings, stack displacements, late frame layout, or assembler parse trees.

Pseudo lowering owns generic semantic disposition. The dedicated D2 shared
call-lowering contract owns ABI transport. The target chain owns required target legalization and
expansion. Out-of-SSA and spill/reload own only their listed variants. Shared
BIR regalloc remains the sole home-assignment and pressure authority; MIR is a
strict verified mapping consumer.

## Transactional publication

Each mutating owner forks one private candidate, completes all graph and
derived-fact updates, invokes the shared constraint projection authority,
freezes one exact revision, and invokes the verifier
profile required at its boundary. Failure publishes no instruction subset,
function subset, property, stage key, or reusable analysis result. D3 mints the
first `PseudoBir`; D4 and later mutators publish replacement immutable
revisions only after their required full gate. In particular, D5 consumes only
the fully reverified D4 revision and publishes its replacement only after full
Pseudo reverification proves copy coverage and absence of phi semantics; E1
cannot consume an incrementally checked candidate. After stable E3, the
subordinate D5 copy-resolution closure publishes a new private candidate only
after its full pre-E4 gate proves all parallel groups resolved. E4 forbids
`ParallelCopy` and `CopyScratch`; the predecessor capability remains unchanged
and cannot be relabeled as the successor.
