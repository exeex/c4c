Status: Active
Source Idea Path: ideas/open/383_string_authority_guard_unclassified_symbols.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Replace HFA Return Lane String Authority

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by replacing `HfaReturnLaneMap`'s raw
`std::string` key with a typed named-BIR-value key.

- `src/backend/bir/lir_to_bir/lowering.hpp` now defines `HfaReturnValueKey`
  from named `bir::Value` identity fields and uses it as the
  `HfaReturnLaneMap` key.
- `src/backend/bir/lir_to_bir/calling.cpp` stores multi-lane HFA return lanes
  using the lowered call result value as the key authority.
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp` derives the same typed
  key through `lower_value(store.val, hfa_facts->lane_type, value_aliases_)`
  before probing the lane side table. The existing single-lane fallback still
  uses the lowered value directly.
- The focused guard output no longer reports `HfaReturnLaneMap`.

## Suggested Next

Execute the next packet against `PreparedValueHomeIndexes::homes_by_name`,
replacing the by-name prepared-home lookup authority with the existing
`PreparedValueId`/`homes_by_id` path where the surrounding prepared records can
provide ids.

## Watchouts

- Do not edit `scripts/string_authority_classifications.json`.
- Do not rename reported symbols just to evade the scanner.
- For `homes_by_name`, avoid preserving the same authority under a less obvious
  name; the durable authority should be `PreparedValueId`.
- For the computed-value lookup maps, changing `std::string_view` to
  `ValueNameId` is a real authority move only if all producers and recursive
  consumers use the interned id consistently, including the recursion guard.

## Proof

Ran the delegated proof command and preserved combined output in
`test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^string_authority_guard$'`

The build completed successfully. The `string_authority_guard` test still exits
nonzero because the remaining in-scope reports are:

- `PreparedValueHomeIndexes::homes_by_name`
- `PreparedComputedValueLookup::named_binaries`
- `PreparedComputedValueLookup::named_global_loads`
