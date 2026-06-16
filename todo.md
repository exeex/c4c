Status: Active
Source Idea Path: ideas/open/296_rv64_runtime_scalar_local_foundation.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Try One Tiny Local-Slot Case

# Current Packet

## Just Finished

Step 4 from `plan.md` completed: `local_temp.c` is registered in the rv64 qemu
runtime subset with expected exit code 5, and the rv64 prepared-BIR emitter now
supports the narrow same-width i32 frame-slot path needed by that case.

`backend_rv64_runtime_local_temp` emits RISC-V assembly, assembles and links
with clang, and passes under qemu without accepting fallback BIR or LLVM text.

## Suggested Next

Execute Step 5 from `plan.md`: run the focused backend validation requested by
the supervisor, preserving the known `backend_riscv_prepared_edge_publication`
baseline caveat unless that baseline is separately repaired.

## Watchouts

- Keep qemu runtime execution as the acceptance boundary.
- Do not accept BIR or LLVM fallback text as RISC-V assembly.
- Do not add testcase-name or exact-source-shape shortcuts.
- `EXPECTED_RUN_CODE` is a generic harness contract for qemu process status; it
  should not be used to hide wrong semantics.
- The local-slot path is intentionally narrow: non-volatile same-width i32
  frame-slot load/store using prepared memory-access and frame-plan facts.
- Do not expand this slice into arrays, aggregates, globals, pointer
  provenance, dynamic stack, or calls plus local frames without a separate
  packet.
- The supervisor-noted baseline failure
  `backend_riscv_prepared_edge_publication` still appears in the delegated
  proof subset.

## Proof

Passed for this packet with the expected widened-subset baseline failure:
`bash -o pipefail -c 'cmake --build --preset default && ctest --test-dir build -R '\''^backend_rv64_runtime|backend_riscv|backend_codegen_route_riscv64'\'' --output-on-failure' 2>&1 | tee test_after.log`

All 14 `backend_rv64_runtime_*` cases passed, including
`backend_rv64_runtime_local_temp`. The only failing test in the delegated
widened subset was the known pre-existing
`backend_riscv_prepared_edge_publication` failure. Proof log:
`test_after.log`.
