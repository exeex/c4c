Status: Active
Source Idea Path: ideas/open/288_extract_aapcs64_variadic_hfa_lane_expansion_helper.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Prove Fail-Closed Carrier Edge Coverage

# Current Packet

## Just Finished

Step 4 added focused `backend_lir_to_bir_notes` coverage for outgoing AAPCS64
variadic HFA carrier arrays that must fail closed after helper extraction.

The tests now cover missing aggregate alias facts, carrier leaf-slot count
mismatch, and carrier slot-type mismatch. Each fixture expects semantic call
lowering to fail in the direct-call family instead of silently falling back to
byval pointer lowering or partially expanding lanes. No implementation changes
were needed.

## Suggested Next

Execute `Step 5: Broader Validation and Closure Readiness`.

## Watchouts

- Keep idea 289 out of scope for this plan.
- Do not broaden AArch64 ABI classification or prepared/prealloc call-plan
  behavior.
- The helper contract remains intentionally private in `calling.cpp`; no new
  build target or public header was needed.
- `Rejected` must keep meaning recognized-but-incomplete carrier facts, not a
  soft fallback to byval pointer lowering.
- The Step 4 mismatch fixtures deliberately keep local-memory lowering valid
  and move the inconsistency to the carrier helper boundary: the count case
  loads a two-lane float carrier but calls with a three-lane HFA shape, and the
  type case loads two integer slots but calls with a two-float HFA shape.
- No fixture blocker remains for the requested fail-closed carrier edges.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$' > test_after.log`

Result: passed. `test_after.log` contains the delegated focused CTest output.
