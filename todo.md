Status: Active
Source Idea Path: ideas/open/383_string_authority_guard_unclassified_symbols.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Current Authority Usage

# Current Packet

## Just Finished

Lifecycle activation created the active runbook for Step 1.

## Suggested Next

Execute Step 1 from `plan.md`: inspect the four reported string-authority
symbols, trace their producers and consumers, and choose the typed or
structural replacement authority for each before editing implementation code.

## Watchouts

- Do not edit `scripts/string_authority_classifications.json`.
- Do not rename reported symbols just to evade the scanner.
- Keep the first implementation packet limited to localization unless the
  replacement authority is obvious and tightly scoped.

## Proof

Activation evidence supplied by supervisor:
`ctest --test-dir build -j --output-on-failure -R '^string_authority_guard$'`
currently fails with four unclassified symbols:

- `src/backend/bir/lir_to_bir/lowering.hpp:429: HfaReturnLaneMap: pattern=string-keyed-alias`
- `src/backend/mir/aarch64/module/module.hpp:97: PreparedValueHomeIndexes::homes_by_name: pattern=by-name-member`
- `src/backend/prealloc/regalloc/value_homes.hpp:18: PreparedComputedValueLookup::named_binaries: pattern=string-keyed-container`
- `src/backend/prealloc/regalloc/value_homes.hpp:19: PreparedComputedValueLookup::named_global_loads: pattern=string-keyed-container`
