Status: Active
Source Idea Path: ideas/open/306_rv64_c_testsuite_undefined_main_triage_and_followups.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Decide Repair Ordering

# Current Packet

## Just Finished

Step 4, Decide Repair Ordering, consumed
`build/rv64_c_testsuite_probe_v2/classification.md` and
`build/rv64_c_testsuite_probe_v2/classification.tsv`, then wrote
`build/rv64_c_testsuite_probe_v2/repair_order.md` with a focused repair
sequence. The order starts with the shared
`rv64_text_only_fail_closed` RV64 `.text`-only output-contract cleanup, uses
`unused_extern_no_storage` as the minimal control, and keeps the secondary
feature buckets as separate follow-up initiatives instead of merging them into
one giant c-testsuite repair.

## Suggested Next

Begin Step 5 follow-up idea creation. Create a first open idea for
`rv64_text_only_fail_closed` output-contract cleanup, explicitly proving that a
prepared RV64 `main` emits `.globl main`, a `main:` label, and a body for the
`unused_extern_no_storage` control case before pursuing feature-heavy buckets.

## Watchouts

- Keep Step 5 ideas split by repair family:
  `rv64_text_only_fail_closed`, `string_literals_and_extern_calls`,
  `aggregate_global_storage`, `pointer_global_storage`,
  `floating_global_storage`, and the smaller scalar/control follow-ups.
- `string_literals_and_extern_calls` is the largest secondary bucket, but it
  should follow the shared function-emission contract repair because current
  `.text`-only output masks its real failure mode.
- `aggregate_global_storage`, `pointer_global_storage`, and
  `floating_global_storage` carry different data/relocation/representation
  risks and should not be folded into one broad global-storage patch.
- Do not register the full 220-case sweep as required CTest coverage.
- Do not treat empty `.text` output as a valid successful unsupported path.
- Keep root-level scratch logs clean.

## Proof

Ran the delegated proof command:

```sh
{ cmake --build --preset default && test -f build/rv64_c_testsuite_probe_v2/classification.md && test -f build/rv64_c_testsuite_probe_v2/classification.tsv && test -f build/rv64_c_testsuite_probe_v2/repair_order.md && rg -n 'rv64_text_only_fail_closed|string_literals_and_extern_calls|aggregate_global_storage|pointer_global_storage|floating_global_storage|unused_extern_no_storage' build/rv64_c_testsuite_probe_v2/repair_order.md; } > test_after.log 2>&1
```

Result: passed. The proof log is `test_after.log`.
