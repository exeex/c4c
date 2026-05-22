Status: Active
Source Idea Path: ideas/open/349_aarch64_recursive_call_argument_preservation.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh Current Recursive Call Preservation Proof

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by refreshing the recursive/nested call
representative proof for `c_testsuite_aarch64_backend_src_00176_c` and
`c_testsuite_aarch64_backend_src_00181_c`.

The focused subset passed 2/2 with 0 failures. Neither representative exposes
stale caller-clobbered post-call argument use as a current in-scope first bad
fact for idea 349.

## Suggested Next

Hand off to the supervisor for lifecycle close/park decision under the current
close-gate policy.

## Watchouts

- Do not continue from stale historical `00176` or `00181` notes without fresh
  artifacts.
- Do not reopen block-label ordering, local/formal frame-slot publication,
  stack-preserved pointer formal publication, indexed aggregate writeback, or
  other split owners unless fresh first-bad-fact evidence proves this source
  idea owns the current failure.
- Do not special-case `00176`, `00181`, `quicksort`, `Hanoi`, one argument
  index, one register pair, one block, or one emitted instruction sequence.

## Proof

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c)$' > test_after.log 2>&1
```

Result: build was up to date; focused subset passed 2/2 with 0 failures.
Proof log: `test_after.log`.
