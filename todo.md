Status: Active
Source Idea Path: ideas/open/609_rv64_global_data_consumer.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Emit prepared relocation records for object data

# Current Packet

## Just Finished

- Finished Step 3 (`Emit prepared relocation records for object data`) by
  updating stale RV64 object-emission tests for prepared relocation-bearing
  object data.
- `backend_riscv_object_emission` now asserts that relocation-only prepared
  pointer object data reserves writable `.data`, defines the prepared object
  symbol, references the prepared target symbol, and emits exactly one matching
  `.data` `R_RISCV_64` relocation at prepared offset 0.
- The mixed bytes-plus-relocation case now asserts the prepared `.data` bytes,
  prepared object symbol, prepared target symbol, and exactly one matching
  `.data` `R_RISCV_64` relocation at prepared offset 8.
- Invalid and missing relocation-fact diagnostics remain covered by the
  existing fail-closed tests.

## Suggested Next

Address the remaining global-object publication/linkage gap exposed by
`src/20010924-1.c`: the first post-relocation blocker is undefined `a4` at link
time after object generation succeeds.

## Watchouts

- Do not produce missing prepared/global authority in RV64.
- Do not infer object bytes, relocation slots, target identity, or access
  widths inside RV64 when prepared facts are absent.
- Keep expectation, unsupported-marker, allowlist, timeout, runtime/link, and
  accounting changes out of the proof.
- Duplicate same-label object-data rows can appear with different extents; the
  RV64 consumer may supersede earlier same-label object symbols only when the
  existing symbol is an object with compatible binding. Non-object or binding
  conflicts remain fail-closed.
- The prepared direct-call fixture also emits text relocations; tests should
  continue to assert the exact prepared `.data` relocation record without
  requiring the whole module relocation list to contain only data relocations.

## Proof

- Delegated Step 3 proof was run:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
- Result: build passed; CTest ran 346 backend tests, 0 failed.
- Proof log path: `test_after.log`.
