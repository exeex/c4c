# BIR Node-Kind-Centric Storage and Pass Contract

Status: Closed (capability complete)
Type: Architecture review and implementation-contract refinement
Source Context:
- `ideas/closed/735_bir_phase_a_import_raw_document_convergence.md`
- `ideas/closed/736_bir_phase_b_canonical_document_convergence.md`
Primary Follow-up Targets:
- `src/backend/bir/core/README.md`
- `src/backend/bir/core/ir.hpp`

## Goal

Review and solidify the future BIR storage/pass contract so BIR is described as
a simple data-oriented node graph whose behavior is determined by `NodeKind`,
not by class hierarchy, payload-specific hidden behavior, operand-index output
conventions, or stage-local ad hoc side structures.

The resulting accepted work should revise the core BIR documentation and, where
appropriate, the core IR declarations so future phase-A and phase-B work has a
single durable contract for node storage, node-kind schema, pass dispatch,
operand ownership, and Raw-to-Canonical vocabulary transition.

## Why This Exists

The closed phase-A and phase-B document convergence ideas established the
current Raw and Canonical contracts, but their wording still reflects an early
bootstrap container shape: `InstData` has an opcode, payload, operand vector,
and result vector, while `ModuleData` carries many storage/index concerns in
one class-heavy private layout.

The intended future direction is sharper:

- high-level BIR node kinds are LLVM-IR-like semantic operations;
- later node kinds become pseudo- and machine-like operations;
- lowering is primarily replacing, inserting, deleting, or merging nodes;
- each pass accepts a bounded node vocabulary, switches explicitly on
  `NodeKind`, and fails closed on unhandled kinds;
- payload stores node parameters but does not redefine the operation's behavior;
- side-effect, memory, terminator, result, arity, and type rules come from a
  `NodeKind` schema/query table;
- operands are input uses only, not a mixed input/output index convention;
- future operand storage should prefer simple arena-backed spans or similarly
  compact data-oriented ranges instead of per-node heap-owning `std::vector`
  storage.

Without making this explicit, future work can keep adding fields to
`ModuleData`, preserve output-as-result-vector conventions, or build passes
that inspect payloads and side maps instead of enforcing a clear node-kind
contract.

## In Scope

- Re-read the closed 735/736 contracts as historical contracts to revise or
  extend; do not edit or reopen those closed files.
- Review `src/backend/bir/core/README.md` against the node-kind-centric model
  and update its Raw/Canonical storage language so it no longer implies that
  future pass behavior is driven by class hierarchy, dense vector positions, or
  payload-local semantic reinterpretation.
- Review `src/backend/bir/core/ir.hpp` and make the minimal schema-level edits
  needed to align names, comments, or declarations with the intended contract.
- Define the durable meaning of a BIR node:
  - one simple data record;
  - one `NodeKind`/opcode vocabulary tag;
  - payload as parameters only;
  - operand range as input uses only;
  - result/value identity represented by the node/value identity and concrete
    type, never by operand index convention;
  - `NodeId` identity provided by arena offset/slot handles, not stored inside
    the node record;
  - behavior queried from `NodeKind` schema.
- Describe B1-B7 as node-vocabulary transition over the same underlying graph
  storage: Raw semantic nodes become Canonical target-independent semantic
  nodes.
- Clarify that SSA is a B4 canonical property for ordinary value flow, not an
  A-phase input requirement and not a claim that Canonical BIR is MIR-like.
- Clarify that Canonical BIR remains target-independent and unallocated; pseudo,
  ABI, register, spill, frame-action, machine-node, and MIR-ready choices remain
  later C/D/E/F ownership.
- Establish pass dispatch expectations: every stage enumerates accepted
  `NodeKind` cases, handles each explicitly, and treats default/unhandled kinds
  as verifier or pass failure.

## Out of Scope

- Editing `ideas/closed/735_bir_phase_a_import_raw_document_convergence.md` or
  `ideas/closed/736_bir_phase_b_canonical_document_convergence.md`.
- Promoting this draft, activating it, creating `plan.md`/`todo.md`, or
  implementing it before explicit user approval.
- Rewriting unrelated BIR phases, pass framework documents, verifier documents,
  target preparation, MIR, emission, register allocation, or frontend IR.
- Changing LIR/HIR semantics or making LIR less structured to fit BIR.
- Claiming B1-B7 implementation completeness merely by updating terminology.
- Replacing the whole current container with a broad phoenix rewrite unless a
  separately approved idea requests that scope.

## Proposed Contract Shape

The design target is a flat, data-oriented node representation conceptually
close to:

```cpp
struct Node {
  NodeKind kind;
  TypeId type;            // void/invalid means no ordinary single result
  OperandSpan operands;   // input uses only
  PayloadRef payload;     // kind-specific immediates/descriptors/refs
};
```

`TypeId` is part of the recommended durable node/value contract. A future
implementation may prove that an exact side table is a better storage tradeoff,
but the design should not start from "kind alone determines type" or from
late, pass-local type reconstruction.

The exact checked-in names may remain local to the existing BIR codebase, but
the durable contract is:

- `NodeId` is an arena-provided offset/slot handle, similar in spirit to stable
  ID tables such as `src/shared/text_id_table.hpp`. It is chosen so node
  identity survives arena/vector reallocation, memcpy movement, and storage
  compaction strategies that would invalidate raw pointers.
- `Node` does not store its own `NodeId`. The arena, graph, or view maps
  `NodeId` to `base + offset` storage.
- source origin/debug provenance is side-table or payload-adjacent metadata,
  not a required field on the core node record.
- `OperandSpan` and `PayloadRef` are compact arena/range references, not
  per-node heap-owning `std::vector` storage.
- operands are input uses only. Outputs/results are represented by the
  node/value identity and its concrete type.
- `TypeId` is carried by the node/value record in the preferred contract. A
  void/invalid type means the node has no ordinary value result.
- the `OperandSpan` element type remains a follow-up design choice: if
  parameters, constants, and value-like entities are nodes, operands can be
  `NodeId`; if the current separate value model remains, operands should be
  `ValueId` or `ValueRef`. The invariant is stable typed input-use identity
  and `NodeKind`-defined operand roles.

This intentionally mirrors the useful LLVM semantic split without adopting
LLVM's pointer identity model. LLVM `Instruction` identity is pointer-based and
depends on stable allocation; LLVM's `Value` carries type, `User` carries input
uses, and `Instruction` supplies opcode/kind behavior. c4c should preserve
those semantic ideas while using arena offset/slot `NodeId` handles instead of
raw instruction pointers.

`NodeKind` owns the typing rule, but it does not by itself always determine a
concrete result type. For example, add-like nodes derive the concrete type from
their operands, cast/load/call-like nodes may need payload or signature facts,
and branch/store-like nodes have no ordinary result. Therefore the core graph
must provide fast and verifiable concrete type lookup through a `TypeId` field
or explicitly justified side table.

The contract should reject these patterns:

- result/output identity encoded as an operand-list index convention;
- side effects inferred from arbitrary payload contents instead of the
  operation's `NodeKind` schema;
- pass acceptance based on "unknown node ignored" behavior;
- per-pass meaning invented outside the node-kind vocabulary;
- class-heavy storage growth where a simple table/range/schema record would
  suffice.

`NodeKind` owns the operation's behavior. Payload only carries the parameters
needed by that behavior. Operand order is meaningful for input uses, but not as
an output slot namespace. Side-effect, memory, and terminator behavior are
queried from the `NodeKind` schema. The query may inspect payload for special
cases such as call attributes or inline-asm `sideeffects`, but `NodeKind`
remains the dispatch authority and the pass switch key.

## Consolidated Phase B/C/D/E/F Audit Conclusion

Five read-only phase audits converged on the same durable core shape:

```cpp
struct Node {
  NodeKind kind;
  TypeId type;            // void/invalid means no ordinary single result
  OperandSpan operands;   // input uses only
  PayloadRef payload;     // kind-specific immediates/descriptors/refs
};
```

`NodeId` remains arena-provided slot/offset identity and is not stored in the
node. This preserves stable identity across arena/vector reallocation, memcpy
movement, compaction, and future dense storage strategies.

`TypeId` should be kept as a core node/value field unless a later
implementation proves the side-table tradeoff. Phases B through F all need hot
concrete type lookup: typed RAUW, phi/call/load/cast/select verification,
ABI/call facts, constraint binding, copy/liveness/allocation/reload/spill
typing, and MIR-ready one-record mapping. `NodeKind` owns the typing rule, but
the concrete type is a node/value property.

`OperandSpan` is input uses only. The operand element type remains an explicit
follow-up decision: use `NodeId` if parameters/constants/value-like entities
are nodes, or `ValueId`/`ValueRef` if the separate value model remains. In
either case, every operand must be a stable typed input-use identity, and every
operand index's role is defined by the `NodeKind` schema.

Do not add generic side-effect fields to `Node`. Memory, effect, terminator,
trapping, volatility, atomic/fence, one-record-realizability, stage-legality,
and operand-role behavior belongs to a `NodeKind` schema/query. The query may
be payload-aware for call attributes, inline asm, and similar operations, but
the dispatch and behavior authority remains `NodeKind`.

Do not add phase-specific products to `Node`: allocation homes, liveness,
spill state, frame layout, target encoding, relocation, verifier tokens,
analysis facts, preparation products, constraints, revision/stamp lineage, and
similar products are exact revision-keyed external maps keyed by `NodeId`,
value identity, operand ordinal, edge key, frame object, or the owning product
identity.

The one unresolved hard fork is multi-result representation. The preferred
final contract is LLVM-like: each node has at most one ordinary value result,
and `NodeId` is the value identity for value-producing nodes. If BIR needs
multi-output call, inline-asm, checked-overflow, or pseudo forms, they must be
normalized into aggregate/projection/extract/result-node forms before
single-result consumers, or the design must explicitly add a compact
`ResultSpan` or external result table. Outputs must never be encoded through an
operand-index convention.

D5 copy destinations are not input operands and must not revive generic result
vectors. If copy destinations need explicit representation, they belong in a
`NodeKind`-governed payload or endpoint span with verifier-owned semantics.

## NodeKind Traits and Schema

The same `NodeKind` system should replace the kind of TableGen-style `.td`
usage c4c needs for backend node classification, operand schema, and behavior
queries. c4c does not need a second external DSL merely to classify node kinds,
define operand meanings, or answer pass-dispatch questions.

C++ enum values cannot literally inherit from one another, but c4c can model
multiple inheritance or derivation through C++20/23 type traits, tag lists, and
`constexpr` schema. A node kind can have multiple tags:

```text
Add         = Ssa + Binary + Pure + Integer
Store       = NonSsa + Memory + MayWrite
MachineAddi = NonSsa + Machine + RegisterDef + RegisterUse
```

The internal traits machinery may use templates, type lists, specialization, or
`constexpr` descriptor tables, but that complexity must stay inside the schema
layer. Pass authors should see a small helper API:

```cpp
template<NodeKind K>
inline constexpr bool is_op_binary_v = /* schema query */;

template<NodeKind K>
inline constexpr bool is_op_ssa_v = /* schema query */;

template<NodeKind K>
inline constexpr bool op_may_write_v = /* schema query */;

bool is_op_binary(NodeKind kind);
bool is_op_ssa(NodeKind kind);
bool op_may_write(NodeKind kind);
```

The same schema should answer all behavior questions that are stable properties
of a node kind:

- SSA or non-SSA value model;
- semantic, pseudo, or machine family;
- operand count and variable-arity policy;
- operand role by index;
- operand type or constraint policy;
- concrete result type policy;
- memory/effect policy;
- terminator/control-flow policy;
- legal stage interval or stage mask;
- one-record MIR mapping legality;
- payload requirement and payload shape.

This keeps one `Node` struct and one `NodeKind`/traits/schema system across
backend phases A through F. High-level node kinds can be LLVM-IR-like semantic
operations, later node kinds can be pseudo- or machine-like operations, and
lowering remains a vocabulary transition over the same graph model. There must
not be a separate MIR or machine-node schema that duplicates `NodeKind` traits
for the same classification and operand-role facts.

Passes may still use explicit `switch (node.kind)` dispatch, but any repeated
classification such as binary, SSA, memory-writing, machine, or MIR-ready
should route through schema helpers rather than open-coded template internals
or duplicated tables.

## Phase-B Interpretation

B1-B7 should be documented as a target-independent node vocabulary
normalization route:

```text
Raw semantic node graph
  -> B1 legal Raw vocabulary
  -> B2 normalized scalar/comparison/select vocabulary
  -> B3 canonical CFG vocabulary
  -> B4 SSA-canonical ordinary value-flow vocabulary
  -> B5 canonical memory/effect-bearing semantic vocabulary
  -> B6 canonical aggregate/vector semantic vocabulary
  -> B7 canonical target-independent intrinsic vocabulary
  -> B8 verified CanonicalBir publication
```

This route may rewrite, insert, delete, split, or merge nodes, but it must not
select ABI transport, register classes, stack homes, spill/reload actions,
machine opcodes, or MIR records. Those remain later target-aware stages.

## Acceptance Criteria

- `src/backend/bir/core/README.md` clearly states the node-kind-centric storage
  and pass contract, including input-only operands, node-kind-owned behavior,
  schema/query ownership for effects and arity, and fail-closed per-stage
  dispatch.
- The README explicitly revises or extends the relevant implications of closed
  ideas 735 and 736 without editing or reopening them.
- `src/backend/bir/core/ir.hpp` is either updated to match the contract or the
  accepted result documents why no code-level declaration change is yet needed.
- Any retained `results` or `std::vector`-based bootstrap field is described as
  current implementation state or compatibility scaffolding, not the durable
  future pass contract.
- The accepted design either commits to single ordinary result nodes or names an
  explicit compact `ResultSpan`/external result table design; it does not leave
  multi-result behavior hidden behind the old `results` vector.
- Any conceptual node shape excludes node-owned `NodeId` and required
  node-owned origin fields; identity and provenance are handled by arena/view
  handles and side metadata.
- The accepted design documents the LLVM comparison accurately: c4c adopts the
  value/type, user/input-use, instruction/kind concepts but uses arena offset
  IDs rather than pointer identity.
- Concrete result type lookup is specified as a node/value property or side
  table derived under the `NodeKind` typing rule, not as something inferred from
  kind alone. The preferred result keeps `TypeId` in the core node/value record
  unless the accepted implementation explicitly justifies a side-table tradeoff.
- The accepted design documents a single C++ `NodeKind` traits/schema system as
  the owner of node classification, operand roles, type policy, effect policy,
  stage legality, and one-record MIR mapping legality.
- Helper APIs hide template/type-list internals and provide both compile-time
  queries such as `is_op_binary_v<K>` and runtime wrappers such as
  `is_op_binary(kind)`.
- The B1-B7 description preserves the distinction between Canonical BIR and
  later MIR-ready pseudo/machine BIR.
- Proof includes at least a documentation diff review and a build or compile
  check if `ir.hpp` changes.

## Reviewer Reject Signals

- The change edits or reopens the closed 735/736 idea files instead of citing
  them as historical contracts.
- The change claims implementation completeness from wording changes alone.
- Any pass contract allows unknown node kinds to be silently ignored, carried
  forward without an explicit compatibility rule, or accepted by catch-all
  visitor behavior.
- Payload fields, rendered text, vector positions, names, or side maps become
  semantic authority that overrides `NodeKind`.
- Output/result identity is encoded as an operand-list index convention.
- A generic `side_effect`, memory-effect, terminator, trapping, volatility,
  atomic, fence, stage-legality, or one-record-realizability field is added to
  every `Node` instead of being specified through `NodeKind` schema/query, unless
  that field is explicitly justified as part of the accepted core-node contract.
- The durable node architecture stores `NodeId` inside each `Node`, relies on
  raw pointer identity, or requires source/origin provenance as a core node
  field in a way that undermines arena offset identity.
- Concrete result type is claimed to be determined by `NodeKind` alone, with no
  node/value type field or side-table lookup for operand/payload-derived types.
- Allocation homes, liveness state, spill state, frame layout, target encoding,
  relocations, verifier tokens, analysis products, preparation facts,
  constraints, or revision/stamp lineage are added to `Node` instead of kept as
  exact revision-keyed external products/maps.
- A `.td`, TableGen-like external DSL, or generated side language is introduced
  for `NodeKind` classification, operand schema, or behavior queries without a
  later separately approved scope that justifies why C++ traits/schema is
  insufficient.
- A second independent MIR or machine-node schema duplicates `NodeKind` traits
  for SSA/non-SSA classification, operand roles, effect policy, stage legality,
  or one-record mapping.
- Pass code depends directly on template/type-list internals instead of using
  the approved helper API and explicit `NodeKind` dispatch.
- Multi-result semantics remain unresolved behind the old generic `results`
  vector, or D5 copy destinations are represented as input operands or generic
  result-vector entries instead of `NodeKind`-governed payload/endpoints.
- Canonical BIR is described as MIR-like, target-selected, allocated,
  ABI-expanded, or machine-op-ready.
- B4 SSA is treated as an A1/A2 import prerequisite instead of a canonical
  phase-B property for ordinary value flow.
- Broad rewrites outside `src/backend/bir/core/README.md` and
  `src/backend/bir/core/ir.hpp` are included without a separately approved
  scope expansion.
- `std::vector` or class-heavy storage is defended as the durable desired
  architecture without addressing arena/range-oriented alternatives.
- The same old ambiguity is preserved behind a renamed `Node`, `Opcode`,
  `InstData`, or pass-framework abstraction.

## Closure Record

Disposition: capability complete.

The bounded idea landed one C++ `NodeKind` traits/descriptor authority for the
current 16-kind BIR vocabulary, compile-time and runtime helper surfaces, and
fail-closed payload and arity queries. The verifier now consumes the shared
schema for payload matching and generic arity validation. The core README
publishes the durable node/storage/pass contract, identifies retained vectors
and result indices as bootstrap compatibility, and selects single ordinary
results with normalization/projection as the preferred durable direction while
reserving any compact `ResultSpan` or external result table for separately
approved future scope.

Accepted implementation and documentation commits:

- `2bfee915a` — NodeKind schema/helpers and verifier payload consumer;
- `cef885b15` — generic arity verifier gate and compatibility-storage comments;
- `6e803a664` — durable BIR core storage/pass contract documentation.

Supervisor-owned acceptance evidence recorded at closure:

- backend-enabled configure and fresh build completed successfully;
- matching canonical `^backend_` guard passed 6/6 before and 6/6 after,
  including `backend_bir_node_kind_schema`, with the accepted
  allow-non-decreasing comparison;
- `git diff --check` passed.

The broader checkpoint is not green: all 1312 registered tests ran and 40
failed outside this idea's bounded backend core/schema ownership, covering
frontend metadata, macOS ABI/linker/LLVM, parser/EASTL recipes, and external
cases. Configure also skipped the external c-testsuite tests because the
expected submodule contents were absent. These limitations are preserved here
and are not claimed as acceptance evidence for broader repository health.
