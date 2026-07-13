# P03 CFG Canonicalization Pass Contract

Status: closed architecture contract. Implementation is deferred until the BIR
architecture is accepted.

`P03` is the `S04` function transformation. It converts the exact committed
P02 output into one structurally canonical CFG. A block terminator and its
ordered successor slots are the sole stored source of control-flow edges.
Predecessor sets, reachability, traversal order, and dense numbering are
recomputed analysis facts and are never written into BIR as competing truth.

## 1. Exact input and edge model

The input is one immutable function view with exact epoch, module revision,
`FunctionId`, function revision, and `PassProperty::ScalarsCanonical`. P02
postconditions and the configured input verifier must accept that same
revision. Missing or malformed terminators, foreign block references, stale
capability stamps, or a stored edge side table reject the invocation before
mutation.

Every successor occurrence has the stable structural key:

```text
EdgeKey = { source BlockId, successor-slot ordinal }
destination = source terminator.successor[successor-slot ordinal]
```

The slot ordinal is interpreted by the typed terminator schema. Two slots may
name the same destination, so switches, indirect branches, asm-goto, and other
parallel edges remain distinct. `BlockId` identifies a block; `EdgeKey`
identifies one edge occurrence. Names, rendered text, layout adjacency, and
fallthrough guesses identify neither.

## 2. Closed P03 authority

P03 owns deterministic unreachable-block removal, branch and switch folding,
successor-slot canonicalization, block split/merge, and registered critical-
edge preparation required by the canonical CFG profile. It may reorder blocks
only by the closed semantic block-order rule; block order never adds an edge.
It preserves inline-assembly text and constraints opaquely while retaining the
typed asm-goto successor topology.

All edits use a function transaction and a CFG editor. An edit plan describes
old `EdgeKey`s, replacement terminators and slots, new blocks/IDs, instruction
moves, and the complete phi or block-argument incoming rewrite. In particular:

- splitting an edge replaces its old key with the two explicit new edges and
  transfers the old incoming value to the new destination-facing key;
- splitting a block moves one suffix, installs valid terminators on both
  blocks, and rewrites affected incoming keys in the same transaction;
- merging blocks requires one eligible connecting occurrence, composes the
  instruction lists, and remaps every affected incoming by explicit edge key;
- folding or deleting a successor slot deletes exactly that occurrence and its
  incoming, even when another slot has the same destination;
- deleting an unreachable region removes its instructions, uses, edges, and
  incoming entries as one closed plan.

P03 does not construct SSA, choose phi values, schedule copies, change memory
or call semantics, interpret inline-assembly constraints, or make target,
layout, placement, allocation, or machine decisions.

## 3. Analysis use and invalidation

P03 may request exact-revision CFG and reachability facts to plan a proposal.
Dominance may be requested only by a registered subpass whose rule names it.
Analysis handles are immutable planning inputs and cannot edit the graph.

Any committed terminator, successor slot, block membership/order, or relevant
instruction/use change invalidates CFG, dominance, loop, liveness,
publication/value-flow, and SSA-derived results unless the framework validates
a declared complete preservation proof and installs a new result under the
new revision key. Old handles remain stale. P03 cannot patch an analysis cache
as a substitute for mutating the terminator and repairing its uses.

## 4. Transaction, output, and repetition

`PassId::CfgCanonicalize` is a `Function` pass requiring
`ScalarsCanonical`, establishing `CfgCanonical`, and using
`RepeatContract::Idempotent`. It inventories candidate rewrites in canonical
stable-ID order, reserves deterministic IDs, builds the complete edit plan,
applies it to the private candidate, derives the authoritative mutation
summary, and runs CFG plus def-use postconditions and verifier-on-commit.

Success publishes one immutable S04 function revision in which every block has
one valid terminator, every edge is derivable from a successor slot, every phi
or block-argument incoming covers the exact live `EdgeKey` multiset, and all
split/merge effects are reflected in core def-use. An unchanged invocation
keeps the revision and proves the no-op condition.

Any stale input/analysis, unsupported terminator topology, unrepairable
incoming, invalid split/merge, deterministic budget exhaustion, cancellation,
or verifier failure rolls back the complete occurrence candidate. No partial
block, edge, incoming repair, analysis result, property, or revision is
published.
