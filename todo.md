# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory and migrate AArch64 dispatch authority

## Just Finished

- Plan Step 1 migrated the call-result source-publication dispatch family in
  `src/backend/mir/aarch64/codegen/dispatch.cpp` away from a locally rebuilt
  Route6 call-use source index. Dispatch now relies on the existing prepared
  call-result late-publication and indexed value-home queries; absent or
  inconsistent prepared authority leaves the publication unavailable rather
  than reconstructing producer reasoning in dispatch.

## Suggested Next

- Review the remaining AArch64 named-handoff materializers and select the next
  coherent executable-route family for migration to common prepared queries.

## Watchouts

- `record_call_result_source_register` still exposes optional Route6 evidence
  internally, but dispatch no longer constructs or supplies that authority;
  keep subsequent migrations from recreating the removed index elsewhere.
- The focused baseline retains the known scalar-FP literal-add failure (test
  354, missing `bl printf`) and has no additional failures.

## Proof

- Ran `set -o pipefail; { cmake --build --preset default && ctest --test-dir
  build -j --output-on-failure -R '^backend_(codegen_route|cli)_aarch64_'; } 2>&1
  | tee test_after.log`. Build succeeded; 33/34 tests passed, with only the
  baseline test 354 failure. Proof log: `test_after.log`.
