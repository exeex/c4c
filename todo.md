Status: Active
Source Idea Path: ideas/open/391_rv64_variadic_prologue_save_area_publication.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Rerun `va-arg-13.c` And Route The Next Boundary

# Current Packet

## Just Finished

Step 5: Rerun `va-arg-13.c` And Route The Next Boundary is complete.

Reran the RV64 gcc-torture object-route representative after Step 4. The
runtime result is still:

```text
[RV64_BACKEND_RUNTIME_MISMATCH]
clang_exit=0 c4c_exit=Subprocess aborted
```

The save-area publication boundary from idea 391 appears to have advanced in
the generated object: `test` now stores incoming variadic GPR payloads into the
prepared backing area (`sd a1, 0x48(sp)` through `sd a7, 0x78(sp)`), so the
incoming `1234` payload is present at the expected first post-named GPR save
slot.

The remaining abort is a later `va_list` call-argument/value boundary. Before
the first `dummy` call, c4c initializes the `va_start` slot at `sp+0x80` with
the backing save-area pointer `sp+0x48`, but then builds the `dummy` argument
object with the `va_list` slot address (`sp+0x80`) rather than the initialized
payload (`sp+0x48`). In `dummy`, `va_arg` therefore loads from the pointer slot
object instead of the saved variadic payload and reaches `abort()`.

## Suggested Next

Supervisor/plan-owner should route the later boundary separately from idea
391: prepared `va_list` expression/call-argument value publication still needs
to copy the initialized save-area pointer payload into the `dummy` parameter
object for this path. This may be a repair or follow-up to the prepared-call
`va_list` publication route, but it is not additional RV64 variadic prologue
save-area publication work.

## Watchouts

- Idea 391's save-area publication itself is visible in the c4c disassembly,
  but the representative is not completion-proven because the process still
  aborts.
- The next boundary should not be solved by hard-coding `va-arg-13.c`,
  `dummy`, offset `0x80`, or offset `0x48`; the evidence points to general
  `va_list` value publication for call-argument objects.
- The `qemu-riscv64 -strace` evidence log was collected with `-L
  /usr/riscv64-linux-gnu` and confirms the c4c binary aborts.

## Proof

Delegated proof command:

```sh
mkdir -p build/agent_state build/rv64_gcc_c_torture_backend/src_va-arg-13.c && { echo 'Step 5 va-arg-13 RV64 object runner for idea 391'; cmake --build --preset default --target c4cll >/dev/null; case_log=build/agent_state/391_step5_va-arg-13.case.log; set +e; cmake -DCOMPILER=/workspaces/c4c/build/c4cll -DCLANG=$(command -v clang) -DQEMU_RISCV64=$(command -v qemu-riscv64) -DSRC=/workspaces/c4c/tests/c/external/gcc_torture/src/va-arg-13.c -DROOT=/workspaces/c4c -DOUT_CLANG_BIN=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-13.c/clang.bin -DOUT_OBJECT=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-13.c/c4c.o -DOUT_C4C_BIN=/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-13.c/c4c.bin -P /workspaces/c4c/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake > "$case_log" 2>&1; rc=$?; set -e; echo "case_exit=$rc"; cat "$case_log"; exit 0; } > test_after.log 2>&1
```

Result: runner completed, representative failed comparison with
`case_exit=1`; `test_after.log` preserved the runtime mismatch.

Evidence logs:

- `test_after.log`
- `build/agent_state/391_step5_va-arg-13.case.log`
- `build/agent_state/391_step5_va-arg-13.c4c-disasm.log`
- `build/agent_state/391_step5_va-arg-13.clang-disasm.log`
- `build/agent_state/391_step5_va-arg-13.qemu-strace.log`
