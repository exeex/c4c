Status: Active
Source Idea Path: ideas/open/383_string_authority_guard_unclassified_symbols.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Replace Prepared Home By-Name Lookup

# Current Packet

## Just Finished

Completed `plan.md` Step 3 by replacing
`PreparedValueHomeIndexes::homes_by_name` with prepared-value-id resolution.

- `PreparedValueHomeIndexes` now keeps authoritative homes only in
  `homes_by_id`.
- A separate `ValueNameId -> PreparedValueId` index resolves legacy named BIR
  operands to ids before probing `homes_by_id`.
- The call and dispatch local `find_value_home(ValueNameId)` bridges now resolve
  a prepared value id first instead of returning a home from a by-name home map.
- The focused guard output no longer reports
  `PreparedValueHomeIndexes::homes_by_name`.

## Suggested Next

Execute the next packet against `PreparedComputedValueLookup::named_binaries`,
moving computed binary lookup from raw `std::string_view` keys to interned
`ValueNameId` authority across producers, consumers, and recursion guards.

## Watchouts

- Do not edit `scripts/string_authority_classifications.json`.
- Do not rename reported symbols just to evade the scanner.
- For the computed-value lookup maps, changing `std::string_view` to
  `ValueNameId` is a real authority move only if all producers and recursive
  consumers use the interned id consistently, including the recursion guard.
- `PreparedValueHomeIndexes::value_ids` is an id resolver only; home authority
  remains `PreparedValueId` through `homes_by_id`.

## Proof

Ran the delegated proof command and preserved combined output in
`test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(string_authority_guard|backend_aarch64_instruction_dispatch)$'`

The build completed successfully and `backend_aarch64_instruction_dispatch`
passed. The `string_authority_guard` test still exits nonzero because the
remaining in-scope reports are:

- `PreparedComputedValueLookup::named_binaries`
- `PreparedComputedValueLookup::named_global_loads`
