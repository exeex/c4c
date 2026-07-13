# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 1.1
Current Step Title: Establish executable pipeline identity and transaction foundations

## Just Finished

- Completed Plan Step 1.1c: integrated the pipeline identity axes into actual
  core BIR storage and the verified publication boundary.
- `ModuleData` now owns its exact `ModuleRevision`; every `FunctionData` owns
  its exact `FunctionRevision`. Initial builder construction uses revision zero
  for both axes because construction is not a committed pipeline transform.
- Verified `ModuleBuilder::publish()` freezes one `PipelineStageStamp` from the
  module epoch, module revision, explicit `function_order_`, and each resolved
  live function revision. Digest construction failures return typed
  `PipelineIdentityFailed` publication failures with the exact identity error;
  there is no assertion, slot sort, unordered traversal, or fallback digest.
- `RawBir` and `CanonicalBir` expose the immutable stamp owned by their storage;
  `ModuleView` and `FunctionView` expose exact revisions. The current no-op
  canonicalization moves the same storage and preserves every stamp axis.
- Expanded pipeline-identity coverage for empty and populated Raw publication,
  exact revision access, helper-matching digest, separate-builder epoch
  identity, declaration-plus-definition order, and exact Canonical preservation.

## Suggested Next

- Continue Plan Step 1.1 with a separate bounded private-candidate/revision-bump/
  rollback packet using the completed identity and execution-control
  primitives. Keep it limited to private owning candidate/checkpoint state,
  exact committed revision increments, and typed discard/rollback behavior; do
  not widen into pass dispatch, target facts, analysis products, or editor APIs.

## Watchouts

- Builder graph edits remain construction only and deliberately do not advance
  module or function revisions. Revision increments belong to a later committed
  candidate transaction packet.
- The digest source is the module's explicit function order, including both
  declarations and definitions; never replace it with slot or hash-map order.
- This slice introduces no editor, candidate fork, rollback, cancellation,
  pass dispatch, target fact, allocation fact, diagnostic, or analysis product.

## Proof

- `cmake --preset default`: configured and generated successfully.
- `cmake --build --preset default`: full default build succeeded.
- Exact supervisor-selected command:
  `ctest --test-dir build -j --output-on-failure -R '^backend_'`.
  Baseline `test_before.log`: 3/3 passed. Current `test_after.log`: 3/3 passed,
  including the expanded `backend_bir_pipeline_identity`; zero failures.
- Passed `git diff --check`.
- Owned files: `src/backend/bir/core/ir.hpp`,
  `src/backend/bir/core/view.hpp`, `src/backend/bir/core/builder.hpp`,
  `src/backend/bir/core/builder.cpp`,
  `tests/backend/bir/backend_bir_pipeline_identity_test.cpp`, `todo.md`, and
  canonical `test_after.log`.
