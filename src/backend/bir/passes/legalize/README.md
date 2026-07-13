# P01 Legalize Pass Contract

Contract-Status: under-review
Implementation-Status: absent
Kind: pass
Phase-ID: B1 / P01
Upstream: one accepted immutable verified target-independent unallocated A2 `RawBir`
Downstream: one immutable `TypesLegal` revision consumed by B2 / P02
Owner-Path: `src/backend/bir/passes/legalize/README.md`
Last-Reconciled-Commit: none

## Purpose

P01 is the first target-independent canonical pass. It consumes exactly the
move-only, verified and unallocated `RawBir` accepted by closed Child A and
converts only P01-owned portable type, constant, opcode, predicate, cast and
truth-boundary forms into their closed legal semantic representation.

P01 is not recovery for a malformed draft and is not machine legalization.
Inherited A2 failure is reported without repair. A successful occurrence
publishes one exact immutable B1 checkpoint satisfying `RawVerified` plus
`TypesLegal`; B2 alone owns the next scalar/comparison/select normalization.

## Owns

- the closed `PassId::Legalize` / B1 occurrence and its P01 form registry;
- lossless exact-bit constant-payload normalization;
- closed portable type/opcode/predicate/cast alias normalization;
- explicit target-independent `i1` truth-boundary formation;
- width-explicit arbitrary-width integer representation and closed semantic
  classification of extended-float, complex and checked-result operations;
- deterministic full-module inventory, mutation proposal and P01 cumulative
  postcondition checking;
- the `TypesLegal` property request supplied to the pass framework only after
  the exact occurrence transaction and postcondition succeed.

Every P01-owned form has exactly one row in the exhaustive disposition matrix:
`Normalize`, `Preserve`, or `Reject`. There is no implicit fallthrough.

## Does Not Own

- A1 import, Raw storage, A2 verification/publication, malformed-input repair,
  missing facts, unresolved reservations/fixups or compatibility recovery;
- P02 folding, algebraic/cast/comparison/select canonicalization; P03 CFG; P04
  SSA/phi; P05 memory/address/atomic; P06 aggregate; or P07 intrinsic work;
- target/profile selection, target layout, ABI classification, preparation,
  constraint interpretation/binding, pseudo lowering, allocation, frame state,
  MIR, encoding, emission or assembler parsing;
- target-dependent width legality, helper choice, instruction sequences,
  address modes, calling locations, register classes/homes or stack placement;
- analysis algorithms, cache authority, pass occurrence publication, revision
  allocation, verifier rules or B8 `CanonicalBir` publication.

P01 never parses names, rendered types/IR, compatibility strings, inline-asm
text, pointer values, vector positions, cache indices or legacy routes to
create semantic facts.

## Inputs

The only input is one move-only immutable, verified, target-independent and
unallocated published `RawBir` carrying the exact
`{ModuleEpoch, ModuleRevision}`, ordered `(FunctionId, FunctionRevision)`
digest, `PassProperty::RawVerified` and a complete successful Raw verifier
stamp for that same revision. It is target-independent and unallocated.

P01 may trust typed stable IDs, deterministic orders, exact def-use,
terminator-owned CFG successors, closed descriptors, complete semantic calls,
memory, aggregates, intrinsics and ordinary inline-asm value edges. It may
preserve source-semantic typed sizes, alignments and address spaces already
owned by accepted Raw rows. Those are not target layout.

### Exact input acceptance matrix

| Input axis | Required exact state | Accepted optional/error form | Rejection and stable failure |
|---|---|---|---|
| owning stage capability | one move-only published `RawBir`; never draft/candidate storage | none | missing/wrong capability is `WrongInputCapability` |
| module identity | exact nonzero `ModuleEpoch` and current `ModuleRevision` | a proven no-op may later retain the same module revision | stale/foreign identity is `StaleInput` |
| function identity | canonical ordered digest of every `(FunctionId, FunctionRevision)` | declarations have no body edit but remain inventoried | missing/mixed/stale function revision is `StaleInput` |
| verification/property | complete Raw report and `RawVerified` for the same revision | Raw-admitted critical edges, unreachable blocks, memory form and noncanonical portable P01 forms | missing/mismatched proof or property is `WrongInputCapability` |
| semantic authority | typed IDs, types, constants, descriptors, orders, def-use and terminators | opaque inline-asm payload and non-authoritative origin attachments | unresolved/malformed/text-derived fact is inherited A2 `RawContractFailure`; P01 performs no repair |
| target/allocation exclusion | no target context, ABI/preparation, pseudo, home, spill, frame, MIR or emission fact | source-semantic typed size/alignment/address-space facts with an accepted Raw owner | any forbidden stage fact is `ForbiddenAuthority` |
| analyses | no analysis handle is required by P01 | revision-matching cached facts may exist but grant no rewrite authority | an attached/stale result cannot satisfy input and is ignored or rejected as `StaleAnalysis` by its owner |

No importer map, fixup table, compatibility side record, unsupported valid
row, partial function/global set, diagnostic-only report, target context or
allocation fact is an input. Any such state proves A2 did not publish the
required object.

## Outputs

A successful P01 occurrence publishes one immutable B1 checkpoint with the
same module epoch, a framework-derived exact module/function revision set, a
complete `MutationSummary`, accumulated `RawVerified` and newly established
`TypesLegal`, and the P01 occurrence recorded as ordinal B1 in the pipeline
stage stamp. Mutation increments affected revisions exactly once; a proven
no-op retains semantic revisions while still recording the successful B1
occurrence/property checkpoint.

The output contains only target-independent typed semantics. Source-semantic
sizes, alignments and address spaces retain their exact accepted values;
`target_profile`, rendered `data_layout`, target triple, pointer-width/address-
space layout selection and every other C1/C2 fact remain absent.

### Exact output handoff matrix

| Output/product | Exact consumer | Required binding and acceptance | Failure / forbidden escape |
|---|---|---|---|
| immutable B1 BIR revision | B2 / P02 scalar | exact epoch/module/function revisions and B1 occurrence stamp; no P01-owned Raw form remains | no draft, mutable editor, earlier checkpoint or mixed revision may be substituted |
| accumulated properties | B2 precondition and later cumulative B8 check | `RawVerified` retained; framework establishes `TypesLegal` only after P01 postcondition/verifier-on-commit | P01 cannot self-assert a property or trade a diagnostic report for it |
| stable semantic graph | B2 and exact-revision analyses | stable IDs/orders, def-use, terminators, calls/memory/aggregate/intrinsic owners and opaque asm payload remain coherent | no hidden side table, compatibility identity, partial rewrite or stale analysis crosses |
| `MutationSummary` | analysis manager and pass framework | derived from the committed transaction and exact changed owners/effects | declaration-only preservation claims cannot override observed mutation |
| structured diagnostics on failure | pipeline observation only | stable rule/entity/source key, deterministic ordering, no success capability | diagnostics cannot select a fallback result or authorize B2 |

The exact B2 consumer clause requires this same revision, `TypesLegal`, the
P01 postcondition and configured input verifier. B2 rejects Raw scalar aliases,
unowned special operations and stale/mismatched capability stamps.

## Adjacent-Stage Contract

Closed Child A owns the complete immutable Raw input. Core owns storage, IDs,
orders and def-use; the shared Raw verifier owns A2 validity. The pass framework
owns P01's transaction, revisions, occurrence publication, property transition
and analysis invalidation. The pipeline owns fixed `B1 -> B2` order.

P01 alone owns the forms marked `Normalize` below. A `Preserve` row remains
lossless under its named later owner; P01 may validate it but cannot normalize
it. A `Reject` row fails before mutation and cannot be converted to an opaque
placeholder. B2 accepts only the exact successful P01 output described above.

## Ordered Behavior

1. Validate the owning `RawBir` capability, exact revisions, Raw stamp and
   absence of forbidden target/allocation state.
2. Inventory every module/function/block/instruction/type/constant form in
   deterministic semantic order without mutation, including declarations and
   unreachable definitions.
3. Assign every applicable item exactly one matrix disposition. Accumulate all
   stable pre-mutation failures deterministically; any reject/ownerless form
   aborts the complete occurrence.
4. Fork one private module occurrence transaction. Intern exact constants in
   semantic key order, then apply descriptor-compatible edits followed by
   deterministic insert/typed-RAUW/erase operations.
5. Repair exact def-use and composed origin attachments inside the transaction;
   derive the authoritative `MutationSummary` from actual edits.
6. Run the full P01 cumulative postcondition and configured verifier-on-commit
   against that same private revision.
7. On success, the framework atomically publishes one B1 checkpoint and
   establishes `TypesLegal`; on any failure it rolls back and discards the
   entire candidate.

## Invariants

- Integer widths and floating formats/payloads remain exact; no host floating
  conversion, convenient carrier rounding or target-width policy is allowed.
- `Undef`, `Poison`, NaN payloads/classes, signed zero, relocation fragments,
  pointer identity, exceptional behavior and result roles remain distinct.
- Stable IDs are retained for descriptor-compatible edits. Shape-changing
  replacements use deterministic new IDs, typed atomic RAUW, complete def-use
  repair and composed origins; names/positions never supply identity.
- Storage types, source-semantic sizes/alignments/address spaces, function/call
  signatures, aggregate topology, CFG edges/phi keys, symbol identity, memory/
  atomic effects and opaque inline-asm bytes remain lossless.
- P01 introduces no P02-P07 noncanonical work except the exact typed portable
  handoff named by the owning later row. It cannot invoke another pass.
- Success is idempotent under the same semantic options; a second P01 run finds
  no P01-owned Raw form and proposes no mutation.

## Exhaustive P01 Raw-Form Disposition Matrix

`Normalize` is P01 mutation authority. `Preserve` means the form is already
valid Raw semantics owned by the named later pass/stage and must remain exact.
`Reject` is a stable whole-occurrence failure before mutation. The matrix is
closed: an unlisted applicable Raw-only form is the final reject row.

| Raw form/family | Exact facts that must remain lossless | Disposition | Exact P01 result and stable failure | Next owner / B2 visibility | Implementation truth |
|---|---|---|---|---|---|
| integer literal with noncanonical storage padding | semantic integer type, width and exact bits | Normalize | canonical exact-bit payload in the same typed constant; mismatch is `SemanticDrift` | B2 sees canonical literal | absent |
| floating literal with noncanonical padding/encoding | exact format, semantic bits, NaN payload/class and signed zero | Normalize | canonical exact-format bit payload, never host-float round-trip; mismatch is `SemanticDrift` | B2 sees canonical literal | absent |
| portable type alias or spelling-only signedness form | exact semantic kind, width, shape and source-owned attributes | Normalize | one closed signedness-free semantic type identity; mismatch is `SemanticDrift` | B2 consumes legal type | absent |
| admitted scalar opcode alias | operands/results, width, flags and exceptional semantics actually present | Normalize | closed portable semantic opcode; mismatch is `SemanticDrift` | B2 owns expression form | absent |
| admitted comparison predicate alias | operand domain, ordered/unordered and signedness semantics | Normalize | closed portable predicate identity without reorientation; mismatch is `SemanticDrift` | B2 owns comparison orientation/folding | absent |
| admitted cast alias | source/destination types, value identity and conversion semantics | Normalize | closed portable cast identity without chain folding; mismatch is `SemanticDrift` | B2 owns redundant-cast/chain work | absent |
| non-`i1` scalar truth boundary | source value/type and poison/undef behavior | Normalize | explicit target-independent compare-to-zero/null yielding `i1`; mismatch is `SemanticDrift` | B2 consumes canonical condition | absent |
| arbitrary-width integer semantic operation | exact width, signedness-sensitive semantics, operands/results and flags | Normalize | width-explicit `WideIntegerOp`; no lossless form is `UnsupportedPortableSemantics`, other mismatch is `SemanticDrift` | B2 or registered later semantic owner | absent |
| extended-floating semantic operation | exact format, rounding/exception contract, operands/results | Normalize | exact-format `ExtendedFloatOp`; no lossless form is `UnsupportedPortableSemantics`, other mismatch is `SemanticDrift` | B2 or registered later semantic owner | absent |
| complex semantic operation | component type, one semantic result and exact operands | Normalize | typed `ComplexOp` with no ABI lane split; mismatch is `SemanticDrift` | B2/P06 according to registry | absent |
| checked/overflow arithmetic | operation, value result, overflow result and exact flags | Normalize | `CheckedOverflowOp` with both explicit results; mismatch is `SemanticDrift` | B2 | absent |
| switch/indirect successor and other valid CFG form | terminator-owned ordered successor slots and stable block IDs | Preserve | byte/identity-equivalent typed CFG payload; change is `PreservedFormChanged` | P03 | absent |
| phi/forward-reference form already resolved by A2 | exact value IDs, `EdgeKey`s, definitions and uses | Preserve | unchanged stable IDs/edges; change is `PreservedFormChanged` | P04 | absent |
| address, memory, stack and atomic semantic form | typed addresses, access size/alignment/address space, volatility, ordering, scope and effects | Preserve | unchanged complete target-independent payload; change/address selection is `PreservedFormChanged` | P05 | absent |
| aggregate/by-value semantic form | exact type/topology/path and source-semantic attributes | Preserve | unchanged typed topology; target decomposition/ABI placement is `ForbiddenAuthority` | P06 | absent |
| registered intrinsic semantic form | registry ID, types, immediates, effects and ordinary edges | Preserve | unchanged typed payload; change is `PreservedFormChanged` | P07 | absent |
| `InlineAsm` ordinary semantic node | ordinary inputs/results, exact opaque template/constraint bytes, clobbers and effects | Preserve | byte-for-byte unchanged payload/edges; change is `OpaquePayloadChanged` | P07 validates final semantics; C9 alone later binds constraints | absent |
| source-semantic typed object/type attributes | accepted typed size, alignment and address-space facts with their Raw owner | Preserve | identical source-semantic attributes; drift is `SemanticDrift`, target derivation is `ForbiddenAuthority` | B2 and later canonical owners | absent |
| valid form with no lossless portable representation | complete typed source form and stable entity anchor | Reject | `UnsupportedPortableSemantics`; publish nothing | none | absent |
| valid form with no declared P01/later owner | complete typed source form and stable entity anchor | Reject | `MissingDownstreamDisposition`; publish nothing | none | absent |
| unknown/unlisted applicable Raw-only form | descriptor ID, type and stable entity anchor | Reject | `UnknownLegalizeForm`; publish nothing; no catch-all preservation | none | absent |

## Verification and Publication

The P01 postcondition runs on the exact private occurrence revision and proves:

- the complete Raw registry still holds and no P01-owned Raw form remains;
- every normalized constant/type/opcode/predicate/cast/truth/special form
  matches its unique matrix result without semantic drift;
- every preserved row is byte/identity/edge equivalent except for typed generic
  reference repair caused by an owning P01 rewrite;
- stable IDs, deterministic orders, exact def-use and terminator-only CFG
  authority remain coherent;
- source-semantic typed attributes retain exact values and no target/profile/
  ABI/preparation/allocation fact was introduced;
- every remaining special form names its exact later owner.

Only after this postcondition and verifier-on-commit are green may the pass
framework publish the occurrence and establish `TypesLegal`. P01 cannot mint
`RawBir`, `CanonicalBir`, a stage token or its own property stamp. B8 later
reruns the cumulative P01 obligation on the exact frozen P07 revision.

## Failure and Diagnostics

Stable P01 failures include `WrongInputCapability`, `StaleInput`, inherited A2
`RawContractFailure`, `ForbiddenAuthority`, `UnknownLegalizeForm`,
`UnsupportedPortableSemantics`, `MissingDownstreamDisposition`,
`SemanticDrift`, `PreservedFormChanged`, `OpaquePayloadChanged`, deterministic resource exhaustion,
cancellation, verifier rejection and transaction/commit failure.

Diagnostics carry stable rule ID, exact revision and typed entity/source anchor
and are sorted in semantic order. Text is presentation only. Failure publishes
no revision, property, partial rewrite, analysis result/cache entry, reusable
green report, stage token or alternate checkpoint. The private candidate and
all proposed IDs/edits are destroyed; the last published Raw checkpoint is
unchanged and is not relabeled as B1 success.

## Analysis and Invalidation

P01 requires no analysis result. Any existing analysis is a read-only exact-
revision observation and cannot grant mutation or preservation authority.

On successful mutation, the framework derives one `MutationSummary` and
invalidates every cached result whose declared axes observe changed constants,
types, descriptors, operands/results, definitions/uses, origins, function body
or module tables, including transitive dependents. A result survives only when
its descriptor and semantic-equality audit prove preservation for the exact
output key. A no-op occurrence may retain results only under the same checked
rule. Failure publishes no new/updated analysis entry and leaves the prior
checkpoint cache unchanged. B2 and its comparison analysis must request facts
whose complete key matches the exact P01 output revision.

## Target and ABI Rules

P01 has no `TargetProfile`, target-layout table, rendered `data_layout`, target
triple, pointer-width/address-space layout-selection policy, ABI/preparation
product, target feature switch, register file, allocator, frame or MIR input.
C1 independently selects the exact `TargetProfile`; C2 derives target layout
after immutable Canonical publication.

P01 preserves accepted source-semantic typed sizes, alignments and address
spaces without interpreting them as target layout. It cannot decide whether a
width is legal on a machine, select a helper/opcode/address mode, classify a
call, realize an inline-asm constraint or split a value into physical lanes.

## Implementation State

Implementation is absent. This directory contains only this README. No P01
translation unit, pass registration, `PassId::Legalize` implementation, build
edge, focused test or complete verifier/postcondition implementation is
checked in. Pass-framework scaffolding elsewhere does not implement this
semantic owner. All matrices above are the accepted design target, not runtime
coverage.

## Proof Requirements

- require the exact metadata spine and core-first/detail heading order;
- require one nonempty disposition/result/failure/next-owner/implementation
  entry for every closed Raw form row and exactly the three disposition values;
- prove the sole input is the exact accepted Raw revision and the sole output
  is the exact `TypesLegal` revision accepted by B2;
- prove one atomic transaction, deterministic inventory, exact revision/stamp,
  complete rollback and framework-owned property publication;
- prove mutation-derived invalidation and stale/mixed analysis rejection;
- prove source-semantic attributes remain exact while target/profile/layout/
  ABI/preparation/allocation facts remain absent;
- verify current implementation/build truth and resolve every relative link;
- reject names/text/legacy recovery, catch-all preservation, unsupported
  downgrades, expectation weakening and testcase-shaped proof.

## Open Questions

No open P01 question authorizes target policy, producer repair, an adjacent-
owner edit or implementation. A newly discovered Raw form must receive an
explicit lossless P01/later-owner disposition or the stable closed-table
failure before this contract changes.

## Review Checklist

- [x] Metadata spine and core-first/detail order are exact.
- [x] Input is one immutable exact-revision verified target-independent unallocated `RawBir`.
- [x] Every P01 form has exactly one Normalize/Preserve/Reject disposition and stable result/failure.
- [x] Source-semantic sizes/alignments/address spaces remain distinct from C1/C2 target layout.
- [x] Transaction, revision, rollback, cumulative postcondition and property publication are explicit.
- [x] Analysis invalidation is mutation-derived and stale/mixed results cannot cross revisions.
- [x] Failure publishes no revision/property/cache/partial result/capability.
- [x] B2 accepts only the exact immutable `TypesLegal` P01 output.
- [x] Implementation is truthfully absent and design prose is not runtime coverage.
