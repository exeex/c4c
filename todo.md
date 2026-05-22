Status: Active
Source Idea Path: ideas/open/349_aarch64_recursive_call_argument_preservation.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh Current First Bad Fact

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by refreshing the current generated-code first bad
fact for recursive/nested-call representatives `00176.c` and `00181.c`.

The exact delegated proof is green. No current `00176.c` or `00181.c` first
bad fact remains under this focused subset, so there is no fresh evidence that
the active recursive call argument preservation source idea owns a current
failure.

## Suggested Next

Supervisor lifecycle routing. With both representatives green, this packet has
no in-scope preservation boundary to localize in Step 2.

## Watchouts

- Do not continue from stale historical `00176` or `00181` notes without fresh
  artifacts.
- The refreshed representative tests both pass, so any next execution packet
  should start from a newly identified failing artifact rather than historical
  notes.
- Do not reopen block-label ordering, local/formal frame-slot publication,
  stack-preserved pointer formal publication, indexed aggregate writeback, or
  other split owners unless fresh first-bad-fact evidence proves this source
  idea owns the current failure.
- Do not special-case `00176`, `00181`, `quicksort`, `Hanoi`, one argument
  index, one register pair, one block, or one emitted instruction sequence.

## Proof

Proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c)$'
```

Result: passed, 2/2 selected tests green:

- `c_testsuite_aarch64_backend_src_00176_c`
- `c_testsuite_aarch64_backend_src_00181_c`

Proof log: `test_after.log`.
