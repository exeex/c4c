Status: Active
Source Idea Path: ideas/open/610_rv64_move_bundle_target_materialization.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Close-Readiness Review

# Current Packet

## Just Finished

Completed `plan.md` Step 5 close-readiness review for idea 610. No
implementation, lifecycle, expectation, unsupported-marker, allowlist,
timeout/accounting, runtime, prepared/prealloc, ABI, terminator, or global-data
files were changed.

Recommendation: close idea 610, with remaining non-close work split or left to
separate existing authority tracks. The acceptance criteria are satisfied for
the source idea's RV64/MIR consumer scope:
- Multiple authorized move-bundle rows now progress through RV64 lowering.
  The direct object probe summary under
  `build/agent_state/610_step3_move_bundle_residual_probe/summary.tsv`
  records 41 compile-through rows after the Step 2 consumer rule, including
  `src/20000314-1.c`, `src/20040309-1.c`, `src/pr63641.c`, and the source
  idea's representative `src/20020206-2.c`.
- The implementation movement is target consumption, not producer repair:
  Step 2 consumed explicit prepared source/destination homes for authorized
  `block_entry/out_of_ssa_parallel_copy/phi_join_register_to_register`
  register-to-register moves and preserved fail-closed behavior for unsupported
  or ambiguous rows.
- Rows lacking required authority remain rejected with owner-specific or
  boundary-specific diagnostics instead of being forced through the RV64
  consumer.

Remaining residual owner split:
- `src/pr71631.c` is the only live same-family audit row, but it contains
  repeated stack destinations. That requires destination fan-in/order or
  mutually-exclusive authority, which idea 610 explicitly treats as out of
  scope and blocked by separate authority research. Closing 610 does not claim
  support for `src/pr71631.c`.
- 10 rows are destination fan-in residuals classified as
  `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`
  with `diagnostic_owner=rv64_prepared_move_bundle_consumer`.
- 13 rows are select-publication residuals with explicit
  `select_publication_rejection_reason` values.
- 3 rows are ABI/call-boundary residuals, 14 are generic before-instruction
  stack-destination move shapes, 5 are evidence-gap or malformed select-carrier
  facts, and 19 reroute to downstream non-move-bundle owners such as
  instruction fragments, local memory, terminators, scalar-compare
  publication, and inline asm.
- Rows that only rerouted to downstream owners are not counted as target support
  for idea 610.

## Suggested Next

Ask the plan owner to close idea 610 if the supervisor accepts this
close-readiness recommendation. Any follow-up for `src/pr71631.c` should be a
separate destination fan-in/order authority initiative or an existing
destination fan-in plan, not an expansion of the RV64 target materialization
runbook.

## Watchouts

- Do not infer destination ordering or mutual exclusion from an encodable RV64
  move sequence. `src/pr71631.c` remains fail-closed until explicit fan-in/order
  authority exists.
- Do not claim target support for rows that only moved to downstream owners;
  they are useful route-separation evidence only.
- `src/pr47337.c` remains an evidence gap under the generic target-shape
  diagnostic and should not be used as proof that a new RV64 materialization
  rule is safe.
- No expectation, unsupported marker, allowlist, timeout/accounting, or runtime
  changes are part of the idea 610 close evidence.

## Proof

Proof command run for this Step 5 close-readiness packet:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: passed, 346 backend tests, 0 failures. Backend subset used:
`^backend_`. Log path: `test_after.log`. The proof is sufficient for this
close-readiness evidence packet.
