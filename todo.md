Status: Active
Source Idea Path: ideas/open/243_aarch64_variadic_machine_node_consumption.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Consume Prepared Facts For `va_copy`

# Current Packet

## Just Finished

Step 5 completed `va_copy` selected machine-node consumption from prepared
source/destination `va_list` homes, prepared `va_list_layout` size/alignment and
field records, and helper scratch resources. AArch64 now emits structured
`VariadicVaCopy` records, per-field copy records, effects, central mnemonic
mapping, and printer output from prepared facts, with missing source/destination
homes or incomplete layout field facts still fail-closed.

## Suggested Next

Execute Step 6 by validating and summarizing the accepted selected-node
consumption route for `va_start`, scalar `va_arg`, aggregate `va_arg`, and
`va_copy`.

## Watchouts

- `va_copy` field copy coordinates intentionally mirror prepared
  `va_list_layout` field offsets; do not add AAPCS64 layout reconstruction in
  AArch64 lowering.
- Step 6 should decide whether this helper-family milestone needs reviewer or
  lifecycle closure after the broader backend proof.

## Proof

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`
passed: 139/139 backend tests green. `test_after.log` is the proof log.
