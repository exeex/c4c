# Pass-Ready BIR Contract

This is the target contract for canonical, mutable BIR. It consumes the current
hazards in Step 1 and the C/A/L/P/X authority partition in Step 2. It does not
prescribe a particular arena implementation, but every implementation must
provide the observable semantics below.

## Ownership, storage, and identity

`Module` exclusively owns symbol/type/global/function tables and allocates all
semantic IDs. Each function owns its blocks, instructions, values, parameters,
and local storage objects. Ownership does not move when storage grows.

```text
FunctionId = {module_epoch, slot, generation}
BlockId    = {FunctionId, slot, generation}
InstId     = {FunctionId, slot, generation}
ValueId    = {FunctionId, kind, slot, generation}
```

- IDs are opaque, nonzero semantic identities, never vector positions, names,
  addresses, traversal numbers, or prepared lookup keys.
- A live ID is unique within its owning module epoch. Slot reuse must increment
  generation; stale IDs never resolve to a new object.
- `FunctionId` remains valid across module growth and function reorder.
  `BlockId`, `InstId`, and function-local `ValueId` remain valid across any
  legal mutation of their function except explicit erasure of that entity.
- Erasing an instruction invalidates its `InstId` and any result `ValueId` only
  after the mutator proves no live uses. Erasing a block similarly invalidates
  its block, contained instruction, and contained value IDs.
- Constants, globals, functions, block arguments/phi results, parameters, and
  instruction results have explicit value kinds. Symbol values use module IDs;
  function-local `ValueId` never aliases a module symbol ID.
- A `Handle<T>` is `{owner pointer, ID}` and resolves on every access. It may be
  copied across mutations but not across module destruction. A temporary
  `Ref<T>`/iterator may contain an address and is valid only until the next
  structural mutation of that owner. APIs must not expose persistent raw
  pointers.
- Storage may be slot-map, arena plus indirection, or equivalent. Tombstones
  and generation checks are required; compacting storage may change addresses
  and dense indices without changing live IDs.
- Semantic iteration order is explicit: module function order, function block
  layout order, and block instruction order are maintained in order lists of
  IDs. Allocation-slot order has no semantics. Mutation APIs state where an ID
  enters those lists.

Display names are optional debug attachments. `LinkNameId` or its future symbol
ID names link-visible objects; stable IDs name IR entities. `BlockLabelId`,
`SlotNameId`, instruction indices, and route indices are not substitutes for
`BlockId`, local-object IDs, or `InstId`.

## Construction state and builders

The public state sequence is:

```text
ModuleBuilder -> RawBir -> CanonicalizationSession -> CanonicalBir
```

`ModuleBuilder` owns interning and creates functions/globals. A
`FunctionBuilder(FunctionId)` provides:

```text
create_block(position, debug_name?) -> BlockId
append_inst(block, opcode, operands, attributes) -> {InstId, ValueId?}
insert_inst(block, before|after InstId, opcode, operands, attributes)
set_terminator(block, Terminator)
add_block_argument(block, type) -> ValueId
create_local(type, size, alignment, attributes) -> LocalId
```

Builders reject foreign-owner IDs and type-invalid operands immediately. They
may temporarily permit incomplete blocks only inside a transaction; publishing
`RawBir` requires every live block to have exactly one terminator and no
unresolved forward-reference token. Phi construction uses `(BlockId, ValueId)`
pairs, never label strings.

## Traversal and mutation API

All structural mutation goes through `FunctionEditor`; direct container access
is const. An editor has exclusive access to one `FunctionId`, a mutation
transaction, and its function analysis manager.

```text
instructions(BlockId) -> stable-ID range
operands(InstId|TerminatorRef) -> mutable ValueUse range
users(ValueId) -> stable Use range
successors(BlockId) -> BlockId range derived from terminator
predecessors(BlockId) -> CFG analysis range

insert_before(InstId, InstSpec) -> InstId
insert_after(InstId, InstSpec) -> InstId
replace_inst(InstId, InstSpec, ResultMapping) -> InstId
erase_inst(InstId)
set_operand(Use, ValueId)
replace_all_uses_with(ValueId old, ValueId replacement)   // RAUW
split_block(BlockId, before InstId) -> BlockId
redirect_edge(BlockId from, BlockId old_to, BlockId new_to,
              PhiTransferPlan)
erase_block(BlockId)
```

Operand traversal includes ordinary operands, phi/block-argument incoming
values, call target/arguments, memory address components, atomic operands, and
terminator conditions/returns. Successor traversal includes every target of
the terminator opcode. New opcodes cannot be registered without traversal and
verifier support.

### Instruction operations

Insertion preserves existing IDs and updates only the explicit order list.
`replace_inst` defaults to preserving `InstId` when opcode/result arity permits;
otherwise it creates a new ID and requires a complete `ResultMapping` before
retiring the old one. Erasure fails while results have users, while the
instruction is referenced by a non-recomputable semantic attachment, or while
it is the block terminator (terminators use their own APIs).

RAUW is typed and atomic. It rejects owner mismatch, type mismatch, illegal
self/cycle formation where applicable, and replacement of constants/symbols in
the wrong namespace. It walks the canonical use-list, including phi and
terminator uses, then verifies the old value has zero users. Analyses are never
RAUW targets; they are invalidated/recomputed.

### CFG operations

The terminator is the sole CFG source of truth. There is no authoritative
successor/predecessor side table.

`split_block(block, before)` creates a block immediately after `block` in layout
order, moves the suffix without changing any moved `InstId`/`ValueId`, transfers
the old terminator to the new block, installs an unconditional branch from old
to new, and rewrites successor phi predecessor keys from old to new. The edit is
transactional; failure restores the original function.

`redirect_edge(from, old_to, new_to, plan)` changes every selected matching
terminator successor and applies one explicit phi plan: map incoming values,
create specified undef/poison values where legal, or prove no phi/block argument
is required. It removes obsolete `from` incomings from `old_to`, adds exactly
one consistent incoming to `new_to`, rejects duplicate/ambiguous edges unless
the terminator models edge identity explicitly, and commits only after local
verification.

`erase_block` requires no incoming edges (or an atomic predecessor rewrite
transaction), removes all outgoing phi incomings, proves contained values have
no external users, and then retires contained IDs.

## Use-lists and invariants

Canonical operands are `ValueId`s. Every operand slot has one reciprocal use
entry `(user entity, operand role/index)`. Mutators maintain def-use eagerly;
the verifier can rebuild and compare it. A definition dominates each ordinary
use; phi/block-argument incoming values dominate the corresponding predecessor
edge. Instruction results are owned by their defining instruction and cannot be
reparented independently.

Each block belongs to exactly one function and each function to exactly one
module. Each live instruction occurs exactly once in one block order list. Each
function-local operand references the same function. Symbol references resolve
through the owning module. Layout order may change without changing identity.

## Verifier

`verify(Module|Function, VerifyMode)` is mandatory at RawBir publication, after
each canonical pass in debug/test configurations, and before constructing
`CanonicalBir`. Failure is structured `(rule, owner ID, entity ID, operand
role, message)` rather than text-only.

The verifier checks:

1. ID epoch/owner/generation validity, uniqueness, tombstones, order-list
   membership, and single ownership.
2. Opcode/result/operand arity and types; constant payload/type agreement;
   symbol, global, local, memory, call, atomic, intrinsic, and attribute rules.
3. Exact reciprocal def-use lists, no live use of erased IDs, and dominance,
   including phi edge dominance.
4. Exactly one legal terminator per block; terminator-derived successors exist
   in the same function; entry and reachability policy; predecessor/phi key
   equality; no missing or duplicate phi incoming per modeled edge.
5. Function signatures and call sites are structurally compatible before ABI
   preparation; declarations have no body and definitions satisfy entry rules.
6. Canonical BIR contains no L fields, P fields, raw pointer/index identity, or
   X access. Optional capsule data is opaque and cannot affect success.
7. Cached A results, when audited, name the current function revision and have
   valid IDs; stale caches are rejected or discarded, never trusted.
8. `CanonicalBir` additionally satisfies the canonicalization profile (no
   forbidden raw opcodes/forms and all required normalization invariants).

## Analyses, revisions, and invalidation

Each function has a monotonically increasing `FunctionRevision`. A committed
transaction increments it once and emits a `MutationSummary` containing changed
instructions, operands, blocks, edges, symbols, and attributes. Analysis results
are immutable `{FunctionId, revision, result}` objects. Dense analysis indices
are local to that object and never escape as semantic IDs.

The function analysis manager supports `get<A>(FunctionId)`, recomputation, and
explicit preservation. A pass returns `PreservedAnalyses`; preservation is
accepted only if the analysis declares that every mutation-summary category is
safe. Unknown mutation or undeclared opcode impact invalidates all function
analyses. Minimum rules:

| Mutation | Required invalidation |
|---|---|
| debug-name-only | Preserve all semantic analyses. |
| operand change / RAUW / instruction replace | Def-use dependent value, provenance, memory, comparison, call/return-chain analyses; CFG only if terminator unchanged. |
| insert/erase/reorder ordinary instruction | Def-use, dominance-sensitive ordering, memory/effect, producer/publication, comparison, call/return analyses. |
| terminator edit, split, redirect, block insert/erase | CFG, predecessors, reachability, dominators/postdominators, loops, phi/SSA, publication, path/provenance/effect, and every analysis declaring CFG dependence. |
| signature/symbol/global change | Call graph and all module/interprocedural analyses; affected function analyses as declared. |

No pass manually edits cache-valid bits. Analyses may be recomputed lazily, but
the revision check makes stale access impossible. Module analyses similarly use
a `ModuleRevision`; adding/moving/removing a function invalidates function-order
and interprocedural results without invalidating stable IDs of untouched bodies.

## Safe function processing and parallelism

Per-function canonical passes receive a read-only `ModuleSymbolView` plus an
exclusive `FunctionEditor`. They may resolve module types, globals, functions,
and immutable facts but cannot add, erase, rename, or mutate them. This permits
parallel processing of distinct functions: each worker owns one editor and one
function analysis manager; module storage and symbol tables are frozen for the
parallel phase.

Cross-function changes use a separate `ModuleEditor` barrier. It stops parallel
function work, takes exclusive module ownership, applies symbol/function-table
changes transactionally, increments `ModuleRevision`, and invalidates call graph
and other interprocedural analyses. Moving a function changes layout order only;
its `FunctionId` and nested IDs remain stable. A module-growth phase completes
before new parallel function work begins.

Workers cannot retain module object pointers, function iterators, analysis
references, or prepared outputs across a barrier. They may retain stable IDs and
resolve them afterward. Diagnostics are collected per function and merged by
module function order for deterministic output.

## Hazard closure matrix

| Step 1 hazard | Prevention or detection |
|---|---|
| instruction insertion/removal/reorder | Stable `InstId` plus explicit order lists; editor maintains use-lists; revision summaries invalidate positional analyses; verifier checks membership and dominance. |
| RAUW | Typed atomic `replace_all_uses_with`, complete operand traversal, reciprocal use-list audit, zero-old-users postcondition. |
| block split | Transactional `split_block` preserves moved IDs, transfers terminator, rewrites phi predecessor IDs, invalidates all CFG-dependent analyses. |
| edge redirect | Terminator-only CFG authority plus required `PhiTransferPlan`; local verifier prevents terminator/phi divergence. |
| function movement | Indirect stable `FunctionId` storage; layout order is separate; module revision invalidates order/interprocedural analyses. |
| module growth | Frozen module during function-parallel phases; exclusive barrier for growth; stable IDs survive reallocation; prepared products and module analyses are invalidated. |
| stale raw pointers/indices | Persistent APIs expose handles/IDs only; temporary references are mutation-scoped; generation and revision checks detect stale access. |
| duplicated side-table authority | Step 2 A data is recomputed and revision-bound, P data is post-canonical output, X is inaccessible, and the verifier rejects L/P/X fields in canonical BIR. |

Every required mutation is therefore either performed atomically through a
contracted API or rejected before commit; every derived consumer is tied to a
revision and cannot silently survive an invalidating edit.
