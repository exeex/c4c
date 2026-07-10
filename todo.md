Status: Active
Source Idea Path: ideas/open/662_prepared_backend_contract_and_cli_publication.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair The Selected Contract Or CLI Rule

# Current Packet

## Just Finished

Step 3: Repair The Selected Contract Or CLI Rule repaired the prepared
block-entry publication CLI exposure for non-immediate predecessor-edge moves.
`src/backend/prealloc/prepared_printer/value_locations.cpp` now emits the
existing destination-home publication row and, when an available block-entry
move publishes a distinct non-immediate source value into that destination,
also emits a source-identity publication row using the move's `from_value_id`
with the destination value name/home/register facts. This publishes the
`logic.rhs.end.3 -> logic.end.4` `%t6` source identity as
`to_value_id=9 to=%t8` without changing required snippets, expectations,
unsupported markers, or allowlists.

## Suggested Next

Next recommended packet: return to supervisor-selected prepared/CLI family
triage. The selected block-entry publication row is repaired and the narrow
CLI proof is green; remaining known red rows from the prior broadened run are
separate f128 value-id, local-arg value-id, and RV64 FPR ABI/frame fact owners.

## Watchouts

- This packet intentionally did not edit `plan.md`, the source idea, required
  snippets, expectations, unsupported markers, allowlists, timeout/runtime
  policy, AArch64 dispatch, RISC-V object emission, or unrelated f128/local-arg
  value-id rows.
- The semantic rule is generic for prepared block-entry publications: expose a
  source-identity row only when the existing available move has a distinct,
  non-zero, non-immediate `from_value_id`. Immediate block-entry publications
  and already-coalesced `from == to` rows are left as single destination rows.
- The printer also now resolves block-entry destination values from any named
  instruction result in the successor block, not only pre-out-of-SSA PHIs, so
  prepared dumps with select-materialized joins can still align route evidence
  with the destination value.

## Proof

Delegated proof run and preserved in `test_after.log`:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_cli_dump_prepared_bir_exposes_contract_sections$' > test_after.log 2>&1
```

Result: build completed and the delegated CTest passed, 1/1 tests passing.
Proof log: `test_after.log`.
