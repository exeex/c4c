Status: Active
Source Idea Path: ideas/open/656_rv64_20140828_callee_result_frame_slot_runtime.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh 20140828 Runtime Evidence

# Current Packet

## Just Finished

Step 1 refreshed semantic BIR, prepared BIR, ASM, object, disassembly, and
clang/c4c qemu runtime evidence for
`tests/c/external/gcc_torture/src/20140828-1.c` under
`build/agent_state/656_step1_20140828_runtime_evidence/`.

Evidence summary:
`build/agent_state/656_step1_20140828_runtime_evidence/summary.md`.

The `%t6` pointer-source publication remains present in semantic BIR as
`%t6 = bir.add ptr %lv.a.0, 2`; prepared branch RHS authority remains
available for `%t6` with `pointer_status=proven`; and RV64 object
materialization still emits `addi t4,sp,6` for the first branch RHS.

The first downstream wrong-value boundary is before the abort at the callee
result path for `f`: object code stores the integer `*d` payload into `f`'s
slot `0(sp)` and then reloads that same slot as the returned pointer, so
`main` receives a bad `%t4`/`a0` value and aborts at the first
`f(a, 1, &d) != &a[1]` branch before reaching the `d != 1` reload check.

## Suggested Next

Execute Step 2 of `plan.md`: add focused contract coverage that proves the
callee result/frame-slot owner without using the GCC torture row as the only
proof surface. The next packet should distinguish return propagation for
`%t10`/`a0` from the adjacent indirect `*d` store writeback route.

## Watchouts

- Preserve the `%t6` pointer-source publication and branch RHS authority from
  idea 653.
- Do not special-case `src/20140828-1.c`, `%t6`, `f(a, 1, &d)`, or final
  branch compare shapes.
- Do not change expectations, unsupported markers, allowlists, timeout
  accounting, or runtime policy.
- The generated qemu `-d in_asm,cpu,nochain` attempt returned logging help with
  rc `1`; use ordinary qemu run rc plus BIR/prepared/disassembly evidence from
  this packet, not `c4c-qemu-trace.*`, for the classification.
- The first observed runtime failure is the first branch abort, so the caller
  `d` reload path is adjacent risk rather than the first proven boundary.

## Proof

Ran exactly:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: build was up to date; CTest exited `8` with the known red backend
subset, `333 passed, 32 failed, 365 total`. Proof log path: `test_after.log`.
