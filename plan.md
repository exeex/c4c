# Backend Regex Failure Family Inventory Plan

Status: Active
Source Idea: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Refresh the main-build backend-regex failure inventory after the byval
aggregate lane publication plan closed, then select the next focused semantic
owner before any implementation work starts.

Goal: turn the current `ctest -R backend` residual surface into classified
failure families, then activate or split one focused repair idea only when the
evidence identifies a real backend capability.

Core Rule: do not treat the backend regex as one monolithic failure bucket, and
do not reopen closed or parked owners from counts alone.

## Read First

- `ideas/open/295_backend_regex_failure_family_inventory.md`
- Recent focused/parked residuals before selecting a new owner:
  `ideas/open/326_aarch64_variadic_hfa_floating_residual.md`,
  `ideas/open/327_aarch64_fixed_formal_entry_publication.md`,
  `ideas/open/329_aarch64_variadic_aggregate_va_arg_call_setup.md`,
  `ideas/open/331_aarch64_variadic_stdarg_cursor_format_residual.md`,
  `ideas/open/332_aarch64_movi_zero_extension_materialization.md`, and
  `ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md`.
- Current `test_before.log` / `test_after.log` only as historical artifacts;
  regenerate inventory logs when this plan executes.

## Current Targets

- Main build tree backend regex inventory from `/workspaces/c4c/build`.
- External `c_testsuite_aarch64_backend_*` residuals selected by
  `ctest -R backend`.
- Failure-family classification that can justify a focused semantic owner.

## Non-Goals

- Do not implement compiler or backend fixes under this umbrella plan.
- Do not change expectations, unsupported classifications, allowlists, CTest
  registration, timeout policy, runner behavior, or proof-log policy.
- Do not archive parked ideas or mutate `ideas/closed/*`.
- Do not reopen closed AArch64 focused owners without generated-code or proof
  evidence contradicting their closure boundary.
- Do not split a new idea from filename recurrence alone.

## Working Model

- The backend regex includes local backend tests plus registered AArch64
  c-testsuite backend runtime tests.
- Ideas 347 and 328 have closed, and the prior umbrella pass selected 328 from
  a 21-failure external AArch64 residual surface.
- Several open focused residuals are parked or scope-satisfied; they should
  only be reactivated if fresh first-bad-fact evidence falls inside their
  stated scope.
- The next useful action is a fresh inventory and classification pass, not
  implementation.

## Execution Rules

- Capture current results from the main build tree.
- Classify failures by observable first bad fact: local backend/unit failure,
  frontend/prepared-node or machine-printer diagnostic, semantic `lir_to_bir`
  admission, assembler/linker failure, runtime nonzero/crash, runtime
  mismatch, timeout, or output-storm.
- Compare candidates against open parked ideas before creating a new idea.
- If a focused owner is found, write the durable owner decision into `todo.md`;
  lifecycle authority can then switch to that focused idea in a later packet.
- Keep broad runtime scans bounded and avoid relying on stale processes or
  stale generated artifacts.

## Steps

### Step 1: Capture Fresh Backend Regex Inventory

Goal: replace stale residual counts with a current main-build inventory.

Primary target: `/workspaces/c4c/build`.

Actions:

- Build with `cmake --build --preset default`.
- Run the backend regex inventory from the main build tree:
  `ctest --test-dir build -j10 -R backend --output-on-failure | tee test_after.log`.
- Record selected, passed, failed, and timed-out counts in `todo.md`.
- Note whether local backend/unit/CLI tests are clean or failing separately
  from `c_testsuite_aarch64_backend_*`.

Completion check:

- `todo.md` records a fresh backend-regex result with current counts and
  distinguishes local backend failures from external AArch64 c-testsuite
  failures.

### Step 2: Classify Current Failure Families

Goal: group the current failures by semantic owner instead of by filename.

Primary target: failing tests and generated artifacts under the main build
tree.

Actions:

- Extract failing test names and failure modes from `test_after.log`.
- For compile-stage failures, inspect diagnostics and available generated or
  prepared artifacts.
- For runtime failures, inspect representative output and generated AArch64
  only enough to classify the first bad fact.
- Quarantine timeout or output-storm cases unless a safe bounded reproduction
  exists.
- Record the family buckets and representative evidence in `todo.md`.

Completion check:

- `todo.md` lists failure buckets with representative tests and first-bad-fact
  evidence sufficient to decide whether a focused semantic owner exists.

### Step 3: Select The Next Focused Owner

Goal: decide whether the inventory justifies activating or creating a focused
repair idea.

Actions:

- Compare the strongest current bucket against existing open parked ideas.
- Prefer reactivating an existing open idea when the first bad fact falls
  inside its scope.
- Create a new focused idea only when the bucket is distinct and includes
  concrete semantic evidence, in-scope/out-of-scope boundaries, acceptance
  criteria, and reviewer reject signals.
- Do not select a bucket whose evidence is only a failing count, stale
  artifact, or named testcase recurrence.

Completion check:

- `todo.md` names the selected focused owner or explains why no owner is ready.
- If a new focused idea is required, the idea file exists under `ideas/open/`
  with reviewer reject signals.

### Step 4: Deactivate Or Switch From The Umbrella

Goal: leave the lifecycle ready for focused implementation, not broad
inventory execution.

Actions:

- Preserve durable inventory findings in the umbrella source idea only if a
  lifecycle switch or deactivation requires them.
- Switch to the selected focused idea when lifecycle authority is delegated to
  do so.
- If no owner is ready, deactivate with a clear blocker and remaining
  classification needs.

Completion check:

- The active lifecycle state is no longer this umbrella before implementation
  starts, or `todo.md` explains the exact ambiguity blocking a focused owner.
