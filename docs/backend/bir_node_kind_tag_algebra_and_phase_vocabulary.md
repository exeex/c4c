# BIR NodeKind Tag Algebra and Phase Vocabulary

Status: Normative contract in progress (idea 801, Step 2)

## 1. Authority, scope, and terminology

This artifact defines the closed classification algebra for BIR `NodeKind`.
All phases use the same flat node record, graph, arena, and stable-ID model.
An enum value does not inherit from another enum value: inheritance-like
classification means that one hidden schema entry selects one reviewed member
from each exclusive group and zero or more members from explicitly composable
groups. There is no free-form bag of boolean properties.

The schema entry is the sole authority for kind classification. A descriptor,
compile-time trait, runtime query, verifier check, or pass admission test is a
derived view of that entry, never an independent table. The query mechanics
are intentionally pending Step 3; this step fixes the meanings they must
preserve.

Terms used below are normative:

- **kind**: one value in the closed `NodeKind` vocabulary;
- **schema entry**: the one authoritative declaration assigning a kind its six
  axis records and payload alternatives;
- **axis**: a finite classification dimension with named exclusive and
  composable groups;
- **published stage**: the stage claimed by a successfully verified graph;
- **admitted**: legal in a particular published-stage vocabulary;
- **static classification**: a fact about a kind, optionally qualified by a
  stage, not evidence about a graph instance;
- **dynamic validity**: a verifier-proved fact about nodes, edges, definitions,
  uses, dominance, and publication state in one graph instance.

Adding an axis, member, composable modifier, or previously unapproved
combination is a schema-contract change. Review must state its semantics,
cross-axis requirements and prohibitions, publication stages, verifier rules,
and focused positive and negative proof. Unknown values and incomplete schema
entries are invalid; no default category or all-stage admission is inferred.

## 2. Closed validation model

Every kind has exactly one `KindSchema` record with these required subrecords:

1. `ValueModel`;
2. `SemanticFamily`;
3. `EffectControl`;
4. `StageVocabulary`;
5. `ShapePolicy` for operands, results, type, and payload;
6. `MirRealizability`.

Validation is structural and relational. Structural validation requires one
member of every exclusive group, validates every finite set against its closed
enum, rejects duplicate or unknown members, and rejects an empty stage
admission set. Relational validation evaluates the required and forbidden
combinations listed by all six axes. A schema entry is not available to any
query, pass, or verifier unless this validation succeeds.

Composable tags are permitted only where this document names a composable
group. Even there, they are not arbitrary: their prerequisites and conflicts
are validated. Useful predicates such as `Pure`, `ValueProducing`, or
`MirReady` should be derived from axis records when derivation is exact, rather
than stored as drift-prone duplicate booleans.

Extension review must answer all of the following:

- which existing exclusive member or new reviewed member is selected on every
  axis;
- which composable members are present and why their prerequisites hold;
- which published stages admit the kind, with no implicit carry-forward;
- how operand roles, result shape, concrete type, and payload agree;
- how effects/control and MIR disposition agree with the semantic family;
- which invalid-combination and stage-admission tests fail closed.

## 3. Axis 1: value model and SSA participation

### Exclusive groups

Every kind selects exactly one `ResultForm`:

- `NoOrdinaryResult`: produces no ordinary value;
- `SingleOrdinaryResult`: produces one ordinary typed value;
- `AggregateMultiResult`: produces one aggregate value whose components may
  later be projected; it is not a generic unbounded result vector;
- `ProjectionResult`: produces one ordinary value by projecting a component
  from an aggregate/multi-output semantic source.

Every kind also selects exactly one `SsaParticipation`:

- `NeverSsa`: not governed as an ordinary SSA definition;
- `SsaEligible`: participates in ordinary SSA only when the kind is admitted
  to an SSA-governed published vocabulary.

These two exclusive groups are composable with each other and with the other
five axes. There is no independent `IsValue`, `IsSsa`, or `NonSsa` boolean.
`ValueProducing` is derived from `ResultForm != NoOrdinaryResult`.

### Required and invalid combinations

- `SsaEligible` requires `SingleOrdinaryResult`, `AggregateMultiResult`, or
  `ProjectionResult`; `SsaEligible + NoOrdinaryResult` is invalid.
- `NeverSsa` is required for stores, control-only terminators, allocation/frame
  actions without an ordinary result, and machine operations whose outputs are
  physical/virtual register effects rather than ordinary BIR SSA values.
- `ProjectionResult` requires a projection/extract semantic family or an
  explicitly reviewed equivalent and a result-count policy of exactly one.
- `AggregateMultiResult` requires an aggregate result type policy and cannot
  mean several unrelated entries in generic result storage.
- A result-count policy and `ResultForm` that disagree are invalid.
- A Raw/Canonical-shared kind for which SSA participation changes at B4 cannot
  carry an unqualified timeless `SsaEligible` answer; it must use the
  stage-qualified rule in Section 9 or split into distinct Raw and Canonical
  kinds.

### Extension rule

A new value form or SSA participation state requires a contract revision. A
new kind using existing members must prove result/type-policy agreement and
state whether any stage-qualified SSA answer is required.

## 4. Axis 2: semantic family

### Exclusive group

Every kind selects exactly one `PrimarySemanticFamily`:

- `Arithmetic`, `Compare`, `Conversion`, `Memory`, `Aggregate`, `Call`,
  `Control`, `PhiMerge`, `Authority`, `Intrinsic`, `Preparation`, `Pseudo`,
  `AllocationAction`, or `Machine`.

This is semantic ownership, not a claim that only an `Arithmetic` kind is
"semantic." Memory and call operations are semantic operations too; the old
`NodeFamily::Semantic` versus `Memory`/`Call` distinction is not preserved.

### Composable refinements

The closed `FamilyRefinement` set is:

- `BinaryForm`, `UnaryForm`, `TernaryForm`;
- `LoadForm`, `StoreForm`, `AddressForm`;
- `DirectCallForm`, `IndirectCallForm`;
- `ConditionalControl`, `UnconditionalControl`, `ReturnControl`;
- `ExtractForm`, `InsertForm`;
- `TargetSpecific`, `AbiSpecific`.

Zero or more refinements may compose with the primary family only where their
name is semantically applicable. Refinements classify stable operation shape;
they do not replace operand-role or effect records.

### Required and invalid combinations

- `BinaryForm` requires an operand-role schema with two semantic input roles;
  it does not itself imply purity, arithmetic, or SSA eligibility.
- load/store/address refinements require `Memory`; direct/indirect call
  refinements require `Call`; control refinements require `Control`;
  extract/insert refinements require `Aggregate`.
- `PhiMerge` requires predecessor-qualified incoming operand roles and cannot
  be a terminator.
- `Preparation`, `Pseudo`, `AllocationAction`, and `Machine` require compatible
  stage ownership; they cannot be admitted to Raw or Canonical merely because
  they share the same node storage.
- `Machine` requires `TargetSpecific`; `AbiSpecific` requires
  `TargetSpecific`. Target-specific semantic kinds in an earlier stage require
  an explicit exception and stage contract, never inference.
- Conflicting shape refinements such as unary plus binary, load plus store, or
  conditional plus unconditional are invalid.

### Extension rule

New primary families or refinements require explicit review. A new kind in an
existing family must prove that no existing refinement is being overloaded
and that effect, roles, stage, and MIR disposition are independently stated.

## 5. Axis 3: effects and control

### Exclusive groups

Every kind selects exactly one `MemoryEffect`:

- `NoMemoryEffect`, `ReadsMemory`, `WritesMemory`,
  `ReadsAndWritesMemory`, or `PayloadRefinedMemory`.

Every kind selects exactly one `TrapBehavior`:

- `CannotTrap`, `MayTrap`, or `PayloadRefinedTrap`.

Every kind selects exactly one `ControlBehavior`:

- `FallsThrough`, `TerminatorNoSuccessor`, `TerminatorOneSuccessor`,
  `TerminatorTwoSuccessors`, or `TerminatorVariableSuccessors`.

### Composable tags and derived facts

The closed composable effect tags are `CallLike`, `HasVolatileSemantics`,
`HasAtomicSemantics`, and `HasOrderingSemantics`. Each requires a payload and
semantic-family rule that defines its refinement. `Pure` is derived, not
stored: it means `NoMemoryEffect + CannotTrap + FallsThrough`, no composable
effect tag, and no other externally observable effect defined by the schema.

`PayloadRefinedMemory` and `PayloadRefinedTrap` mean the kind establishes a
conservative upper bound and the payload may select a narrower reviewed case.
Payload cannot erase the kind's dispatch authority or produce an effect
outside that upper bound.

### Required and invalid combinations

- Store form requires a write-containing memory effect; load form requires a
  read-containing memory effect.
- A non-control family with a terminator control behavior is invalid. A
  `Control` terminator requires an exact successor count/role policy matching
  its control behavior.
- `CallLike` requires `Call` or a reviewed intrinsic/pseudo/machine call form
  and cannot be classified `Pure` by payload omission.
- volatile requires a memory effect; atomic or ordering semantics require a
  compatible memory/control operation and exact payload policy.
- `NoMemoryEffect` with store form, or `FallsThrough` with a terminator role
  schema, is invalid.

### Extension rule

New observable effects or control shapes require contract review and verifier
obligations. They cannot enter as generic flags. Payload-dependent refinement
must name its conservative kind-level bound and closed payload alternatives.

## 6. Axis 4: stage vocabulary

### Exclusive publication stage and explicit admission set

A graph publication has exactly one `PublishedStage`:

- `Raw`, `Canonical`, `Prepared`, `PseudoPreallocation`, `Allocated`, or
  `MirReadyMachine`.

Each kind schema contains a non-empty finite `AdmittedStages` set drawn from
those values. Set members compose, but there is no `AllStages` default and no
implicit inheritance from an earlier to a later stage. A pass checks the graph
publication and kind admission together.

Each kind additionally selects exactly one `StageOwner`, using the same six
values, meaning the stage where its semantics first become authoritative.
Admission before `StageOwner` is invalid. Admission after it is allowed only
when a later boundary explicitly retains that kind unchanged; Step 4 will
enumerate those transition decisions.

### Required and invalid combinations

- `Preparation`, `Pseudo`, `AllocationAction`, and `Machine` families require
  owners `Prepared`, `PseudoPreallocation`, `Allocated`, and
  `MirReadyMachine`, respectively, unless a reviewed family-specific contract
  names a stricter later owner.
- A kind with `Machine` family or machine-only realizability is invalid in Raw,
  Canonical, Prepared, or PseudoPreallocation admission.
- Canonical SSA publication may admit `SsaEligible` kinds only subject to the
  B4 dynamic proof in Section 9.
- Empty admission, an unknown stage, admission before ownership, and a broad
  all-stage fallback are invalid.
- Shared node storage, payload compatibility, or stable arena identity is not
  evidence for cross-stage admission.

### Extension rule

Adding a stage changes this contract. Adding a stage to a kind's admission set
requires an explicit retain/lower rule, verifier obligations, and proof that
the kind's semantics, roles, results, effects, and ownership remain exact.

## 7. Axis 5: operand, result, type, and payload policy

### Exclusive groups

Every kind selects exactly one member from each group:

- `OperandArity`: `FixedOperands` or `VariableOperands`;
- `ResultCount`: `ZeroResults`, `OneResult`, or `AggregateResult`;
- `ConcreteTypeSource`: `NoResultType`, `StoredValueType`,
  `OperandDerivedType`, `PayloadDerivedType`, `SignatureDerivedType`, or
  `ConstraintResolvedType`;
- `PayloadShape`: `NoPayload`, `OneClosedPayload`, or
  `ClosedPayloadAlternatives`.

`FixedOperands` carries an exact count. `VariableOperands` carries finite
minimum/maximum bounds or a reviewed unbounded upper marker. Every operand
position or repeated range belongs to a closed `OperandRoleSchema`; raw indices
without roles are invalid. Payload alternatives are a closed set owned by the
same schema entry.

### Composable policy qualifiers

The closed qualifiers are `PredecessorQualifiedOperands`,
`CommutativeOperands`, `TiedOperandResult`, and `VariadicCallOperands`. They
compose only with an exact role schema and applicable family.

### Required and invalid combinations

- Result count must agree with Axis 1: no ordinary result maps to zero;
  single/projection maps to one; aggregate-multi maps to aggregate result.
- `NoResultType` requires zero results; every value-producing form requires a
  concrete type source that is not `NoResultType`.
- `PredecessorQualifiedOperands` requires `PhiMerge`; variadic-call operands
  require call-like semantics; tied operands/results require a value-producing
  form and exact role pairs.
- Payload-derived facts require a closed payload shape. `NoPayload` cannot be
  combined with payload-derived type/effect refinement.
- Fixed arity whose exact count disagrees with its role schema, variable arity
  without repeated roles/bounds, or result/type disagreement is invalid.

### Extension rule

New arity, role, result, type-source, payload-shape, or qualifier semantics
require review. A new kind must define roles and type authority without
reconstructing them in a pass-local table.

## 8. Axis 6: MIR realizability

### Exclusive group

Every kind selects exactly one `MirDisposition`:

- `OneRecordRealizable`: can correspond to one MIR-ready machine record once
  its required target facts exist;
- `RequiresExpansion`: must become multiple later-stage nodes/records;
- `RequiresAllocationOrFrameFacts`: cannot become MIR-ready until allocation,
  spill, frame, or equivalent exact products exist;
- `MachineOnly`: is already a machine-vocabulary operation and is forbidden
  before the machine stage;
- `ForbiddenAtMirBoundary`: must disappear, project, merge, or be rejected
  before MIR-ready publication.

This group composes with the other axes. `MirReady` is derived from machine
stage admission plus a disposition and verifier result; it is not a free tag.

### Required and invalid combinations

- `MachineOnly` requires `Machine + TargetSpecific`, stage owner and admission
  at `MirReadyMachine`, and `NeverSsa` for ordinary BIR SSA participation.
- `RequiresAllocationOrFrameFacts` requires a Prepared, PseudoPreallocation,
  or Allocated owner and cannot be published MIR-ready without the named
  product proof.
- `ForbiddenAtMirBoundary` cannot include `MirReadyMachine` in admission.
- `OneRecordRealizable` does not itself permit early machine admission and does
  not promise identity preservation through lowering.
- A kind admitted to MIR-ready publication while still requiring expansion or
  missing allocation/frame facts is invalid.

### Extension rule

New dispositions require a contract revision. A new kind must identify its
disposition even if MIR lowering is not implemented; `unknown` is not a
permitted production default.

## 9. Static SSA classification versus B4 graph proof

`SsaEligible` answers exactly this static question:

> When this kind is admitted to an SSA-governed published vocabulary, is its
> ordinary result required to participate in ordinary SSA rules?

It does not assert that a node has one definition, that every use resolves,
that definitions dominate uses, that phi incoming values correspond to exact
predecessor edges, or that a graph has successfully reached B4. Those are
dynamic properties of a graph instance. B4 establishes them and the Canonical
publication verifier proves them before claiming SSA-valid Canonical BIR.

Accordingly, these APIs must remain conceptually separate:

- a kind query, such as stage-qualified `is_ssa_eligible(kind, stage)`, returns
  classification/admission only;
- a graph verifier result, such as `verify_b4_ssa_publication(graph)`, returns
  dynamic validity only after checking definitions, uses, dominance, phi
  edges, and the full publication contract.

No helper named `is_op_ssa`, `is_ssa`, or similar may ambiguously answer both.
A timeless compile-time `is_ssa_eligible_v<K>` is allowed only if the schema
proves that every admitted occurrence of `K` has the same eligibility. If a
kind exists in Raw before SSA establishment and Canonical after B4 with a
different static participation meaning, the implementation must either:

1. expose a stage-qualified query derived from the kind's stage record; or
2. split it into distinct Raw and Canonical `NodeKind` values whose timeless
   classifications are exact.

Choosing between qualification and splitting is a per-kind schema decision;
convenience cannot justify a false timeless answer. Even after a split,
`SsaEligible` still does not replace dynamic B4 verification.

`Phi` illustrates the boundary: its family and predecessor-qualified role
schema can be static, and its Canonical admission can be `SsaEligible`, but
only B4 graph proof establishes exact predecessor coverage, incoming use-def
validity, dominance semantics, and a valid SSA publication.

## 10. Representative algebra examples (non-enumerative)

These rows illustrate validation only. They neither add production kinds nor
define the complete future vocabulary.

| Representative | Selected algebra facts | Important constraint |
| --- | --- | --- |
| Existing `Binary` | single ordinary result; stage-qualified SSA eligibility; arithmetic with `BinaryForm`; pure derived from no memory/no trap/fallthrough; fixed two semantic inputs; operand-derived or stored concrete type | Raw versus post-B4 eligibility must be stage-exact; binary shape alone implies neither purity nor SSA validity. |
| Existing `Store` | no ordinary result; never SSA; memory with `StoreForm`; write-containing effect; fixed value/address roles; no result type | A store cannot acquire a result or become pure through payload refinement. |
| Existing `Phi` | single ordinary result; Canonical `SsaEligible`; `PhiMerge`; no memory effect; predecessor-qualified variable inputs; stored/constraint-checked type | Static tags do not prove predecessor coverage or graph SSA; B4 does. |
| Contract-only prepared category | preparation family; Prepared owner/admission; never SSA or stage-qualified eligibility as its exact result contract requires; explicit MIR disposition | This is a proof category, not a future production enum name; it cannot leak into Raw/Canonical. |
| Contract-only pseudo category | pseudo family; PseudoPreallocation owner/admission; exact roles/effects; explicit expansion or allocation disposition | It requires an explicit later transition; shared storage gives no catch-all retention. |
| Contract-only machine category | machine plus target-specific; MachineOnly; MirReadyMachine owner/admission; never ordinary BIR SSA | Physical/register effects are not ordinary SSA results, and machine admission is fail-closed before F. |

## 11. Single schema and query authority

### 11.1 One closed C++17 inventory

The implementation has exactly one authoritative per-kind inventory. Its
conceptual value type is the validated `KindSchema` from Section 2:

```cpp
struct KindSchema {
  NodeKind kind;
  ValueModel value;
  SemanticFamily family;
  EffectControl effects;
  StageVocabulary stages;
  ShapePolicy shape;
  MirRealizability mir;
  PayloadAlternativeSet payloads;
};
```

In C++17 the preferred representation is one hidden `inline constexpr` closed
registry of these records. A constexpr lookup by `NodeKind` serves both
template queries and runtime wrappers. The public enum must expose a closed
count/sentinel or another compile-time completeness proof so validation can
show that every known enum value occurs exactly once and no registry entry is
unknown.

If enum spelling or ABI constraints make that representation impractical, one
internal X-macro kind registry may emit both the enum inventory and the
constexpr `KindSchema` registry. That macro is C++ implementation plumbing,
not TableGen, a `.td` file, code generation, reflection, or a pass-visible DSL.
It must contain each kind and its schema entry once. It may mechanically emit
lookup cases or array rows; authors must not maintain a second list beside it.

Both forms enforce the same rule: there is one kind inventory and one schema
entry per kind. Separate hand-written compile-time specializations plus a
runtime switch, a copied descriptor array, verifier-local classifications, or
pass-local kind lists are prohibited even if tests currently keep them equal.
The two repeated landed-746 dispatch inventories are migration evidence, not a
contract to preserve.

The registry and its type-level adaptation live in the schema implementation
namespace. Passes see neither the registry, X-macro, typelist, specialization
machinery, bit encoding, nor validation implementation.

### 11.2 Validation precedes queryability

A `consteval` facility is unavailable in C++17, so the complete registry is
checked by `constexpr` validation plus namespace-scope `static_assert`. The
validation pipeline is ordered:

1. prove the registry contains every known `NodeKind` exactly once and no
   unknown kind;
2. validate every finite enum/set member and the structural rules in Section
   2, including a non-empty explicit admission set;
3. validate each axis's required, forbidden, and derived combinations from
   Sections 3 through 8;
4. validate payload alternatives, arity bounds, operand roles, result form,
   concrete-type source, effects, stage owner/admission, and MIR disposition as
   one entry;
5. prove any emitted compact runtime descriptor is a projection of that entry,
   never separately authored data.

Only a registry for which the aggregate `static_assert` succeeds may define
the query templates or runtime view. A deliberately invalid schema entry must
fail compilation in a validation proof; production code cannot represent
"invalid but queryable." This is how descriptor drift and free-form tag
combinations fail closed before execution.

### 11.3 Derived compile-time surface

The stable pass-facing compile-time surface is conceptually:

```cpp
template<NodeKind K>
inline constexpr KindSchemaView node_kind_schema_v = /* validated registry */;

template<NodeKind K, class Tag>
inline constexpr bool node_has_tag_v = /* derived from schema view */;

template<NodeKind K, PublishedStage S>
inline constexpr bool node_admitted_in_v = /* stages.admitted */;

template<NodeKind K, PublishedStage S>
inline constexpr bool is_ssa_eligible_in_v =
    node_admitted_in_v<K, S> && node_has_tag_v<K, SsaEligibleTag>;
```

`Tag` is a closed schema tag type whose axis and member are known at compile
time. Unknown tag types are ill-formed rather than false. Named helpers are
thin derived aliases for common semantic questions, including at minimum:

- `is_value_producing_v<K>` and `has_ordinary_result_v<K>`;
- `is_ssa_eligible_in_v<K, S>`;
- `is_memory_op_v<K>`, `is_call_like_v<K>`, and
  `is_terminator_v<K>`;
- `may_read_memory_v<K>`, `may_write_memory_v<K>`, and `may_trap_v<K>`;
- `node_admitted_in_v<K, S>`;
- `requires_expansion_v<K>`, `requires_allocation_or_frame_v<K>`, and
  `is_machine_only_v<K>`.

A timeless `is_ssa_eligible_v<K>` may exist only where registry validation
proves the answer invariant across every admitted stage, as required by
Section 9. No named helper stores another fact: each is a formula over the
same schema view and tag semantics.

### 11.4 Derived runtime surface

Runtime code receives a small value-oriented API:

```cpp
std::optional<NodeKindSchemaView> node_kind_schema(NodeKind) noexcept;
bool node_has_tag(NodeKind, NodeTag) noexcept;
bool node_admitted_in(NodeKind, PublishedStage) noexcept;
bool is_ssa_eligible_in(NodeKind, PublishedStage) noexcept;
```

It also receives runtime counterparts for the named helpers above, such as
`is_value_producing`, `is_memory_op`, `is_call_like`, `is_terminator`,
`may_read_memory`, `may_write_memory`, `may_trap`, `requires_expansion`, and
`is_machine_only`. Each wrapper performs one checked lookup in the same
validated registry and applies the same derived predicate used by its
compile-time counterpart.

`NodeKindSchemaView` is immutable and pass-facing. It exposes reviewed axis
values and closed role/payload views, not template types or mutable registry
storage. If a compact descriptor is useful, it is mechanically projected from
`KindSchema` during constant evaluation; there is no descriptor initializer to
edit independently.

Payload admission follows the same rule. Each schema entry owns its closed
payload alternative set and the predicate/visitor needed to test the current
closed variant. Operand bounds and roles, result/type policy, effects, and
stage admission come from that same entry. The runtime payload checker may be
mechanically emitted from the single registry, but a second manually repeated
kind switch is forbidden.

### 11.5 Fail-closed semantics

- An unknown runtime `NodeKind`, `NodeTag`, or `PublishedStage` yields no schema
  view and every boolean query returns `false`.
- An unknown compile-time kind or tag is ill-formed; it cannot instantiate a
  permissive primary template.
- An invalid schema or incomplete/duplicate inventory fails the aggregate
  compile-time validation before queries exist.
- A runtime descriptor/view cannot drift because it is a projection of the
  validated entry; an independently initialized descriptor is forbidden.
- A known kind at a stage absent from its explicit admission set is rejected.
- Payload, arity, role, result/type, effect, and MIR checks reject values not
  admitted by the entry; payload refinement cannot broaden kind-level bounds.
- Classification never authorizes implicit pass-through. Step 4 must give each
  accepted input an explicit transition rule; an accepted kind with no rule,
  or an input outside the accepted set, is an unhandled-transition failure.

The runtime API must distinguish an unknown value from a known negative fact
where diagnostics need that distinction, using the optional schema view or a
closed error result. Boolean convenience wrappers remain fail-closed.

### 11.6 Bounded feasibility proof categories

If Step 6 requires a C++ proof, it is limited to these categories:

- existing semantic representatives: `Binary`, `Store`, and `Phi`;
- one contract-only prepared schema entry;
- one contract-only pseudo schema entry;
- one contract-only machine/MIR-ready schema entry.

The contract-only entries may live in a compile-time proof fixture rather than
the production `NodeKind` enum. They prove multi-axis validation and paired
compile-time/runtime derivation without inventing production names, a complete
future vocabulary, or any phase pass. The proof must cover invalid
combinations, unknown runtime kind/tag/stage, illegal stage admission, and
compile-time/runtime agreement. Production logic must not recognize testcase
names or special-case these representatives.

## 12. B-through-F vocabularies and transitions

### 12.1 Group notation and product separation

The names in this section are closed schema groups, not proposed production
enum spellings. A group is a reviewed predicate over the six axes; every known
kind admitted by a phase belongs to exactly one row for that phase. Adding a
kind to a group still requires its schema entry, explicit stage admission, and
the row's exact outcome. A group cannot mean "all other kinds."

Node vocabulary and exact-revision products are different authorities:

- node groups classify operations stored with the shared `Node`/graph model;
- analysis products are immutable recomputable semantic facts keyed by exact
  function/module revision;
- preparation/allocation/frame products are immutable target-keyed facts tied
  to the exact Canonical graph revision and prior product identities;
- products may constrain or select a later lowering, but do not silently add
  tags to, or mutate, their source nodes.

Every boundary is transactional. It accepts only the input groups listed below,
uses only the named prerequisite products, and publishes only after its named
verifier succeeds. "Retain" means an explicit row proved all six-axis semantics
unchanged; shared storage or a stable arena slot never implies retention.

### 12.2 Phase B — target-independent canonicalization

Phase B consumes a verified Raw publication and produces verified Canonical
BIR. Its closed input groups are:

- `B.RawValueSemantic`: target-independent arithmetic, compare, conversion,
  aggregate, memory, call, intrinsic, or authority operations with an ordinary
  result, including documented raw forms;
- `B.RawEffectSemantic`: target-independent zero-result memory/call/intrinsic/
  authority operations;
- `B.RawControl`: terminators and branch/return forms whose successor roles are
  explicit or are documented Raw forms to normalize;
- `B.RawPhiMerge`: phi/merge candidates and their predecessor-qualified inputs;
- `B.RawImportOnly`: documented import/legalization scaffolding that must
  lower, project, merge, or disappear before Canonical publication.

Its closed output groups are:

- `B.CanonicalSsaValue`: target-independent ordinary value definitions governed
  by B4 SSA rules;
- `B.CanonicalEffect`: target-independent non-value semantic effects;
- `B.CanonicalControl`: exact terminators with canonical successor roles;
- `B.CanonicalPhiMerge`: canonical predecessor-qualified SSA merges;
- `B.CanonicalOpaqueTargetToken`: only a target-independent opaque semantic
  intrinsic/inline-assembly token; target constraints and realization are not
  present.

B requires verified Raw shape/type/ownership, contracted editors, exact
def-use/CFG analyses keyed to the current revision, and family-specific
legalization facts. It cannot use target ABI, allocation, frame, or instruction
selection products. The Canonical publication verifier checks legal kinds,
payloads, roles, types, exact def-use, terminator-derived CFG, and B4
single-definition/dominance/phi-edge SSA validity.

### 12.3 Phase C — immutable target preparation

Phase C reads a verified Canonical publication without mutating its semantic
nodes. Its admitted input groups are the five `B.Canonical*` output groups.
They are explicitly admitted as immutable `C.CanonicalReference` groups with
the same value, family, effect/control, shape, and MIR-disposition facts. This
is later-stage admission by reference, not retagging or a cloned node graph.

Phase C publishes a `PreparedBir` envelope containing that exact Canonical
revision plus external products:

- `TargetContext`/data-layout identity;
- `AbiPlan` and `CallPlan` for parameter/result classification, register/stack
  assignments, varargs/byval/sret and call moves;
- `AddressPlan` for target address materialization choices;
- preparation constraint/selection facts that are not yet allocation, frame,
  or final machine instructions.

There is no mandatory new C-owned production node group. If a future design
needs a preparation operation, it must enter the closed `C.PreparedAction`
group with `Preparation` family, Prepared ownership/admission, exact roles and
effects, and an explicit C-to-D row; it cannot mutate or override a Canonical
semantic node. This group is contract-only until separately implemented.

The Prepared publication verifier requires a valid Canonical token, exact
canonical/module revisions, target identity, internally complete and mutually
consistent typed plans, and absence of writes back into Canonical storage. It
rejects allocation/frame results or machine kinds published prematurely.

### 12.4 Phase D — pseudo formation and out-of-SSA

Phase D consumes immutable `C.CanonicalReference` groups, optional reviewed
`C.PreparedAction`, and exact Prepared products. Its output node groups are:

- `D.PseudoValue`: target/preallocation pseudo operations with explicit virtual
  register-like def/use roles and `NeverSsa` ordinary-BIR participation;
- `D.PseudoEffect`: memory, call, intrinsic, or target preparation pseudos with
  exact effects and constraints;
- `D.PseudoControl`: target/preallocation control pseudos with exact successor
  and call/return roles;
- `D.ParallelCopy`: out-of-SSA copies scheduled on exact predecessor edges;
- `D.ExpansionPlaceholder`: reviewed pseudos that must expand before F and name
  their exact expansion contract.

Phase D requires the Canonical B4 proof, CFG/dominance and phi-edge analyses at
the exact Canonical revision, Prepared target/ABI/call/address plans, and a
parallel-copy/critical-edge plan. Its publication verifier proves that no
`PhiMerge` remains, no ordinary SSA-only contract survives, copy cycles and
critical edges are represented exactly, pseudo roles/effects/constraints are
valid, and every retained source reference is diagnostic/provenance only.

### 12.5 Phase E — allocation, spill, and frame realization

Phase E consumes exactly the five `D.*` output groups plus their exact Prepared
products. It publishes:

- `E.AllocatedOperation`: pseudo value/effect operations whose virtual def/use
  roles have exact allocated homes;
- `E.AllocatedControlCall`: allocated control/call operations with resolved
  ABI endpoints and preserved control/effect semantics;
- `E.SpillReloadAction`: explicit spill, reload, and required copy actions;
- `E.FrameAction`: frame-object placement, stack adjustment,
  prologue/epilogue, and related exact actions;
- `E.FinalExpansion`: a closed allocated/pre-machine group whose remaining
  expansion is specified for F.

Phase E requires liveness/interference results tied to the D publication,
target register constraints, the exact ABI/call/address plans, and
`AllocationPlan` plus `FramePlan` products whose revisions and target identity
match. Products remain external authority; nodes reference exact product keys
or resolved operands but do not absorb mutable allocation side tables.

The Allocated publication verifier checks complete virtual-value homes,
register-class and tied-operand constraints, spill/reload coverage, stack/frame
alignment and object placement, call clobbers, and exact product lineage. It
rejects unresolved SSA/phi state, unallocated required operands, stale plans,
or machine encoding claims.

### 12.6 Phase F — MIR-ready machine boundary

Phase F consumes exactly the five `E.*` groups and matching immutable products.
It publishes only:

- `F.MachineOperation`: one-record target machine operations with exact opcode,
  physical/virtual endpoint policy, operand roles, types/widths, and effects;
- `F.MachineMemory`: exact target memory operations and address realization;
- `F.MachineControlCall`: exact machine branch, call, return, and terminator
  operations;
- `F.MachineFrame`: fully realized frame/prologue/epilogue instructions where
  those are represented as machine records;
- `F.MachineDataReference`: machine-level symbol/constant/relocation references
  that obey the object-data ownership contract.

All F output groups carry Machine family or its reviewed machine-level
subfamily, `TargetSpecific`, `MachineOnly`, `MirReadyMachine` ownership and
admission, and `NeverSsa` for ordinary BIR SSA. Phase F requires exact E
publication, target instruction-selection rules, allocation/frame plans,
address realization, and ABI/call plans. The MIR verifier checks legal target
opcodes, exact operands/constraints, realized expansions, control-flow and
terminators, memory/address legality, frame/call agreement, product lineage,
and absence of Prepared/Pseudo/Allocation-only kinds.

### 12.7 B-to-C transition matrix

| Accepted B output | C outcome | Tags retained | Tags added | Tags removed | Prerequisites and publication proof |
| --- | --- | --- | --- | --- | --- |
| `B.CanonicalSsaValue` | retain immutable Canonical node by exact-revision reference | value/SSA, semantic family, effects/control, shape/type, Canonical ownership | explicit Prepared admission-by-reference only | none | Canonical token and B4 proof; Prepared verifier proves revision/target/product agreement and no mutation. |
| `B.CanonicalEffect` | retain immutable reference | non-value, family, effects, roles/type | Prepared admission-by-reference | none | Canonical effect/shape proof plus exact target preparation products. |
| `B.CanonicalControl` | retain immutable reference | control family, terminator/successor roles, effects | Prepared admission-by-reference | none | Canonical CFG proof; plans may classify but not rewrite successors. |
| `B.CanonicalPhiMerge` | retain immutable reference pending D out-of-SSA | SSA eligibility, phi family, predecessor roles/type | Prepared admission-by-reference | none | B4 phi-edge proof remains authoritative; C cannot eliminate phi. |
| `B.CanonicalOpaqueTargetToken` | retain semantic token; derive target constraints externally | opaque intrinsic semantics, roles and conservative effects | Prepared admission-by-reference; external preparation facts | no node tags | TargetContext and closed constraint product; no machine opcode/encoding yet. |

No B output expands, splits, merges, projects, or disappears in C unless a
future reviewed `C.PreparedAction` row is added. Preparation facts are products,
not node tag mutations.

### 12.8 C-to-D transition matrix

| Accepted C input | D outcome | Tags retained | Tags added | Tags removed | Prerequisites and publication proof |
| --- | --- | --- | --- | --- | --- |
| Canonical arithmetic/compare/conversion/aggregate value reference | lower, or expand/project when its reviewed result contract requires | semantic operation, type/effect intent, provenance | Pseudo family/admission, explicit virtual def/use roles, exact MIR disposition | Canonical owner/admission; ordinary `SsaEligible` after uses are rewritten | Prepared constraints plus exact use-def; D verifier proves complete pseudo mapping and no SSA-only residue. |
| Canonical memory/effect reference | lower or expand | memory/call/intrinsic effect bounds, semantic operand intent | Pseudo family/admission, target constraint/address-plan keys | Canonical owner/admission; Canonical-only roles | exact Address/ABI/Call plans; D verifier proves roles/effects were not weakened. |
| Canonical control reference | lower or split critical edges | branch/call/return semantics and successor intent | Pseudo control roles; any explicit split-edge nodes | Canonical owner/admission | exact CFG and edge plan; D verifier proves successor equivalence. |
| Canonical phi/merge reference | disappear after edge-local lower/expand into `D.ParallelCopy`; cycles may split through temporaries and critical edges may split | value type and source/destination equivalence | parallel-copy/pseudo tags, explicit edge and temporary roles | `PhiMerge`, predecessor-qualified phi form, `SsaEligible` | B4 phi proof, liveness/use-def and copy schedule; D verifier rejects every residual phi. |
| Canonical opaque target token | lower or expand to pseudo effect/value/control forms | conservative semantics, effects, provenance | target-specific pseudo constraints and expansion disposition | opaque Canonical ownership | closed target constraint plan; D verifier rejects unsupported/unhandled realizations. |
| reviewed `C.PreparedAction` | lower, expand, merge, or disappear exactly as its schema row declares | only semantics proved identical by that row | applicable pseudo tags | Preparation family/admission | exact product key and explicit row; no default handling. |

Projection is permitted only for an aggregate/multi-result contract and must
create explicit projection value nodes; merge is permitted only when the row
proves one resulting operation preserves every consumed semantic effect and
result. No other C input is accepted.

### 12.9 D-to-E transition matrix

| Accepted D output | E outcome | Tags retained | Tags added | Tags removed | Prerequisites and publication proof |
| --- | --- | --- | --- | --- | --- |
| `D.PseudoValue` | retain operation identity only where roles/results/effects stay exact, then bind allocated homes; otherwise replace | pseudo semantic intent, types, constraints, NeverSsa | Allocated admission and exact home/product references | unresolved virtual-home state | liveness/interference and AllocationPlan; E verifier proves every required def/use home. |
| `D.PseudoEffect` | allocate operands; lower or expand when constraints require | effect/control bounds, type and operand intent | allocated roles, spill/reload dependencies | unresolved pseudo constraints | allocation/call/address products; E verifier proves clobber and memory agreement. |
| `D.PseudoControl` | allocate endpoints; split only for exact control/call realization | successor/call/return semantics | allocated control/call roles | unresolved virtual endpoints | CFG, ABI/CallPlan, AllocationPlan; E verifier proves edge and clobber consistency. |
| `D.ParallelCopy` | lower/expand into allocated copies plus spill/reload actions; merge redundant exact copies; disappear only when proven identity copy | value type and transfer equivalence | allocated endpoints, `SpillReloadAction` where required | parallel-copy scheduling state | allocation homes and cycle schedule; E verifier proves all transfers realized. |
| `D.ExpansionPlaceholder` | expand into allocated operations/actions or fail | source semantic/effect obligations | exact allocated/expansion tags | placeholder/pseudo-only tag | named expansion rule and products; no placeholder survives Allocated publication. |

E may insert spill/reload and frame actions only from exact allocation/frame
products. It may not turn those products into mutable annotations on D nodes.

### 12.10 E-to-F transition matrix

| Accepted E output | F outcome | Tags retained | Tags added | Tags removed | Prerequisites and publication proof |
| --- | --- | --- | --- | --- | --- |
| `E.AllocatedOperation` | select one machine record, or expand/project into an explicit machine sequence | operation/effect/type/allocated endpoint intent | Machine, TargetSpecific, MachineOnly, MirReadyMachine, exact opcode/roles | Pseudo/Allocated owner tags and selection placeholders | instruction-selection rule plus allocation/address products; MIR verifier checks exact realization. |
| `E.AllocatedControlCall` | lower or expand into machine control/call sequence | successors, call/return effects, allocated ABI endpoints | machine control/call opcode and terminator roles | allocated/pseudo tags | ABI/CallPlan and target selection; MIR verifier proves CFG, clobber, and return agreement. |
| `E.SpillReloadAction` | lower/expand into exact machine memory/copy records; merge only where effects and endpoints remain exact | transfer type, memory effect, frame object identity | machine memory/copy tags and addressing | spill/reload action tag | Frame/Allocation/Address plans; MIR verifier proves no unresolved action. |
| `E.FrameAction` | lower/expand into machine frame records, merge into a machine instruction when exact, or disappear only for a proven zero-action frame | frame/stack effect and unwind obligations | machine frame/opcode/operand tags | frame-action/allocated owner tags | exact FramePlan and target ABI; MIR verifier proves layout/alignment/unwind agreement. |
| `E.FinalExpansion` | expand, split, project, or merge according to its one explicit selection rule | all source semantic/effect/result obligations | final machine tags | every preparation/pseudo/allocation placeholder | target selection rule and all exact products; no final-expansion node survives F. |

F may place optional BIR IDs only in provenance/debug links. MIR records own
their machine identity; retained arena slots do not make BIR IDs MIR identity.

### 12.11 Uniform boundary rejection

At every phase and boundary, the admission check rejects:

- unknown enum/tag/stage values and schema-invalid combinations;
- a known kind absent from the phase's closed input groups;
- a premature later-stage kind or a stale earlier-stage kind required to have
  disappeared;
- stale analysis/preparation/allocation/frame products whose graph revision,
  target identity, or prerequisite product identity differs;
- payload, arity, role, result/type, effect/control, or MIR disposition that
  disagrees with the schema entry;
- an accepted group omitted from the boundary matrix;
- an accepted kind for which no explicit group row applies, or a row whose
  prerequisite cannot be proved;
- any default/catch-all pass-through, including retention justified only by
  shared `Node` storage or stable arena identity.

Failure is atomic: no later-stage token or product is published, and the last
verified input remains the rollback anchor. The exact identity consequences and
publication gates follow.

## 13. Semantic identity across lowering

### 13.1 `NodeId` preservation gate

`NodeId` denotes one operation and, for a value-producing node, its ordinary
result identity. It is not the address or index of storage. Reusing an arena
slot, mutating a record in place, or preserving a generation does not prove
semantic identity.

A transformation may preserve a source `NodeId` only when it proves all of
these facts remain exact:

1. operation meaning and semantic-family ownership;
2. ordinary result identity, result form/count, and concrete result type;
3. operand count, order, roles, ownership, and the meaning of every use;
4. kind-level and payload-refined memory, trap, call, and control effects;
5. successor/terminator behavior and any edge ownership;
6. stage owner and the operation's authority. Adding an explicit later-stage
   admission without changing the node, as C does by reference, is not a node
   rewrite; changing from Canonical to Pseudo/Allocated/Machine ownership is;
7. payload semantics and every product reference, including exact revision and
   target lineage;
8. provenance meaning and all verifier obligations applicable to the result.

Changing a kind spelling is insufficient evidence either way. A same-owner
normalization may preserve `NodeId` only if all eight conditions hold. A change
of result identity/count/type, operand roles, effects/control, stage owner,
product authority, or operation meaning mandates replacement even when the
replacement fits in the same bytes.

Phase C is the important non-rewrite case. `PreparedBir` holds an immutable
exact-revision reference to Canonical nodes and external products. Those nodes
retain their Canonical `NodeKind`, `NodeId`, stage owner, and graph revision;
they are merely admitted as read-only preparation inputs. C does not mutate
them into Prepared-owned nodes and does not mint aliases with new semantic
identity. A reviewed `C.PreparedAction`, if one is later implemented, is a new
node with a new `NodeId` in its owning publication.

MIR is the opposite boundary. Every F machine record has distinct
machine-stage identity. A BIR `NodeId` may be copied into an optional
provenance/debug link, but cannot serve as MIR identity, allocation authority,
or a key whose continued liveness proves the machine record equivalent.

### 13.2 Consequences by transformation shape

| Shape | Identity rule | Uses and result mapping | Provenance | Revision and invalidation |
| --- | --- | --- | --- | --- |
| retain unchanged | Preserve `NodeId` only after the Section 13.1 gate; C admission-by-reference performs no mutation. | Existing uses remain exact; no remap is created. | Existing provenance remains attached. | A pure read/reference does not change graph revision; a no-semantic-change rewrite still follows editor revision policy and invalidates anything not explicitly preserved. |
| lower/replace one-to-one | Preserve only for a same-owner normalization satisfying all eight conditions; stage-owner change or any semantic/shape/effect change creates a new node and retires the source. | Transaction rewrites every source-result use to the exact replacement result, with type and role checks; zero/multi-result mappings must be explicit. | Record source-to-replacement derivation; this link is not identity. | One successful transaction advances the owning graph revision once and invalidates all analyses/products affected by kind, operands, CFG, effects, types, or target facts. |
| insert | Always allocate a fresh `NodeId`; insertion cannot borrow the identity of an adjacent/source node. | Add reciprocal uses and definitions atomically; inserted results begin with only explicitly created uses. | Record all contributing source IDs and the reason/phase. | Advance revision and invalidate dependencies named by the mutation summary. |
| delete/disappear | Retire the source identity. Disappearance is legal only when no result use remains and all effect/control obligations are absent or explicitly realized by other output nodes/products. | Rewrite or remove all uses before retirement; a live ordinary result forbids deletion without an exact mapping. | Preserve a tombstone/derivation record when required for diagnostics; never leave a resolvable semantic alias. | Advance revision; invalidate def-use, CFG, effects, dominance, liveness and downstream products as applicable. |
| expand one-to-many | Source identity is retired; every output gets a fresh ID. Arena-slot reuse is forbidden as proof that one output is the source. | Provide a total result mapping to one output or explicit projections; rewrite all uses only after the complete expansion validates. | Every output may cite the source plus its expansion role/ordinal. | Commit all outputs and rewrites in one revision; invalidate analyses/products for every affected semantic category. |
| project | Projection nodes have fresh IDs and exact component types/roles. The aggregate source may retain its ID only if it remains a live unchanged operation; otherwise it is retired. | Each component use maps to one explicit projection result; generic operand-index output conventions are forbidden. | Projection records cite source and component role. | Advance revision for inserted projections/use rewrites; invalidate value-flow/type/liveness products. |
| split | New operations and any new block/edge entities receive fresh identities; the split source is retired unless one unchanged survivor independently passes Section 13.1. | Partition operands/results/effects with a total mapping; redirect successor and phi/copy edge roles transactionally. | Each part cites the source and split role; block provenance records the source edge/block. | One atomic graph revision; invalidate CFG, dominance, phi/SSA, liveness and all target products derived from them. |
| merge many-to-one | Create a fresh result ID unless exactly one survivor remains semantically unchanged and every other source is resultless/effectless or its obligations are already represented. | Map every consumed result use and prove no effect/control obligation is dropped or duplicated. | Result cites every source and the merge rule; provenance order cannot define semantics. | One atomic revision; invalidate union of all source mutation categories and their downstream products. |

Copy elimination is a `disappear` only when source and destination denote the
same exact allocated value/home and removal changes no ordering, liveness,
effect, or debug obligation. Frame-action disappearance requires a proven
zero-action frame. Phi disappearance requires the complete parallel-copy/edge
realization; deletion alone is never out-of-SSA.

### 13.3 Atomic rewriting and lineage

All mutating shapes execute through an exclusive editor transaction. Before
publication the transaction owns a total old-result-to-new-result mapping,
reciprocal def-use updates, CFG/edge updates, and type/role checks. Verification
occurs against the tentative complete output. On any error, no insertion,
retirement, use rewrite, revision increment, provenance mapping, or product is
observable.

A successful transaction advances the owning graph revision exactly once and
publishes a mutation summary. Analyses are either explicitly preserved by that
summary or invalidated. Every preparation, pseudo, allocation, frame, and
machine product names its exact input revision, target identity, and required
predecessor product identities; any graph mutation makes a mismatching product
stale and unobservable. Provenance maps survive only as diagnostic derivation
facts and never authorize stale product reuse or semantic-ID equivalence.

## 14. Publication verifier obligations

### 14.1 Common gate

Every publication verifier first performs the same fail-closed checks:

- known `NodeKind`, tag, and stage values; one complete validated schema entry;
- membership in exactly one admitted closed group and explicit legality at the
  claimed stage;
- valid tag combinations across all six axes;
- payload alternative, operand arity/roles, result form/count, concrete type,
  effects/control, and MIR disposition consistent with the schema;
- valid owner/generation/order membership, reciprocal def-use, and no retired
  identity still resolving;
- exact revision/target/product lineage for every external fact consumed;
- every node covered by an explicit transition outcome, with no unknown,
  omitted, stale, premature, or catch-all-retained kind.

Failure produces no stage token. A boolean convenience query returning false
does not replace a diagnostic that distinguishes unknown kind, illegal stage,
invalid combination, stale product, and unhandled transition.

### 14.2 Raw publication

Raw admits only the five `B.Raw*` groups. Its verifier proves stable ID
ownership, closed payload/shape/type validity, explicit input-use roles,
terminator/successor well-formedness at the Raw profile, and that every
`B.RawImportOnly` form is documented for a B transition. It rejects Prepared,
Pseudo, Allocated, and Machine owners and any target/allocation/frame fact in
core nodes. Raw does not claim SSA validity; `SsaEligible` classification is not
a dominance or single-definition proof.

### 14.3 Canonical publication and B4 SSA

Canonical admits only the five `B.Canonical*` groups. In addition to the common
gate it rejects every residual Raw/import-only, preparation, pseudo,
allocation, and machine form; proves canonical types and roles, exact
terminator-derived CFG, reciprocal use-def, and target independence; and runs
the B4 dynamic SSA proof for all admitted `SsaEligible` results. B4 checks one
definition, dominance of non-phi uses, exact predecessor coverage and incoming
edge ownership for phi/merge, and valid use-def across the whole publication.
Static classification alone cannot mint the Canonical token.

### 14.4 Prepared publication

Prepared admits the five Canonical groups only as immutable exact-revision
references plus an optional separately owned reviewed `C.PreparedAction` group.
The verifier revalidates the Canonical token/revision, proves that Canonical
node kinds, IDs, stage owners, payloads, and graph revision were not changed,
and validates `TargetContext`, `AbiPlan`, `CallPlan`, `AddressPlan`, and any
constraint product against that revision and target. Products must be complete,
internally consistent, immutable, and external to Canonical nodes. Allocation,
frame, pseudo, and machine facts are premature and rejected.

### 14.5 PseudoPreallocation publication

PseudoPreallocation admits only the five `D.*` groups. Its verifier proves a
total explicit C-to-D transition for every consumed Canonical/prepared input,
valid pseudo payloads/roles/types/effects/constraints, exact target/product
lineage, and complete use/provenance mappings for replacements and expansions.
It rejects all phi/merge nodes, ordinary SSA-only definitions or uses, residual
Canonical ownership except diagnostic references, and missing/duplicate
parallel copies. Critical-edge splitting, cyclic-copy temporaries, successor
equivalence, and the absence of any post-out-of-SSA SSA-pass claim are dynamic
publication checks.

### 14.6 Allocated publication

Allocated admits only the five `E.*` groups. Its verifier proves exact D input
revision, liveness/interference and target-constraint lineage, a complete
`AllocationPlan` and `FramePlan`, one legal home for every required virtual
def/use, tied and register-class constraints, call clobbers, spill/reload and
copy coverage, frame-object placement/alignment, stack effects, and all
insert/replace/retire identity mappings. It rejects unresolved phi/SSA,
unallocated required operands, stale plans, surviving expansion placeholders,
and any claim of final machine opcode/encoding authority.

### 14.7 MirReadyMachine publication

MirReadyMachine admits only the five `F.*` groups. Its verifier proves distinct
machine identities; legal target opcode, operand/constraint, width/type,
memory/address, terminator/CFG, call/return/clobber, frame/unwind, symbol and
relocation contracts; exact Allocation/Frame/Address/ABI/Call product lineage;
and total realization of every E input and effect/result/control obligation.
Every node is `TargetSpecific + MachineOnly` and admitted at
`MirReadyMachine`; no Raw, Canonical, Prepared, Pseudo, Allocation-only,
expansion, unresolved virtual-home, or SSA/phi kind survives. A source BIR
`NodeId` may appear only as optional provenance and is rejected wherever a
machine identity is required.

## 15. Pending later steps

Step 6 must review this complete artifact against idea 801, decide whether the
bounded C++ schema/query proof is necessary, and record the decision. Any proof
and final completion handoff remain Steps 7 and 8. Adoption requirements for
idea 732 remain deferred to its later user-authorized lifecycle revision.
