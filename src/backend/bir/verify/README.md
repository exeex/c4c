# BIR verifier design

Contract-Status: Raw through MirReadyMachine verifier architecture converging
under idea 732
Implementation-Status: partial Raw foundation; Canonical, Prepared-admission,
PseudoPreallocation, Allocated, and MirReadyMachine gates are unimplemented
unless a narrower section names checked-in evidence

This directory owns structural and semantic validation of published BIR. The
first implementation target is **RawBir**: the result of LIR lowering and the
input to the ordered canonicalization pipeline. Raw means “not canonicalized”; it
does not mean malformed, partly built, text-identified, or safe only because a
later pass happens to repair it.

The verifier is a trust boundary. A successful result permits passes and
analyses to use typed IDs, complete operand traversal, exact definitions, and a
well-formed CFG without defensive fallback. Raw/Canonical verification does
not certify target ABI, register allocation, frame layout, target opcodes,
encodings, or final emission. The later Allocated gate certifies E4's exact
private frame placements and explicit actions.

The [root authority spine](../README.md#common-contract-authority-spine)
assigns ownership. The
[normative NodeKind/tag contract](../../../../docs/backend/bir_node_kind_tag_algebra_and_phase_vocabulary.md)
owns the six published-stage meanings, kind admission, identity gate, and
common rejection obligations. This file owns concrete graph/product validity
rules and unforgeable publication results; it does not redefine a tag table,
schedule a pass, mutate a candidate, or decide analysis preservation.

Every gate first rejects an unknown kind/tag/stage, illegal admission,
schema/payload/arity mismatch, unhandled transition row, unresolved/stale
identity, or stale/foreign/mixed revision/target/product key. It then applies
the stage-specific structural and semantic rules. A false schema query is not
a diagnostic and cannot distinguish unknown from known-negative input. Failure
publishes no token or partial capability.

## Checked-in Raw foundation and deferred final reconciliation

The build-included `FoundationVerifier` is real shared-code infrastructure. It
checks typed storage/order/ownership, current NodeKind schema, payload/arity,
value-definition, terminator/CFG, module-table, and publication invariants for
the bounded forms accepted by the current builder/importer. It does not parse
opaque inline-assembly text into target semantics or prove C9 constraint and
allocation legality.

The old one-opcode bootstrap description is obsolete. The dated factual
baseline is [idea 732 Step 1](../../../../docs/backend/bir_732_audit/step1_current_owners_and_factual_inputs.md).
Idea 732 Step 8 must enumerate the final landed importer kinds, payloads,
roles, rejection behavior, and exact Raw publication API before this document
claims complete A2 coverage. This Step 2 correction intentionally does not
prejudge parallel importer work.

## Profiles and stage boundary

```text
LIR lowering -> private Raw candidate -> Raw gate -> RawBir
                                      \-> diagnostics-only candidate report
ordered pass transaction --private verify-and-publish(Canonical)--> CanonicalBir
C1 target selection + Canonical --verify_preparation_input--> VerifiedPreparationInput
private complete D2 call-lowering candidate --verify-and-publish(Pseudo)--> PseudoBir
complete D4/D5 transaction --full verify-and-republish(Pseudo)--> PseudoBir
private E3 rewrite --AssignedAllocationCandidateGate--> immutable E1/E2 retry input
stable E3 candidate --D5 copy resolution--> private resolved candidate
resolved candidate --E4 frame-action materialization + final exact-current closure--> final candidate
final candidate --AssignedAllocationCandidateGate + Allocated gate--> AllocatedBir
                                                     \--> E4 MIR-readiness capability
                                                     \--> borrowed MirReadyBirView
```

The arrow from the private Raw candidate to `RawBir` is an unforgeable
publication boundary, not a verifier that receives an already-created
`RawBir`. The final exact candidate type/API name is deferred to Step 8; the
currently landed adapter is `ModuleBuilder::publish() &&`. The gate must freeze
one exact candidate revision, run full Raw verification, and consume it into
`RawBir` only after a successful report.
`verify_candidate` is diagnostic-only and can never manufacture a stage token,
even when its report is green. Canonical pass publication uses the analogous
private transaction capability. Public
`verify(ModuleView)` exists to re-check an already published snapshot, not to
legitimize an unverified one. `RawBir` never means “verification disabled,”
including in unit tests: tests that intentionally build malformed storage may
inspect a candidate/report through a private test fixture but cannot obtain a
`RawBir` stage token.

The [external C1 target-profile boundary](../../../target_profile/README.md)
alone selects, normalizes and validates the explicit target request and
produces the validated `TargetProfile`/`TargetFingerprint`. This verifier does
not select or default target axes; it alone binds that validated fingerprint to
the exact unchanged Canonical stamp and may produce `VerifiedPreparationInput`.

| Profile | Required now | Meaning |
|---|---:|---|
| `Raw` | yes | Structurally complete target-independent BIR. Memory form, legal raw op families, critical edges, unreachable blocks, and absence of phi nodes are allowed. |
| `Canonical` | later | The exact immutable B7 / P07 revision published by B8: Raw rules plus all P01-P07 normal forms, including B3 / P03 reachability. It is target-independent and is not a request to validate an arbitrary post-Raw snapshot. |
| `PreparedInput` | later | A non-mutating C1 input-gate rule set over one already-published `CanonicalBir` plus one validated `TargetProfile`. It binds their exact stamp/fingerprint and proves complete typed semantic inputs for C2-C9; it is not a BIR publication profile and contains no prepared facts. |
| `Pseudo` | later | The public `PseudoPublicationGate`: Raw/graph safety plus the closed pseudo schema, exact target/product binding, complete D1 lowering and [D2 call lowering](../passes/call_lowering/README.md), and stage-specific realizability rules. It publishes D3/D4/initial-D5 `PseudoBir` and rejects assignments and spill state. |
| `Allocated` | later | The private `AssignedAllocationCandidateGate` retains every applicable graph/Pseudo rule while admitting exact assignments/spill state and the post-materialization bounded frame-action family; E4 requires final projection/E1/E2/E3/frame/target products before atomic MIR-ready publication. |

The semantic profiles are cumulative. `PreparedInput` cannot weaken
`Canonical`, `Canonical` cannot weaken `Raw`, `Pseudo` retains every applicable
graph/identity obligation while replacing Canonical instruction alternatives
with its closed schema, and `Allocated` retains the complete applicable Pseudo
contract while adding final home/spill obligations. A builder-only object may
temporarily violate the rules, but it is not a published stage and must not be
passed to an analysis or downstream consumer.

There is deliberately no verifier profile named simply `Prepared`.
`PreparedInput` verifies the immutable semantic input to target preparation;
prepared plans are different typed products with their own verifiers. Adding
ABI locations, allocation assignments, spill/reload state, frame facts, target
operations, helpers, or encodings to BIR and then calling that object “Prepared
BIR” would blur authority. Such facts are forbidden in Raw, Canonical,
PreparedInput, and `PseudoPublicationGate`. Private E3
retry, D5-resolved, and E4-materialized candidates instead pass
`AssignedAllocationCandidateGate`, which admits and requires their exact
abstract assignments and spill schema; only the E4 interval also admits
explicit frame-action nodes. The gate cannot publish a second `PseudoBir` type.
Candidates remain private until the Allocated boundary succeeds.

The exact additional `PreparedInput` promise is closed: the B8 stage capability
and selected target fingerprint are current and mutually bound, and every
preparation-facing function/call signature and convention, variadic boundary
and promotion, address/object/relocation descriptor with required provenance,
original inline-assembly description with complete ordinary value identities,
and helper-eligible semantic operation descriptor is present and typed. The
gate reruns applicable Canonical and forbidden-fact checks, but it neither
decides target eligibility nor derives layout, ABI, call, variadic, address,
inline-assembly-table, helper, or constraint facts. Those decisions belong to
C2-C9 and may reject the input.

Raw allowances are explicit:

- blocks may be unreachable; Raw verification diagnoses them deterministically
  but never rejects them merely for being unreachable or allows an option to
  strengthen Raw into the Canonical reachability profile;
- the CFG may have critical edges, duplicate semantic destinations from distinct
  switch cases, loops, irreducible regions, and blocks in non-RPO storage order;
- memory-resident variables and repeated loads/stores are valid;
- phi nodes are optional before SSA construction; if a raw phi is present, it is
  already a real edge-defined value and must obey the structural phi rules below;
- target-independent raw opcodes that a later ordered pass legalizes are valid
  only when enumerated in the Raw opcode schema. “Unknown” is never a profile.

An operation that is semantically known but not yet lowerable by the current
backend is also not “unknown.” It may be published only if the Raw schema has a
closed, typed alternative and an explicit later legalizer/fallback owner. A
source construct that cannot be represented losslessly is rejected by the LIR
importer before publication; an `Unsupported` instruction, string payload, or
opaque side record is not valid Raw BIR.

### Canonical profile and B8 publication

The `Canonical` profile is the exact cumulative profile of the immutable B7 /
P07 output, not a general request to accept any well-formed post-Raw module. B8
accepts one private frozen candidate only when its `PipelineStageStamp` proves
the canonical-v1 occurrence lineage, exact plan/options fingerprints, P07 as
completed ordinal 7, `PassProperty::IntrinsicsCanonical`, exact
`ModuleEpoch`/`ModuleRevision`, and the ordered digest of every
`(FunctionId, FunctionRevision)`. The candidate presented to verification must
be the same owning revision frozen by P07. A reconstructed view, a stamp copied
from another candidate, a stale analysis result, or a module that merely has an
equal semantic hash is not eligible for publication.

The full Canonical check is cumulative. It runs the complete Raw registry plus
the following closed post-pass obligations on that one frozen revision:

Before the per-pass checks, the admitted node vocabulary must be exactly the
five normative groups `B.CanonicalSsaValue`, `B.CanonicalEffect`,
`B.CanonicalControl`, `B.CanonicalPhiMerge`, and
`B.CanonicalOpaqueTargetToken`. This verifier derives membership from the one
NodeKind schema authority; it owns no second table. Every node must match one
group and the complete P07-to-B8 handoff must account for every node.

1. P01 legal semantic types, constants, casts, predicates, truth uses, and
   portable opcode families hold; no `legalize`-owned Raw form remains.
2. P02 scalar expressions, comparisons, selects, exceptional-value behavior,
   and helper-eligible semantic operations have their unique portable forms.
3. P03 terminators and ordered successor slots are the sole CFG edge authority;
   `EdgeKey` occurrence identity and canonical block/topology policy hold, and
   every retained block is reachable from the function entry after P03's
   deterministic unreachable-region removal.
4. P04 explicit-`Phi` canonical SSA form and exact phi incoming `EdgeKey`
   coverage, complete def-use, dominance, and alias normalization hold.
5. P05 memory, address/GEP, access, atomic, stack-state, and memory-intrinsic
   descriptors have their unique semantic forms and preserve P03/P04.
6. P06 aggregate values, copies, paths, complex/multivalue operations, and
   by-value boundaries have unique layout-independent forms and preserve every
   earlier profile.
7. P07 intrinsic registry identity, signatures, immediates, portable feature
   requirements, and effects are closed, and every admitted operation is
   represented or has already failed the P07 transaction.

`InlineAsm` remains one ordinary instruction in the generic value graph for
Canonical verification. Its ordinary inputs/results, original opaque template
and constraint payload, ordered clobber spellings, and declared semantic
effects must satisfy the closed descriptor. Canonical verification does not
parse or rewrite either text, infer roles or ties, validate machine vocabulary,
or create a parallel assembly graph. The same opaque semantic node that entered
the pipeline must survive P01-P07 except for typed generic-edge repair required
by an owning earlier rewrite.

Canonical facts remain target-independent. B8 rejects every unknown, illegal,
omitted, unhandled, import-only, target-selected, preparation-owned, pseudo,
allocation/spill, frame, machine-instruction, encoding, or MIR kind/tag/fact,
as well as a fresh P01-P06 noncanonical form emitted by a later pass. It also
rejects a missing/stale/foreign analysis or product key and any matrix-coverage
hole. A portable feature requirement is semantic metadata, not a support
decision. Target support, helper selection, and constraint binding occur only
after immutable `CanonicalBir` publication.

B8 is one fail-closed transaction:

1. it consumes the private owning B7 / P07 candidate capability and freezes its
   exact stage stamp; no edit or analysis publication may race the gate;
2. it runs all cumulative framework postcondition checkers and the complete
   `Canonical` verifier registry against that same frozen candidate;
3. any diagnostic, revision/stamp change, missing property, cancellation, or
   deterministic resource failure discards the candidate and returns one
   structured publication failure. It publishes no function subset,
   `CanonicalBir`, property, cache entry, or reusable capability;
4. only a completely green result atomically consumes the candidate and the
   verifier-private token to mint exactly one immutable `CanonicalBir` carrying
   the verified stamp.

Diagnostic-only `verify_candidate(Canonical)` and re-checking
`verify(ModuleView, Canonical)` never mint or repair a stage capability. No
earlier green report can be cached across a P07 edit, and failure cannot roll
back to an earlier checkpoint and call that snapshot Canonical. Pipeline
rollback may retain its explicitly permitted last-good checkpoint, but only a
new exact P07 occurrence followed by a successful B8 transaction can publish
`CanonicalBir`.

### Pseudo publication and private assigned-candidate intervals

The public `PseudoPublicationGate` accepts only one private frozen candidate carrying a
complete `PseudoStageKey`. That key must name the exact current
`PipelineStageStamp`, parent Canonical stamp, `TargetFingerprint`, layout and
pseudo-schema fingerprints, `VerifiedPreparationBundle` fingerprint,
Canonical `BoundConstraintSet` fingerprint, exact current
`ProjectedConstraintKey`, and the ordered fingerprints of every D1, D2, and
applicable D4/D5 occurrence. Equal module revisions, equal semantic
hashes, compatible targets, or copied reports do not establish freshness.

The cumulative publication check rejects:

1. every Canonical semantic leftover, `GenericCall` at D3 or later, unknown or
   stage-ineligible pseudo alternative, target opcode, concrete register,
   frame offset, encoded instruction, allocation assignment, spill state, and hidden
   scratch/spill convention;
2. malformed operands, results, types, effects, terminators, CFG, def-use, SSA,
   ownership, or descriptor/version combinations;
3. malformed `InlineAsm`: changed opaque bytes, missing/duplicate/foreign
   ordinary edges, incomplete ordinal coverage, invalid read/write identity,
   illegal ties or early-clobbers, unresolved clobbers, mismatched `AsmGoto`
   slots/effects, or a binding digest/key that is stale for the instruction;
4. a missing, stale, mixed-transaction, or partially rebuilt layout,
   preparation, `ProjectedConstraintSet`, call-lowering, analysis, or
   realizability product; and
5. a partial function publication, changed revision during verification,
   active editor, incomplete pass lineage, unpublished predecessor, or attempt
   to reuse a diagnostic-only report as publication authority.

D3 additionally requires the D2 transaction's `ProjectedConstraintSet` keyed
to this exact candidate; Canonical C9 and the D1 projection are lineage only.
It proves that D1 eliminated every Canonical instruction family
and D2 eliminated every `GenericCall` while preserving the exact plan-derived
ABI requirements. Unassigned allocatable values are valid; no allocation
completeness rule runs. D3 atomically mints the first immutable `PseudoBir` only
after the full module and every function pass.

The same allocation-free gate republishes D4 and initial-D5 `PseudoBir`.
Initial D5 may contain only its bounded `ParallelCopy`/`CopyScratch`
intermediates; it still rejects assignments, spill objects, and `Spill` or
`Reload`. This is the complete published Pseudo interval.

D4 is an always-on target-realizability transaction even when its required
rewrite chain is empty. Each mutation invalidates all affected revision-bound
facts. Each occurrence invokes the sole shared `ConstraintProjectionTransaction`
before its output can be checked, and the empty chain must still prove an
exact current-revision projection. After all required legalization and enabled
reviewed optional entries,
the runner recomputes invalidated facts and reruns the entire `Pseudo` profile,
not merely changed-function checks. The post-D4 gate also proves every
non-`InlineAsm` instruction directly maps to exactly one machine instruction
without adding a use, definition, temporary, CFG edge, or allocation action.
`InlineAsm` instead retains the specified one-node-to-one-opaque-record rule.

D5 accepts only that exact fully reverified D4 publication. Before D5, the
profile rejects `ParallelCopy`, `EdgeCopy`, and `CopyScratch`; after initial D5
publication, it requires the D5 fingerprint, rejects every phi instruction,
incoming map, and
SSA-only edge use, and checks each copy against the exact terminator-derived
`EdgeKey` occurrence and its edge-local placement. Copy destinations are the
stable former join-result allocation identities and are valid only in admitted
copy assignment roles. `EdgeCopy` must be a typed singleton. `ParallelCopy`
must have canonical entry order, unique typed destinations, and simultaneous
read-before-write semantics. Every entry in a multi-entry potential-alias
component must have exactly one typed edge-local `CopyScratch` identity with
finite legal-home requirements. The reservation set is allocation-only,
non-spillable, mutually non-aliasing when simultaneously needed, disjoint from
the transferred homes, and has no execution semantics. Every planned incoming
transfer must appear once and no unplanned transfer or scratch reservation may
appear.

Any D5 CFG split, join removal, or copy insertion advances the revision and
invalidates affected CFG, dominance, SSA, lowered def-use/value-flow, liveness,
constraint, and realizability products. The runner recomputes the required
facts, invokes the shared projection authority with the complete D5 mutation
and tombstone map, and requires its exact current `ProjectedConstraintSet`
before rerunning the entire module `Pseudo` profile on one frozen candidate.
Only the green full gate atomically publishes the D5 `PseudoBir` capability
accepted by E1; incremental verification cannot mint it.

E3 retry candidates and the final stable post-E3 candidate are private and use
`AssignedAllocationCandidateGate`, not `PseudoPublicationGate`. This private
gate retains all applicable graph, identity, Pseudo-schema, stage-key,
transaction, and failure-atomicity rules, but admits and requires exact-current
E2 assignments and explicit E3 spill objects/`Spill`/`Reload`. Before D5
resolution it also admits and checks `ParallelCopy` and `CopyScratch`; after
resolution it forbids both. It admits the finite E4 frame-action family only
after `FrameActionMaterializationTransaction` and then requires exact final
assignment, spill, frame, and target coverage. `PseudoPublicationGate` always
rejects frame-action nodes. This private gate cannot mint `PseudoBir` or create
a new A-F stage.

On the final stable post-E3 candidate, and only after the current E1 product
models every `CopyScratch` interval/interference and E2 has assigned each
reservation a finite non-spillable non-aliasing home, the subordinate D5
`CopyResolutionTransaction` runs before E4. The stable candidate plus its
exact current E1, E2, and E3 facts is its sole input. Its pre-E4 gate requires
an exact `CopyResolutionInputKey` naming the current revision and D5, layout,
exact current `ProjectedConstraintSet`, E1 liveness, E2 assignment, E3
spill-state, and scratch
fingerprints. It checks the immutable plan and preservation record, replays
each emitted `EdgeCopy` sequence over assigned alias units, and proves that
acyclic moves execute only after their sources are safe, overlapping writes
first save every simultaneously endangered source in distinct assigned
non-aliasing scratch homes, and cycles use the group's canonical assigned
scratch home. The output has a new exact revision and
`CopyResolutionFingerprint`, no assignment change, no new allocatable
identity, and no new spill/reload. It stages no successor product.

E4 then derives a private deterministic draft from exact D5/E3/C2/C3/C4/D2/C6
and target facts and runs `FrameActionMaterializationTransaction`. The
materializer inserts only bounded fixed-role `FrameAdjust`,
`FrameBaseSetup`/`FrameBaseRestore`, `FrameCalleeSave`/`FrameCalleeRestore`, and
`FrameProbe` one-record nodes at exact planned points. It advances the revision
and emits a mutation summary and `FrameActionFingerprint`; it cannot add an
allocatable result or scratch request.

D2 `AbiPreserve`/`AbiRestore` cover per-call live-value transport only. E4
`FrameCalleeSave`/`FrameCalleeRestore` cover exactly the post-allocation used
callee-saved units selected from D2's abstract function obligations at function
frame entry/exit points. The gate rejects a missing obligation, an E4 save for
an unused unit, or duplicate call-site/function-frame coverage. Likewise, core
`StackSave`/`StackRestore`, `DynamicAlloc`, and lifetime nodes retain source
semantics; E4 `FrameBaseSetup`/`FrameBaseRestore`/`FrameAdjust` are separate
placement actions and cannot replace or reinterpret those semantic nodes.

Only then does the shared projection authority project the final revision,
followed by E1 recomputation, E2's non-reallocating assignment validator, E3's
non-mutating spill-state validator, non-mutating final
`FrameRealizationPlan`/`FrameRealizationKey` derivation, and the target
realizability registry/checker. Their outputs must be exact-current for the
materialized `PipelineStageStamp`, `CopyResolutionFingerprint`, and
`FrameActionFingerprint`, and each consumer checks every preceding key. The
final plan covers every explicit action and stack/call/spill/frame access; the
`TargetRealizabilityKey` incorporates its exact `FrameRealizationKey`. Every original simultaneous
transfer must map to exactly one proved sequence. Stable IDs, unchanged homes
or spill nodes, preservation records, and structural equality cannot rekey a
predecessor product.

The gate rejects stale/missing products, any E1 recomputation, E2 assignment
validation, E3 spill-state validation, or realizability-check failure, an unassigned or aliasing scratch
identity, a source clobbered before its final read, a changed home, an illegal
or non-direct move, an unresolved group, and any partial rewrite. Success
requires no `ParallelCopy` or `CopyScratch` node and permits only
single-instruction-realizable `EdgeCopy` nodes before E4. Thus
`ParallelCopy` is intermediate-only: neither E4 nor MIR may accept, resolve,
schedule, or repair one.

All graph and product installation remains inside the failure-atomic
`AllocatedPublicationTransaction`. A failed owner rolls back copy resolution,
frame-action materialization, and every staged product, preserves predecessor
products unchanged, and cannot mint an E4 capability.

Any failure discards the complete D3, D4, or D5 candidate and publishes no function
subset, stage capability, property, cache entry, or derived product. Public
rechecks and incremental edit verification diagnose only; they cannot mint or
repair `PseudoBir`.

### Allocated profile and E4 publication

The `Allocated` profile accepts only the final private candidate produced by
D5 copy resolution followed by E4 frame-action materialization.
Its stage key names the exact module epoch/revision and ordered function-revision
digest plus the target, layout, preparation, exact `ProjectedConstraintKey`,
pseudo-schema, D4/D5, frame-action,
liveness, assignment, spill, copy-resolution, frame-realization, and final
target-realizability fingerprints. Every named
product must be fresh for that same revision and publication transaction; equal
semantic hashes, copied reports, compatible targets, or predecessor-only keys
do not establish identity.

Verification is cumulative and fail-closed. It reruns the private
`AssignedAllocationCandidateGate` for the materialized interval, then the frame,
direct-realizability, out-of-SSA, and Allocated rules. It never invokes the
allocation-free `PseudoPublicationGate` on this candidate. It then proves that:

1. every allocatable definition/result, fixed-home occurrence, copy role,
   call/inline-asm role, and use has exactly one legal abstract assignment or
   explicit verified spill residency at that program point;
2. assignments satisfy class/group/slot eligibility, group width/alignment,
   ties, early-clobbers, interference, aliases, reserved units, call clobbers,
   and the resolved-copy preservation record;
3. each spill object is unique and type/class compatible, each `Spill` consumes
   an assigned resident value at a legal dominance/liveness point, and each
   `Reload` produces an assigned value dominating all and only its covered
   uses; no hidden transition, unassigned reload result, unresolved eviction,
   or pressure deficit remains;
4. no `ParallelCopy` or `CopyScratch` node remains, every former group has one
   exact resolution sequence, and each surviving `EdgeCopy` has one legal
   single-instruction target mapping;
5. every required frame action occurs exactly once at its planned point as an
   admitted fixed-role one-record node; prologue, epilogue, adjustment,
   save/restore, probe, and every other record-producing action are explicit;
   every node retains one verified target mapping under the exact immutable
   `FrameRealizationPlan`; every required stack/call/spill/frame access is
   directly one-record realizable, and every target/layout,
   preparation, constraint, call, inline-asm, liveness, assignment, spill,
   copy-resolution, frame-realization, and realizability binding is present, unique,
   revision-matched, and fresh; and
6. final projection, `LivenessInterferenceKey`, `AssignmentKey`,
   `SpillStateKey`, `FrameRealizationKey`, and `TargetRealizabilityKey` all
   name the materialized revision and `FrameActionFingerprint`; and
7. target opcodes, concrete registers, frame offsets, encodings, machine
   instructions, and MIR facts remain absent from BIR.

E4 freezes the candidate once and performs these checks in one transaction.
Any diagnostic, active editor, revision/fingerprint change, missing product,
cancellation, or deterministic resource failure discards the candidate and
publishes no function subset or capability. Success atomically mints one
owning `AllocatedBir`, one E4 MIR-readiness capability bound to that
same immutable revision, and borrowing read-only `MirReadyBirView` instances.
Neither readiness capability nor view owns graph storage or can outlive the
owning token. Diagnostic candidate checks and public rechecks cannot mint,
repair, or refresh any of the three.

MIR consumes only `MirReadyBirView`. It rechecks the exact revision and product
fingerprints, applies the already verified frame/object placements and target
mapping, and selects one machine record per allocated pseudo node. It cannot
choose frame offsets, bases, stack displacements or adjustments; it cannot
change assignments, introduce an allocatable temporary or capacity
spill/reload, create scratch, resolve or schedule copies, reinterpret
constraints, insert a frame record, expand instructions or calls, or hide a missing transition.
Failure to map or encode the verified view fails the MIR transaction and
requires an upstream schema/legalization change.

## Proposed public API

The fenced C++ in this section is documentation-only and proposed. Names may
change with the core schema and do not settle idea 732 Step 8's final Raw
boundary. The API must consume read-only views, not renderer text or legacy
structs.

`VerifyProfile::Pseudo` names only `PseudoPublicationGate`. The
`AssignedAllocationCandidateGate` is an internal candidate verifier that takes
the exact assignment/spill product keys explicitly; it is not a public profile
or publication authority.

```cpp
enum class VerifyProfile : std::uint8_t {
  Raw,
  Canonical,
  PreparedInput,
  Pseudo,
  Allocated,
};

struct VerifyOptions {
  std::size_t max_diagnostics = 256;
  bool warnings_as_errors = false;
};

enum class DiagnosticSeverity : std::uint8_t { Note, Warning, Error };
enum class VerifyPhase : std::uint8_t {
  Storage,
  Module,
  SymbolsAndGlobals,
  FunctionShape,
  Cfg,
  InstructionSchema,
  Types,
  DefUse,
  Ssa,
  StageBoundary,
};

enum class VerifyRule : std::uint16_t {
  ModuleEpochInvalid = 0x0100,
  ModuleRevisionInvalid = 0x0101,
  RevisionChanged = 0x0102,
  IdWrongEpoch = 0x0103,
  IdWrongOwner = 0x0104,
  IdOutOfRange = 0x0105,
  IdTombstone = 0x0106,
  IdStaleGeneration = 0x0107,
  IdWrongKind = 0x0108,
  ForeignReference = 0x0109,
  OrderMissing = 0x010A,
  OrderDuplicate = 0x010B,
  OrderForeign = 0x010C,
  StorageOrphan = 0x010D,
  ReservationUnresolved = 0x010E,
  ReservationDuplicate = 0x010F,
  ReservationKindMismatch = 0x0110,
  ReservationDefinitionMismatch = 0x0111,
  ForwardReferenceUnresolved = 0x0112,
  ActiveEditAtFreeze = 0x0113,

  DataLayoutInvalid = 0x0200,
  TypeKindInvalid = 0x0201,
  TypeChildInvalid = 0x0202,
  TypeRecursionInvalid = 0x0203,
  TypeRoleInvalid = 0x0204,
  IntegerWidthInvalid = 0x0205,
  FloatFormatInvalid = 0x0206,
  AddressSpaceInvalid = 0x0207,
  TypeLayoutInvalid = 0x0208,
  ConstantKindInvalid = 0x0209,
  ConstantTypeMismatch = 0x020A,
  ConstantPayloadInvalid = 0x020B,
  UndefPoisonRoleInvalid = 0x020C,

  SymbolIndexMismatch = 0x0300,
  SymbolNameInvalid = 0x0301,
  SymbolDuplicate = 0x0302,
  SymbolKindConflict = 0x0303,
  DeclarationConflict = 0x0304,
  DefinitionDuplicate = 0x0305,
  LinkageInvalid = 0x0306,
  VisibilityInvalid = 0x0307,
  SectionInvalid = 0x0308,
  TlsInvalid = 0x0309,
  AliasInvalid = 0x030A,
  AliasCycle = 0x030B,
  DirectiveInvalid = 0x030C,
  GlobalStateInvalid = 0x030D,
  GlobalLayoutInvalid = 0x030E,
  InitializerKindInvalid = 0x030F,
  InitializerShapeInvalid = 0x0310,
  InitializerBoundsInvalid = 0x0311,
  RelocationTargetInvalid = 0x0312,
  RelocationRangeInvalid = 0x0313,
  RelocationOverlap = 0x0314,
  BlockAddressInvalid = 0x0315,
  TopLevelAsmDependencyInvalid = 0x0316,

  FunctionShapeInvalid = 0x0400,
  SignatureInvalid = 0x0401,
  FunctionAttributeInvalid = 0x0402,
  ParameterMismatch = 0x0403,
  ParameterDefinitionInvalid = 0x0404,
  LocalInvalid = 0x0405,
  EntryBlockInvalid = 0x0406,
  BlockShapeInvalid = 0x0407,
  TerminatorMissing = 0x0408,
  TerminatorSchemaInvalid = 0x0409,
  ReturnOperandInvalid = 0x040A,
  EdgeTargetInvalid = 0x040B,
  EdgeKeyInvalid = 0x040C,
  SuccessorSlotInvalid = 0x040D,
  SwitchCaseInvalid = 0x040E,
  IndirectTargetSetInvalid = 0x040F,
  AsmGotoPairInvalid = 0x0410,
  UnreachableBlock = 0x0411,

  OpcodeInvalid = 0x0500,
  OpcodeProfileInvalid = 0x0501,
  DescriptorMissing = 0x0502,
  DescriptorMismatch = 0x0503,
  OperandArityInvalid = 0x0504,
  OperandKindInvalid = 0x0505,
  OperandTypeMismatch = 0x0506,
  ResultArityInvalid = 0x0507,
  ResultTypeMismatch = 0x0508,
  DuplicateDefinition = 0x0509,
  MissingDefinition = 0x050A,
  ResultDefinitionInvalid = 0x050B,
  DefUseMismatch = 0x050C,
  StaleUse = 0x050D,
  PhiPlacementInvalid = 0x050E,
  PhiEdgeMultisetMismatch = 0x050F,
  PhiTypeMismatch = 0x0510,
  UseBeforeDefinition = 0x0511,
  UseNotDominated = 0x0512,
  CrossComponentUse = 0x0513,

  CallCalleeInvalid = 0x0600,
  CallSignatureMismatch = 0x0601,
  CallArgumentMismatch = 0x0602,
  CallEffectsInvalid = 0x0603,
  CallOperandBundleInvalid = 0x0604,
  CallReturnMismatch = 0x0605,
  TailCallInvalid = 0x0606,
  NoReturnContractInvalid = 0x0607,
  VarArgInvalid = 0x0608,

  MemoryAccessInvalid = 0x0700,
  MemoryAlignmentInvalid = 0x0701,
  GepInvalid = 0x0702,
  GepIndexInvalid = 0x0703,
  DynamicAllocInvalid = 0x0704,
  DynamicSizeOverflow = 0x0705,
  StackStateInvalid = 0x0706,
  MemoryIntrinsicInvalid = 0x0707,
  AtomicTypeInvalid = 0x0708,
  AtomicOrderingInvalid = 0x0709,
  AtomicResultInvalid = 0x070A,
  FenceInvalid = 0x070B,

  AggregatePathInvalid = 0x0800,
  AggregateLayoutMismatch = 0x0801,
  VectorLaneInvalid = 0x0802,
  VectorMaskInvalid = 0x0803,
  IntrinsicIdInvalid = 0x0804,
  IntrinsicSchemaInvalid = 0x0805,
  InlineAsmStructureInvalid = 0x0806,
  InlineAsmPayloadInvalid = 0x0807,
  InlineAsmValueEdgeInvalid = 0x0808,
  InlineAsmClobberInvalid = 0x0809,
  InlineAsmEffectInvalid = 0x080A,

  DebugReferenceInvalid = 0x0900,
  DebugCardinalityInvalid = 0x0901,
  ProvenanceInvalid = 0x0902,
  ForbiddenStageFact = 0x0903,
  ForbiddenCompatibilityPayload = 0x0904,

  PseudoStageKeyInvalid = 0x0A00,
  PseudoSchemaInvalid = 0x0A01,
  PseudoStageLegalityInvalid = 0x0A02,
  PseudoSemanticLeftover = 0x0A03,
  PseudoTargetFactInvalid = 0x0A04,
  PseudoDerivedProductStale = 0x0A05,
  PseudoInlineAsmBindingInvalid = 0x0A06,
  PseudoCallLoweringIncomplete = 0x0A07,
  PseudoForbiddenAllocationFact = 0x0A08,
  PseudoDirectRealizabilityInvalid = 0x0A09,
  PseudoPublicationIncomplete = 0x0A0A,
  PseudoCopyPlacementInvalid = 0x0A0B,
  PseudoParallelCopyInvalid = 0x0A0C,
  PseudoPhiLoweringIncomplete = 0x0A0D,
  PseudoCopyCoverageMismatch = 0x0A0E,
};

struct ReservationSite {
  FunctionId owner;
  InstId instruction;
  std::uint64_t reservation_nonce;
};

using VerifyEntity = std::variant<
    std::monostate, ModuleEntityId, FunctionEntityId, EdgeKey, SuccessorSlot,
    ReservationSite, OperandSite, InitializerPath>;

// ModuleEntityId is the exact core variant containing TypeId, TypeNameId,
// ConstantId, InitializerId, SymbolId, GlobalId, DirectiveId, FunctionId,
// DebugFileId, DebugScopeId, DebugLocId, and OriginId. FunctionEntityId contains
// LocalId, BlockId, InstId, and ValueId. monostate denotes the module as a whole.

struct VerifyLocation {
  FunctionId function{};
  BlockId block{};
  InstId instruction{};
  std::optional<std::uint32_t> operand_index;
  std::optional<std::uint32_t> result_index;
};

struct VerifyDiagnostic {
  VerifyRule rule;
  VerifyPhase phase;
  DiagnosticSeverity severity;
  VerifyEntity entity;
  VerifyLocation location;
  std::optional<VerifyEntity> related_entity;
  std::string detail;       // presentation only
};

struct VerificationReport {
  VerifyProfile profile;
  std::vector<VerifyDiagnostic> diagnostics;
  bool truncated = false;
  [[nodiscard]] bool ok() const noexcept;
};

class ModuleDraft;          // move-only unpublished storage
class CandidateModuleView;  // read-only borrow of one frozen draft revision
class VerifiedPreparationInput;  // borrowing Canonical/target C1 capability
class PreparationInputFailure;   // report plus target validation cause

class PublicationFailure {
 public:
  PublicationFailure(PublicationFailure&&) noexcept = default;
  PublicationFailure& operator=(PublicationFailure&&) noexcept = default;
  PublicationFailure(const PublicationFailure&) = delete;
  PublicationFailure& operator=(const PublicationFailure&) = delete;

  [[nodiscard]] const VerificationReport& verification() const noexcept;
  [[nodiscard]] const std::optional<BirError>& preceding_error() const noexcept;

 private:
  PublicationFailure(VerificationReport verification,
                     std::optional<BirError> preceding_error);

  VerificationReport verification_;  // always present, including early failure
  std::optional<BirError> preceding_error_;

  friend class RawPublisher;
};

[[nodiscard]] VerificationReport verify(
    ModuleView module, VerifyProfile profile, VerifyOptions options = {});
[[nodiscard]] VerificationReport verify_candidate(
    CandidateModuleView module, VerifyProfile diagnostic_profile,
    VerifyOptions options = {});
[[nodiscard]] Result<RawBir, PublicationFailure> verify_and_publish_raw(
    ModuleDraft&& draft, VerifyOptions options = {});
[[nodiscard]] Result<VerifiedPreparationInput, PreparationInputFailure>
verify_preparation_input(const CanonicalBir& canonical,
                         const TargetProfile& validated_target,
                         VerifyOptions options = {});
[[nodiscard]] VerificationReport verify_function(
    ModuleView module, FunctionId function, VerifyProfile profile,
    VerifyOptions options = {});
[[nodiscard]] VerificationReport verify_after_edit(
    CandidateModuleView module, const MutationSummary& edit,
    VerifyProfile profile, VerifyOptions options = {});
```

In this proposal, `RawPublisher` is verifier-private implementation machinery used only by
`verify_and_publish_raw`; no builder, importer, or caller can construct or copy
`PublicationFailure`. Even an early storage/build failure receives a complete
Raw-profile report (possibly containing one quarantining rule) plus the original
`BirError` in `preceding_error_`.

### Stable rule registry

The enum above is the complete registry for this schema revision. Numeric values
are serialization/test API and never change meaning; new rules receive unused
values rather than renumbering an existing rule. Messages are presentation only.

| Range | Rule IDs | Required coverage |
|---|---|---|
| `0x0100–0x0113` | `ModuleEpochInvalid` through `ActiveEditAtFreeze` | module revision; stale/foreign IDs; exact order/storage membership; every `ReservedInst` resolved exactly once with matching owner/results/types; no active edit at freeze |
| `0x0200–0x020C` | `DataLayoutInvalid` through `UndefPoisonRoleInvalid` | type graph, recursion, role legality, integer/float/address-space domains, object layout, constants including exact x87 extended-80/IEEE binary128 payloads, `undef`, and `poison` |
| `0x0300–0x0316` | `SymbolIndexMismatch` through `TopLevelAsmDependencyInvalid` | declarations/definitions, linkage/visibility/section/TLS, aliases/directives, typed top-level-asm dependencies, global state/layout, initializer trees, relocations, label differences, and block addresses |
| `0x0400–0x0411` | `FunctionShapeInvalid` through `UnreachableBlock` | signatures/attributes/parameters/locals, entry and block shape, one terminator, optional return operand, exact `SuccessorSlot`/`EdgeKey`, switch/indirect/asm-goto edges, profile-specific reachability diagnosis |
| `0x0500–0x0513` | `OpcodeInvalid` through `CrossComponentUse` | opcode/profile/descriptor closure, operand/result arity/kind/type, unique definitions, exact def-use, phi placement and edge-key multiset, same-block order and dominance |
| `0x0600–0x0608` | `CallCalleeInvalid` through `VarArgInvalid` | direct `SymbolId` or indirect callee, signature/arguments, `CallEffects`, typed operand bundles, optional semantic result/return, tail/noreturn, and variadics |
| `0x0700–0x070B` | `MemoryAccessInvalid` through `FenceInvalid` | loads/stores/address spaces/alignment, GEP, dynamic allocation and stack state, memcpy/memmove/memset, atomics and fences |
| `0x0800–0x080A` | `AggregatePathInvalid` through `InlineAsmEffectInvalid` | aggregates, layouts, vector lanes/masks, intrinsic registry/schema, and opaque inline-asm payload plus ordinary value edges, clobber order, and effects |
| `0x0900–0x0904` | `DebugReferenceInvalid` through `ForbiddenCompatibilityPayload` | debug/provenance structure and the absence of route/preparation/regalloc/MIR/text-placeholder authority |
| `0x0A00–0x0A0E` | `PseudoStageKeyInvalid` through `PseudoCopyCoverageMismatch` | exact pseudo revision/product lineage, closed and stage-legal pseudo schema, absence of semantic/machine/allocation leftovers, complete call and inline-asm binding, D4 direct realizability, D5 edge placement/simultaneous-copy/phi-removal coverage, and atomic whole-stage publication |

The coverage ledger below maps every feature family to these IDs. A feature
cannot be called verifier-covered until its negative tests assert the mapped
rule IDs.

`verify_function` still receives the module so symbol, type, and global
references can be checked. Scope and profile are explicit entry-point inputs,
not mutable options that can silently retain `Raw` while publishing a
`Canonical` object. `CandidateModuleView` is immutable to the
verifier but represents a builder/pass transaction that has not been
published; it is not constructible from arbitrary storage. `verify_after_edit`
is an optimization, never a weaker contract: it expands the edit to all
affected owners, CFG neighbors, users, and dominance dependents. Publication
and CI run the same full Raw rule set, but only the one owning Raw publication
entry can atomically convert its successful candidate to the public stage
type. In the documentation-only proposal that entry is
`verify_and_publish_raw(ModuleDraft&&)`; the current landed adapter is
`ModuleBuilder::publish() &&`. Diagnostic entry points cannot issue a module
publication proof even when their report is green.

`VerifyProfile::PreparedInput` names the cumulative rule set reported by
`verify_preparation_input`; it is not accepted by the target-less generic
`verify`, `verify_candidate`, `verify_function`, or `verify_after_edit` entry
points. Only the dedicated C1 gate can return `VerifiedPreparationInput`, and
that move-only borrowing capability expires when either the Canonical
`PipelineStageStamp` or validated target fingerprint changes. Failure returns
no capability and cannot cache a green Canonical recheck as target-binding
authority.

The checked-in bootstrap names (`FoundationVerifier`, `VerificationResult`, and
`VerifyProfile::FoundationRaw`) remain the implementation adapter until this
API lands atomically. They are not a second verification contract. In
particular, builder publication must not run the bootstrap verifier and then
advertise the result as satisfying this document's full `Raw` profile.

`VerificationReport` is a value, not a second `Result` template or a thrown
exception. Core builder operations continue to use `BirResult<T>` (the common
`Result<T, BirError>` alias). The publication entry point returns
`Result<RawBir, PublicationFailure>`, where `PublicationFailure` owns the
complete `VerificationReport` on every failure and may additionally carry the
preceding `BirError` that prevented or accompanied semantic verification. The
LIR importer wraps this exact move-only `PublicationFailure` in its
`ImportFailure`; it does not extract/rebuild the report, translate `VerifyRule`
IDs, or return candidate storage. This keeps import diagnostics, builder errors,
and verification findings layered without competing success types.

Diagnostics are sorted deterministically by phase, module order, function order,
block order, instruction order, operand/result index, rule, and semantic ID.
Traversal order, hash iteration order, pointer values, debug names, and rendered
BIR never affect acceptance or output order. Diagnostics are deduplicated by
structured rule/location/entity key, fully sorted, and only then truncated to
`max_diagnostics`; stopping at the first N hash-discovered failures would be
nondeterministic. An implementation safety cap uses the same deterministic
key-retention rule and sets `truncated`. Invalid references are quarantined so
later phases do not dereference them. A diagnostic may relate two IDs, for
example a duplicate definition and the first definition.

## Validation phase order

The ordering prevents semantic checks from trusting corrupt storage:

1. **Storage and ID safety.** Establish resolvable live objects, owners,
   generations, kinds, and exact order membership.
2. **Module/type universe.** Validate the target-independent semantic type
   graph and bounded module state without consulting target, ABI, or codegen
   policy.
3. **Symbols and globals.** Build semantic symbol indices and validate
   declarations, definitions, object data, and initializer references.
4. **Function shape.** Check signature, parameter definitions, declaration/body
   split, local objects, and ordered ownership.
5. **CFG.** Decode every ordered `SuccessorSlot`, derive incoming slot
   multisets/deduplicated adjacency as separate views, and classify reachability.
6. **Instruction schema.** Check bounded opcode/terminator alternatives and the
   exact operand/result roles of each.
7. **Types and feature-specific rules.** Check operands, results, calls, memory,
   atomics, aggregates, intrinsics, and inline assembly.
8. **Complete def-use.** Recompute definitions and uses from the semantic IR and
   compare core's canonical eager value-use store exactly.
9. **SSA/dominance.** Apply profile-specific use ordering, phi edge, and
   dominance rules.
10. **Stage boundary.** Reject legacy route data and any prepared/MIR authority.

If a prerequisite phase is corrupt, dependent checks emit at most a concise
“not checked because …” note. They must not guess through names or side tables.

## Storage, stable IDs, and order ownership

Raw publication requires:

- module epoch is nonzero; every ID has the module epoch of the view being
  verified;
- `FunctionId` belongs to the module; `BlockId`, `InstId`, `ValueId`, and local
  object IDs belong to exactly one `FunctionId`;
- slot is in range, live rather than tombstoned, generation is exact, and the
  ID’s discriminant agrees with stored object kind;
- every live function occurs exactly once in module order; every live block
  exactly once in its function’s block order; every live instruction exactly
  once in exactly one owned block’s instruction order;
- every ordered ID resolves to a live object of the expected owner, and no live
  storage object is orphaned from its owning order;
- every result `ValueId` resolves to exactly one `InstResultDef(inst, index)` and
  the instruction’s result list points back; every parameter resolves to exactly
  one `ParameterDef(ordinal)` and the signature order points back;
- deletion/reuse makes old generations stale. A stale, foreign-module, or
  foreign-function ID is always an error even if the slot currently contains an
  object with similar text or type;
- every `ReservedInst` is defined exactly once before draft freeze using its
  unexpired move-only capability. Owner, `InstId`, result IDs/count/types, block
  membership, and order remain identical to the reservation. A dropped token
  leaves `ReservationUnresolved`; duplicate/expired consumption is
  `ReservationDuplicate`; owner/kind disagreement is
  `ReservationKindMismatch`; result/order disagreement is
  `ReservationDefinitionMismatch`. Other symbol/type/function declarations use
  explicit complete declaration states rather than undefined reservations, and
  every semantic forward reference must resolve (`ForwardReferenceUnresolved`);
- freeze rejects any live builder/editor capability (`ActiveEditAtFreeze`); no
  later completion is allowed against the frozen revision;
- physical slot index is not program order. Passes use explicit order and IDs;
  renderer/debug names are non-authoritative.

Function-local IDs make accidental cross-function operands, branch targets,
phi predecessors, and local objects diagnosable as `WrongOwner`; module epochs
make IDs retained from a replaced module diagnosable as `WrongEpoch` or
`StaleGeneration`.

## Module, types, symbols, and globals

### Type universe

Raw and Canonical BIR carry no semantic `target_profile`, target triple,
rendered `data_layout`, language-ABI mode, pointer-width/address-space layout
selection, or other C1/C2 target context. Validation-only source origin and
parity evidence is explicitly non-semantic: it cannot create Raw identity,
target facts, type layout, or acceptance authority. C1 independently selects
and validates one exact `TargetProfile`; C2 derives the target-layout facts
bound to that profile. Neither decision is imported into Raw or Canonical
storage.

Every type alternative must be bounded and recursively well-formed. Raw coverage
must eventually include: void only in permitted roles; booleans; signedness-free
integer widths including 1/8/16/32/64/128; F32/F64/F128; pointers with address
space and, if the chosen schema retains it, pointee/function signature; arrays;
vectors/scalable-vector metadata; structs/unions with complete, opaque, packed,
and named forms; and function types including variadicness and calling
convention. Rules include:

- no void value, parameter, load/store element, aggregate field, or phi input;
- positive legal integer/vector widths and array counts; no size arithmetic
  overflow;
- referenced child `TypeId`s resolve in the module; recursive records use the
  explicit named/opaque mechanism rather than cyclic value storage;
- field offsets, explicit size/alignment, when present in Raw, agree internally
  and satisfy nonzero/power-of-two and bounds rules. Target-computed layout is
  not invented by this verifier;
- function return/parameter types and calling convention are legal IR facts;
  ABI classes, registers, and stack locations are not.

The verifier contract assumes immutable, module-owned `TypeId` and `ConstantId`
entities. Constants appear in typed `Operand` alternatives and therefore have
uses but are not fake `ValueId` definitions; exact value def-use remains limited
to parameters and instruction results. Canonical v1 does not admit block
arguments.

The current `core/type.hpp` scalar enum is only bootstrap coverage and must not
be read as the final backend feature set.

`F128` cannot be verified as a universal synonym for IEEE binary128. The
source/reference backends use the same language-level long-double family for
IEEE binary128 on AArch64/RISC-V and x87 extended precision with padded
10/12/16-byte storage on x86 targets. The final type schema therefore records
floating semantics separately from object storage size/alignment (for example
`IeeeBinary128` versus `X87Extended80`). Constants carry exact semantic bits and
defined padding policy. The verifier rejects a target/profile combination whose
semantic format, payload width, or storage layout disagree; it never host-
round-trips the payload through `double`.

Legacy `Vrm1`/`Vrm2`/`Vrm4`/`Vrm8` and LIR `VrmRegister` are target carrier
types, not portable scalar types. Import must translate them to an equivalent
semantic vector/aggregate when lossless, or report the accepted receiving-side
validation failure. Raw verification rejects VRM register-group width as value
type authority.

### Symbols and declarations

`SymbolId`/`GlobalId`/`FunctionId` are semantic identity. Link spelling is an
attribute indexed to that identity, never a parallel authority. The module must
enforce:

- nonempty link names where linkage requires a name and uniqueness in the
  relevant linker namespace;
- exact agreement between symbol index and owned function/global;
- compatible repeated declarations if the schema permits them, at most one
  definition, and no function/object kind collision;
- function declaration: signature and attributes are valid, no blocks,
  instructions, parameters-as-values, or local objects;
- function definition: body is present, entry block is first in semantic order,
  parameter count/types equal the signature, and linkage/visibility attributes
  are coherent;
- global declaration/extern: no object bytes requiring emitted storage;
- global/common/tentative/defined forms obey their explicit linkage model;
  defined storage has a valid size/alignment and a permitted initializer;
- constructors, destructors, aliases, weak/visibility/version metadata, TLS,
  sections, and top-level assembly reference declared semantic symbols where
  applicable. Alias chains are kind/type compatible, remain in one valid linker
  namespace, and are acyclic; constructor/destructor entries have valid
  function signatures and deterministic priorities. Section/linkage/visibility,
  TLS model, explicit alignment, and used/retain state obey their closed
  combinations. None of these facts can be recovered by parsing display text.

Top-level assembly uses the exact core carrier:

```cpp
struct TopLevelAsmSpec {
  std::string source_text;
  std::vector<SymbolId> symbol_dependencies;
  OriginId origin;
};
```

The verifier checks that every `symbol_dependencies` entry resolves in the same
module epoch, has a link-visible symbol kind usable by assembly, and appears in
deterministic declared order. Duplicate IDs are rejected unless the schema
later gives dependency multiplicity meaning. `origin` resolves normally.
`source_text` is preserved for the assembler but is never scanned to discover,
add, reorder, or repair symbol identity. Missing/foreign/stale dependencies are
`TopLevelAsmDependencyInvalid`; malformed directive structure remains
`DirectiveInvalid`.

### Global initializer and object data rules

Initializer validation recursively walks every element and relocation slot:

- initializer kind is legal for the global’s type and declaration state;
- emitted byte count fits the object, required padding is explicit or defined by
  the initializer schema, alignment is valid, and range arithmetic cannot wrap;
- zero-sized objects/arrays are accepted only when the selected language
  extension and object schema explicitly permit them; they retain distinct
  symbol identity and never justify underflowed bounds or overlapping
  relocation slots;
- integer/floating bit payload has the declared width; arrays/vectors have the
  declared count; structs/unions obey selected field/layout representation;
- zero, scalar, string/wide string/char16 data, compound data, address,
  address-plus-addend, label difference, and relocation-bearing bytes have
  explicit schemas;
- every symbol/label relocation resolves by ID, has supported width/alignment,
  lies within object bounds, and relocation slots do not overlap illegally;
- an address initializer targets a compatible function/global/TLS/label kind;
  text spelling may be printed but cannot repair a missing ID;
- label-address and label-difference initializers carry an owning `FunctionId`
  plus live local `BlockId`s; both labels in a difference belong to that same
  function. Their semantic addend/width is validated here, while target
  relocation encoding and whether a particular object format supports the
  expression are checked later;
- extern declarations do not smuggle initializer/object data; defined objects do
  not depend on prepared `object_data` records as their semantic source.

## Functions, blocks, terminators, and CFG

Every definition has at least one block and one semantic entry block. Each block
has an instruction order and exactly one terminator stored separately from
ordinary instructions. No instruction may appear after a terminator. Each
terminator variant has an exact schema:

```cpp
struct ReturnTerm { std::optional<Operand> value; };
struct SuccessorSlot {
  BlockId source;
  SuccessorRole role;
  std::uint32_t index;
  BlockId target;
};
struct EdgeKey {
  BlockId source;
  SuccessorRole role;
  std::uint32_t successor_index;
};
```

- `return`: one optional semantic operand; it is absent exactly for void and
  otherwise has the function's semantic return type, including an aggregate
  type when applicable. Raw has no hidden ABI return lane or multivalue return
  carrier (`ReturnOperandInvalid`);
- `jump`: one local live target;
- `cond_jump`: local I1 condition and two local live successor slots; equal
  true/false targets are legal Raw shape because slot role preserves both edge
  occurrences, while B3 / P03 folds the redundant conditional into its unique
  canonical successor form;
- `switch`: integer selector, unique case constants representable by its type,
  one default, and all local targets;
- `indirect_jump`: pointer/code-address operand and a nonempty ordered list of
  possible local target slots when required for analysis;
- `asm_goto`: references a structured `InlineAsm` instruction that is the final
  ordinary instruction in the same block, plus one explicit fallthrough slot
  and ordered goto-target slots. Constraint label slots match terminator slots
  one-for-one (`AsmGotoPairInvalid`); no mid-block instruction owns hidden exits;
- `unreachable`: no operands or successors;

CFG successor authority is the ordered `SuccessorSlot` sequence derived solely
from semantic terminators. Each slot has
`EdgeKey{source, successor_role, successor_index}` and a target. Parallel slots
remain distinct even when they share source and target—for example two switch
cases or asm labels targeting the same block. An analysis may additionally
derive a deduplicated adjacency set, but it cannot replace slot authority.
Ordinary non-goto inline asm contributes no edge; an importer encountering asm
goto splits the source block at that operation and publishes `asm_goto` as the
first block's terminator. This avoids pretending that values defined after a
mid-block exit dominate its target. CFG edges are not stored as authoritative
side tables (`SuccessorSlotInvalid`, `EdgeKeyInvalid`, `EdgeTargetInvalid`).
Entry reachability is computed from slot targets. Raw accepts unreachable
blocks unconditionally but still validates their local contents, IDs, types,
def-use, and internal dominance as separate roots. `UnreachableBlock` is a
deterministic Raw note and is not promoted by `warnings_as_errors`. Canonical
requires every retained block to be entry-reachable because B3 / P03 owns
deterministic unreachable-region removal; at B8 the same rule is an error and
publication fails.

## Instruction schema and complete operand traversal

Each opcode owns a declarative schema: legal profiles, fixed/variadic operand
roles, result arity, operand/result type predicates, side-effect class, may-trap
flag, memory effect, and control-flow contribution. The verifier dispatches on
the bounded opcode variant, not strings. Unknown, valueless, mismatched payload,
or stage-illegal alternatives are errors.

A single semantic visitor must enumerate **every** `ValueId`, `BlockId`,
`FunctionId`, `GlobalId`, local object, type, and symbol reference in an
instruction/terminator/initializer. The verifier, def-use builder, liveness,
rewriter, cloning, and deletion preconditions share this visitor or generated
schema. Special payloads are not exceptions: indirect callee, call arguments,
phi incoming values and predecessor IDs, GEP indices, atomic operands, intrinsic
arguments, inline-asm inputs/outputs/ties/goto labels, switch values, and return
values all participate.

Exact def-use means:

- every used `ValueId` resolves locally and has one coherent definition;
- every instruction result is unique and its declared result type matches the
  stored `ValueDef`;
- recomputed `(user, operand-role/index)` multisets equal core's canonical eager
  `users(ValueId)` state;
- replacing/removing/cloning instructions cannot leave stale users or duplicate
  definitions;
- constants, globals, blocks, and local objects use their own typed references,
  not fake value names;
- no consumer may discover a use by parsing printer output or by maintaining a
  second hand-written partial visitor.

## LIR import gate and current-intake dispositions

Raw verification is not a substitute for lossless import. The importer first
validates the LIR alternative, converts every semantic reference to a typed BIR
ID, and records a structured source location. It then submits one unpublished
candidate to full Raw verification. A failed conversion or verification returns
an error and publishes no `RawBir`.

The import dispatch is exhaustive by `std::variant` alternative **index**. The
current source has 38 unique `LirInst` alternatives and six `LirTerminator`
alternatives; inventory tests pin both counts and require an explicit import or
rejection path for every index. The legacy stub and typed forms
(`LirLoad`/`LirLoadOp`, `LirCall`/`LirCallOp`, and peers) are separate source
alternatives even when they lower to one BIR opcode.

The accepted phase-A 38-instruction, 6-terminator, and 18-metadata-family
matrices are the authority for the complete immutable current-LIR intake. Every
existing fact keeps its named container, importer-wiring, or validation-only
disposition. A desired form absent from current LIR is outside this intake; it
is not a current-source gap and does not authorize LIR or producer-schema work.

| Current LIR source family | Raw publication requirement | Gap/deferred disposition |
|---|---|---|
| constants, scalar ops, casts, comparisons, select, aggregate/vector insert/extract/shuffle | typed operands, exact bit payloads, result types, predicates and indices | malformed or text-only opcode/type is `UnsupportedSourceForm`; legalization of a representable operation is a later pass |
| load/store/GEP, hoisted and inline alloca, memcpy/memset, stack save/restore | one semantic memory/object operation with typed address, size, alignment, volatility and address space | `alloca_insts` is merged into semantic entry order; it is never a second instruction list |
| direct/indirect calls and variadic operations | typed callee identity or callee value, complete function signature, fixed/extra argument boundary and every argument value | `callee_name`, `args_str`, or incomplete extern return-only data cannot supply missing identity/signature; ABI classification is deferred |
| branch/conditional/return/switch/indirect branch/unreachable | one typed terminator per block | `LirIndirectBrOp` must agree with and be consumed into terminator authority; disagreement or an instruction-only carrier is rejected |
| inline asm | generic typed value operands/results plus opaque original template/constraint payload, ordered clobbers, and side effects | `args_str` and compatibility result/type text are never authority; current typed roles/indices follow the accepted phase-A matrix, parsed constraint facts are deferred, and desired symbol/address-space/goto forms absent from current LIR are outside this intake |
| globals, strings, externs, struct declarations and initializers | typed symbol/type/object identity and recursive initializer/relocation tree | `init_text`, pool names, or initializer name scans are compatibility text, not importer authority; every current field follows its exact accepted container/wiring/validation disposition, while desired absent initializer forms are outside this intake |
| atomics | closed load/store/RMW/cmpxchg/fence payload with ordering and result mode | current `LirInst` has no structured atomic alternative, so atomic forms are outside this intake; this does not authorize a new LIR alternative or a legacy parallel table |
| i128/f128, complex/multivalue and runtime-helper-capable operations | preserve full semantic type, exact constant bits, operands and semantic results | target helper choice, split lanes and physical return carriers are deferred to legalization/preparation/MIR |
| specialization/layout observations and printer/debug text | retain only downstream semantic IDs or optional non-authoritative diagnostics | resolved specialization is reflected in symbols before import; observations/text never repair missing semantic data |

Every rejected source family reports a stable import error containing source
function/block/instruction index (or module field), source alternative index,
feature family, and missing semantic carrier. It must not create a placeholder
instruction and hope a later pass repairs it. The Raw verifier still rejects a
candidate containing any compatibility placeholder in case an importer bug
bypasses this gate.

## Type checking by instruction family

The final opcode table is owned with core IR design. The Raw verifier must at
least cover these compiler-backend families:

- integer/float/vector unary, binary, shifts, division/remainder and comparison;
  comparison result is I1, shift count is integer, and opcode/type domains are
  explicit;
- select condition is I1 and both alternatives/result have the same type;
- casts have a legal `(opcode, source type, destination type)` relation,
  including integer width changes, FP changes/conversions, pointer/integer, and
  representation-preserving bitcasts;
- copy/materialize/constant results agree exactly with payload type. Integer,
  floating, null, zero, `undef`, and `poison` are distinct closed typed operand
  alternatives; `undef`/`poison` are never magic names or missing `ValueId`s,
  cannot appear where the opcode forbids them, and retain their distinct
  semantics through cloning and rewriting;
- aggregate insert/extract/copy and multi-result operations use in-range field or
  lane paths and compatible aggregate types;
- stack/static local allocation has sized non-void element type, nonzero legal
  alignment, and coherent constant/dynamic size;
- all memory, call, phi, intrinsic, asm, and terminator rules below.

I128 and F128 are ordinary semantic types, not implicit helper calls. Their
constants retain all 128 bits; integer division/remainder, floating arithmetic,
conversions, comparisons, and complex/multivalue forms use the same closed
opcode/type rules as narrower forms. Raw acceptance requires exact semantics and
a registered later legalization owner. The legalizer may derive a target
capability classification (`Native`, `Expand`, `RuntimeFallback`, or
`UnsupportedForTarget`) outside the instruction; the Raw verifier does not
choose a helper. `RuntimeFallback` selection, helper
symbol/signature, split lanes, calling convention, and result bridging are later
facts. `UnsupportedForTarget` may be diagnosed at the ordered legalization gate,
but can never be converted into a silently valid no-op or an opaque Raw opcode.

## Calls

Direct calls reference one declared `SymbolId` of function kind and its function
type. If that symbol has a body, its unique `FunctionId` relation must agree;
external declarations need no body ID. A `FunctionId` alone is ownership of a
body, not linker identity. Indirect calls carry a local pointer/callable operand
plus an explicit function signature; they do not carry a contradictory direct
callee or a link-name fallback (`CallCalleeInvalid`). For both:

- result arity is zero exactly for void and otherwise matches the semantic return
  type as exactly one result; aggregate return remains one aggregate-typed
  semantic result, not sret/register lanes (`CallReturnMismatch`);
- fixed argument count and types match the prototype;
- non-variadic calls have no extra arguments; variadic calls have at least the
  fixed arguments, `num_fixed_args` agrees with the prototype, and extra
  arguments already reflect language-level default promotions promised by LIR;
- calling convention and function attributes (`noreturn`, etc.) agree with the
  declaration or the explicit indirect-call type;
- `CallEffects` is a closed semantic record covering return behavior
  (`ReturnsOnce`, `ReturnsTwice`, `NeverReturns`), unwind behavior, memory
  effect, convergence, and duplication permission. It must agree with
  declaration attributes and cannot be reconstructed from a callee name
  (`CallEffectsInvalid`);
- semantic call-site attributes such as tail kind are closed flags with legal
  combinations. A `noreturn`
  call is followed only by an `unreachable` terminator; values live only across
  a `returns_twice` call are not optimized under ordinary single-return
  assumptions. Setjmp/longjmp behavior is expressed by these semantic flags,
  never inferred from a callee spelling;
- by-value aggregate intent lives in typed per-argument attributes
  (object type/size/alignment), not a register class, stack offset, copy slot,
  or move plan;
- every operand bundle has a registered typed tag (`Deopt`, `Funclet`,
  `GcTransition`, or `Assume` in this schema), closed payload schema,
  permitted call/signature/effect combination, and descriptor-visible operands;
  unknown tags or opaque byte/text bundles fail as `CallOperandBundleInvalid`;
- indirect callee, every argument, every bundle operand, and any semantic result
  is visited by the shared descriptor and exact def-use audit;
- inline assembly and intrinsics use their own opcodes, not magic callee names.

Raw verification must reject legacy `CallArgAbiInfo`, `CallResultAbiInfo`, sret
slot names, result lanes, call move records, chosen source routes, physical
registers, stack offsets, and helper-name inference. `PreparedInput` requires
the complete typed aggregate/calling-convention semantic inputs enumerated by
its closed gate promise, but it still must not contain any computed fact.

## Memory, addresses, GEP, and atomics

Memory is expressed with typed pointer/value/object identities:

- load: pointer address space is legal, loaded type is sized/non-void, result
  matches, alignment is zero/default or a legal power of two, and volatile is a
  semantic access flag;
- store: stored value and memory element type agree, pointer/address space and
  alignment are valid, and there is no result;
- local/global/TLS/string/label address formation references a live entity of
  the right kind; TLS address formation preserves the declared address space
  and thread-local symbol attribute. A block-address/computed-goto constant is
  a typed `(FunctionId, BlockId)` semantic reference whose owners agree, not a
  printable label or ordinary data symbol; a generic pointer address uses a
  `ValueId`;
- GEP has pointer base, structured source element type, correctly typed indices,
  in-range struct indices, explicit inbounds semantics if present, and pointer
  result/address space. Dynamic array indices are allowed in Raw;
- pointer arithmetic and int-pointer round trips remain explicit operations;
  provenance/debug annotations may describe them but cannot authorize them;
- memcpy/memmove/memset have pointer operands, integer size, legal alignment,
  address spaces, overlap semantics, and no hidden prepared address plan;
- dynamic allocation has a sized element, integer count/byte size whose
  multiplication cannot overflow, legal alignment, and an explicit dynamic
  lifetime. Stack-save defines a distinguished function-local stack-state
  pointer result; stack-restore consumes a value derived only from a dominating
  matching save through token-preserving copy/phi operations. Stack-state
  pointers cannot escape through stores/calls/returns, participate in pointer
  arithmetic, or cross functions. Each restore closes the lifetime of dynamic
  objects allocated after that save on its outgoing paths; a use reached after
  closure without a new dominating allocation is invalid. Multiple restore
  sites are legal when CFG/lifetime analysis proves their paths coherent. No
  frame offset or physical SP decision appears in BIR.

Atomic rules include legal width/type, pointer/value agreement, alignment,
address space, and ordering matrix: load forbids release/acq_rel; store forbids
acquire/acq_rel; RMW has an opcode supported for its scalar domain; compare
exchange has compatible expected/desired and legal success/failure pairing
(failure is never release/acq_rel and is no stronger than success); fence uses a
fence ordering; result type matches old-value versus boolean-success semantics.
There must be one semantic atomic instruction, not an ordinary memory op plus a
parallel `atomic_operations` agreement table.

## Phi, SSA, dominance, and edges

BIR values always have exact single definitions even before the SSA construction
pass. “Pre-SSA Raw” means addressable variables can remain in memory and phi
insertion is not required; it does not permit ambiguous named definitions.

For every phi that does exist in Raw:

- phis form a contiguous prefix of the block;
- result and every incoming have one identical non-void type;
- each incoming is `(EdgeKey, Operand)`. Its key resolves to one live local
  `SuccessorSlot` whose target is the phi block;
- the incoming `EdgeKey` multiset equals the exact derived incoming-slot
  multiset (`PhiEdgeMultisetMismatch`). It is not one-per-predecessor-block:
  parallel same-destination switch/indirect/asm slots each require their own
  incoming, and no key may be omitted or duplicated;
- an incoming value is used on its labeled successor slot: its definition must
  dominate that edge's source terminator (or be a phi-edge-legal self/loop
  value), not the phi instruction text position;
- asm-goto predecessors participate exactly like other terminator predecessors;
- critical edges and loop-carried values are legal in Raw.

Non-phi use policy:

- parameters dominate every reachable block in their function;
- an instruction result used in its defining block must be earlier in instruction
  order; self-use is invalid except through a phi backedge;
- across reachable blocks, the defining block must dominate the use block;
- terminator operands are after all ordinary instructions in their block;
- unreachable components are checked using a synthetic-root dominator forest.
  Cross-component uses are invalid; within a component, ordinary dominance
  applies. For an unreachable SCC with no incoming edge from another
  unreachable SCC, the synthetic root connects to every block in that root SCC,
  avoiding an arbitrary chosen block that would falsely dominate its peers.
  This avoids accepting arbitrary uses merely because textbook entry dominance
  is undefined there.

Raw therefore already enforces valid SSA identity/use dominance for values that
exist. The future `Canonical` profile adds pipeline promises such as required
mem2reg coverage, absence of designated raw memory pseudo-ops, canonical phi
placement/order, and B3 / P03 entry reachability for every retained block.
Those are not retroactively required of Raw.

## Aggregates, intrinsics, variadics, and inline assembly

Aggregate operations verify complete type/path/size/alignment relationships.
Opaque records cannot be inspected by field; unions require an explicit chosen
representation; bitfield/vector lane indices and counts are in range; aggregate
copies have compatible sizes and address spaces. Semantic complex/multivalue
operations cannot be encoded as hidden physical return lanes.

Intrinsic opcodes are a closed registry. Each entry declares legal profiles,
target-independent feature class, operand/result schema, immediate constraints,
memory effects, and fallback/legalization requirement. Unknown intrinsic IDs and
magic call spellings fail. Runtime helper selection is later lowering policy.
Coverage includes overflow operations, bit operations, SIMD/vector operations,
CRC/crypto when represented with closed semantics, fences, memory intrinsics,
frame/return-address queries, and thread-pointer queries. A target-flavored
intrinsic retains an explicit required-feature tag and receives target support
validation before preparation; it never smuggles a selected machine opcode into
Raw BIR.

Variadic semantic operations (`va_start`, scalar/aggregate `va_arg`, `va_copy`,
`va_end`) validate function variadicness, pointer/list object types, result/object
type, size/alignment, and operation arity. Register-save areas, GP/FP offsets,
overflow areas, helper operand homes, and HFA register plans are forbidden.

Inline assembly has exactly one Raw representation: an ordinary instruction
whose inputs and outputs live exclusively in the generic operand/result graph,
plus `InlineAsmPayload` containing original opaque asm text, original opaque
constraint text, ordered clobber spellings, and the side-effect flag. A
read/write position is represented by an incoming use and a distinct result;
Raw verification never collapses their identities.

The Raw verifier checks the closed opcode/payload alternative, same-function
live operand ownership, exact instruction-result definitions and indices,
known value types, deterministic clobber storage, and presence of the original
payload required by the admitted bounded shape. It does not parse either text,
recover values from renderer fields, derive roles/ties/classes, validate target
register vocabulary, or rewrite the payload. Richer symbol/address-space and
asm-goto forms fail import until their ordinary typed value/CFG carriers are
defined; they are not smuggled into Raw storage as parsed side tables.

The later constraint-binding stage is the sole interpreter of original
constraint text. It binds that text to the exact generic operand/result order
using revision-bound target tables. Unknown target tokens fail there, never as
silently generic registers. Raw/Canonical BIR stores no parsed constraint
authority, allocated homes, spill slots, spill/reload nodes, target operations,
or rewritten assembly. Current LIR `insn_r` opcode/funct metadata and legacy
`unsupported_facts` are likewise not Raw semantic authority: the former is
later target data and the latter becomes a structured import failure.

## Debug and provenance are non-authoritative

Source locations, debug names, original spellings, comments, and provenance
records may be absent. If present, their IDs/ranges and parallel-array lengths
must be structurally valid, but deleting all of them cannot change whether the
semantic BIR verifies. Provenance cannot make an otherwise illegal GEP, symbol
reference, or memory access legal. Renderer output is never reparsed to recover
identity, type, CFG, or call facts.

## Stage-forbidden facts

Raw, Canonical, PreparedInput, and the D3/D4/initial-D5
`PseudoPublicationGate` reject allocation facts. The private
`AssignedAllocationCandidateGate` admits only exact revision-bound assignments
and explicit abstract spill state, plus bounded fixed-role frame-action nodes
after E4 materialization, and never publishes `PseudoBir`; Allocated adds exact
frame coverage and publication rules. Every gate rejects duplicate side-table
authority and MIR/emission facts, including:

- legacy Route1–Route8 producer/publication/comparison/memory/call indices,
  route/view pointers, agreement records, selected proof paths, and lookup
  agreement mirrors;
- concrete ABI registers, hidden side-record argument/result assignments, call
  boundary moves absent from ordinary Pseudo nodes, hidden sret storage
  selection, variadic entry homes, helper selection, or concrete incoming
  stack offsets; Pseudo/Allocated admit only their ordinary abstract fixed-home
  requirements and exact revision-bound assignments;
- concrete/virtual machine-register assignment and live intervals as persistent
  authority; pre-Allocated profiles also reject value homes, spill/reload
  instructions or objects, rematerialization recipes, and coalescing decisions;
- frame indices resolved to offsets, final stack size/alignment, prologue/
  epilogue decisions, callee-saved sets, and dynamic-stack realization in the
  semantic BIR graph or unkeyed side state; the E4-owned immutable private
  `FrameRealizationPlan` is the sole exact-revision exception;
- selected instruction encodings, target opcodes, relocation encodings, emitted
  object bytes as a second semantic initializer, or prepared printer records.

Analyses may cache derived data keyed by module/function revision outside core
IR. The verifier may recompute and compare a cache in debug mode, but a cache
never repairs or overrides core facts.

## Transactional construction and publication

The builder/pass transaction owns mutable candidate storage; `RawBir`,
`CanonicalBir`, and preparation input views expose only successfully published
immutable stage snapshots. Regardless of the final Step 8 API name, Raw
publication is one atomic operation:

1. the owning publication entry rejects active edit capabilities and freezes
   the candidate's exact revision;
2. it creates one read-only candidate view, checks reservation
   completeness, and runs the full Raw registry against that same revision;
3. if any error or revision change is observed, it returns
   move-only `PublicationFailure` retaining the complete structured report plus
   any preceding `BirError`, and publishes no `RawBir`; candidate IDs cannot
   resolve through any public stage view;
4. on success, it atomically consumes the frozen candidate and uses the private
   verifier token to create exactly one `RawBir`. Old IDs cannot resolve in a
   replacement module merely because slot numbers were reused.

Calling `verify_candidate` before publication is optional diagnostic work and
does not shorten or replace step 2. A green diagnostic report cannot be traded
for a token, cached across a revision, or supplied to a separate constructor.

Function edit callbacks are failure-atomic too. Returning `BuildError` or
throwing cannot leave appended blocks, instructions, results, symbol entries,
or order mutations in a candidate that the caller assumes was rolled back.
Implementations may use copy-on-write, an undo log, or a disposable child
builder, but the observable rule is commit-on-success. Nested capability tokens
expire on both commit and rollback.

Verification itself is side-effect free: it does not intern names, repair
indices, fill predecessor/use caches, canonicalize order, or rewrite malformed
payloads. Any repair is a separate explicit transaction followed by a new full
publication check. Concurrent mutation of the candidate invalidates the view;
the verifier reports `RevisionChanged` rather than accepting a mixed snapshot.

The checked-in entry is `ModuleBuilder::publish() &&`; the larger
`ModuleDraft`/`verify_and_publish_raw` surface above remains documentation-only.
Step 8 reconciles the landed final boundary rather than assuming either name.
The full Raw profile must not be declared implemented until the audited
importer, rollback behavior, and full verifier obligations are proven together.

## Incremental verification and mutation contracts

Full verification is required after LIR-to-Raw publication, before/after a pass
in debug/CI configurations, at Canonical publication, and at the
`PreparedInput` gate.
Incremental verification is for edit loops:

- value/operand edit: containing instruction schema, both definitions, all old
  and new users, type/dominance, and core def-use;
- instruction insertion/removal/move: old/new blocks, order membership, all
  results/users, same-block ordering, and dominance dependents;
- terminator/asm-goto edit: source and old/new successor neighborhoods, complete
  CFG, affected phis, reachability, and dominance;
- signature/symbol/global/type edit: module scope, because callers,
  initializers, and recursive types may be affected.

`MutationSummary` is produced by trusted builders and contains semantic IDs plus
the pre/post revision. Missing or stale summaries force full verification. A
local green result must never be advertised as module publication proof.

## Legacy-to-rule coverage

This table extracts semantic rules; it does not preserve legacy layout or
agreement side tables.

| Legacy source anchor | Rule adopted into new verifier | Disposition |
|---|---|---|
| `legacy/bir_validate.cpp::validate`, `validate_link_name_id`, `validate_global_link_name_matches_visible_name` | unique semantic symbols; IDs resolve and agree with owned declaration | Adopt by typed ID; reject name fallback |
| `legacy/bir_validate.cpp::validate_named_value`, `find_function`, `find_global` | symbol pointer references target declared function/global of correct kind | Adopt; no `@` stripping/text lookup |
| `legacy/bir_validate.cpp::validate_params` | parameters are unique exact definitions matching signature ordinal/type | Adopt and strengthen |
| `legacy/bir_validate.cpp::validate_phi` | typed incoming values and local predecessor identity | Adopt and strengthen to exact `EdgeKey`/incoming-`SuccessorSlot` multiset plus edge dominance; do not preserve legacy one-per-block identity |
| `legacy/bir_validate.cpp::validate_call` | direct/indirect callee shape, declared direct target, all args/callee/result traversed | Adopt; ABI/sret storage fields move later |
| `legacy/bir_validate.cpp::validate_local_slot_names`, `find_local_slot` | local object ID belongs to function, size/alignment/type valid | Adopt by `LocalId`; reject spelling agreement |
| `legacy/bir_validate.cpp::validate_load_local`, `validate_load_global`, `validate_store_local`, `validate_store_global` | memory entity exists, value/address/type/order valid | Adopt and generalize to typed memory ops |
| `legacy/bir_validate.cpp::validate_initializer_symbol_link_name` and global loop in `validate` | declaration/definition and recursive initializer symbol/object rules | Adopt and extend to relocation bounds/object data |
| `legacy/bir_validate.cpp::validate_terminator`, `validate_return` | local live CFG targets and typed condition/return | Adopt and extend to switch/indirect/unreachable/asm-goto |
| `legacy/bir.hpp::TypeKind`, `Value`, `Inst`, `Terminator`, `Module` | bounded alternatives and feature inventory | Coverage input only; schema is not target truth |
| `legacy/bir.hpp::MemoryAddress`, `AtomicOperation` | address/atomic payload must be coherent | Fold into semantic instructions; delete parallel atomic agreement table |
| `legacy/bir.hpp::CallArgAbiInfo`, `CallResultAbiInfo`, `Function::atomic_operations` and local-array/global-static route records | some early semantic facts are needed, but prepared/route authority is not BIR | Extract type/address/operation rules; reject side-table authority |
| `legacy/bir_route1.cpp::route1_build_producer_index` and `route1_find_same_block_scalar_producer` | exact definitions and complete uses make producer discovery deterministic | Replace with core def-use; no route index |
| `legacy/bir_route2.cpp::route2_build_select_chain_value_index` | select/cast/binary dependency traversal sees every operand | Replace with shared operand visitor |
| `legacy/bir_route3_memory.cpp::route3_build_memory_access_index` | memory effects, base identity, ranges, volatility/atomicity are explicit | Replace with instruction schema plus analysis |
| `legacy/bir_route4_publication.cpp`, `bir_route5_publication.cpp` | CFG-edge/phi value availability follows dominance and phi semantics | Replace with CFG/SSA verifier |
| `legacy/bir_route6_call_publication.cpp` | calls expose every argument/result use and semantic aggregate relationship | Replace with typed call schema; prepared source selection forbidden |
| `legacy/bir_route7_comparison.cpp`, `bir_comparison_view.cpp` | comparisons have typed operands/result and conditions use the result | Replace with opcode schema and def-use |
| `legacy/bir_control_flow_view.cpp`, `bir_select_dependency_view.cpp`, `bir_memory_access_view.hpp`, `bir_publication_view.hpp` | consumers require complete CFG/operand access | Satisfied by core views/analysis; views are not verifier inputs |
| `legacy/lir_to_bir.cpp::try_lower_to_bir_with_options` and string-pointer rewrite helpers | LIR publication must produce typed symbol/value identity before verification | Adopt publication gate; reject post-hoc spelling repair |
| `legacy/prealloc/prepared_contract_verifier.cpp::verify_prepared_decoded_home_storage_contract` | no ambiguous storage-kind payload | Prepared/MIR verifier rule, not Raw; Raw rejects home storage |
| `verify_prepared_call_boundary_move_contract` | call boundary data must be internally complete | Later prepared/MIR rule; Raw validates only semantic call |
| `verify_prepared_variadic_entry_plan_contract`, `verify_prepared_variadic_entry_helper_operand_homes_contract` | semantic variadic op prerequisites exist early | Raw adopts op/type/function rules; homes/plans remain later |
| `verify_prepared_rematerializable_integer_immediate_contract`, `verify_prepared_pointer_base_plus_offset_contract` | constants and pointer derivations are typed and exact | Raw adopts semantic constant/GEP rules; rematerialization/home decision later |
| `verify_prepared_selected_local_storage_contract`, `verify_prepared_selected_object_data_contract` | local/global object initialization is coherent before preparation | Raw owns initializer/object semantics; selection and prepared object data later |
| `classify_prepared_frame_slot_*_source_route_contract`, `classify_prepared_local_frame_address_materialization_source_route_contract` | source references cannot conflict or be missing | Raw exact ID/def-use replaces cross-route agreement; frame route is forbidden |
| `verify_prepared_call_argument_binary_producer_materialization_contract`, `verify_prepared_raw_call_argument_abi_coherence_contract` | argument producer/type is available and call semantics coherent | Raw def-use/call typing; ABI placement and materialization later |

## Reference compiler: adopt and reject

The second research source is
`ref/claudes-c-compiler/src/{ir,backend}`. It is a feature/consumer oracle, not a
schema to copy.

| Reference anchor | Adopt | Reject or strengthen |
|---|---|---|
| `ir/instruction.rs::{Instruction, Terminator, BasicBlock}` | closed instruction families; explicit phi, switch, indirect branch, and asm-goto feature coverage | raw `u32` IDs without owner/generation; public vectors as integrity boundary; strengthen reference mid-block asm-goto edges into a split-block terminator |
| `Instruction::used_values`, `Terminator::used_values` and `backend/liveness.rs::for_each_operand_in_instruction/terminator` | one exhaustive semantic operand traversal used across backend consumers | multiple hand-maintained visitors that can silently diverge; generate/share schema |
| `ir/analysis.rs::{build_label_map, build_cfg}` | predecessor/successor analysis is disposable and must include asm-goto control flow | silently ignoring unresolved labels or retaining hidden mid-block exits; new CFG is derived from typed terminators after import splitting |
| `ir/mem2reg/promote.rs` | phis at block prefix, incoming values filled per successor edge, dominance-based SSA construction | treating pass-produced shape as implicit correctness; publish through verifier |
| `ir/mem2reg/phi_eliminate.rs` | phi inputs are edge uses and critical edges may need splitting later | putting phi copies or target decisions in Raw verifier |
| `ir/module.rs::{IrModule, IrGlobal, GlobalInit, IrFunction}` | broad symbol/global/initializer/backend feature inventory and recursive reference traversal | name strings as semantic identity; target pointer size inside generic initializer size |
| `Instruction::ParamRef` and `Get/SetReturn*Second` | parameters and complex returns must remain explicitly representable | no parameter-load pseudo-op in Raw: parameters are definitions; no physical “second return register/lane” pseudo-op: calls and `ReturnTerm` use one optional aggregate-typed semantic operand until later lowering |
| `backend/generation.rs` and `backend/liveness.rs` | evidence that terminators, calls, GEP, phi, asm, intrinsics, atomics, variadics all need complete operand/CFG contracts | backend `unreachable!`, `panic!`, or forgiving lookups as validation strategy |
| architecture codegen `debug_assert!`/`unreachable!` patterns | convert assumptions about legalized op/type pairs into earlier named profile rules | release-build-only trust in assertions and target-specific validation of Raw |

## Backend coverage status ledger

`Contracted` means this design has stable rule IDs and a closed validation
contract; it does not claim implementation. An unimplemented rule does not
become optional: no adapter may advertise the full Raw or Canonical profile
until it enforces every required rule. `Outside current intake` means a desired
form has no alternative in the complete immutable current LIR; it does not
authorize LIR/schema work, an opaque substitute, or a skipped verifier rule.
Current facts instead retain their exact accepted phase-A receiving
dispositions. `Deferred stage` means the semantic Raw input is verified here
but the named decision belongs after BIR.

| Feature family | Status | Stable rules / disposition |
|---|---|---|
| module epoch/revision, deterministic order, stale/foreign IDs, instruction reservations | Contracted | `ModuleEpochInvalid`–`ActiveEditAtFreeze`; `VerifyEntity` embeds exact core `ModuleEntityId`/`FunctionEntityId`, publication requires every `ReservedInst` defined and every forward reference resolved |
| I1–I128, pointers/address spaces, arrays, records/unions, packed/opaque, vectors and functions | Contracted | `DataLayoutInvalid`–`TypeLayoutInvalid`; named-record diagnostics retain `TypeNameId` through `ModuleEntityId` |
| F32/F64 and long double | Contracted | `FloatFormatInvalid`, `TypeLayoutInvalid`, `ConstantPayloadInvalid`; explicitly distinguishes IEEE binary128 from x87 extended-80 semantic bits and padded 10/12/16-byte storage |
| constants, null/zero, `undef`, `poison`, scalar/cast/compare/select | Contracted | `ConstantKindInvalid`–`UndefPoisonRoleInvalid`, `OpcodeInvalid`–`ResultTypeMismatch` |
| linkage/visibility/sections/TLS/aliases/symver/constructors/destructors/top-level asm | Contracted | `SymbolIndexMismatch`–`TopLevelAsmDependencyInvalid`; `TopLevelAsmSpec::symbol_dependencies` is ordered typed authority and `source_text` is never parsed for identity |
| globals and recursive initializer/relocation/object data | Contracted Raw schema; current fields follow the accepted phase-A receiving dispositions and desired absent forms are outside this intake | `GlobalStateInvalid`–`BlockAddressInvalid`; `InitializerId` is carried by `ModuleEntityId`, and compatibility text never supplies identity |
| functions, signatures, attributes, parameters, `LocalId`, optional semantic return | Contracted | `FunctionShapeInvalid`–`ReturnOperandInvalid` |
| jump/conditional/switch/indirect/asm-goto/unreachable CFG | Contracted | `EdgeTargetInvalid`–`UnreachableBlock`; exact ordered `SuccessorSlot` authority preserves parallel destinations |
| phi, SSA, edge uses, dominance, unreachable components | Contracted | `PhiPlacementInvalid`–`CrossComponentUse`; exact incoming `EdgeKey` multiset |
| descriptor traversal, definitions and exact def-use | Contracted | `DescriptorMissing`–`StaleUse`; all value-bearing call bundles/asm/terminator payloads participate |
| direct/indirect/by-value/variadic calls, `CallEffects`, typed bundles, aggregate result | Contracted Raw schema; current fields follow the accepted phase-A receiving dispositions and desired absent forms are outside this intake | `CallCalleeInvalid`–`VarArgInvalid`; direct identity is `SymbolId` |
| local/static/dynamic allocation, stack save/restore, lifetime | Contracted | `LocalInvalid`, `DynamicAllocInvalid`, `DynamicSizeOverflow`, `StackStateInvalid` |
| load/store/GEP/address formation, memcpy/memmove/memset | Contracted | `MemoryAccessInvalid`–`MemoryIntrinsicInvalid` |
| atomic load/store/RMW/cmpxchg/fence | Contracted Raw schema; no current `LirInst` atomic alternative, so these forms are outside this intake | `AtomicTypeInvalid`–`FenceInvalid`; absence does not authorize LIR/schema work or a legacy parallel agreement table |
| aggregates, complex values, vector lanes/masks | Contracted | `AggregatePathInvalid`–`VectorMaskInvalid`; physical return lanes are deferred stage facts |
| semantic intrinsics, overflow, bit/memory/SIMD/CRC/crypto | Contracted; target support is deferred stage | `IntrinsicIdInvalid`, `IntrinsicSchemaInvalid`; no selected opcode/helper in BIR |
| opaque inline asm and asm-goto | Bounded non-goto generic SSA transport follows the accepted phase-A matrix; desired symbol/address-space/asm-goto forms absent from current LIR are outside this intake; parsed constraint objects belong only to the later constraint product | current `FoundationVerifier` uses `BoundedAlternative` and `ValueDefinition`; fuller payload/value-edge/clobber/effect and `AsmGotoPairInvalid` rules remain target rules |
| D5 phi destruction and edge copies | Contracted; implementation deferred | `PseudoCopyPlacementInvalid`–`PseudoCopyCoverageMismatch`; exact `EdgeKey` provenance, edge-local execution, simultaneous cycle-safe bundles, lowered assignment roles, and no residual phi semantics before E1 |
| debug files/scopes/locations and provenance origins | Contracted | `DebugReferenceInvalid`–`ProvenanceInvalid`; `DebugFileId`, `DebugScopeId`, `DebugLocId`, and `OriginId` arrive through `ModuleEntityId` and have zero semantic authority |
| ABI placement and abstract allocation/spill state | Contracted at the later Pseudo/Allocated boundaries; D2 and E1-E3 own production | semantic profiles use `ForbiddenStageFact` and `ForbiddenCompatibilityPayload`; `Allocated` requires exact same-revision product keys, complete assignments, explicit legal `Spill`/`Reload`, and fail-closed E4 publication |
| frame layout and explicit frame actions | Contracted at E4; implementation deferred | private deterministic draft, bounded action materialization, `FrameActionFingerprint`, final exact-current products, and atomic Allocated publication |
| target operation/relocation encoding/emission | Deferred from BIR | F1 applies registered mappings; encoding/emission is never BIR graph authority |

## Proof plan

1. **Rule unit negatives:** one minimal malformed module per `VerifyRule`, plus
   paired valid boundary cases. Assert structured rule/entity/location, never
   full rendered text.
2. **ID corruption matrix:** wrong epoch, wrong function owner, out of range,
   tombstone, stale generation, wrong kind, duplicate/missing order membership,
   and foreign block/value/global references.
3. **Opcode table properties:** generate every opcode with each legal and illegal
   arity/type/profile combination; require exhaustive registry coverage when an
   opcode is added.
4. **Def-use properties:** generate functions, recompute all uses independently,
   mutate one hidden operand role at a time (call callee/arg, phi edge, GEP,
   asm, intrinsic, terminator), and require exact mismatch detection.
5. **CFG/SSA properties:** random reducible/irreducible graphs, unreachable
   components, parallel same-destination successor slots, loops, critical edges,
   asm-goto, phi `EdgeKey` permutations/missing/extra entries, and dominance
   violations.
6. **Initializer fuzzing:** recursive object data with bounded depth, relocation
   overlap/out-of-bounds, symbol-kind mismatch, overflowed sizes, and string
   width/padding cases.
7. **Feature-family negatives:** direct/indirect/variadic calls, atomics ordering,
   aggregate paths, intrinsic immediates, and inline-asm cardinality/ties/goto.
8. **Mutation differential:** after random builder edits, compare incremental
   diagnostics to full verification after deterministic sorting.
9. **Parser-independent fuzzing:** mutate in-memory typed BIR, not textual dumps,
   under ASan/UBSan; verifier must return diagnostics without crash or hang.
10. **Legacy/reference corpus:** lower existing backend tests to Raw, verify,
    and maintain explicit expected failures for features not yet modeled. Never
    weaken a rule merely to accept a legacy side table.

## Closed A2 and Step 7 choices

The verifier consumes these settled core choices; they are not profile options:

- recursive named records reserve one module-owned `TypeNameId` and `TypeId`,
  define that reserved body's recursive references exactly once before draft
  freeze, and become immutable at draft finish;
- equal true/false conditional targets are legal Raw shape as two distinct
  role-keyed successor occurrences, and B3 / P03 owns their deterministic
  canonical fold;
- Raw/Canonical v1 has no local exception edge: `CallEffects::MayUnwind` permits
  escape from the current function only, while a source call requiring a local
  unwind destination fails import;
- core's immutable `SemanticDataLayout` owns byte order, pointer layouts, and
  floating semantic/storage layouts; resolved aggregate field offsets, size,
  and alignment are semantic `TypeId` facts checked against that environment,
  not ABI placement or target-instruction policy;
- `DebugFileId`, `DebugScopeId`, and `DebugLocId` own the debug graph, while
  `OriginId` is the sole non-authoritative import/pass provenance link;
  parallel debug or origin arrays are not semantic authority; and
- `users(ValueId)` is canonical eager core def-use state maintained by every
  builder/editor transaction. Verification independently recomputes and
  compares it; a revision-bound analysis may cache other reference indexes but
  cannot replace core value def-use.

Step 7 closes the two preparation-facing choices:

1. `verify_preparation_input` is the only target-bound C1 input gate. It
   rechecks the exact B8 `CanonicalBir`, binds its full `PipelineStageStamp` to
   one validated `TargetFingerprint` supplied by the [external C1 selection
   authority](../../../target_profile/README.md), and proves only the complete
   typed semantic prerequisites enumerated above. It is non-mutating, publishes no
   BIR revision or prepared fact, and may not stand in for C2-C9.
2. C9's sole public interpretation API is the all-module `bind_constraints`
   transaction defined by `regalloc/constraints`. It consumes the exact
   Canonical snapshot, validated target, `VerifiedPreparationInput`,
   `VerifiedTargetLayout`, and C3-C8 `VerifiedPreparationBundle`; reads original
   descriptions and ordinary identities from that snapshot; and publishes one
   immutable `BoundConstraintSet` or nothing. C7 remains tables-only.
3. C9's subordinate `ConstraintProjectionTransaction` is the sole shared
   later-revision projection/preservation authority. D1, D2, every D4
   occurrence, initial D5, every E3 retry, and the final E4 frame-action
   materialization invoke it inside failure-atomic transactions. Private D5
   copy resolution contributes its mutation summary to that E4 projection and
   does not publish a separately current projection. Each verifier consumes only
   the resulting `ProjectedConstraintSet` keyed to the exact candidate stamp;
   Canonical C9, predecessor products, stable IDs, structural equality, and
   copied records are not later-revision freshness proof.

## Research anchors inspected

- New partial implementation: `bir/core/{ids,storage,type,ir}.hpp`,
  `bir/verify/{verifier.hpp,verifier.cpp}`.
- Legacy Raw/schema and validator: `legacy/bir.hpp`, `bir_private.hpp`,
  `bir_validate.cpp`, `lir_to_bir.cpp`.
- Legacy consumers: control-flow, memory-access, publication, comparison,
  select-dependency, call-boundary views and Route1–Route8 implementation files.
- Legacy prepared boundary: `prealloc/prepared_contract_verifier.{hpp,cpp}` and
  its decoded-home, call-move, variadic, object/storage, frame-source,
  materialization, and ABI-coherence contracts.
- Reference compiler: `ref/claudes-c-compiler/src/ir/{instruction,module,
  analysis}.rs`, `ir/mem2reg/{promote,phi_eliminate}.rs`, and backend
  `liveness.rs`, `regalloc.rs`, `generation.rs`, plus architecture codegen
  invariant patterns.
