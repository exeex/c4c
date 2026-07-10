Status: Active
Source Idea Path: ideas/open/676_rv64_pointer_global_local_publication_runtime_contract.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Establish Runtime/Semantic Proof Harness

# Current Packet

## Just Finished

Step 1 established a repo-native RV64 semantic proof path for
`backend_cli_failure_riscv64_pointer_global_local_publication_live_load_rejection`.
`build/tests/backend/riscv64_pointer_global_local_publication_live_load_rejection.o`
links with `clang --target=riscv64-linux-gnu --gcc-toolchain=/usr` and runs
under `qemu-riscv64 -L /usr/riscv64-linux-gnu`, returning `0`, the expected
zero-initialized global short value. Evidence and command details are in
`build/agent_state/676_step1_runtime_contract/summary.md`.

## Suggested Next

Update the stale expected-failure contract for the now-emitting live-load row,
or delegate the next plan step if the supervisor wants a separate expectation
policy packet.

## Watchouts

- Do not accept `test_baseline.new.log` while it contains candidate-only
  failures.
- Compare by stable test name, not numeric row id.
- Do not reopen closed ideas unless fresh evidence contradicts their closure
  notes.
- Do not change expectations, unsupported markers, allowlists, timeouts,
  runtime policy, or baseline accounting.
- The focused CTest remains red because the expected-failure wrapper reports
  `[BACKEND_OBJ_EXPECTED_FAIL] ... unexpectedly succeeded`; the runtime harness
  itself is not blocked.
- Host `objdump -dr` cannot disassemble this RISC-V object here
  (`architecture UNKNOWN`); use `riscv64-linux-gnu-objdump -dr` for readable
  object evidence.
- The call-arg row is split to
  `ideas/open/677_rv64_call_arg_local_frame_address_object_materialization.md`;
  do not bundle it into this packet.

## Proof

Ran the delegated proof into `test_after.log`:
`cmake --build --preset default && { ctest --test-dir build -j --output-on-failure -R 'backend_cli_failure_riscv64_pointer_global_local_publication_live_load_rejection'; <runtime/link/readelf/objdump probes>; }`.
Build succeeded. The focused CTest failed as expected for a stale
expected-failure row after object emission started succeeding. The semantic
probe linked and ran
`build/tests/backend/riscv64_pointer_global_local_publication_live_load_rejection.o`
with qemu exit `0`; known-good publication comparison returned `7`.
