Status: Active
Source Idea Path: ideas/open/288_extract_aapcs64_variadic_hfa_lane_expansion_helper.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Define the Target ABI Helper Contract

# Current Packet

## Just Finished

Step 2 introduced the private AAPCS64 variadic HFA carrier expansion contract
in `src/backend/bir/lir_to_bir/calling.cpp`.

The new request type carries structured target/profile, variadic-call,
argument index, source operand, aggregate layout, type-decl/structured-layout,
aggregate-alias, local aggregate-slot, local scalar-slot, structured
call-argument alignment, and ordered leaf-slot facts. The result type now
distinguishes no expansion, rejected HFA candidate, and ordered expansion.
Expanded output carries lane value, lane BIR type, and matching
`bir::CallArgAbiInfo` together.

`append_aarch64_variadic_hfa_carrier_arg_lanes` now builds the request, calls
the helper, and only appends lanes from an `Expanded` result. `NoExpansion` and
`Rejected` continue to fall through to the existing byval/fail-closed path, so
observable call lowering behavior is unchanged.

## Suggested Next

Execute `Step 3: Move HFA Carrier Expansion Policy Behind the Helper`.

## Watchouts

- Keep idea 289 out of scope for this plan.
- Do not broaden AArch64 ABI classification or prepared/prealloc call-plan
  behavior.
- The helper contract is intentionally private in `calling.cpp`; no new build
  target or public header was needed for Step 2.
- `Rejected` is currently behavior-preserving because the append lambda returns
  `false` and the existing byval path still owns fail-closed call-family
  failure.
- Do not weaken supported-path test expectations or convert recognized
  incomplete HFA carriers into silent byval pointer lowering.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_notes$' > test_after.log`

Result: passed. `test_after.log` contains the delegated focused CTest output.
