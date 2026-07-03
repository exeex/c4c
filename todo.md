Status: Active
Source Idea Path: ideas/open/578_rv64_20000622_1_foo_logical_select_runtime_abort.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair The General Publication Rule

# Current Packet

## Just Finished

- Step 4 Repair The General Publication Rule completed the RV64 object repair
  for select-materialization predecessor publications whose source is a binary
  compare result.
- The object path now routes out-of-SSA `phi_join_register_to_register` moves
  through select-edge binary source materialization before falling back to raw
  register copies. This covers block-entry publication shapes where no indexed
  edge-publication record exists, provided the prepared parallel-copy move is
  a select-materialization carrier and the binary source has only the select
  carrier consumer.
- The existing edge-publication path now uses the recovered publication record
  even when the generic move intent cannot render the source as a raw move,
  while still failing closed for unsupported operands, consumers, destinations,
  or malformed join-transfer metadata.
- Added focused NE coverage for both carrier-alias-authorized and
  only-carrier-use shapes. Both require the predecessor edge to materialize
  `ne i64 %fallback, 1` into the select carrier instead of copying the raw
  operand register.
- Removed the temporary `C4C_DEBUG_SELECT_FALLBACK` instrumentation before the
  final proof.

## Suggested Next

- Supervisor should review the Step 4 diff and decide whether this active plan
  needs broader RV64 object-route validation or lifecycle closure/deactivation.

## Watchouts

- The representative now passes and its refreshed object code materializes the
  two logical-select predecessor compares with `xor` plus `snez` before
  publishing the select carriers.
- The repair is metadata-shaped, not testcase-shaped: it does not mention the
  representative filename, function name, prepared value names, or offsets.
- The first preservation move can still copy the raw operand into the source
  home; the repaired semantic is that the select carrier destination receives
  the binary compare result, not the raw source home.

## Proof

- Proof command:
  `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'; } > test_after.log 2>&1`
- Result: green. Build completed and `backend_riscv_object_emission` passed.
- Proof log: `test_after.log`.
- Representative command:
  `cmake -DCOMPILER="$PWD/build/c4cll" -DCLANG="$(command -v clang)" -DQEMU_RISCV64="$(command -v qemu-riscv64)" -DSRC="$PWD/tests/c/external/gcc_torture/src/20000622-1.c" -DROOT="$PWD/tests/c/external/gcc_torture" -DTARGET_TRIPLE="riscv64-linux-gnu" -DSYSROOT="/usr/riscv64-linux-gnu" -DOUT_CLANG_BIN="$PWD/build/agent_state/578_rv64_20000622_1_foo_logical_select_runtime_abort/step4/clang.bin" -DOUT_OBJECT="$PWD/build/agent_state/578_rv64_20000622_1_foo_logical_select_runtime_abort/step4/c4c.o" -DOUT_C4C_BIN="$PWD/build/agent_state/578_rv64_20000622_1_foo_logical_select_runtime_abort/step4/c4c.bin" -DCASE_TIMEOUT_SEC=20 -P tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake > build/agent_state/578_rv64_20000622_1_foo_logical_select_runtime_abort/step4/representative.log 2>&1`
- Representative result: green. Log:
  `build/agent_state/578_rv64_20000622_1_foo_logical_select_runtime_abort/step4/representative.log`.
