Status: Active
Source Idea Path: ideas/open/557_bir_local_memory_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 11
Current Step Title: Repair Load Local-Memory Admission

# Current Packet

## Just Finished

Step 11 - Repair Load Local-Memory Admission completed the selected
producer-side load repair for `src/20000314-1.c`.

Implemented facts:

- `lower_memory_load_inst` now records SSA values produced by local `i64`
  scalar loads as integer pointer candidates.
- `lower_scalar_or_local_memory_inst` now handles `inttoptr i64 <loaded local
  integer>` by publishing a producer-owned opaque byte-compatible
  `PointerAddress` for the resulting pointer value.
- The published fact uses the `inttoptr` result as the `PointerValue` memory
  base and keeps byte offset `0`, storage/type text `i8`, `value_type Void`,
  and unknown provenance so byte loads receive requested range `[0, 1)` with
  `OpaqueCompatibility` / `UnknownCompatible` while wider typed opaque access
  remains fail-closed.
- Existing known-address `ptrtoint` / `inttoptr` recovery remains earlier in
  the cast path and was not changed.

Focused BIR coverage added:

- `expect_inttoptr_loaded_local_i64_byte_load_publishes_opaque_pointer_base`
  covers local `i64` store/load, `inttoptr i64`, and a follow-on `i8` load. It
  verifies the direct local integer load remains a local-slot load and the byte
  load uses `MemoryAddress::BaseKind::PointerValue` based on the `inttoptr`
  result with opaque-compatible range metadata.

RV64 representative result:

- `src/20000314-1.c` moved off semantic `load local-memory` admission.
- Current row failure is downstream prepared/object lowering:
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64
  object lowering`.

## Suggested Next

Step 13 - Repair The Next Remaining Semantic Family.

Recommended next packet: inspect and repair the GEP representative
`src/20000717-4.c`, which is still classified as semantic `gep local-memory`.
Keep the downstream `src/20000314-1.c` object-lowering failure out of this
local-memory semantic producer route unless the supervisor opens a separate
route for prepared/object instruction support.

## Watchouts

Reject target exclusions, testcase/helper-name shaped rules, expectation
rewrites, unsupported-marker changes, allowlist changes, runtime-comparison
changes, and RV64/MIR inference.

The `src/20000314-1.c` row is no longer a `load local-memory` semantic
admission failure. Its current failure is downstream object-route support.

The `src/20001026-1.c` row is no longer a store local-memory semantic admission
failure. Its current failure is downstream object-route support, so do not keep
classifying that row as an unchanged BIR producer gap.

Existing deliberate fail-closed coverage around casted byte-pointer opaque
`i32` access was preserved. This packet admits only the byte-compatible
`inttoptr` load fact needed by the selected representative and does not add a
typed opaque local-integer pointer recovery path.

## Proof

Proof log: `test_after.log`.

Commands run:

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`
- Result: passed, `345/345` backend tests.
- Appended RV64 representative command:
  `ALLOWLIST=build/agent_state/557_step12_20000314.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh`
- Result: nonzero with `0/1` passed, but the row moved from semantic
  `load local-memory` admission to downstream `unsupported_instruction_fragment`.

Inspected case log:
- `build/rv64_gcc_c_torture_backend/src_20000314-1.c/case.log`
