Status: Active
Source Idea Path: ideas/open/662_prepared_backend_contract_and_cli_publication.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Broaden Within The Prepared/CLI Family

# Current Packet

## Just Finished

Step 4: Broaden Within The Prepared/CLI Family refreshed the prepared/CLI
family proof after the block-entry publication repair. The broadened subset is
still red, but the selected focused row now stays green:
`backend_cli_dump_prepared_bir_exposes_contract_sections` passes, along with
`backend_prepare_liveness`.

Remaining first-owner split from `test_after.log`:

- `backend_prepare_frame_stack_call_contract`: real prepared contract
  publication owner. The test still reports `rv64 FPR ABI/frame fact contract:
  prepared dump hides FPR facts`.
- `backend_prepared_printer` and `backend_prealloc_inline_asm`: stale f128
  value-id row. Both require `f128_carrier p.value value_id=0`, while the
  prepared dump consistently publishes `home p.value value_id=1`,
  `storage p.value value_id=1`, and `f128_carrier p.value value_id=1`.
- `backend_cli_dump_prepared_bir_local_arg_call_contract`: stale local-arg
  value-id row. The required snippet expects
  `storage %t1 value_id=3 ... reg=rbx`, while the dump publishes `%t0` as
  `value_id=3` and `%t1` as `value_id=4`; the call result and return moves
  also use `from_value_id=4 to_value_id=4`.

## Suggested Next

Next recommended packet: repair
`backend_prepare_frame_stack_call_contract` by focusing on the missing RV64 FPR
ABI/frame facts in the prepared dump. Keep the f128 carrier and local-arg
value-id failures out of that packet unless there is evidence they are real
prepared publication defects rather than stale required snippets.

## Watchouts

- This packet intentionally did not edit `plan.md`, the source idea, source or
  test implementation files, expectations, unsupported markers, allowlists,
  runtime policy, or baseline files other than `test_after.log`.
- The repaired block-entry CLI contract row remained green in the broadened
  subset, so do not reopen that producer unless a later broader run finds new
  evidence.
- The f128 carrier and local-arg failures are value-id expectation mismatches
  in current evidence, not proven prepared publication gaps. Treating them as
  expectation churn without supervisor approval would be overfit risk.

## Proof

Delegated proof run and preserved in `test_after.log`:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_prealloc_inline_asm|backend_cli_dump_prepared_bir_exposes_contract_sections|backend_cli_dump_prepared_bir_local_arg_call_contract)$' > test_after.log 2>&1
```

Result: build completed and the delegated CTest subset failed, 2/6 tests
passing. Passing rows: `backend_prepare_liveness` and
`backend_cli_dump_prepared_bir_exposes_contract_sections`. Failing rows:
`backend_prepare_frame_stack_call_contract`, `backend_prepared_printer`,
`backend_prealloc_inline_asm`, and
`backend_cli_dump_prepared_bir_local_arg_call_contract`.
Proof log: `test_after.log`.
