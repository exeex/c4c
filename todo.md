# Execution State

Status: Active
Source Idea Path: ideas/open/64_shared_text_identity_and_semantic_name_table_refactor.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Extract The Shared Text Table Layer
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Completed `plan.md` Step 1 by moving the shared text-id, link-name, and
member-symbol table substrate out of `src/frontend/` into `src/shared/` and
updating current frontend/codegen include sites to use the shared header.

## Suggested Next

Introduce the next semantic-id layer on top of `TextTable` for backend-visible
domains, starting with the first typed name-table contracts needed by
`src/backend/prealloc/prealloc.hpp`.

## Watchouts

- Keep this runbook on idea 64 scope only. Do not pull in idea 62 CFG or idea
  63 phi algorithm work while introducing semantic ids.
- `src/backend/prealloc/prealloc.hpp` still exposes symbolic names as
  `std::string`; the next packet should replace those public identity surfaces
  with typed semantic ids rather than bare `TextId`.
- The shared text table is header-only today, so include-path churn is the main
  blast radius for this slice.

## Proof

Baseline and post-slice proof use `cmake --build --preset default` plus
`ctest --test-dir build -j --output-on-failure -R '^backend_'`; preserve
`test_before.log` and `test_after.log` for comparison. The baseline currently
includes a pre-existing failure in
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`.
