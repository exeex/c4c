Status: Active
Source Idea Path: ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Choose One Producer Authority Family

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

## Suggested Next

Implement Step 3 for the selected mutual-exclusion family by publishing the
prepared/prealloc `stack_destination_fan_in_authority` fact only when the
producer can prove exactly two mutually exclusive register sources targeting one
stack destination at the consumer program point.

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

## Proof

No build/test proof was required for this todo-only Step 2 classification
packet. Did not create or overwrite `test_after.log`.
