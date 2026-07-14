# Current Packet

Status: Active
Source Idea Path: ideas/open/748_lir_memcpy_selected_pointer_object_authority_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Define and populate the selected memcpy typed authority

## Just Finished

- Lifecycle handoff from prerequisite idea 749 is complete. Idea 749 is closed
  as accepted, and its authority contract is recorded in both the closed source
  and this resumed source.

## Suggested Next

- Execute Step 1 only: retry exactly the selected fixed aggregate byval
  parameter materialization row in `src/codegen/lir/hir_to_lir/lvalue.cpp`.
  Populate the selected `LirMemcpyOp` destination/source pointer, object, and
  lifetime facts only from
  `LirFunction::selected_memcpy_pointer_authority`.

## Watchouts

- Idea 749 did not change `LirMemcpyOp` schema/publication/verifier behavior,
  builtin memcpy, Raw-BIR, or any other memcpy producer. This resumed idea owns
  the selected memcpy publication and verifier boundary, still with no display
  text fallback and no scope expansion to additional memory families.
- The accepted carrier contains only `byval_parameter` and
  `destination_alloca` current-function pointer definitions, each with a valid
  value ID, `ptr` type, distinct local object, selected current-function owner,
  role, and `live_at_selected_site=true`.

## Proof

- Prerequisite proof accepted for idea 749: `cmake --preset default && cmake
  --build --preset default && ctest --test-dir build -j --output-on-failure -R
  '^backend_' | tee test_after.log` passed 5/5 backend tests, including
  `backend_lir_selected_pointer_authority`.
- Supervisor regression guard passed with `--allow-non-decreasing-passed`
  against canonical `test_before.log` and `test_after.log`, both 5/5 with no
  new failing tests.
- After Step 1 implementation in this resumed idea: run a fresh build plus
  focused selected memcpy producer coverage; supervisor owns matching
  regression logs and broader checks.
