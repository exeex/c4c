Status: Active
Source Idea Path: ideas/open/557_bir_local_memory_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 10
Current Step Title: Inspect Load Local-Memory Producer Boundary

# Current Packet

## Just Finished

Step 10 - Inspect Load Local-Memory Producer Boundary completed as an
inspection-only packet for `src/20000314-1.c`.

Exact load-family boundary:

- The RV64 case log still reports semantic `lir_to_bir` failure in `main` at
  `load local-memory`.
- LLVM-path output for `tests/c/external/gcc_torture/src/20000314-1.c` shows
  ordinary local scalar loads from `%lv.winds` lowering first:
  `load i64, ptr %lv.winds`.
- The first load-family boundary is the later integer-as-pointer path:
  `%t6 = load i64, ptr %lv.winds`, `%t7 = inttoptr i64 %t6 to ptr`,
  `%t8 = load i8, ptr %t7` in `block_2`, with the same shape repeated as
  `%t31`/`%t32`/`%t33` in `logic.rhs.27`.
- `lower_memory_load_inst` can lower the direct non-pointer `%lv.winds` loads
  through `try_lower_local_slot_load` / `try_lower_nonpointer_local_slot_load`.
  The subsequent `inttoptr` path in `lower_scalar_or_local_memory_inst` only
  republishes pointer facts when the integer operand is already in
  `pointer_address_ints`, `global_object_address_ints`, or
  `global_address_ints`. It does not publish an opaque/runtime
  `PointerAddress` for an arbitrary loaded local `i64` scalar.
- Therefore the later `load i8, ptr %t7` reaches
  `try_lower_pointer_provenance_load` without a `pointer_value_addresses_`
  entry for `%t7`, is not a local-slot load, and fails semantic load admission.

This is a producer fact gap, not an RV64/MIR route issue: BIR needs to publish
or admit an opaque runtime pointer-value address for `inttoptr i64 <loaded local
scalar>` before the byte load can emit a `LoadLocalInst` with
`MemoryAddress::BaseKind::PointerValue`, requested range `[0, 1)`, and opaque
layout authority.

## Suggested Next

Step 11 - Repair Load Local-Memory Admission.

Next executor packet:

- Add focused BIR coverage in `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`.
- Suggested test name:
  `expect_inttoptr_loaded_local_i64_byte_load_publishes_opaque_pointer_base`.
- Test shape: local `i64` alloca/store/load, `inttoptr i64 %loaded to ptr`,
  then `load i8, ptr %cast.ptr`. The expected passing contract should verify
  the byte load lowers to `LoadLocalInst` with a `PointerValue` address based
  on the `inttoptr` result, byte offset `0`, size `1`, requested range `[0, 1)`,
  and opaque/unknown-compatible provenance. It should also ensure the repair
  does not collapse the dynamic pointer load to `%lv.winds` direct local-slot
  addressing.
- Implement the minimal producer-side publication in the LIR-to-BIR
  scalar/cast plus load-provenance path. Keep existing known-address
  `ptrtoint`/`inttoptr` recovery behavior intact and avoid target-specific or
  testcase-name logic.

## Watchouts

Reject target exclusions, testcase/helper-name shaped rules, expectation
rewrites, unsupported-marker changes, allowlist changes, runtime-comparison
changes, and RV64/MIR inference.

The `src/20001026-1.c` row is no longer a store local-memory semantic admission
failure. Its current failure is downstream object-route support, so do not keep
classifying that row as an unchanged BIR producer gap.

Step 10 is inspection-first. Do not implement a load repair until the missing
producer fact and focused BIR coverage gap are named in `todo.md`.

Existing deliberate fail-closed coverage around casted byte-pointer opaque
`i32` access should not be weakened blindly. This representative needs an
`i8` load from an integer-as-pointer value; if the implementation generalizes
opaque `inttoptr` loads beyond byte access, the next packet should justify the
typed-access provenance contract explicitly.

No expectation, unsupported-marker, allowlist, runtime comparison, or semantic
admission weakening was performed in this packet.

## Proof

Inspection packet only; no build or CTest run was required and `test_after.log`
was preserved.

Commands run:

- `sed -n '1,240p' build/rv64_gcc_c_torture_backend/src_20000314-1.c/case.log`
- `./build/c4cll --dump-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000314-1.c`
- `./build/c4cll --codegen llvm tests/c/external/gcc_torture/src/20000314-1.c`
- focused reads under `src/backend/bir/lir_to_bir/memory/` and
  `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`

The `--dump-bir` probe reproduced the semantic load local-memory failure before
prepared handoff. The LLVM-path output exposed the failing `inttoptr` plus byte
load shape.

Suggested RV64 representative proof command after repair:

`cmake --build --preset default && ALLOWLIST=build/agent_state/557_step12_20000314.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh`

Inspected case logs:
- `build/rv64_gcc_c_torture_backend/src_20000314-1.c/case.log`
