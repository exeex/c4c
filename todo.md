Status: Active
Source Idea Path: ideas/open/675_post_wave_residual_baseline_failures.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Capture Candidate Delta And First Owners

# Current Packet

## Just Finished

Step 1 captured the accepted/candidate/history baseline delta in
`build/agent_state/675_step1_candidate_delta/summary.md`. The candidate and
history logs match at 9 failures by stable test name; the accepted baseline has
11 failures. Candidate-only rows are
`backend_cli_failure_riscv64_pointer_global_local_publication_live_load_rejection`
and `backend_cli_riscv64_call_arg_local_frame_address_materialization`.

First-owner evidence:

- `backend_cli_failure_riscv64_pointer_global_local_publication_live_load_rejection`
  now fails because the expected-failure object wrapper unexpectedly succeeds;
  the generated object contains the direct-global publication plus live reload.
  The next owner is test-contract/baseline routing unless a runtime/semantic
  probe proves the emitted object is invalid.
- `backend_cli_riscv64_call_arg_local_frame_address_materialization` still
  fails object-byte proof: text route emits `addi a0, sp, 0`, but object route
  emits `mv s1, sp; mv a0, s1` and lacks expected bytes `13050100`.
  The first implementation owner remains RV64 object-route consumption of
  `LocalFrameAddressMaterialization`.

## Suggested Next

Split Step 2 by owner: first decide the
`backend_cli_failure_riscv64_pointer_global_local_publication_live_load_rejection`
contract route with a runtime/semantic object probe or explicit test-contract
review, then separately repair or bless the
`backend_cli_riscv64_call_arg_local_frame_address_materialization` object-route
direct frame-address materialization contract.

## Watchouts

- Do not accept `test_baseline.new.log` while it contains candidate-only
  failures.
- Compare by stable test name, not numeric row id.
- Do not reopen closed ideas unless fresh evidence contradicts their closure
  notes.
- Do not change expectations, unsupported markers, allowlists, timeouts,
  runtime policy, or baseline accounting.
- The pointer/global-local row is no longer an implementation rejection in the
  focused proof; treating it as a code repair without a semantic/runtime
  failure would be route drift.
- The call-arg row has text/object divergence, so expectation churn alone would
  be overfit unless the object shape is explicitly accepted as the intended
  contract.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_cli_failure_riscv64_pointer_global_local_publication_live_load_rejection|backend_cli_riscv64_call_arg_local_frame_address_materialization' > test_after.log 2>&1`

Result: build passed (`ninja: no work to do`); focused CTest failed as expected
for this evidence packet with both candidate-only rows still red. Proof log:
`test_after.log`.
