# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 1.1
Current Step Title: Establish executable pipeline identity and transaction foundations

## Just Finished

- Completed Plan Step 1.1a: added the compiled target-independent pipeline
  identity foundation under `src/backend/bir/pipeline/identity.*` and exported
  it through `bir.hpp`.
- Added strong `ModuleRevision` and `FunctionRevision` value axes, stable
  `Fingerprint128`/`FunctionRevisionDigest` values, and `PipelineStageStamp`
  equality over module epoch, module revision, and ordered function-revision
  digest.
- Added deterministic digest construction that preserves the caller-supplied
  core canonical `(FunctionId, FunctionRevision)` sequence exactly. Zero epoch,
  invalid/foreign IDs, and duplicate live slots return indexed typed errors
  without a partial digest or silent sorting/normalization.
- Added dedicated `backend_bir_pipeline_identity` coverage and CMake
  integration for deterministic construction, every freshness axis, changed
  identity/revision, empty input, arbitrary valid order and order sensitivity,
  invalid/foreign IDs, duplicate-slot rejection, and clean construction after
  failure.

## Suggested Next

- Continue Plan Step 1.1 with a separate bounded transaction/cancellation
  packet: introduce the smallest private stage-candidate/checkpoint state and
  typed cancellation/failure behavior using these exact identity primitives.
  Do not widen that packet into pass dispatch, product registries, target facts,
  or publication-token implementation.

## Watchouts

- The caller must supply core's explicit canonical module function order. This
  identity-only helper preserves and hashes that sequence but cannot prove its
  provenance until future core-revision integration. It rejects duplicate live
  slots, including different generations of the same slot, rather than sorting.
- `FunctionRevisionDigest` uses an explicitly byte-ordered, implementation-
  local stable 128-bit mixer rather than pointer, string, `std::hash`, container
  iteration, or target identity.
- This packet implements identity values only. It does not implement or imply
  mutation, cancellation, rollback, stage publication, pass scheduling,
  analysis products, pseudo facts, or allocation state.

## Proof

- `cmake --preset default`: configured and generated successfully.
- `cmake --build --preset default`: built `c4c_backend`,
  `backend_bir_pipeline_identity_test`, the existing backend interface test,
  and the default tree successfully; the final confirmation reported
  `ninja: no work to do`.
- Exact supervisor-selected command:
  `ctest --test-dir build -j --output-on-failure -R '^backend_'`.
  Baseline `test_before.log`: 1/1 passed. Current `test_after.log`: 2/2 passed,
  including `backend_bir_pipeline_identity`; zero failures.
- Passed `git diff --check`.
- Owned files: `src/backend/bir/pipeline/identity.hpp`,
  `src/backend/bir/pipeline/identity.cpp`, `src/backend/bir/bir.hpp`,
  `tests/backend/bir/backend_bir_pipeline_identity_test.cpp`,
  `tests/backend/bir/CMakeLists.txt`, `todo.md`, and canonical
  `test_after.log`.
