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

## 12. Pending Step 4: B-through-F vocabularies and transitions

Pending. Step 4 will define exact admitted B/C/D/E/F vocabularies and every
B-to-C, C-to-D, D-to-E, and E-to-F transition. This section intentionally does
not invent transition rows, retained/added/removed tag sets, or pass behavior.

## 13. Pending later steps

Node identity and publication-verifier obligations remain pending Step 5.
Final review, the bounded C++ proof decision, and adoption requirements for
idea 732 remain pending Steps 6 through 8.
