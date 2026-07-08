# Destination Authority Rule

## Question

What destination legality rule is needed for the `125` non-parallel
multi-source stack-destination rows: ordering, mutual exclusion, merge
authority, or explicit rejection?

## Evidence Base

The Step 1 boundary in
`docs/destination_fan_in_authority/01_current_failure_shapes.md` classifies
the family as rows whose first stopping diagnostic is
`prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`
or the equivalent short classifier rejection for ambiguous non-parallel
multi-source stack-destination authority. Representative detailed logs include
`build/rv64_gcc_c_torture_backend/src_pr43236.c/case.log`,
`build/rv64_gcc_c_torture_backend/src_20000113-1.c/case.log`,
`build/rv64_gcc_c_torture_backend/src_20040409-1w.c/case.log`,
`build/rv64_gcc_c_torture_backend/src_20021010-2.c/case.log`, and
`build/rv64_gcc_c_torture_backend/src_20021120-3.c/case.log`.

Those detailed rows report the same shape:

- `unsupported_prepared_move_bundle_classification: non-parallel`
- `register-source fan-in to one stack destination has no ordering or mutually-exclusive authority`
- `authority=none`
- `parallel_copy=no`
- two `register` sources
- one shared `stack_slot` destination
- `diagnostic_owner=rv64_prepared_move_bundle_consumer`
- `fragment_status=producer_authority_missing_for_register_fan_in_stack_destination`

The relevant authority context is:

- `ideas/closed/587_prepared_value_freshness_authority_mvp.md` introduced
  queryable source freshness authority and explicitly separated move-bundle
  source freshness from destination authority.
- `ideas/closed/588_shared_prealloc_move_operand_source_freshness_inventory.md`
  records that `classify_prepared_object_move_bundle_consumer` already queries
  `PreparedValueFreshnessUseKind::MoveBundleSource` through
  `find_prepared_value_freshness_authority`, while direct edge-publication
  moves and several adjacent shared-prealloc consumers remain contract-blocked.
- `ideas/open/607_destination_fan_in_authority_research.md` requires this
  research to decide whether non-parallel move bundles may choose among
  multiple candidate stack destinations before implementation proceeds.

This means the current rows are not proven by source freshness alone. A source
can be fresh enough to read while the destination write remains illegal because
two different source writes target the same stack slot in a non-parallel
bundle with no published destination authority.

## Alternatives Considered

### Ordering Authority

Rule shape:

Accept the bundle if the prepared/prealloc producer publishes an ordered
sequence for the same stack destination, such as "write source A first, then
source B", and marks the final write as the authoritative destination state at
the relevant program point.

Required producer facts:

- a stable program-point ordering for every move in the non-parallel bundle;
- proof that the ordered writes are semantically intentional, not an artifact
  of source vector order or current traversal order;
- a designated final authoritative source for the stack destination;
- proof that any overwritten earlier write is dead or is an intentional
  temporary write;
- source freshness facts for each accepted source at its ordered use point.

Decision:

Not implementation-ready for the current family. The representative logs say
`authority=none` and explicitly name the missing fact as no ordering or
mutually-exclusive authority. Accepting the first or last listed move would be
source-order accident, not destination authority. RV64 final assembly
convenience also cannot supply this fact because the producer must state why
the destination sequence is legal before target materialization consumes it.

### Mutual Exclusion Authority

Rule shape:

Accept the bundle if the producer proves that only one source can reach the
shared stack destination for the concrete execution path being materialized.
The apparent fan-in would then be a compact representation of mutually
exclusive alternatives, not two writes that both execute.

Required producer facts:

- branch/path predicates or carrier metadata tying each candidate write to a
  mutually exclusive predecessor, edge, or condition;
- proof that exactly one candidate is active at the destination program point;
- a selected active candidate, or enough predicate metadata for the consumer
  to materialize guarded/edge-specific copies;
- source freshness facts for each candidate under its own predicate;
- a fail-closed status when predicates are missing, overlapping, or stale.

Decision:

Not implementation-ready for the current family. The documented rows are
`event_kind=before_instruction_copies`, `phase=before_instruction`, and
`parallel_copy=no`, with no predicate, edge, or carrier field proving mutual
exclusion. Treating same-destination moves as mutually exclusive merely
because a testcase happens to execute one path would overfit runtime shape and
would not repair prepared/prealloc destination authority.

### Merge Authority

Rule shape:

Accept the bundle if the producer publishes a merge fact proving that multiple
candidate sources represent the same semantic value for the stack destination,
or that the destination has an explicit merge operation whose result is the
authoritative stack-slot content.

Required producer facts:

- a merge identity tying all candidate sources to the same destination value,
  with value ids or alias facts strong enough for target consumption;
- proof that the merge is semantic equivalence or an explicit merge operation,
  not merely storage reuse;
- selected source freshness for every source that participates in the merge;
- a consumer-visible rule for materializing the merge result, including
  whether one source may be chosen, whether a copy is unnecessary, or whether
  a real merge operation must be emitted;
- fail-closed diagnostics for non-equivalent values, incomplete merge
  metadata, or conflicting destination values.

Decision:

Not implementation-ready for the current family. The Step 1 rows expose
multiple register sources targeting one stack slot, but they do not publish
semantic equivalence, alias closure, or an explicit merge operation. Choosing
the easiest register to store would be target convenience. Assuming equivalence
from a shared stack destination would invert the authority relation: the
destination conflict is the problem to prove, not the proof.

### Explicit Rejection

Rule shape:

Reject non-parallel multi-source writes to one stack destination unless the
producer publishes one of the destination authority facts above. The RV64
consumer remains allowed to consume only bundles with explicit ordering,
mutual-exclusion, or merge authority, and otherwise reports a precise
prepared/prealloc authority gap.

Required producer facts:

- none beyond the current classifier evidence for rejection;
- for a future acceptance route, the producer must publish an ordering,
  mutual-exclusion, or merge fact before RV64 consumption;
- source freshness facts remain required for accepted sources, but freshness
  alone is not enough to accept destination fan-in.

Decision:

Implementation-ready as the current destination authority rule. The rule is:

> A non-parallel move bundle with more than one register source targeting the
> same stack destination must fail closed unless prepared/prealloc producer
> metadata explicitly proves destination ordering, mutual exclusion, or merge
> authority for that stack destination at the consumer program point.

This does not implement a new acceptance path for the `125` rows. It makes the
current rejection the selected authority rule and defines the producer facts
that any later acceptance implementation must add before RV64 may consume the
shape.

## Rejected Routes

The following routes are explicitly rejected as non-authoritative:

- testcase shape: named torture files, function names, and observed runtime
  paths cannot decide destination legality;
- source order accident: first-listed or last-listed move selection is not a
  prepared/prealloc authority fact;
- target convenience: RV64 register choice, instruction count, or final
  assembly appearance cannot supply producer destination authority;
- destination-only inference: a shared stack slot does not prove source
  equivalence, source freshness, ordering, or mutual exclusion;
- source-freshness substitution: `PreparedValueFreshnessUseKind::MoveBundleSource`
  can prove a source is valid to read, but it does not prove that two sources
  may write the same destination in a non-parallel bundle.

## Source Freshness Versus Destination Authority

Source freshness asks whether a particular source is fresh enough for a
particular use. Destination authority asks whether a particular destination
write is legal when more than one source claims the same destination.

Ideas `587` and `588` are necessary context because they prevent stale-source
acceptance and make move-bundle source authority queryable. They do not close
this destination fan-in gap. A future route that accepts ordering, mutual
exclusion, or merge authority must still consult source freshness for the
selected source or sources, but destination authority must be published and
checked as its own producer fact.

## Selected Rule

Selected implementation-ready authority rule: explicit rejection until producer
destination authority exists.

The RV64 prepared move-bundle consumer should continue to fail closed for the
current family. The producer-side follow-up, if prioritized, must add one
specific destination authority contract before target consumption changes:

- ordering authority with a designated final stack destination state;
- mutual-exclusion authority with path/edge predicates and a selected active
  candidate; or
- merge authority with semantic equivalence or an explicit merge operation.

Until one of those producer facts exists, the documented `125` row family must
remain blocked from implementation acceptance. This is a resolved rule, not an
unresolved decision: reject ambiguous non-parallel multi-source stack
destination bundles unless explicit destination authority is present.
