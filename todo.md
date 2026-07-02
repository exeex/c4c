Status: Active
Source Idea Path: ideas/open/557_bir_local_memory_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 13
Current Step Title: Repair The Next Remaining Semantic Family

# Current Packet

## Just Finished

Step 13 - Repair The Next Remaining Semantic Family completed the
scalar/local-memory inspection subpacket for `src/20000519-1.c`.

Boundary found:

- The RV64 case log still reports semantic `scalar/local-memory` failure in
  function `foo`.
- The LLVM/LIR shape in `foo` is `alloca ptr` for `%lv.ap`, `alloca ptr` for
  `%t0`, `llvm.va_start.p0(ptr %lv.ap)`, then
  `llvm.memcpy.p0.p0.i64(ptr %t0, ptr %lv.ap, i64 8, i1 false)` before the
  direct call to `bar(i32 %p.a, ptr %t0)`.
- `lower_local_memory_alloca_inst` publishes both pointer allocas as scalar
  local slots through `local_slot_types_` and `local_pointer_slots_`.
- `lower_runtime_intrinsic_inst` has a direct `LirVaStartOp` lowering path, so
  the first producer boundary is not ordinary scalar arithmetic or the
  `va_start` helper itself.
- `try_lower_immediate_local_memcpy` resolves both `%t0` and `%lv.ap` as
  `LocalMemcpyScalarSlot` values of type `Ptr`, size 8, align 8, but it has no
  scalar-slot-to-scalar-slot copy branch. With a scalar target and a local
  scalar source, it falls through because the pointer-value fallback explicitly
  rejects sources present in `local_pointer_slots`.

Focused BIR test gap to add next:

- Add `expect_local_scalar_pointer_memcpy_copies_between_local_slots` or an
  equivalent focused fixture in `backend_lir_to_bir_notes_test.cpp`.
- The fixture should model two local `ptr` allocas, `LirVaStartOp{ %lv.ap }`,
  `LirMemcpyOp{ dst=%t0, src=%lv.ap, size=8 }`, and a call consuming `%t0`.
- The test should pin semantic BIR lowering of the memcpy as a local scalar
  slot copy, with `LoadLocalInst` from `%lv.ap` and `StoreLocalInst` to `%t0`
  carrying `LocalSlot` `MemoryAddress` provenance/requested range for the
  copied pointer-sized bytes.

## Suggested Next

Recommended next packet: implement the scalar-slot-to-scalar-slot immediate
local memcpy producer path in `try_lower_immediate_local_memcpy`, add the
focused BIR coverage above, and then run backend proof plus the RV64
representative command:

`ALLOWLIST=build/agent_state/557_step13_20000519.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh`

## Watchouts

Reject target exclusions, testcase/helper-name shaped rules, expectation
rewrites, unsupported-marker changes, allowlist changes, runtime-comparison
changes, and RV64/MIR inference.

Do not route this as a variadic runtime helper rewrite unless the next packet
finds different evidence: the local evidence points at immediate local memcpy
between scalar pointer slots after `va_start` has been admitted.

Keep the repair general to local scalar-slot copies. A named `va_list` or
`20000519-1.c` shortcut would overfit the row and miss the producer gap in the
shared memcpy helper.

Keep downstream object-route failures out of this producer packet:
`src/20000314-1.c`, `src/20001026-1.c`, and now `src/20000717-4.c` have moved
off semantic local-memory admission and should not be absorbed back into this
source idea without supervisor/lifecycle direction.

## Proof

Proof log: `test_after.log`.

Inspection-only packet; no build or CTest run was required and `test_after.log`
was preserved.

Inspected case log:
- `build/rv64_gcc_c_torture_backend/src_20000519-1.c/case.log`

Inspection commands:

- `./build/c4cll --codegen llvm --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000519-1.c -o /tmp/20000519-1.ll`
- `./build/c4cll --dump-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000519-1.c`
  confirmed semantic BIR still fails before dumping, with latest function
  failure `foo` in `scalar/local-memory`.
