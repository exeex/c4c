Status: Active
Source Idea Path: ideas/open/662_prepared_backend_contract_and_cli_publication.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair The Selected Contract Or CLI Rule

# Current Packet

## Just Finished

Step 3: Repair The Selected Contract Or CLI Rule refreshed the remaining stale
prepared value-id expectations for `backend_prepared_printer`,
`backend_prealloc_inline_asm`, and
`backend_cli_dump_prepared_bir_local_arg_call_contract`.

The failing dumps showed unchanged contract semantics with stale expected ids:
`p.value` is published consistently as `value_id=1` across home, storage, and
`f128_carrier` rows, while `%t1` is published consistently as `value_id=4`
across home, call-result destination, and storage rows. The assertions now keep
the existing carrier/storage/placement contract snippets and update only those
ids.

## Suggested Next

Next recommended packet: advance Step 4 by rechecking the prepared/CLI family
boundary and deciding whether any remaining prepared contract-publication work
belongs in this plan or should route to a separate source idea.

## Watchouts

- This packet intentionally did not edit `plan.md`, the source idea,
  unsupported markers, allowlists, timeout policy, runtime policy, baseline
  accounting files, or unrelated backend families.
- The edits are expectation-only, but not a contract downgrade: the current
  structured dumps already expose the required sections and storage facts; only
  stale hard-coded value ids were wrong.
- The `backend_prepared_printer` and `backend_prealloc_inline_asm` rows share
  the same f128 carrier assertion path in
  `tests/backend/bir/backend_prepared_printer_test.cpp`.

## Proof

Delegated focused proof run:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_prealloc_inline_asm|backend_cli_dump_prepared_bir_exposes_contract_sections|backend_cli_dump_prepared_bir_local_arg_call_contract)$' > test_after.log 2>&1
```

Result: build completed and the delegated CTest subset passed, 6/6 tests
passing.

Supervisor acceptance also ran backend-only regression guard with
`ctest --test-dir build -j --output-on-failure -R '^backend_'`. The guard
passed with 355/368 tests passing after, up from 352/368 before, resolving
`backend_prepared_printer`, `backend_prealloc_inline_asm`, and
`backend_cli_dump_prepared_bir_local_arg_call_contract` with no new failures.
Canonical proof log was rolled forward to `test_before.log`.
