Status: Active
Source Idea Path: ideas/open/669_byval_prepared_dump_contract_review.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Select Repair Owner And Patch Narrowly

# Current Packet

## Just Finished

Step 2 `Select Repair Owner And Patch Narrowly` aligned only the two stale
byval prepared dump snippets with the current Step 1 prepared facts. The
updated contracts now require `%lv.value` / `source_value_id=22`, stack-copy
aggregate transport, the current before-call stack argument move, and
`dest_reg=a0` for the preserved pointer argument where the dump proves it.
This is expectation-contract alignment over existing prepared facts, not
lowering or compiler progress.

## Suggested Next

Hand back to the supervisor for Step 3 closure/regression readiness. The next
packet should decide whether the focused green dump subset is sufficient for
plan-owner close review or whether a broader byval regression surface should
be captured first.

## Watchouts

- Treat route and runtime byval rows as regression surfaces, not the
  implementation target.
- Do not reopen the closed idea 659 byval runtime/codegen-route repair without
  fresh focused regression evidence.
- Do not work on object-runtime `BinaryInst`; that belongs to idea 670.
- The snippet patch intentionally did not touch route/runtime rows,
  unsupported markers, allowlists, timeout policy, runtime policy, baseline
  accounting, source, plan, or source idea files.
- The remaining decision is lifecycle/proof scope, not a known implementation
  owner.

## Proof

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_dump_riscv64_byval_aggregate_fixed_call|backend_dump_riscv64_byval_preserved_pointer_args)$') > test_after.log 2>&1
```

Result: exit `0`; build completed and both focused dump tests passed. Proof
log: `test_after.log`.
