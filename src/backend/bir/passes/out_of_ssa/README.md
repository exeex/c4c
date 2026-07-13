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
exact target, layout, preparation, current `ProjectedConstraintSet`, schema, and realizability
fingerprints. A merely equivalent graph or a pre-D4 report is stale.

The accepted input has canonical phi or block-argument semantics, exact
incoming coverage by `EdgeKey {source BlockId, successor-slot ordinal}`, valid
core def-use and dominance, and no `ParallelCopy`, `EdgeCopy`, allocation
assignment, `Spill`, or `Reload`. D5 owns only join destruction, edge-local copy
placement, scratch-reservation identities, and CFG splits required to make that
placement explicit. It does not choose homes, coalesce values, create capacity
spills, select concrete move opcodes, or reinterpret ABI or inline-assembly
constraints.

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
are multiple destinations, repeated sources, potential home overlap, or a
cycle. Its semantic rule is simultaneous assignment: all sources are read from
the incoming state and all destinations become visible together. Bundle
entries have canonical destination-ID order, destinations are unique, sources
may repeat, types must match, and a self-copy is removed. A singleton
non-overlapping assignment may use `EdgeCopy`.

D5 also partitions each `ParallelCopy` into deterministic potential-alias
components using the verified layout and abstract requirements and creates one
typed `CopyScratch` reservation identity for every entry in each multi-entry
component. The reservation set is explicit, allocation-only BIR state, not an
implicit temporary or instructions for MIR. Its edge-local live intervals and
finite class/group/width requirements participate in E1 and E2 like every
other allocatable identity. Each reservation is non-spillable and must receive
a legal home that aliases neither the component's transferred homes nor another
simultaneously needed scratch home. This deliberately bounded worst-case set
can snapshot every endangered source even under partial-home overlap;
allocation rejects a candidate that cannot assign it. Reserving the identities
before E1 means neither the later resolver nor MIR introduces a temporary or
repairs allocation.

`EdgeCopy`, `ParallelCopy`, and `CopyScratch` carry the originating `EdgeKey`
as checked provenance; none stores a successor or creates CFG authority. The
semantic `ParallelCopy` and its reservation nodes are intermediate-only. The
D5 copy-resolution closure in section 5 must replace them before E4 while
preserving the exact edge and value transport.

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

Before the initial D5 candidate is verified or published, D5 invokes the sole
shared `ConstraintProjectionTransaction` with the exact D4 projection, new
stamp, initial-D5 occurrence fingerprint, and its join-removal, copy, scratch,
CFG, replacement, and tombstone maps. The resulting `ProjectedConstraintSet`
must be keyed to the initial D5 revision and cover every current occurrence;
projection failure aborts the complete D5 transaction.

Success removes every phi/block-argument instruction and incoming map, admits
only the planned D5 `ParallelCopy`/`EdgeCopy` operations and their explicit
`CopyScratch` reservation identities, advances the exact stage/revision key
with the D5 fingerprint, and reruns the entire `Pseudo` profile over the frozen
module. The full gate checks cumulative D4 realizability for the unchanged
families plus D5 copy legality, exact edge coverage, simultaneous-copy
semantics, complete scratch-reservation coverage, lowered value transport,
CFG/def-use consistency, product freshness, and absence of all phi semantics.
Only that green gate atomically publishes the intermediate D5 `PseudoBir`
consumed by `E1`.

Stale input, incomplete incoming coverage, an unplaceable edge kind, duplicate
copy destination, type mismatch, malformed cycle, ID exhaustion, cancellation,
analysis rebuild failure, or verifier rejection rolls back the complete D5
candidate. Failure publishes no split block, copy subset, tombstone, revision,
analysis result, stage key, or E1 capability; the fully verified D4 publication
remains the last good predecessor.

## 5. Allocation-aware copy-resolution closure

D5 owns a second, subordinate `CopyResolutionTransaction`; it is not a new
top-level stage ID. It runs exactly once on the stable private allocation
candidate after E3 has finished all spill/reload mutation and the final E1/E2
retry has produced current liveness and assignments, and immediately before
the E4 publication gate. It cannot run on an eviction request, a candidate
that still requires an E3 edit, or a candidate carrying stale allocation
facts.

The transaction consumes one `CopyResolutionInputKey` containing the exact
post-E3 `PipelineStageStamp`, D5 fingerprint, target/layout/schema fingerprints,
the current `ProjectedConstraintSet` fingerprint, and the exact E1
liveness, E2 assignment, E3 spill-state, and scratch-reservation fingerprints.
It first builds an immutable `CopyResolutionPlan` for every `ParallelCopy` and
`EdgeCopy`, keyed by instruction ID, originating `EdgeKey`, ordered stable
source/destination identities, assigned abstract homes and alias units, and the
assigned `CopyScratch` reservation set. A plan from an equal-looking
predecessor revision is stale.

For each group, the plan removes assigned-home self-copies, builds the actual
home/alias dependency graph, and emits a deterministic sequence of typed
`EdgeCopy` nodes:

1. acyclic components repeatedly emit a move whose destination cannot destroy
   a still-unread source;
2. overlapping components preserve every simultaneously endangered source in
   distinct assigned non-aliasing scratch homes before the overlapping write;
3. cyclic components save the canonical lowest-destination member to that
   scratch home, rotate the remaining moves, and restore the saved member.

Every emitted `EdgeCopy` has one source endpoint, one destination assignment
role, and one already assigned legal home transfer that the target
realizability table proves maps to exactly one machine instruction. Scratch
endpoints use the preallocated stable `CopyScratch` identity. Resolution never
creates an allocatable identity, changes a home, inserts a capacity spill or
reload, selects a concrete register, or asks MIR to schedule a bundle. An
unused reservation is tombstoned. Used reservation identities remain only as
ordinary assigned endpoints of the emitted copies; their allocation-only
reservation nodes are tombstoned.

The private rewrite preserves unchanged IDs, gives emitted copies fresh
deterministic instruction IDs derived from the plan order, tombstones each
replaced `ParallelCopy`, superseded `EdgeCopy`, and `CopyScratch` node, and
stages one new exact candidate revision plus a
`CopyResolutionFingerprint`. Its preservation record maps every original
simultaneous transfer to the ordered nodes that realize it and proves that
virtual assignments, scratch homes, spill coverage, CFG, and edge-local
placement are unchanged. It publishes no projection or E4 input. The resolved
candidate, replacement map, tombstones, and mutation summary pass privately to
E4's enclosing `AllocatedPublicationTransaction`.

E4 next builds its deterministic frame-action draft from this exact graph and
runs `FrameActionMaterializationTransaction`. Only after the explicit
one-record frame actions have been inserted does the sole shared
`ConstraintProjectionTransaction` project the final revision. E1 then
recomputes `LivenessInterferenceKey`; E2 validates and installs unchanged
assignments without reallocating; E3 validates and installs unchanged spill
state without mutation; the non-mutating final frame owner derives
`FrameRealizationPlan`/`FrameRealizationKey`; and the target registry installs
`TargetRealizabilityKey`. The new frame nodes have fixed ABI/frame roles only,
so E2 cannot choose a home for them.

Each final product names the materialized `PipelineStageStamp`,
`CopyResolutionFingerprint`, `FrameActionFingerprint`, all exact upstream
keys, and applicable schema/target fingerprints. Copy rewriting and frame
materialization invalidate every predecessor projection, E1, E2, E3, frame,
and realizability product. Drafts and predecessor products are immutable
lineage only. Stable IDs, unchanged assignments, and structural equality
cannot relabel or mint a product. The enclosing E4 transaction installs the
materialized revision and all six exact-current products together only after
every owner succeeds.

The pre-E4 verifier gate replays each sequence with alias-unit semantics,
proves read-before-clobber behavior for acyclic, overlapping, and cyclic
groups, checks the preservation record, and requires every remaining copy to
be directly realizable. Any missing/aliasing scratch home, unschedulable
transfer, stale key, illegal move, ID exhaustion, cancellation, or verification
failure discards the complete enclosing candidate. Failure publishes no
partial sequence, tombstone, revision, fingerprint, product, E4 capability, or
MIR input. The predecessor candidate and products remain unchanged. E4 rejects every `ParallelCopy` and
every `CopyScratch` node; MIR therefore receives only directly realizable
single-move `EdgeCopy` nodes and performs no copy resolution or repair.
