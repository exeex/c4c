Status: Active
Source Idea Path: ideas/open/296_rv64_runtime_scalar_local_foundation.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Generalize Binary-Op Value Tracking

# Current Packet

## Just Finished

Step 3 from `plan.md` completed: `return_add_sub_chain.c` is registered in the
rv64 qemu runtime subset with expected exit code 4, and the rv64 emitter now
tracks scalar add/sub chains through prepared rematerializable immediate homes
as well as named register homes.

`backend_rv64_runtime_return_add_sub_chain` emits RISC-V assembly and passes
under qemu without accepting fallback BIR or LLVM text.

## Suggested Next

Execute Step 4 from `plan.md`: try the smallest local-slot runtime case
(`local_temp.c` first unless it proves too broad), then implement only the
minimal rv64 stack/local load-store path needed for that selected case.

## Watchouts

- Keep qemu runtime execution as the acceptance boundary.
- Do not accept BIR or LLVM fallback text as RISC-V assembly.
- Do not add testcase-name or exact-source-shape shortcuts.
- `EXPECTED_RUN_CODE` is a generic harness contract for qemu process status; it
  should not be used to hide wrong semantics.
- The Step 3 emitter path intentionally uses prepared value-home facts for
  rematerializable i32 immediates; do not replace that with source-shape
  matching for arithmetic chains.
- The supervisor-noted baseline failure
  `backend_riscv_prepared_edge_publication` still appears in the delegated
  proof subset.

## Proof

Passed for this packet with the expected widened-subset baseline failure:
`bash -o pipefail -c 'cmake --build --preset default && ctest --test-dir build -R '\''^backend_rv64_runtime|backend_riscv|backend_codegen_route_riscv64'\'' --output-on-failure' 2>&1 | tee test_after.log`

All 13 `backend_rv64_runtime_*` cases passed, including
`backend_rv64_runtime_return_add_sub_chain`. The only failing test in the delegated
widened subset was the known pre-existing
`backend_riscv_prepared_edge_publication` failure. Proof log:
`test_after.log`.
