Status: Active
Source Idea Path: ideas/open/138_call_plan_lookup_ownership_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Update known consumers to include the domain boundary

# Current Packet

## Just Finished

Step 4 updated and verified the known call-plan lookup consumers against the
call-owned declaration boundary.

- Added direct `calls.hpp` boundary includes to direct call fact consumers:
  `src/backend/prealloc/call_plans.cpp`,
  `src/backend/prealloc/prepared_printer/calls.cpp`,
  `src/backend/prealloc/prepared_printer/functions.cpp`,
  `src/backend/mir/aarch64/codegen/traversal.cpp`,
  `src/backend/mir/aarch64/codegen/calls.cpp`, and
  `src/backend/mir/aarch64/codegen/globals.cpp`.
- Kept `prepared_lookups.hpp` in `call_plans.cpp` and `globals.cpp` because
  both still consume non-call prepared lookup APIs alongside call facts.
- Verified `src/backend/mir/aarch64/codegen/dispatch.cpp` already reaches
  prepared call facts through the AArch64 call-lowering facade instead of
  naming call-plan lookup helpers directly.
- No local scans or duplicated preservation, outgoing-stack, or call-boundary
  discovery logic were introduced.

## Suggested Next

Execute Step 5 from `plan.md`: run the backend behavior proof and inspect the
diff for ownership quality, semantic drift, expectation weakening, or
testcase-shaped shortcuts.

## Watchouts

- The builder/query definitions intentionally still live in
  `prepared_lookups.cpp`; moving them would widen ownership to helpers that are
  not call-only.
- `src/backend/mir/aarch64/module/module.hpp` still includes the broad
  prepared lookup facade because `FunctionLoweringContext` owns broad prepared
  lookup pointers as well as the call-plan lookup pointer; this packet did not
  own that header.
- `src/backend/mir/aarch64/codegen/dispatch.cpp` uses local call-lowering API
  calls such as `find_prepared_call_plan` and does not need its own direct
  prealloc call lookup include.
- Do not change ABI classification, call lowering semantics, preservation
  behavior, or AArch64 register handling.
- Do not replace prepared call lookups with local scans in AArch64 consumers.

## Proof

`cmake --build --preset default` succeeded after the include-boundary changes.
No test subset was run for this packet. No root-level proof log was created or
modified by this packet because the delegated proof was a build command and the
packet listed root-level logs under Do Not Touch.
