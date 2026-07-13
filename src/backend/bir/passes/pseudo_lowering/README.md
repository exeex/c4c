# Generic Pseudo Lowering Pass

Status: converged design contract (unimplemented).

## D1 boundary and exact input

`D1` is the sole generic lowering transaction from semantic `CanonicalBir` to
the closed pseudo schema. It consumes all of the following from one Step 7
transaction:

- the immutable `CanonicalBir` and its complete `PipelineStageStamp`;
- `VerifiedTargetLayout` with the exact `TargetFingerprint` and layout-schema
  fingerprint;
- the atomic `VerifiedPreparationBundle`, including its ordered product
  fingerprints; and
- the complete `BoundConstraintSet` keyed to that same Canonical stamp, target,
  layout, bundle, source-description digest, and operand/result identities.

Module-revision equality, a compatible-looking target, or equal semantic text
is insufficient. A missing, mixed, or stale product rejects the transaction
before mutation. D1 reads Canonical storage; it never writes target facts back
into that published revision.

## Sole lowering ownership

D1 converts every admitted Canonical semantic instruction to the corresponding
generic pseudo family declared by the pseudo schema. It selects reviewed
helper interfaces from `RuntimeHelperPlan`, address forms from `AddressPlan`,
and abstract class/group requirements from the verified layout and constraint
facts. It preserves the original bytes and ordinary value edges of
`InlineAsm`, and projects each bound constraint onto those same identities.

Helper-eligible operations become `GenericCall` nodes, not private helper
sequences. Ordinary calls and helper calls therefore reach `D2` through the
same call family. D1 may attach the verified ABI/call requirement handles that
D2 needs, but it may not create argument moves, outgoing-call stores, hidden
result transport, caller-clobber effects, or preservation operations.

D1 does not own shared ABI-aware call lowering (`D2`), target legalization and
one-to-many expansion (`D4`), out-of-SSA (`D5`), home assignment, pressure
eviction, spill/reload insertion, machine instruction selection, assembler
parsing, or late layout. It must not encode an assignment decision in an
instruction variant, operand, requirement, or side product.

## Closed disposition and fail-closed behavior

Every input instruction has exactly one disposition: preserve `InlineAsm` as
specified above, lower it to one admitted generic pseudo node, replace it with
an admitted generic pseudo subgraph, route it to `GenericCall`, or reject it.
There is no `Unknown`, `Unsupported`, opaque semantic escape node, legacy
payload, renderer-text fallback, or allocation escape hatch. A one-to-many
transformation must make every introduced definition, use, effect, and CFG
edge ordinary BIR state.

Lowering rejects unsupported semantics, unavailable helper interfaces,
unrepresentable address or type shapes, absent layout capacity, incompatible
requirements, malformed bound `InlineAsm`, and any operation for which the
schema has no exact disposition. Failure reports stable source identities and
publishes no pseudo revision, mapping, property, or reusable partial result.

## Identity, revision, and transaction

D1 forks one private candidate from the exact Canonical revision. An entity
whose semantic record and ordered ownership are unchanged keeps its stable ID.
A rewritten instruction keeps its `InstId` only when it is the unique semantic
continuation with compatible result identities; additional nodes and results
receive fresh IDs, and removed entities are tombstoned. Unchanged blocks,
values, symbols, types, origins, and debug references keep their IDs. IDs never
stand in for freshness.

Any mutation, including an in-place-ID rewrite, advances the appropriate
function revision and publishes a new complete `PipelineStageStamp`. The
candidate carries a `PseudoStageKey` containing its exact new stamp, its parent
Canonical stamp, target fingerprint, layout fingerprint, preparation-bundle
fingerprint, constraint-set fingerprint, pseudo-schema fingerprint, and D1
lowering-schema fingerprint. A deterministic replacement map records old-to-
new/tombstoned identities for diagnostics and derived-fact projection; it is
not semantic authority.

Lowering, ID repair, def-use rebuild, projected-constraint construction, and
candidate freeze are one transaction. Cancellation, resource failure,
diagnostics, incomplete coverage, a revision change, or failure of the D1
schema check discards the entire candidate and all derived facts. The exact
immutable D1 candidate alone may enter `D2`; D1 does not mint the public
`PseudoBir` capability.

## D2 and D3 handoff

The exact immutable D1 candidate alone may enter the dedicated
[`D2` shared ABI-aware call-lowering contract](../call_lowering/README.md).
That owner consumes the complete D1 and C3/C4/product keys, eliminates every
`GenericCall`, advances the revision, and supplies D3 with one frozen complete
candidate. This section is only the D1 output handoff; the D2 document owns
admitted operations, ABI-rule selection, revision and identity behavior,
preservation/invalidation, failure atomicity, and D3/D4 adjacency.
