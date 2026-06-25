Status: Active
Source Idea Path: ideas/open/356_rv64_object_route_assembler_backed_prepared_text.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Wire RV64 Object Emission to the Shared Traversal Stream

# Current Packet

## Just Finished

Route correction after rollback. The target-local RV64 object-route work was
reverted out of HEAD and preserved in stash for reference only:

- `stash@{1}: wip rv64 object route uncommitted drift before route reset`
- `stash@{0}: staged rv64 object route commits before route reset`

The rejected route shape directly scanned
`control_flow.parallel_copy_bundles`, used
`prepared_object_parallel_copy_event_kind`, and re-found move bundles in RV64
`object_emission.cpp`. That route is not accepted because it bypassed the
shared prepared-object traversal/diagnostic contract from idea 359.

Do not claim `src/20000113-1.c` progressed from that route. Any future
`20000113-1.c` progress must be reproven after RV64 object emission consumes
the shared traversal stream.

## Suggested Next

Execute Step 3: wire RV64 object emission to the shared prepared object
traversal event stream before any target-local fragment repair resumes.

Narrow packet target:

- start from `make_prepared_object_function_traversal()` as the ordered event
  source;
- consume traversal labels, block-entry copies, instructions,
  pre-terminator/edge copies, and terminators in RV64 object emission;
- classify each move event with
  `classify_prepared_object_move_bundle_consumer(event)`;
- report unplaced obligations with
  `collect_unplaced_prepared_object_parallel_copy_obligations()`;
- use shared move/select/value-home/frame diagnostics or classifiers where
  available, adding only RV64-specific target evidence;
- explicitly avoid scanning `control_flow.parallel_copy_bundles` in RV64 to
  reconstruct block-entry/pre-terminator copies.

Narrow proof command:

```sh
CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-multiblock-allowlist.txt scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1 || true
```

## Watchouts

- `20000113-1.c` progress from the stashed route is intentionally not accepted.
  Reprove any progress only through the shared-traversal-first route.
- Do not manually scan `control_flow.parallel_copy_bundles` from RV64 object
  emission to reconstruct copy placement.
- Do not use `prepared_object_parallel_copy_event_kind` or target-local bundle
  search as a replacement for shared traversal/classifier APIs.
- `20030216-1.c` is a separate prepared global-storage/data blocker; do not
  mix float/global data support into the stack-slot object-fragment packet.
- The two passing cases are only current allowlist passes. Do not use them as
  proof of broad RV64 object-route completeness.
- Reject testcase-name shortcuts, expectation weakening, target-local CFG
  reconstruction, and silent block/edge dropping.

## Proof

Not run for route-reset rewrite. This is lifecycle/plan correction only after
the rejected RV64 target-local route was moved to stash.
