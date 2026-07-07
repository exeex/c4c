Status: Active
Source Idea Path: ideas/open/583_rv64_pointer_arithmetic_result_publication.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Representative Route Proof

# Current Packet

## Just Finished

Step 4 reran the retained `src/20000819-1.c` representative after the Step 3
pointer-result publication repair.

The prepared dump still shows the old pointer arithmetic shape in `foo`:
`entry` instruction 7 is `%t4 = bir.add ptr %t1, %t4.byte_offset`, with
`%t4` home `value_id=7` in register `t0` and store-local consumer at `entry`
instruction 8. The RV64 object route no longer reports the old
`unsupported_pointer_arithmetic` compile-time owner for
`function=foo; block=entry; instruction_index=7; owner=ptr %t4`.

The route now builds the C4C object and linked binary, reaches the runtime
comparison, and fails as `[RV64_BACKEND_RUNTIME_MISMATCH]`: clang exits 0 while
the C4C binary aborts. This is a distinct downstream runtime behavior, not the
same pointer-arithmetic owner.

## Suggested Next

Execute Step 5 from `plan.md`: run the supervisor-selected backend closure
proof, normally the focused RV64 object-emission test plus the broader
`^backend_` subset, then decide whether the source idea is ready for
plan-owner closure evaluation or whether the runtime abort belongs to a
separate follow-up idea.

## Watchouts

- Do not select or mutate deferred `ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md`.
- The focused fixture is semantic: do not repair it with filename, function,
  block, value-name, or diagnostic-string shortcuts.
- Preserve fail-closed behavior for pointer arithmetic forms outside prepared
  pointer base plus integer byte-offset add/sub with a prepared destination
  home.
- The Step 3 object-fragment repair intentionally stays inside prepared facts:
  unsupported operand type combinations, missing homes, and unprepared offsets
  remain rejected by the pointer-arithmetic diagnostic path.
- Step 4 advanced past the Step 1 owner coordinates for `20000819-1.c`:
  `function=foo`, `block=entry`, `instruction_index=7`, `owner=ptr %t4`.
- The new failure is runtime-only: `clang_exit=0`, `c4c_exit=Subprocess
  aborted`. No downstream compile-time diagnostic owner was reported by the
  object route.

## Proof

`test_after.log` records the delegated Step 4 proof summary.

Artifacts:

- `build/agent_state/583_rv64_pointer_arithmetic_result_publication/step4/src_20000819-1.c/build-c4cll.*`
- `build/agent_state/583_rv64_pointer_arithmetic_result_publication/step4/src_20000819-1.c/dump-prepared-bir.*`
- `build/agent_state/583_rv64_pointer_arithmetic_result_publication/step4/src_20000819-1.c/object-route.*`

Commands:

- `cmake --build --preset default --target c4cll`
- `build/c4cll -I /workspaces/c4c --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000819-1.c`
- `cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=/usr/bin/clang -DQEMU_RISCV64=/usr/bin/qemu-riscv64 -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/20000819-1.c -DROOT=/workspaces/c4c/tests/c/external/gcc_torture -DTARGET_TRIPLE=riscv64-linux-gnu -DSYSROOT=/usr/riscv64-linux-gnu -DOUT_CLANG_BIN=/workspaces/c4c/build/agent_state/583_rv64_pointer_arithmetic_result_publication/step4/src_20000819-1.c/clang.bin -DOUT_OBJECT=/workspaces/c4c/build/agent_state/583_rv64_pointer_arithmetic_result_publication/step4/src_20000819-1.c/c4c.o -DOUT_C4C_BIN=/workspaces/c4c/build/agent_state/583_rv64_pointer_arithmetic_result_publication/step4/src_20000819-1.c/c4c.bin -DCASE_TIMEOUT_SEC=20 -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake`

Result: build passed, prepared dump passed, object route returned 1 after
advancing to `[RV64_BACKEND_RUNTIME_MISMATCH]` with `clang_exit=0` and
`c4c_exit=Subprocess aborted`.
