# Current Packet

Status: Active
Source Idea Path: ideas/open/749_lir_selected_memcpy_current_function_pointer_object_lifetime_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Populate and validate the selected producer authority

## Just Finished

- Plan Step 2 complete: the fixed aggregate byval parameter materialization
  producer in `lvalue.cpp` now populates the selected current-function
  authority with typed byval-parameter and destination-alloca pointer
  definitions, same-owner `LinkNameId`, distinct local objects, and
  live-at-selected-site facts. Same-feature coverage exercises the production
  HIR-to-LIR lowering path plus nearby malformed authority rejection
  boundaries without using rendered text as semantic authority.

## Suggested Next

- Execute Plan Step 3 only: record the accepted proof and hand this
  prerequisite authority contract back to the blocked memcpy publication route
  so idea 748 can retry the exact selected `lvalue.cpp` producer without this
  blocker changing memcpy schema, publication, or Raw-BIR behavior.

## Watchouts

- The selected authority remains the only carrier and is populated only for the
  fixed aggregate byval parameter materialization producer. `LirMemcpyOp`
  schema/publication/verifier behavior and other memcpy producers were not
  touched. A tiny `LirFunction::alloc_object()` cursor was added because Step 2
  needed distinct selected local object IDs.

## Proof

- Executor proof passed: `cmake --preset default && cmake --build --preset
  default && ctest --test-dir build -j --output-on-failure -R '^backend_' |
  tee test_after.log` passed 5/5 backend tests, including
  `backend_lir_selected_pointer_authority`; proof log is `test_after.log`.
  Supervisor regression guard passed with `--allow-non-decreasing-passed`
  against canonical `test_before.log` and `test_after.log`, both 5/5 with no
  new failing tests.
