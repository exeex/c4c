# Current Packet

Status: Active
Source Idea Path: ideas/open/706_common_mir_named_query_migration.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Replace the Route 4 common adapter and adapt AArch64 compatibility

## Just Finished

- Step 3.2 replaced the common block-entry adapter's Route 4 index build and
  raw-BIR reconstruction with a narrow conversion from the complete
  `PreparedCurrentBlockEntryPublication` payload.
- The common result now exposes only stable successor, destination, type, and
  publication-index semantics; request and result BIR pointer identity fields
  were retired, and every incomplete prepared status fails closed.
- AArch64 now supplies producer proof for the common identity while preserving
  its existing prepared-publication materialization fallback policy.
- Focused common and AArch64 contracts cover complete identity, fail-closed
  incomplete identity, and compatibility behavior.

## Suggested Next

- Supervisor review of the completed Step 3.2 adapter slice, then select the
  next bounded Step 3.3 prepared-authority family.

## Watchouts

- AArch64 target materialization policy remains unchanged: unavailable common
  identity does not erase an otherwise usable prepared publication plan.

## Proof

- `cmake --build --preset default --target
  backend_prealloc_block_entry_publications_test
  backend_aarch64_instruction_dispatch_test && ctest --test-dir build
  --output-on-failure -R
  '^(backend_prealloc_block_entry_publications|backend_aarch64_instruction_dispatch)$'
  > test_after.log 2>&1` passed: 2/2 focused tests; `test_after.log` is the
  canonical proof log.
