Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Publication Authority Leak

# Current Packet

## Just Finished

Step 5 closure review rejected closure for the AArch64 calls emission
consolidation checkpoint.

- The source idea remains open because retained `CallInst::arg_abi` and
  `CallInst::arg_types` publication decisions still exist in
  `calls_dispatch_bridge.cpp` and `calls_argument_sources.cpp`.
- `calls_dispatch_bridge.hpp` still exposes `CallInst`-shaped helper
  boundaries that need to be retired or justified as emission-only after
  publication no longer reconstructs call-plan decisions.
- The close review did not run a close-time regression guard because closure
  was rejected before the close gate.

## Suggested Next

Delegate Step 1 to an executor: select one remaining publication authority
leak, map it to an existing prepared argument/move/boundary-effect fact or a
precise missing-prepared-fact blocker, and record the focused proof command.

## Watchouts

- Do not close `ideas/open/02_aarch64_calls_emission_consolidation.md` while
  publication-helper authority blockers remain.
- Do not touch unrelated
  `ideas/open/03_dispatch_responsibility_reduction.md` dispatch cleanup.
- Reject helper renames, expectation rewrites, and testcase-shaped shortcuts as
  progress.

## Proof

Closure review evidence:

- `rg -n "arg_abi|arg_types|CallInst" src/backend/mir/aarch64/codegen/calls*.cpp src/backend/mir/aarch64/codegen/calls*.hpp src/backend/mir/aarch64/codegen/calls.hpp`
- `rg -n "publication|byval|aggregate|frame address|local frame|CallInst" src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp src/backend/mir/aarch64/codegen/calls_argument_sources.cpp src/backend/mir/aarch64/codegen/calls_dispatch_bridge.hpp`

Durable blockers found:

- `calls_dispatch_bridge.cpp`: `call_argument_allows_local_aggregate_address_publication`
  still reads retained `CallInst::arg_abi` and `CallInst::arg_types`.
- `calls_argument_sources.cpp`: pointer/byval helpers under
  `call_argument_allows_local_frame_address_publication` still read retained
  `CallInst::arg_types` and `CallInst::arg_abi`.
- `calls_dispatch_bridge.hpp`: helper declarations still expose
  `bir::CallInst` parameters on publication/call-boundary helper boundaries.
