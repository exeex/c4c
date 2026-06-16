Status: Active
Source Idea Path: ideas/open/288_extract_aapcs64_variadic_hfa_lane_expansion_helper.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Move HFA Carrier Expansion Policy Behind the Helper

# Current Packet

## Just Finished

Step 3 moved outgoing AAPCS64 variadic HFA carrier-array policy fully behind
the private helper in `src/backend/bir/lir_to_bir/calling.cpp`.

The helper now derives its own ordered carrier leaf slots from the aggregate
slot facts plus the structured aggregate layout, so generic call lowering no
longer precomputes or threads ordered leaf-slot state into the request.
Recognized-but-incomplete carrier facts still return `Rejected`, and each
generic call path continues to fail closed on `Rejected` while using
`NoExpansion` for the ordinary byval fallback.

## Suggested Next

Execute `Step 4: Prove Fail-Closed Carrier Edge Coverage`.

## Watchouts

- Keep idea 289 out of scope for this plan.
- Do not broaden AArch64 ABI classification or prepared/prealloc call-plan
  behavior.
- The helper contract remains intentionally private in `calling.cpp`; no new
  build target or public header was needed.
- `Rejected` must keep meaning recognized-but-incomplete carrier facts, not a
  soft fallback to byval pointer lowering.
- Step 4 should target missing aggregate alias, leaf-slot count mismatch, and
  slot-type mismatch coverage only if fixture support already exists.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$' > test_after.log`

Result: passed. `test_after.log` contains the delegated focused CTest output.
