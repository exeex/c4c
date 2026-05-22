Status: Active
Source Idea Path: ideas/open/383_string_authority_guard_unclassified_symbols.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Current Authority Usage

# Current Packet

## Just Finished

Completed `plan.md` Step 1 localization for the four live
`string_authority_guard` reports.

### HfaReturnLaneMap

- Declaration: `src/backend/bir/lir_to_bir/lowering.hpp:429` aliases
  `std::unordered_map<std::string, std::vector<bir::Value>>`; member
  `hfa_return_lanes_` lives in the LIR-to-BIR lowering state.
- Producer: `src/backend/bir/lir_to_bir/calling.cpp:1337-1345` creates
  `lowered_call.result_lanes` for AArch64 multi-lane HFA returns and stores
  them under `call.result.str()`.
- Consumer: `src/backend/bir/lir_to_bir/memory/local_slots.cpp:680-697`
  looks up `hfa_return_lanes_` with `store.val.str()` while lowering an
  aggregate local-slot store, then fans the lane values into sorted leaf slots.
- Replacement authority: key this side table by the lowered BIR result value
  identity for the call result, not by the LIR spelling. The bounded edit path
  is to introduce a local typed key for named `bir::Value` identity
  (`kind/type/name/pointer_symbol_link_name_id`, with a hash) or an equivalent
  small `HfaReturnValueKey`, store with `*lowered_call.result`, and have the
  store path derive the same key from `lower_value(store.val, hfa_facts->lane_type,
  value_aliases_)` before probing. The lane vector remains the existing
  `std::vector<bir::Value>`.

### PreparedValueHomeIndexes::homes_by_name

- Declaration: `src/backend/mir/aarch64/module/module.hpp:95-98` stores
  `homes_by_id` by `prepare::PreparedValueId` and `homes_by_name` by
  `c4c::ValueNameId`.
- Producer: `src/backend/mir/aarch64/codegen/traversal.cpp:243-255` builds both
  indexes from `PreparedValueLocationFunction::value_homes`; each home already
  carries `value_id` and `value_name`.
- Consumers: `src/backend/mir/aarch64/codegen/calls.cpp:1976-1985` and
  `src/backend/mir/aarch64/codegen/dispatch.cpp:232-241` use the by-name index
  in `find_value_home(context, ValueNameId)`. Adjacent overloads in both files
  already consume `PreparedValueId` through `homes_by_id`.
- Replacement authority: use `prepare::PreparedValueId` as the cached lookup
  authority and keep `homes_by_id` as the only prepared-home index. The bounded
  edit path is to move call sites that currently arrive with a `ValueNameId`
  to a prepared value-id source already present in the surrounding prepared
  records, or to resolve through the prepared value table once and validate
  that the resulting home's `value_name` still matches when a compatibility
  name is all the caller has. Do not keep a replacement member whose authority
  is "by name".

### PreparedComputedValueLookup::named_binaries

- Declaration: `src/backend/prealloc/regalloc/value_homes.hpp:16-19` stores
  binary result instructions by `std::string_view` result spelling.
- Producer: `src/backend/prealloc/regalloc/value_homes.cpp:112-126` walks the
  BIR function and inserts each named `bir::BinaryInst` under
  `binary->result.name`.
- Consumer: `src/backend/prealloc/regalloc/value_homes.cpp:230-238` passes the
  map into `classify_computed_value`; the recursive classifier in
  `src/backend/prealloc/control_flow.hpp:1693-1730` probes it with
  `value.name` and recursively walks immediate/parameter/binary/global-load
  computed values.
- Replacement authority: key binary producers by an interned prepared value
  identity for the result, preferably `ValueNameId` resolved through
  `PreparedNameTables::value_names` for the current function, with recursion
  tracking using the same typed id instead of `std::string_view`. A stronger
  follow-up can thread `PreparedValueId`, but the local bounded edit is a
  typed result-value-id map such as `binaries_by_result_value`.

### PreparedComputedValueLookup::named_global_loads

- Declaration: `src/backend/prealloc/regalloc/value_homes.hpp:16-19` stores
  global-load result instructions by `std::string_view` result spelling.
- Producer: `src/backend/prealloc/regalloc/value_homes.cpp:126-131` walks the
  BIR function and inserts each named `bir::LoadGlobalInst` under
  `load_global->result.name`.
- Consumer: `src/backend/prealloc/regalloc/value_homes.cpp:230-238` and
  `src/backend/prealloc/control_flow.hpp:1728-1730` use the map when a computed
  value is not a binary or parameter, then validate the load result and global
  symbol fields before classifying the global-load base.
- Replacement authority: mirror the binary fix with a typed result-value-id
  map such as `global_loads_by_result_value`, keyed by interned `ValueNameId`
  for the load result inside the current prepared function. The `LoadGlobalInst`
  remains the structural payload for global symbol/address validation; the
  lookup boundary should no longer be raw `std::string_view`.

## Suggested Next

Execute `plan.md` Step 2 by replacing `HfaReturnLaneMap` with the typed BIR
call-result key recorded above, then run the focused build/test proof delegated
by the supervisor.

## Watchouts

- Do not edit `scripts/string_authority_classifications.json`.
- Do not rename reported symbols just to evade the scanner.
- For `homes_by_name`, avoid preserving the same authority under a less obvious
  name; the durable authority should be `PreparedValueId`.
- For the computed-value lookup maps, changing `std::string_view` to
  `ValueNameId` is a real authority move only if all producers and recursive
  consumers use the interned id consistently, including the recursion guard.

## Proof

No build proof required by the delegated packet; this was a localization-only
`todo.md` update. No `test_after.log` was produced.
