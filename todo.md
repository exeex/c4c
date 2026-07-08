Status: Active
Source Idea Path: ideas/open/609_rv64_global_data_consumer.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Lower supported global access widths

# Current Packet

## Just Finished

- Finished Step 4 (`Lower supported global access widths`) by adding focused
  RV64 object-emission coverage for prepared global scalar load/store widths.
- The current checkout has no
  `src/backend/mir/riscv/codegen/global_access.cpp`; the matching implementation
  is `src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp`, which
  already lowers the supported prepared widths without a code change.
- `backend_riscv_object_emission` now asserts byte, halfword, word,
  doubleword, and pointer-width prepared global loads and stores from explicit
  prepared global-symbol memory facts, checking the emitted RV64 memory opcode
  `funct3` and the prepared object symbol width.
- Pointer-width fixtures use explicit selected symbol-pointer object-data
  authority so the test does not infer pointer storage or relocation targets in
  RV64.

## Suggested Next

Proceed to Step 5 consumer handoff/residual ownership review using the current
RV64 global-data allowlist and the now-covered prepared global access-width
consumer boundary.

## Watchouts

- Do not produce missing prepared/global authority in RV64.
- Do not infer object bytes, relocation slots, target identity, access widths,
  or pointer object-data inside RV64 when prepared facts are absent.
- Keep expectation, unsupported-marker, allowlist, timeout, runtime/link, and
  accounting changes out of the proof.
- The Step 4 packet proved backend unit coverage only; Step 5 should still
  record which gcc-torture rows moved, which rows remain producer-authority
  gaps, and whether route review is needed before closure.

## Proof

- Delegated Step 4 proof was run:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
- Result: build passed; CTest ran 346 backend tests, 0 failed.
- Proof log path: `test_after.log`.
