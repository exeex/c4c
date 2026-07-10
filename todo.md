Status: Active
Source Idea Path: ideas/open/662_prepared_backend_contract_and_cli_publication.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair The Selected Contract Or CLI Rule

# Current Packet

## Just Finished

Step 3: Repair The Selected Contract Or CLI Rule repaired the prepared RV64 FPR
ABI/frame publication gap for `backend_prepare_frame_stack_call_contract`.
Prepared call-plan dumps now expose the complete fixed saved-register slot
placement for preserved callee-saved values by joining preserved call facts to
the function frame plan, and the focused contract now asserts the actual
structured `float.carry` value id instead of a stale literal.

The selected row now passes. The delegated subset remains red only on the
already-isolated stale value-id rows: `backend_prepared_printer`,
`backend_prealloc_inline_asm`, and
`backend_cli_dump_prepared_bir_local_arg_call_contract`.

## Suggested Next

Next recommended packet: decide whether the remaining stale value-id expectation
rows should be updated under supervisor approval or split out, since the
prepared publication defects proven by this packet are repaired and
`backend_prepare_frame_stack_call_contract` is green.

## Watchouts

- This packet intentionally did not edit `plan.md`, the source idea,
  unsupported markers, allowlists, runtime policy, baseline accounting files, or
  stale f128/local-arg value-id rows.
- The RV64 FPR facts were present in structured production; the blocker was
  prepared dump publication plus a stale hard-coded focused-test value id. The
  printer now publishes the frame slot placement on preserved call-plan rows.
- The f128 carrier and local-arg failures remain value-id expectation
  mismatches in current evidence, not proven prepared publication gaps.

## Proof

Delegated proof run and preserved in `test_after.log`:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_prealloc_inline_asm|backend_cli_dump_prepared_bir_exposes_contract_sections|backend_cli_dump_prepared_bir_local_arg_call_contract)$' > test_after.log 2>&1
```

Result: build completed and the delegated CTest subset failed, 3/6 tests
passing. Passing rows: `backend_prepare_liveness`,
`backend_prepare_frame_stack_call_contract`, and
`backend_cli_dump_prepared_bir_exposes_contract_sections`. Failing rows:
`backend_prepared_printer`, `backend_prealloc_inline_asm`, and
`backend_cli_dump_prepared_bir_local_arg_call_contract`.
Proof log: `test_after.log`.
