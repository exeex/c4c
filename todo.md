Status: Active
Source Idea Path: ideas/open/206_prepared_memory_volatility_address_space_carrier.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extend The Shared Prepared Carrier

# Current Packet

## Just Finished

Completed Step 2, `Extend The Shared Prepared Carrier`.

Implemented shared prepared memory facts:
- `src/backend/prealloc/prealloc.hpp` now exposes typed
  `PreparedMemoryAccess::address_space` and `PreparedMemoryAccess::is_volatile`
  fields with default BIR semantics.
- `src/backend/prealloc/stack_layout/coordinator.cpp` threads those facts from
  `bir::MemoryAddress` through all ten existing `PreparedMemoryAccess{...}`
  builders while leaving base kind, identity, byte offset, size, alignment, and
  `can_use_base_plus_offset` behavior unchanged.
- `rg` confirms the ten coordinator `PreparedMemoryAccess` construction sites
  all set both new fields from `inst.address`.

Focused test updates:
- `tests/backend/backend_prepare_stack_layout_test.cpp` now injects volatile
  and non-default address-space facts through direct BIR fixtures and asserts
  typed prepared fields rather than rendered text.
- Coverage includes direct frame-slot, global-symbol, string-constant,
  local-instruction symbol-backed, and pointer-indirect prepared accesses.
- The manual `PreparedMemoryAccess` designated-initializer contract fixture was
  updated to prove default and explicit new-field initialization still compile
  and preserve lookup behavior.

## Suggested Next

Execute Step 3 from `plan.md`: review the new focused preservation coverage
against the source idea, add only any missing representative assertion if the
supervisor wants stricter Step 3 proof, and record the targeted prepared-memory
test result.

## Watchouts

- The carrier now preserves BIR-provided facts, but current LIR/frontend memory
  producers still do not appear to publish volatile or non-default
  address-space facts for ordinary loads/stores.
- Target MIR lowering remains out of scope for this idea; consumers should read
  the typed prepared fields rather than infer from names, dumps, or target-local
  patterns.
- No expectation downgrades or unsupported-test rewrites were made.

## Proof

Command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_stack_layout$'; } > test_after.log 2>&1`

Result: pass.

Regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: pass; before and after both reported 1 passing test and 0 failures.

Supervisor broader validation:
`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: pass; 114 tests run, 114 passed, 12 disabled.

Proof log: `test_after.log`.
