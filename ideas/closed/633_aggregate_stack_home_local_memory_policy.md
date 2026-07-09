# Aggregate Stack-Home Local-Memory Policy

Status: Closed
Type: Implementation
Parent: `ideas/closed/614_rv64_pointer_local_memory_consumption.md`
Related:
- `ideas/closed/614_rv64_pointer_local_memory_consumption.md`
- `ideas/open/624_prepared_outgoing_stack_argument_destination_offsets.md`
- `ideas/open/627_pointer_stack_result_call_policy.md`
- `ideas/open/629_prepared_return_destination_home_authority.md`
Owning Layer: aggregate/sret/byval stack-home local-memory policy
Queue Order: 33
Prerequisites: aggregate, sret, byval, or pointer stack-home facts must
identify source value, destination home, stack slot, offset, size, and memory
use authority before RV64 local-memory emission
Proof Surface: local-memory rows involving sret/byval or aggregate pointer
stack homes that remain outside idea 614's frame-slot consumer support

## Goal

Define the policy and authority boundary for local-memory accesses through
aggregate, sret, byval, or pointer stack homes without inferring stack-home
semantics inside the generic RV64 local-memory consumer.

## Why This Exists

Idea 614 closed after supporting explicit prepared frame-slot local-memory
facts. Step 4 residuals kept sret/byval and aggregate pointer stack-home rows
out of 614 because they require stack-home policy and producer authority beyond
plain selected frame-slot access.

## In Scope

- Refresh residual local-memory rows that access aggregate, sret, byval, or
  pointer stack homes.
- Separate producer-publication gaps from RV64 consumer gaps using explicit
  stack-home, offset, size, value identity, and memory-use facts.
- Implement RV64 consumption only when those facts are complete and shared
  across a row family.
- Keep diagnostics fail-closed for missing stack-home authority, ambiguous
  aggregate lanes, incomplete size/offset facts, and stale value identity.

## Out Of Scope

- Outgoing stack argument destination publication owned by idea `624`.
- Pointer stack-result call policy owned by idea `627`.
- Prepared return destination-home authority owned by idea `629`.
- Generic frame-slot local-memory support already closed by idea 614.
- String/global local-memory policies, F128 width policy, expectations,
  unsupported markers, allowlists, timeouts, runtime, or accounting.

## Acceptance Criteria

- A refreshed probe identifies whether aggregate stack-home residuals belong
  to producer authority, ABI policy, return policy, or RV64 local-memory
  consumption.
- At least one complete-authority aggregate stack-home local-memory family
  moves past its current owner, or all such rows are reclassified into more
  precise existing/new producer ideas.
- Negative proof keeps plain frame-slot, string/global, 16-byte/F128, ABI
  transport, and return-only rows outside this policy.

## Reviewer Reject Signals

- Reject RV64 inference of aggregate stack homes from final assembly layout,
  ABI convention, source filenames, or object shape.
- Reject testcase-shaped handling for a single aggregate/sret/byval row.
- Reject broad ABI or return rewrites that bypass ideas `624`, `627`, or
  `629`.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes as capability progress.
- Reject helper renames or diagnostic wording changes that leave stack-home
  local-memory authority missing.

## Closure Notes

Closed after the aggregate stack-home runbook moved the verified byval/sret
stack-home local-memory families past the local-memory owner or reclassified
the remaining rows into existing residual owners.

Final Step 11 evidence:
- `src/20020215-1.c`, `src/950628-1.c`, `src/pr30185.c`, and
  `src/pr60017.c` moved from `unsupported_local_memory_access` to
  `unsupported_call_abi`.
- `src/pr38969.c` and `src/struct-ret-1.c` had already moved past stack-home
  local memory in Step 8.
- `src/941110-1.c` did not move, but current prepared evidence shows ordinary
  local-slot/frame-slot pointer publication with an unknown-layout 8-byte
  pointer store rather than byval/sret stack-home authority. It is documented
  here as a possible future residual, not included in idea 633 scope.

The remaining failures are owned by later call ABI/call-instruction work or by
existing residual ideas for move-bundle fan-in, F128/16-byte width policy,
pointer-loaded-from-global local memory, large selected pointer offsets,
branch stack-load clobber safety, aggregate global materialization, mixed
local/global publication, and runtime/global mismatch.

Close gate:
- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_object_emission|backend_prepare_stack_layout)$' > test_after.log`
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
- Result: PASS, before `passed=2 failed=0 total=2`, after
  `passed=2 failed=0 total=2`.
