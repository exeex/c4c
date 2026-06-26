Status: Active
Source Idea Path: ideas/open/388_rv64_object_route_variadic_va_end_boundary.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Rerun `va-arg-13.c` And Route Next Boundary

# Current Packet

## Just Finished

Step 5 reran `tests/c/external/gcc_torture/src/va-arg-13.c` through the RV64
gcc-torture backend object runner after `llvm.va_end.p0` no-op lowering.

Result:

- The representative now gets past the previous unresolved `llvm.va_end.p0`
  link boundary. `build/agent_state/388_step5_va-arg-13.readelf.log` contains
  no `llvm.va_end.p0` symbol or relocation.
- The case still does not pass: the runner reports
  `[RV64_BACKEND_RUNTIME_MISMATCH]` with `clang_exit=0` and
  `c4c_exit=Segmentation fault`.
- The produced object links and runs far enough to segfault at runtime; this is
  a later boundary than idea 388's `va_end` link failure.

Later-boundary evidence:

- `build/agent_state/388_step5_va-arg-13.objdump.log` shows the first
  `va_start` materialization in `test` stores the overflow-area pointer with
  `sd t1,0(s1)` at `.Ltest_entry+0x14` / object offset `0x7c`.
- The same store shape appears for the second `va_start` at offset `0xac`.
- `s1` is saved in the prologue but no preceding instruction in `test`
  materializes the destination va_list address into `s1` before the store.
- Step 1 prepared evidence said the `va_start` helper operand had
  `dst_va_list_addr=%lv.state.8:register:reg=s1`; the runtime code is now
  exposing that destination-address publication/materialization gap.

Likely owner:

- This is not a `va_end` problem anymore. Idea 388's selected boundary is
  resolved enough for the representative to advance from link failure to a
  runtime boundary.
- The likely next owner is a new lifecycle idea for RV64 `va_start`
  destination va_list address materialization/publication: prepared
  `dst_va_list_addr` in a GPR home must be proven/materialized before
  `fragment_for_prepared_variadic_va_start` stores through it.
- No matching `ideas/open/` owner was found by a narrow scan for `va_start`,
  `destination address`, `overflow-area`, and `va_list`, so supervisor should
  route this to plan-owner as a new follow-up idea or close idea 388 and then
  activate that new boundary.

## Suggested Next

Ask plan-owner to decide lifecycle handling: close/complete idea 388 as the
`llvm.va_end.p0` boundary is resolved, and create or activate a follow-up idea
for RV64 `va_start` destination va_list address materialization/publication.

## Watchouts

- Keep idea 386 closed; do not reopen frame-slot-address call argument
  lowering under this plan.
- Keep idea 387 sret work out of scope.
- Do not hide unresolved `llvm.va_end.p0` with fake symbols or linker
  workarounds.
- The current prepared variadic entry plan does not publish `va_end` helper
  operands, so a no-op route must either be justified by exact callee/argument
  facts on the direct extern call or add preparation support for a real
  `va_end` helper classification.
- Do not generalize ordinary `direct_extern_fixed_arity` intrinsic calls unless
  the route is guarded to `llvm.va_end.p0` and the one-argument va_list shape.
- Unsupported variants that remain out of scope: `va_copy`, scalar `va_arg`,
  aggregate `va_arg`, changes to `va_start`, ordinary extern calls, indirect
  calls, direct extern variadic calls, memory-return calls, outgoing stack
  argument calls, malformed `llvm.va_end.p0` calls with missing or multiple
  arguments, mismatched prepared call-plan callee names, and any fake symbol or
  linker allowlist workaround.
- Do not repair the newly exposed `va_start` runtime segfault under idea 388
  without an explicit lifecycle decision. That is a distinct route from the
  `llvm.va_end.p0` object/link boundary.

## Proof

Ran the supervisor-selected Step 5 representative command:

```bash
mkdir -p build/agent_state build/rv64_gcc_c_torture_backend/src_va-arg-13.c && { echo 'Step 5 va-arg-13 RV64 object runner for idea 388'; cmake --build --preset default --target c4cll >/dev/null; case_log=build/agent_state/388_step5_va-arg-13.case.log; set +e; cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=$(command -v clang) -DQEMU_RISCV64=$(command -v qemu-riscv64) -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/va-arg-13.c -DROOT=/workspaces/c4c -DOUT_CLANG_BIN=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-13.c/clang.bin -DOUT_OBJECT=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-13.c/c4c.o -DOUT_C4C_BIN=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-13.c/c4c.bin -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake > "$case_log" 2>&1; rc=$?; set -e; echo "case_exit=$rc"; cat "$case_log"; exit 0; } > test_after.log 2>&1
```

Command result: completed; representative case result was `case_exit=1`
because of runtime mismatch.

Proof log: `test_after.log`

Evidence logs:

- `build/agent_state/388_step5_va-arg-13.case.log`
- `build/agent_state/388_step5_va-arg-13.objdump.log`
- `build/agent_state/388_step5_va-arg-13.readelf.log`
