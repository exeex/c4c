Status: Active
Source Idea Path: ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish Selected Producer Authority

# Current Packet

## Just Finished

Step 2 selected the first producer authority family from the refreshed Step 1
evidence: mutual-exclusion stack-destination register fan-in authority for a
non-parallel, two-register-source move bundle where both register sources target
the same stack destination at one consumer program point.

Selected first target row: `src/20021204-1.c` at `main:tern.end.12` before
instruction 1. The refreshed evidence shows owner
`rv64_prepared_move_bundle_consumer`, consumer category
`ambiguous_non_parallel_multi_source_stack_destination`, event kind
`before_instruction_copies`, `authority=none`, `move_count=2`,
`parallel_copy=no`, register sources 19 and 20, stack destination value 18, and
`fragment_status=producer_authority_missing_for_register_fan_in_stack_destination`.

Minimal positive scope:

- Primary positive: `src/20021204-1.c`, two register sources 19 and 20 to stack
  destination value 18 at `main:tern.end.12` before instruction 1.
- Same-family corroborating rows, still not required for the first
  implementation packet: `src/920429-1.c`, `src/930429-1.c`,
  `src/pr34415.c`, `src/ptr-arith-1.c`, and `src/pr70005.c`, each with exactly
  two register sources, one stack destination, `parallel_copy=no`, and current
  status
  `producer_authority_missing_for_register_fan_in_stack_destination`.

Minimal negative scope:

- `src/20011109-2.c` remains separate: it is the only refreshed row with
  `missing_stack_destination_fan_in_authority_fact`, `move_count=3`, and a
  register + preserved-stack + register select-materialized stack destination.
- Reject non-selected family shapes: ordered final-state-only rows, three-or-
  more-source stack fan-ins, register + preserved-stack mixes, stack-to-stack
  sources, parallel-copy bundles, and any bundle where producer evidence cannot
  prove mutual exclusion at the consumer point.
- Negative statuses to preserve:
  `producer_authority_missing_for_register_fan_in_stack_destination`,
  `missing_stack_destination_fan_in_authority_fact`,
  `unsupported_prepared_move_bundle_classification`,
  `ambiguous_non_parallel_multi_source_stack_destination`,
  stale source or destination authority, and bundle-versus-individual-move
  authority mismatch.

Proposed prepared/prealloc fact shape:

- Publish a `stack_destination_fan_in_authority` fact at the consumer program
  point with authority family `mutual_exclusion_register_stack_destination`.
- Required fields: function, block label/index, instruction index, phase
  `before_instruction`, destination value id, destination storage `stack_slot`,
  source value ids, source home kinds all `register`, `move_count=2`,
  `parallel_copy=no`, producer proof of mutual exclusion for the two sources,
  and the source-to-destination move bundle identity consumed by RV64.
- Owner label: `prepared_mutual_exclusion_stack_destination_fan_in_producer`
  for publication, consumed by `rv64_prepared_move_bundle_consumer`.
- Consumer point: the exact prepared/prealloc move bundle before the consuming
  instruction, not source order, move-vector order, final assembly, diagnostic
  wording, or testcase identity.

Residual rows intentionally left out of scope for the first family:
`src/20011109-2.c` and any future row that is select-materialized
preserved-stack, ordered-final-state-only, ambiguous without mutual-exclusion
proof, stale, mismatched between bundle and individual moves, or not exactly a
two-register-source non-parallel stack-destination fan-in.

Step 3 has had two rejected implementation attempts and must be narrowed before
another code packet:

- The reviewer rejected the first attempt in
  `review/647_step3_route_review.md`: it accepted manually stamped
  `StackDestinationRegisterFanIn` authority in consumer/classification paths
  instead of publishing authority from a real prepared/prealloc producer proof.
- The supervisor rejected the follow-up producer attempt: same-block
  source-producer-before-consumer evidence proves source availability/order
  only. It does not prove the required mutual-exclusion predicate or edge
  relationship for the selected family.
- The real selected row `src/20021204-1.c` still dumped `authority=none` and
  failed with
  `producer_authority_missing_for_register_fan_in_stack_destination` after the
  follow-up attempt, so Step 3 remains incomplete.

## Suggested Next

Continue Step 3 only as a narrow producer-proof discovery/publication packet
for the selected mutual-exclusion family:

- Find the real prepared/prealloc evidence that proves the two register-source
  producers for `src/20021204-1.c` are mutually exclusive at
  `main:tern.end.12` before instruction 1. Acceptable evidence must be an
  explicit predicate/edge/control-flow mutual-exclusion proof at the consumer
  program point, not source availability, same-block order, source-producer
  freshness, value id shape, testcase identity, diagnostics, or final assembly.
- If that proof exists, publish `stack_destination_fan_in_authority` with owner
  `prepared_mutual_exclusion_stack_destination_fan_in_producer` from the
  producer surface, starting from `authority=none`, and add focused proof that
  the selected row or fixture is stamped before consumer classification.
- If no mutual-exclusion predicate/edge proof exists for the selected row,
  stop Step 3 and return to Step 2 family selection with evidence. The likely
  revision is to choose an ordered-final-state family or split a separate
  proof-discovery idea rather than broadening mutual-exclusion by order.

## Watchouts

- Do not treat this as another idea 637 select-materialized semantic-merge
  packet.
- Do not infer authority from testcase identity, move-vector order, source
  order, diagnostics, final assembly, or expectations.
- Keep `20011109-2.c` out of this family unless later evidence proves a true
  mutual-exclusion two-register-source shape; current refreshed evidence does
  not.
- Keep the other five same-family rows as corroborating positive candidates,
  not as named-case shortcuts for implementation.
- Fail closed if producer proof is missing, stale, ambiguous, or mismatched
  against the move bundle consumed by RV64.
- Same-block source-producer-before-consumer evidence is insufficient for this
  selected family. It may show that each source exists before the consumer, but
  it does not show that the incoming producers are mutually exclusive.
- Do not revive the manually stamped authority route rejected by
  `review/647_step3_route_review.md`; consumer acceptance can be tested only
  after real producer publication exists.
- Do not route through RV64 consumption before the prepared/prealloc producer
  publishes explicit selected-family authority.

## Proof

No build/test proof was required for this lifecycle repair. Did not create or
overwrite `test_after.log`.
