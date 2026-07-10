Status: Active
Source Idea Path: ideas/open/657_rv64_loop_2e_indirect_store_writeback_runtime.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh Loop-2e Runtime Boundary Evidence

# Current Packet

## Just Finished

Step 1 refreshed loop-2e evidence for
`tests/c/external/gcc_torture/src/loop-2e.c` under
`build/agent_state/657_step1_loop_2e_runtime_boundary/`.

Artifacts include semantic BIR (`loop-2e.bir.txt`), prepared BIR
(`loop-2e.prepared.txt`), RV64 asm (`loop-2e.s`), object
(`loop-2e.o`), disassembly (`loop-2e.objdump.txt`), clang runtime logs,
c4c RV64 runtime compare logs, and `summary.md`.

Preserved idea-653 facts:

- Semantic and prepared BIR still publish `%t23 = bir.add ptr %t21, 156`.
- Prepared branch RHS authority for `%t23` remains
  `pointer_status=proven`, `status=available`, and
  `source_freshness_status=selected`.
- RV64 object emission succeeds.

Runtime/classification:

- Clang RV64 runtime exits `0`.
- c4c RV64 object-runtime compare fails with
  `[RV64_BACKEND_RUNTIME_MISMATCH]`, `clang_exit=0`, and
  `c4c_exit=Subprocess aborted`.
- The first wrong-value boundary is callee `f` block `block_1`: semantic and
  prepared facts distinguish postincrement local writeback
  `bir.store_local %lv.param.q, ptr %t10` from the caller-visible indirect
  store `bir.store_local %t9.store.addr, ptr %t8, addr %t9`, and prepared
  addressing records `base=pointer_value stored=%t8 pointer=%t9`.
- RV64 object code computes both `q + 8` and `&p[i]`, but stores both values
  to `0(sp)`, the local `%lv.param.q` stack home, instead of storing `&p[i]`
  through the old pointer value `%t9`. `main` later reloads unchanged
  `q[39]` from `312(sp)` and aborts when it differs from the correctly
  materialized `%t23`.

## Suggested Next

Execute Step 2: trace the callee `f` destination and updated-pointer facts
from semantic BIR through prepared value homes and RV64 lowering inputs,
distinguishing the local `%lv.param.q` postincrement writeback from the
caller-visible pointer-value store through `%t9`.

## Watchouts

- Do not reopen the completed idea 653 `%t23` source publication route unless
  fresh evidence proves a regression.
- Step 1 shows object/runtime failure after successful RV64 object emission;
  the live owner is indirect store/writeback lowering in callee `f`, not the
  final `%t23` branch RHS path.
- Do not implement stack-destination fan-in authority from ideas 647/655 under
  this plan.
- Do not special-case `loop-2e.c`, `%t23`, `q[39]`, or callee `f`.
- Do not change expectations, unsupported markers, allowlists, timeouts, or
  pass/fail accounting.

## Proof

Focused evidence generation completed through BIR, prepared-BIR, ASM, object,
disassembly, clang runtime, and c4c RV64 runtime comparison.

Delegated proof run exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: CTest exited `8`; `test_after.log` reports `334 passed, 32 failed,
366 total`.
