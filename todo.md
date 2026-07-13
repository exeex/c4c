# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 1.1
Current Step Title: Establish executable pipeline identity and transaction foundations

## Just Finished

- Completed Plan Step 1.1b: added the compiled target-independent
  `passes/execution_control.*` foundation and exported it through `bir.hpp`.
- Added `ResourceBudget` with only the accepted deterministic
  `max_work_units` axis, a read-only copyable `CancellationToken`, and one
  noncopyable/nonmovable external `CancellationSource` whose request state is
  safe to observe concurrently and whose tokens safely outlive the source.
- `checkpoint(work_units)` returns typed `Cancelled` or `ResourceLimit`
  failures, prevents integer wraparound and partial over-budget charge, makes
  cancellation/exhaustion sticky, and gives observed cancellation deterministic
  precedence over resource exhaustion.
- Zero-unit checkpoints are observation-only: they do not charge work, succeed
  while a positive budget remains live, and report an existing cancellation or
  exhaustion. Reaching the exact budget succeeds for that charge and makes all
  later checkpoints fail.
- Added dedicated `backend_bir_execution_control` coverage for independent
  sources, token lifetime, idempotent cancellation, exact-boundary accounting,
  zero-budget/zero-unit behavior, over-budget and overflow rejection, sticky
  failure, cancellation precedence, and repeated deterministic sequences.

## Suggested Next

- Continue Plan Step 1.1 with a separate bounded private-candidate/rollback
  packet using the completed identity and execution-control primitives. Keep it
  limited to private owning candidate/checkpoint state and typed discard/
  rollback behavior; do not widen into pass dispatch, target facts, analysis
  products, or Canonical publication.

## Watchouts

- Checkpoint calls sharing one invocation state are serial by contract;
  external cancellation request/observation is thread-safe. No wall clock,
  global, environment, randomness, target data, or pointer/string identity
  participates in a decision.
- `ResourceBudget{0}` begins exhausted. A zero-unit checkpoint never revives an
  exhausted or cancelled token. Cancellation requested after exhaustion changes
  the reported sticky reason to `Cancelled` because cancellation has explicit
  precedence, but success remains impossible.
- This slice owns no graph edit, stage transaction, resumable pipeline
  checkpoint capability, revision, publication, pass dispatch, diagnostic,
  entity, or other speculative budget axis.

## Proof

- `cmake --preset default`: configured and generated successfully.
- `cmake --build --preset default`: full default build succeeded; final
  confirmation reported `ninja: no work to do`.
- Exact supervisor-selected command:
  `ctest --test-dir build -j --output-on-failure -R '^backend_'`.
  Baseline `test_before.log`: 2/2 passed. Current `test_after.log`: 3/3 passed,
  including `backend_bir_execution_control`; zero failures.
- Passed `git diff --check`.
- Owned files: `src/backend/bir/passes/execution_control.hpp`,
  `src/backend/bir/passes/execution_control.cpp`, `src/backend/bir/bir.hpp`,
  `tests/backend/bir/backend_bir_execution_control_test.cpp`,
  `tests/backend/bir/CMakeLists.txt`, `todo.md`, and canonical
  `test_after.log`.
