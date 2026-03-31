# Backend Test Restructure — Execution State

Source Idea: ideas/open/19_backend_test_restructure.md
Source Plan: plan.md
Status: Active

## Pending

- [x] Step 1: Audit the current test structure
- [x] Step 2: Split helpers — util vs fixture
- [x] Step 3: Add argv[1] test name filter to main()
- [ ] Step 4: Split aarch64 test file by function category — SKIPPED
- [ ] Step 5: Split x86 test file by function category — SKIPPED
- [x] Step 6: Full regression validation

## Notes

### Step 4 / Step 5 — Skipped

Both aarch64 (2708 lines) and x86_64 (2566 lines) files have many file-local
`make_*` fixture helper functions defined at the top of the file that are
shared across multiple test categories (rendering, assembler, linker). Splitting
these files would require either duplicating those helpers or moving them to yet
another shared header, which is not a pure structural refactor and carries
real risk of breakage. The files are large enough to qualify for the split but
the internal structure is not cleanly pre-separated, so the split was deferred.

### Step 3 — Filter scope

The `test_filter()` mechanism and updated `RUN_TEST` macro enable per-test
filtering for tests that use `RUN_TEST` (currently only
`backend_lir_adapter_x86_64_tests`). All other test binaries accept `argv[1]`
but call test functions directly so the filter is not applied per-function —
those binaries will run all their tests regardless of the filter argument.

### Step 6 — Baseline

Pre-existing failures: 137 c_testsuite_aarch64_backend tests failing on this
x86 host (aarch64 runtime not available). Baseline before this refactor:
2534/2671. After refactor: same 2534/2671.
