# Implementation Split

## Question

After selecting explicit rejection as the current destination authority rule,
which single-owner follow-up implementation ideas should be opened, and which
rows must remain blocked?

## Selected Rule Dependency

`docs/destination_fan_in_authority/02_destination_authority_rule.md` selects
explicit rejection as the implementation-ready rule for the current evidence:
a non-parallel move bundle with more than one register source targeting the
same stack destination must fail closed unless prepared/prealloc producer
metadata explicitly proves ordering, mutual exclusion, or merge authority at
the consumer program point.

The current row family therefore remains rejected or blocked. Step 1 records
the authoritative planned population as the `125` rows classified by
`docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md` as
`non-parallel multi-source stack destination`, with representative logs such
as `build/rv64_gcc_c_torture_backend/src_pr43236.c/case.log`,
`build/rv64_gcc_c_torture_backend/src_20000113-1.c/case.log`, and
`build/rv64_gcc_c_torture_backend/src_pr48814-2.c/case.log`.

The detailed representative logs publish the blocking vocabulary:

- `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`
- `unsupported_prepared_move_bundle_classification: non-parallel`
- `register-source fan-in to one stack destination has no ordering or mutually-exclusive authority`
- `authority=none`
- `parallel_copy=no`
- `fragment_status=producer_authority_missing_for_register_fan_in_stack_destination`

That vocabulary assigns the missing fact to producer authority. RV64 target
materialization is the reporting consumer, not the source of destination
legality.

## Producer-Authority Owner

Single owner: prepared/prealloc authority production.

This owner may open one follow-up implementation idea only after accepting a
specific producer contract. The idea must choose exactly one authority family
for the first implementation packet:

| Candidate producer contract | Required producer fact | Proof surface |
| --- | --- | --- |
| Ordering authority | The producer publishes a stable ordered sequence for the same stack destination and designates the final authoritative stack-slot state. | Prepared/prealloc unit or focused backend rows showing the new ordered authority metadata on the bundle and each authorized move; diagnostics must still reject `authority=none`, missing final-state designation, and stale or incomplete source freshness. |
| Mutual-exclusion authority | The producer publishes predicates, edges, or carrier metadata proving exactly one candidate write is active at the consumer program point. | Focused rows where the diagnostic exposes the selected active candidate or predicate evidence; negative rows with missing, overlapping, or stale predicates must remain rejected before RV64 materialization. |
| Merge authority | The producer publishes semantic equivalence or an explicit merge operation for all candidate sources targeting the stack destination. | Prepared/prealloc proof showing the merge identity or explicit merge operation plus source freshness for participating sources; non-equivalent or incomplete merge metadata must fail closed. |

The producer idea must not ask RV64 to choose between multiple sources by
testcase name, move-vector order, or final assembly convenience. It must also
not reuse `PreparedValueFreshnessUseKind::MoveBundleSource` as destination
authority: `ideas/closed/587_prepared_value_freshness_authority_mvp.md` and
`ideas/closed/588_shared_prealloc_move_operand_source_freshness_inventory.md`
establish source freshness surfaces, while Step 2 keeps destination legality
as a separate producer fact.

The current code surface already has fail-closed categories for this boundary:
`src/backend/prealloc/prepared_object_traversal.hpp` names
`ambiguous_non_parallel_multi_source_stack_destination`,
`unsupported_non_parallel_multi_source_stack_destination_authority`, and
`mismatched_stack_destination_register_fan_in_move_authority`. In
`src/backend/prealloc/prepared_object_traversal.cpp`,
`classify_prepared_object_move_bundle_consumer` rejects ambiguous same-stack
fan-in when `authority_kind` is `None`, requires matching
`StackDestinationRegisterFanIn` authority on the bundle and all moves when
that authority is present, and otherwise rejects unsupported destination
authority. A producer follow-up must make those authority facts meaningful; it
must not weaken the rejection.

## RV64 Consumption Owner

Single owner: RV64/MIR prepared move-bundle consumer.

This owner is downstream of producer authority. RV64 may open a follow-up
implementation idea only after prepared/prealloc publishes an accepted
destination authority contract and current rows carry that metadata. The RV64
idea must be limited to consuming already-authorized move bundles.

Allowed RV64 work after producer authority exists:

- materialize an ordered final stack-destination state when the producer
  designates that state;
- materialize a mutually exclusive selected candidate or guarded/edge-specific
  copies when the producer publishes complete predicate authority;
- materialize an explicit merge result or skip a redundant copy only when the
  producer publishes semantic merge authority;
- preserve the current rejection diagnostics for missing, unsupported, or
  mismatched producer authority.

RV64 proof surface:

- positive focused rows whose logs no longer stop at
  `ambiguous_non_parallel_multi_source_stack_destination` because the producer
  has published one accepted destination authority fact;
- negative rows for `authority=none`, unsupported authority kinds, mismatched
  bundle-versus-move authority, and missing source freshness;
- no expectation rewrites, unsupported-marker downgrades, allowlist changes,
  timeout/accounting changes, or named-case-only materialization.

`ideas/open/610_rv64_move_bundle_target_materialization.md` already states
this consumer boundary: RV64 move-bundle materialization requires upstream
prepared source and destination authority, and rows lacking source, stack
slot, branch operand, or destination authority must remain rejected with
accurate diagnostics. The destination fan-in rows from this research remain
outside RV64 ownership until the producer fact exists.

## Blocked Current Rows

The following rows and row shapes must remain rejected or blocked until
producer destination authority exists:

| Row or shape | Current status | Reason |
| --- | --- | --- |
| The `125` July 8 recovery-map rows classified as `non-parallel multi-source stack destination` | Blocked | The selected rule is explicit rejection until producer metadata proves ordering, mutual exclusion, or merge authority. |
| Detailed rows such as `src/pr43236.c`, `src/20000113-1.c`, `src/20040409-1w.c`, `src/20021010-2.c`, and `src/20021120-3.c` | Rejected | Logs show `authority=none`, `parallel_copy=no`, two register sources, and one shared stack destination. |
| Short-form rows such as `src/pr48814-2.c` | Rejected | The classifier reports ambiguous non-parallel multi-source stack-destination authority without a producer fact. |
| Any current-workdir extra hits beyond the planned `125` count | Not a count rewrite | Step 1 treats the raw `128` grep count as representative current-log evidence only; changing the row population belongs to supervisor or plan-owner direction, not this split. |

## Follow-Up Idea Shape

No follow-up idea is opened by this documentation packet. If the supervisor or
plan owner later opens implementation work, it should be one of these
single-owner shapes:

1. Prepared/prealloc producer idea: define and publish exactly one destination
   authority contract for non-parallel register-source fan-in to one stack
   destination, with source freshness checked separately.
2. RV64 consumer idea: after producer authority exists, consume that one
   contract and keep all missing, unsupported, or mismatched authority rows
   rejected.

Do not combine those into one implementation idea. Combining them would make
RV64 appear to prove destination legality through target materialization, which
is the route Step 2 rejected.

## Conclusion

The implementation ownership split is resolved:

- producer/prealloc owns destination authority production;
- RV64 owns only consumption of explicitly authorized destination fan-in;
- the current `125` row family remains blocked by design;
- proof for future producer work must expose ordering, mutual-exclusion, or
  merge metadata before target consumption;
- proof for future RV64 work must demonstrate consumption of already-authorized
  bundles while preserving rejection for blocked, rejected, unsupported, and
  mismatched authority shapes.

This packet documents ownership only. It does not open implementation ideas,
modify lifecycle ideas, or change runtime behavior.
