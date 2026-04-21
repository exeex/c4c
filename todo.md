# Execution State

Status: Active
Source Idea Path: ideas/open/65_semantic_call_family_lowering_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit The Active Semantic Call Family
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Step 1 audit completed for idea 65: `c_testsuite_x86_backend_src_00204_c`
still fails in function `arg` under the `direct-call semantic family`, and the
first concrete seam is direct-call lowering for aggregate `byval` arguments
that come straight from global aggregate storage. The nearest protective
backend coverage is `backend_codegen_route_x86_64_aggregate_param_return_pair_observe_semantic_bir`,
which proves local by-value aggregate direct-call/return lowering, but it does
not cover the first `00204.c` owned miss where `arg` reaches calls such as
`fa_s17(ptr byval(%struct.s17) align 8 @s17)` and the direct-call lowering
path still expects a local aggregate-slot alias rather than a global aggregate
source.

## Suggested Next

Repair the direct-call semantic path so aggregate `byval` arguments can lower
from global aggregate storage, then prove it against
`c_testsuite_x86_backend_src_00204_c` plus the nearest direct-call aggregate
route coverage before widening to later HFA or return-lane seams.

## Watchouts

- Do not reopen the closed idea-62 stack/addressing lane unless a case again
  fails before semantic call lowering begins.
- Do not route semantic call-family failures into idea 61 until semantic
  lowering itself succeeds and the case actually reaches prepared-module or
  prepared call-bundle ownership.
- Avoid call-shaped shortcuts that only make `00204.c` pass without repairing
  the underlying direct-call lowering route.
- The first failing shape is not the already-covered local aggregate pair lane:
  `arg` lowers past the small register-passed struct calls and hits the first
  global aggregate `byval` direct-call handoff at `fa_s17`, with the same seam
  then recurring for larger HFA globals such as `fa_hfa23`, `fa_hfa24`,
  `fa_hfa31`, and mixed calls like `fa3` and `fa4`.

## Proof

Ran delegated proof: `cmake --build --preset default && ctest --test-dir build
-j --output-on-failure -R '^backend_'` and preserved the executor proof log at
`test_after.log`.

Supplemental audit-only observation outside the delegated proof: `ctest
--test-dir build --output-on-failure -R '^c_testsuite_x86_backend_src_00204_c$'`
still fails with `latest function failure: semantic lir_to_bir function 'arg'
failed in semantic call family 'direct-call semantic family'`.
