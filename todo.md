Status: Active
Source Idea Path: ideas/open/288_extract_aapcs64_variadic_hfa_lane_expansion_helper.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Broader Validation and Closure Readiness

# Current Packet

## Just Finished

Step 5 validated closure readiness for idea 288 after the AAPCS64 variadic HFA
carrier helper extraction.

The broader backend proof passed for all `backend_` CTest targets. Read-only
inspection confirmed semantic BIR direct and indirect call lowering both route
outgoing carrier expansion through
`build_aapcs64_variadic_hfa_carrier_expansion`; neither path recomputes HFA
lane shape after the helper classifies the aggregate carrier. The helper derives
lane kind/count from aggregate layout plus local carrier slot metadata and
returns fail-closed `Rejected` results for missing aggregate aliases, leaf-slot
count mismatches, slot-type mismatches, or missing ABI classification.

Prepared/prealloc coverage for AAPCS64 variadic aggregate `va_arg` still consumes
explicit lane-shape facts (`register_save_lanes` and
`register_save_lane_size`) from semantic BIR/prepared access-plan metadata, not a
separate downstream recomputation.

The helper does not consume rendered call-argument text or ABI suffix text as
primary truth. Rendered text remains only in existing legacy byval layout
compatibility paths outside the helper boundary.

## Suggested Next

Ask the plan owner to evaluate whether the active runbook should close idea 288
or retire/split any remaining adjacent ABI cleanup separately.

## Watchouts

- Keep idea 289 out of scope for this plan.
- No closure blocker was found in the helper extraction boundary or focused
  backend fixtures.
- Adjacent cleanup remains outside idea 288: existing no-id/legacy byval layout
  compatibility bridges still use rendered type text where structured
  `LirTypeRef`/`StructNameId` metadata is unavailable. That should stay a
  separate ABI cleanup initiative if pursued.
- No fixture-support follow-up was found for the Step 4 fail-closed carrier
  edges.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed. `test_after.log` reports 180/180 backend tests passed.
