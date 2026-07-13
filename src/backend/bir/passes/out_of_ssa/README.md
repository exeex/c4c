# D5 Out-of-SSA Pass Contract

Status: closed architecture contract. Implementation is deferred until the BIR
architecture is accepted.

`D5` is the only normal phi/block-argument destruction stage. It runs after
target legalization and immediately before `E1` allocation liveness. MIR does
not discover joins, choose incoming values, repair phi transport, split edges,
or perform another out-of-SSA pass.

## 1. Exact input and closed ownership

D5 consumes the exact immutable `PseudoBir` revision published by the complete
D4 full-profile gate. Its `PseudoStageKey` must contain the current module and
ordered function revisions, the applicable D1/D2/D4 fingerprints, and the
exact target, layout, preparation, constraint, schema, and realizability
fingerprints. A merely equivalent graph or a pre-D4 report is stale.

The accepted input has canonical phi or block-argument semantics, exact
incoming coverage by `EdgeKey {source BlockId, successor-slot ordinal}`, valid
core def-use and dominance, and no `ParallelCopy`, `EdgeCopy`, allocation
assignment, `Spill`, or `Reload`. D5 owns only join destruction, edge-local copy
placement, and CFG splits required to make that placement explicit. It does
not choose homes, coalesce values, create capacity spills, select concrete move
opcodes, or reinterpret ABI or inline-assembly constraints.

## 2. Complete edge-copy plan

Before mutation, D5 snapshots every phi/block argument and builds one closed
plan in stable `(destination BlockId, phi/result order, EdgeKey)` order. For
each exact incoming occurrence `e` carrying `source` into join result `dest`,
the plan contains the typed assignment `dest <- source` on `e`. Distinct
successor slots remain distinct even when they have the same source and
destination blocks. Rendered labels, predecessor-vector positions, block
layout, and destination equality never select an incoming value.

The former join-result `ValueId` remains the stable virtual allocation identity
`dest`; ordinary uses keep that ID. Removing its phi instruction changes it
from an SSA definition to a D5 copy-destination identity. Only admitted copy
destination roles may assign such an identity, and the post-D5 verifier checks
that every reachable use receives exactly the planned edge assignment. New
split blocks and copy instructions receive fresh deterministic IDs. Unchanged
blocks, values, and instructions retain their IDs; removed joins are
tombstoned. IDs never preserve a revision-bound product by themselves.

Assignments for one edge are emitted as one atomic `ParallelCopy` when there
are multiple destinations, repeated sources, overlap, or a cycle. Its semantic
rule is simultaneous assignment: all sources are read from the incoming state
and all destinations become visible together. This makes swaps and longer
cycles well-defined without a D5 scratch value. A singleton non-overlapping
assignment may use `EdgeCopy`. Bundle entries have canonical destination-ID
order, destinations are unique, sources may repeat, types must match, and a
self-copy is removed. `EdgeCopy` and `ParallelCopy` carry the originating
`EdgeKey` as checked provenance; neither operation stores a successor or
creates CFG authority. Later allocation and verified MIR mapping may realize
the admitted simultaneous operation, but may not change which edge or values
it transports.

## 3. Edge-local placement and CFG edits

Every bundle must execute on exactly one successor-slot occurrence and on no
other path. D5 chooses the first applicable canonical placement:

1. when the source terminator has exactly one live successor occurrence, put
   the bundle at the end of the source block before that terminator;
2. otherwise, when the destination has exactly one live incoming occurrence,
   put it at the beginning of the destination after the removed join prefix;
3. otherwise split that exact `EdgeKey`, including every critical edge and
   every ambiguous parallel-edge occurrence, and put the bundle in the fresh
   edge block before its unconditional jump.

The split replaces the old successor slot with an edge to the new block and
adds the new block's sole successor to the old destination. The terminators
remain the sole source of edge truth. The transaction records the old key, both
replacement keys, the fresh block/instruction IDs, and all affected uses and
analyses. It does not leave a side table that callers must consult to recover
control flow. Exceptional edge kinds that cannot be split without changing
their registered semantics reject the whole D5 candidate.

## 4. Transaction, invalidation, and output

D5 plans the whole module from the frozen D4 input, reserves deterministic IDs,
applies all join removals, copies, and edge splits to one private candidate,
and derives a complete `MutationSummary`. Any changed instruction, assignment
role, terminator, successor slot, block membership, or function revision
invalidates affected CFG, reachability, dominance, loops, SSA, def-use,
publication/value-flow, liveness/interference, constraints, and realizability
facts unless a registered complete preservation proof applies. Required
post-edit CFG, lowered-value def-use, and copy-coverage facts are recomputed
under the new revision; facts from D4 are never relabeled.

Success removes every phi/block-argument instruction and incoming map, admits
only the planned D5 `ParallelCopy`/`EdgeCopy` operations, advances the exact
stage/revision key with the D5 fingerprint, and reruns the entire `Pseudo`
profile over the frozen module. The full gate checks cumulative D4
realizability for the unchanged families plus D5 copy legality, exact edge
coverage, simultaneous-copy semantics, lowered value transport, CFG/def-use
consistency, product freshness, and absence of all phi semantics. Only that
green gate atomically publishes the D5 `PseudoBir` consumed by `E1`.

Stale input, incomplete incoming coverage, an unplaceable edge kind, duplicate
copy destination, type mismatch, malformed cycle, ID exhaustion, cancellation,
analysis rebuild failure, or verifier rejection rolls back the complete D5
candidate. Failure publishes no split block, copy subset, tombstone, revision,
analysis result, stage key, or E1 capability; the fully verified D4 publication
remains the last good predecessor.
