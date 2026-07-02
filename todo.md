Status: Active
Source Idea Path: ideas/open/516_rv64_multi_source_prepared_move_bundle_classification.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define The Producer Classification Contract

# Current Packet

## Just Finished

Step 2 - Define The Producer Classification Contract completed for the
multi-source prepared move bundle seen in
`tests/c/external/gcc_torture/src/20010518-1.c`.

Producer/classifier boundary:

- Producer record creation starts at
  `src/backend/prealloc/regalloc/consumer_moves.cpp`:
  `append_consumer_move_resolution` records one consumer move per named
  `bir::BinaryInst` operand to the named result, using
  `PreparedMoveAuthorityKind::None` and no parallel-copy step or edge labels.
- Bundle publication is
  `src/backend/prealloc/regalloc.cpp::append_prepared_move_bundle`, which
  groups by phase, authority, block/instruction, and parallel-copy labels. It
  currently preserves two operand records as one non-parallel move bundle.
- The first enforceable consumer boundary before RV64 object emission is
  `src/backend/prealloc/prepared_object_traversal.cpp::classify_prepared_object_move_bundle_consumer`.
  It already owns copy-event placement/authority checks, but currently returns
  `Available` for a non-parallel, `authority=none`, before-instruction bundle
  even when multiple register sources target one stack value.

Contract decision:

- Reject the multi-source-to-one-stack-value shape before RV64 object emission.
  Do not publish out-of-SSA parallel-copy authority for ordinary binary
  consumer operands: the producer has no predecessor/successor edge, no
  execution site, no step ordering, and no parallel-copy owner. Do not split it
  into explicit independent moves: two independent stores to the same stack
  destination value would still require semantic overwrite authority and would
  not represent binary operand consumption.
- Accepted predicate for this family remains narrow: a non-parallel
  `authority=none` before-instruction stack-destination bundle may be consumed
  only when each stack destination has a single coherent producer for that
  destination at that event, with explicit source/destination homes and
  RV64-supported scalar storage facts. Existing single register-to-stack and
  coherent stack-to-stack shapes can remain target-consumable under their
  current RV64 predicates.
- Rejected predicate for Step 3: if a classified non-parallel move bundle has
  `phase=before_instruction`, `authority=none`, `parallel_copy_bundle=nullptr`,
  two or more moves, and at least two moves target the same destination value or
  stack-slot home while their explicit source homes are registers, classify or
  diagnose it as unsupported producer/classifier authority before calling
  `fragment_for_prepared_move_bundle`.

Adjacent fail-closed shapes:

- Missing authority: reject when the bundle needs sequencing or ownership but
  has `authority=none`, no `source_parallel_copy_step_index`, and no
  predecessor/successor labels.
- Contradictory destination ownership: reject when two moves in the same event
  claim different register sources for one `to_value_id`, one stack slot, or
  one destination stack offset without a coherent published owner.
- Unsupported banks: reject unless explicit homes and any storage-plan endpoint
  agree on RV64 GPR-compatible scalar storage for the accepted target shape;
  FPR, `none`, mismatched target arch, or incoherent register identities stay
  unsupported for this contract.
- Unsupported widths: reject non-scalar or multi-register widths, missing
  source/destination size authority, and source/destination size mismatches.
  Accepted ordinary register-to-stack/stack-to-stack target materialization
  remains bounded to the widths already proven by RV64 predicates.
- Missing sequencing: reject any multi-move same-destination bundle that lacks
  an explicit parallel-copy bundle, matching execution site, matching
  predecessor/successor labels, and per-move step indexes. Vector order in
  `moves` is not sequencing authority.

## Suggested Next

Execute Step 3 by adding the minimal producer/classifier fail-closed behavior:
the ambiguous non-parallel, `authority=none`, multi-register-source to one
stack destination bundle should receive a producer/classifier-owned diagnostic
before RV64 object emission asks `fragment_for_prepared_move_bundle` to
materialize it.

## Watchouts

Do not accept the multi-source bundle by dropping a source, choosing the first
or last source by vector order, or inferring ownership from the row name,
argument order, ABI formula, raw BIR text, reason strings, or source variable
names. If Step 3 adds a new classifier status/category, keep existing
single-move and out-of-SSA parallel-copy classifications stable; the new rule
should target the same-destination multi-source ambiguity, not all multi-move
bundles.

## Proof

Contract-only packet; no code-changing proof required and no canonical
regression log was written. Source/probe commands run:

- `c4c-clang-tool-ccdb find-definition` for
  `append_consumer_move_resolution`, `append_prepared_move_bundle`, and
  `classify_prepared_object_move_bundle_consumer`.
- `c4c-clang-tool-ccdb function-callees` for
  `classify_prepared_object_move_bundle_consumer`.
- Focused source reads of the producer, bundler, classifier, RV64 move-bundle
  fragment/rejection path, current consumer-classifier tests, RV64
  move-bundle fail-closed tests, and source ideas 516/514.
- `git diff --check -- todo.md` passed.
