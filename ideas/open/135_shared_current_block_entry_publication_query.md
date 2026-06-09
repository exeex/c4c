# Shared Current-Block Entry Publication Query

## Goal

Clarify the shared prepared/prealloc query for current-block entry publication
facts that AArch64 publication and producer code currently wraps locally.

## Why This Exists

The post-contract dispatch-family audit found that current-block entry
publication collection and register lookup are still AArch64 wrappers over
prepared current-block and value-home facts. The target-neutral relationship
should be queryable from a shared owner while AArch64 keeps register-view
selection and emission.

## In Scope

- `src/backend/mir/aarch64/codegen/dispatch_publication.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- The selected shared prepared or prealloc query files
- Query shape for current-block entry publication collection and prepared
  value-home register lookup

## Out Of Scope

- Broad publication rewrites
- Moving AArch64 register-view selection, publication register recording,
  branch-fusion hooks, relocation operands, or final instruction spelling into
  shared code
- File-count-only publication cleanup
- Replacing prepared facts with local scans

## Acceptance Criteria

- A shared query replaces an actual AArch64 current-block publication or
  value-home fact rediscovery site.
- AArch64 publication register recording, branch-fusion hooks, and
  operand/register spelling remain local.
- The query is target-neutral and reusable by owner semantics, not just a
  renamed AArch64 helper.
- Backend proof covers the affected publication and current-block paths.

## Proof Route

- `cmake --build --preset default`
- `ctest --test-dir build -R '^backend_' --output-on-failure`

Use matching `test_before.log` and `test_after.log` because shared prepared or
prealloc query code is in scope.

## Reviewer Reject Signals

- The route performs a broad publication rewrite instead of replacing a
  concrete rediscovery site.
- AArch64 register-view selection or final emission policy moves into shared
  code.
- The generic query does not replace an actual target-local wrapper.
- Local scans or BIR-name matching are retained behind a new API name.
- Test expectations are weakened or supported current-block publication
  behavior is marked unsupported.
