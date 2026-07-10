Status: Active
Source Idea Path: ideas/open/662_prepared_backend_contract_and_cli_publication.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Broaden Within The Prepared/CLI Family

# Current Packet

## Just Finished

Step 4: Broaden Within The Prepared/CLI Family refreshed the prepared/CLI proof
subset after the RV64 sret call-argument indexing repair, without changing
implementation or expectations.

Focused row status from `test_after.log`:

- `backend_prepare_liveness`: passed. The prior RV64 helper-built aggregate
  `va_arg` AP pointer repair remains green in the broadened family run.
- `backend_prepare_frame_stack_call_contract`: failed with
  `rv64 FPR ABI/frame fact contract: prepared dump hides FPR facts`. First
  owner evidence points at missing prepared dump publication/exposure for RV64
  FPR ABI/frame facts, not the repaired sret GPR indexing path.
- `backend_prepared_printer`: failed on missing snippet
  `f128_carrier p.value value_id=0`; the actual prepared dump publishes
  `f128_carrier p.value value_id=1`, matching `home p.value value_id=1` and
  `storage p.value value_id=1`. First owner evidence points at a stale f128
  value-id expectation/contract row.
- `backend_prealloc_inline_asm`: failed on the same stale f128 value-id snippet
  as `backend_prepared_printer`, with actual `value_id=1` for home, storage,
  and f128 carrier publication. First owner evidence is the shared f128
  expectation/contract row, not inline-asm implementation.
- `backend_cli_dump_prepared_bir_local_arg_call_contract`: failed because the
  CLI snippet expects `storage %t1 value_id=3`, while the actual dump shows
  `%t0 value_id=3` and `%t1 value_id=4`. First owner evidence points at a
  stale CLI value-id snippet for the local-arg call contract.
- `backend_cli_dump_prepared_bir_exposes_contract_sections`: failed on missing
  `block_entry_publication successor=logic.end.4 ... to_value_id=9 to=%t8`.
  First owner evidence points at missing prepared block-entry publication
  producer/exposure for the selected branch/select contract section.

## Suggested Next

Next recommended implementation packet: stay inside the prepared contract
publication/CLI exposure plan by selecting
`backend_cli_dump_prepared_bir_exposes_contract_sections` and repair the
prepared block-entry publication producer/exposure for the missing
`logic.end.4` `%t8` publication. Keep the stale f128 and local-arg value-id
snippet rows separate for supervisor or plan-owner routing because this packet
was not authorized to edit expectations or contract fixtures.

## Watchouts

- This packet did not edit implementation, expectations, unsupported markers,
  allowlists, runtime policy, baseline accounting, source ideas, or `plan.md`.
- The delegated family proof is red: 1/6 tests passed and 5/6 failed.
- The two f128 failures show actual internally consistent `value_id=1`
  publication across home, storage, and carrier rows; changing implementation
  to force `value_id=0` would look like route drift unless the contract owner
  confirms that expectation.
- The local-arg CLI failure likewise shows internally consistent current IDs
  (`%t0` is 3, `%t1` is 4); do not bundle that stale snippet row with the
  block-entry publication producer repair.

## Proof

Delegated proof run and preserved in `test_after.log`:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_prealloc_inline_asm|backend_cli_dump_prepared_bir_exposes_contract_sections|backend_cli_dump_prepared_bir_local_arg_call_contract)$' > test_after.log 2>&1
```

Result: build completed (`ninja: no work to do`), then CTest failed the
delegated family subset with 1/6 passing. Proof log: `test_after.log`.
