# P01 Legalize Pass Contract

Status: closed architecture contract. Implementation is deferred until the BIR
architecture is accepted.

`P01` is the `B1` transformation in the root BIR order. It converts one
verified `RawBir` revision into legal target-independent type and opcode forms.
It is a semantic-schema conversion, not recovery for a malformed draft and not
a machine-facing legalization phase.

```text
A2 published RawBir
  -> B1 / P01 legalize (this contract)
  -> committed B1 TypesLegal revision
  -> B2 / P02 scalar
```

## 1. Exact input checkpoint

The only input is an immutable, published `RawBir` view carrying an exact
`{ModuleEpoch, ModuleRevision}` and `PassProperty::RawVerified`. Every observed
function body is additionally bound by `(FunctionId, FunctionRevision)`. The
complete Raw verifier profile must already have accepted the same revision.

P01 may rely on resolved stable IDs, closed typed instruction descriptors,
reciprocal def-use, valid CFG references, exact constants and object layouts,
and complete semantic call, memory, aggregate, intrinsic, and inline-assembly
transport. In particular, an `InlineAsm` node already exposes its ordinary
ordered inputs and results while retaining its original assembly text,
constraint text, clobber spellings, and side-effect flag as opaque payload.

Input rejection is mandatory when the capability or any revision component is
missing or stale. Missing operands, unresolved references, ill-typed values,
text standing in for semantic operands, and other producer gaps are Raw
publication failures; P01 reports the inherited verifier failure and performs
no repair.

## 2. Closed authority

P01 owns exactly these transformations:

- canonical exact-bit encoding and interning of scalar constants without
  changing their semantic type or value;
- mapping every admitted Raw scalar opcode alias to the closed BIR semantic
  opcode, predicate, or cast vocabulary;
- inserting an explicit target-independent compare-to-zero at a boolean-use
  boundary when the admitted Raw form supplies a non-`i1` truth value;
- mapping an arbitrary-width SSA integer operation to a width-explicit
  `WideIntegerOp` with all signedness, overflow, shift, and result-role
  semantics retained;
- classifying extended floating, complex, checked-overflow, and intrinsic
  operations by a closed target-independent semantic disposition;
- proving that every preserved non-P01 form has exactly one named later
  canonical owner.

The transformation preserves semantic storage types, object layout, access
width and alignment, address spaces, function and call signatures, aggregate
topology, CFG edges, phi edge keys, symbol identity, memory and atomic effects,
and source calling-convention attributes. Metadata-only normalization retains
the instruction and result stable IDs. A replacement with different descriptor
shape uses deterministic new IDs, typed atomic RAUW, complete def-use repair,
and composed debug origins.

The following are outside P01 authority:

- constant folding, algebraic simplification, preferred comparison direction,
  redundant-cast removal, and select canonicalization (`P02`);
- terminator, block, edge, phi-placement, memory, aggregate, or intrinsic
  canonicalization (`P03` through `P07`);
- interpreting inline-assembly template or constraint text, deriving ties or
  alternatives from that text, or changing any byte of either payload;
- selecting runtime helpers, instruction sequences, calling locations,
  register identities, stack locations, allocation decisions, or encodings.

P01 has no target profile, target-layout table, preparation product, allocator,
MIR view, renderer text, legacy route record, or environment-selected backend
input. Its semantic registry is frozen before invocation and contains only
portable aliases and the identity of a later owner.

## 3. Closed Raw-form dispositions

Every admitted Raw-only form has one disposition. An unlisted Raw-only form is
unsupported and cannot flow through by convention.

| Raw form | P01 output | Later owner |
|---|---|---|
| exact integer or floating literal with noncanonical storage padding | same typed value with canonical exact-bit payload | none |
| admitted scalar opcode, predicate, or cast alias | closed semantic opcode, predicate, or cast | `P02` for expression form |
| non-`i1` scalar truth use | explicit typed compare to zero or null, yielding `i1` | `P02` |
| arbitrary-width integer scalar operation | width-explicit `WideIntegerOp` | `P02` or its registered later semantic owner |
| extended-float semantic operation | exact-format `ExtendedFloatOp` | `P02` or its registered later semantic owner |
| complex semantic operation | typed `ComplexOp` with one semantic result | `P02` / `P06` |
| checked arithmetic | `CheckedOverflowOp` with explicit value and overflow results | `P02` |
| switch and indirect successor forms | unchanged | `P03` |
| phi and forward-reference form | unchanged stable IDs and exact `EdgeKey`s | `P04` |
| address, memory, and atomic form | unchanged complete semantic payload | `P05` |
| aggregate and by-value form | unchanged topology and attributes | `P06` |
| registered intrinsic | unchanged ID, types, immediates, and effects | `P07` |
| `InlineAsm` | byte-for-byte unchanged opaque payload and unchanged ordinary value edges | post-canonical constraint stage |

An integer width is never rounded to a convenient carrier. A floating value is
never converted through a host floating type. `Undef`, `Poison`, NaN payloads,
signed zero, relocation fragments, and pointer identity remain distinct.

## 4. Transaction and deterministic mutation

`PassId::Legalize` is a module pass requiring `RawVerified`, establishing
`TypesLegal`, and using `RepeatContract::Idempotent`. It uses the pass
framework's one atomic module transaction:

1. check the complete input capability and revisions;
2. inventory and classify every form in canonical module/function/block/order
   sequence without mutation;
3. accumulate deterministic structured diagnostics for every unsupported or
   ownerless form and abort before editing if any exist;
4. intern exact constants in semantic key order;
5. apply descriptor-compatible edits, then deterministic insert/RAUW/erase
   rewrites;
6. derive the core `MutationSummary` and run the P01 postcondition checker plus
   the configured verifier-on-commit gate;
7. commit one revision and let the framework establish `TypesLegal`, or roll
   back the entire candidate.

Declarations and unreachable definitions are inventoried under the same
rules. Reachability cannot hide unsupported semantics. Cancellation, exhausted
deterministic work/entity/diagnostic budgets, postcondition failure, verifier
failure, and commit failure all roll back the complete candidate.

## 5. Exact output checkpoint

A successful invocation publishes one immutable revision with:

- the same `ModuleEpoch` and a framework-derived `ModuleRevision` (unchanged
  for a proven no-op, incremented exactly once for a mutation);
- correspondingly derived function revisions and a complete
  `MutationSummary`;
- `PassProperty::TypesLegal` established by the executor, never by the pass;
- every Raw-only item consumed or assigned the exact preserved disposition in
  section 3;
- only closed target-independent type/opcode forms;
- opaque inline-assembly text and constraint payload unchanged;
- no facts belonging to post-canonical lowering, placement, allocation, or
  emission.

The output is accepted by P02 only after the P01 postcondition checker proves:
no Raw alias remains, boolean uses have `i1` conditions, exact-bit values did
not drift, every wide or special semantic operation has a disposition, every
preserved family has its declared owner, stable-ID/def-use rules hold, and no
forbidden authority was introduced.

## 6. Unsupported and failure behavior

P01 fails closed with a structured, stable rule ID and entity anchor for:

- `StaleInput` or `WrongInputCapability`;
- `UnsupportedPortableSemantics` when no lossless BIR representation exists;
- `MissingDownstreamDisposition` when a valid form has no named later owner;
- `SemanticDrift`, `OpaquePayloadChanged`, or `ForbiddenAuthority` detected by
  the postcondition checker;
- deterministic resource exhaustion, cancellation, verifier rejection, or
  transaction failure.

Failure publishes no revision, property, partial rewrite, analysis result, or
stage token. Diagnostics may describe the rejected form but cannot turn its
rendered spelling into semantic authority. A failure must never be converted
to a preserved catch-all node.
