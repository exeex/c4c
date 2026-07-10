Status: Active
Source Idea Path: ideas/open/669_byval_prepared_dump_contract_review.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Byval Dump Evidence

# Current Packet

## Just Finished

Step 1 `Refresh Byval Dump Evidence` refreshed the focused prepared-BIR dump
boundary for both residual byval rows. The delegated proof rebuilt cleanly but
the focused CTest subset exited `8` because both dump snippet checks still
fail:

- `backend_dump_riscv64_byval_aggregate_fixed_call`: classified as stale
  snippet expectation. The missing expected line is
  `move from_value_id=20 to_value_id=20 destination_kind=call_argument_abi
  destination_storage=stack_slot`, but current prepared facts publish
  `%lv.value` as `value_id=22`, with `home %lv.value value_id=22 kind=register
  reg=s1`, a `before_call` stack argument move
  `from_value_id=22 to_value_id=22 destination_kind=call_argument_abi
  destination_storage=stack_slot abi_index=0 stack_offset=0`, and a
  prepared-call-plan `arg index=0 value_bank=aggregate_address
  source_value_id=22 ... arg.aggregate_transport=stack_copy`.
- `backend_dump_riscv64_byval_preserved_pointer_args`: classified as stale
  snippet expectation. Current prepared facts publish both calls with
  `%lv.value` as `source_value_id=22`, stack-copy aggregate transport for
  `arg index=0`, `arg index=1` in `gpr:call_argument#0/w1 reg=a0`, explicit
  publication/freshness facts for both arguments, and matching
  `address_materialization ... result=%lv.value` entries for inst indices `4`
  and `6`. The old required snippet still expects `source_value_id=20`,
  `selection_source_value_id=20`, and `dest_reg=a1`, so it no longer matches
  the current prepared call-boundary contract.

## Suggested Next

Execute Step 2: update only the stale byval prepared dump snippets to the
current prepared facts, then rerun the same focused dump subset. Treat this as
contract alignment over already-published facts, not lowering progress.

## Watchouts

- Treat route and runtime byval rows as regression surfaces, not the
  implementation target.
- Do not reopen the closed idea 659 byval runtime/codegen-route repair without
  fresh focused regression evidence.
- Do not work on object-runtime `BinaryInst`; that belongs to idea 670.
- Do not use testcase names, historical fixed value IDs, or final assembly
  shape as the dump-contract authority. The authoritative current facts are
  named values plus prepared call plans, before-call moves, stack-copy
  transport, explicit publication/freshness facts, and address materialization.
- `prepared-call-argument-value-publications` is empty for both rows, but the
  current failures are not missing prepared publication: the call-plan and
  move/addressing facts needed by the snippets are already emitted elsewhere in
  `--dump-prepared-bir`.

## Proof

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_dump_riscv64_byval_aggregate_fixed_call|backend_dump_riscv64_byval_preserved_pointer_args)$') > test_after.log 2>&1
```

Result: exit `8`; build was up to date, and both focused dump tests failed
with `[BACKEND_DUMP_SNIPPET_MISSING]`. For this evidence packet, the nonzero
result is the expected boundary evidence and is sufficient to classify the
current owner. Proof log: `test_after.log`.
