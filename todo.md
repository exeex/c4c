Status: Active
Source Idea Path: ideas/open/395_rv64_object_route_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reclassify Current Instruction-Fragment Residuals

# Current Packet

## Just Finished

Ran Step 1 reclassification for `src/divmod-1.c` and
`src/20000223-1.c`.

Representative results:

- `src/divmod-1.c`: still fails `RV64_C4C_OBJ_COMPILE_FAIL` with
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64
  object lowering`.
- `src/20000223-1.c`: passes the representative RV64 GCC torture backend
  probe.

Prepared-fact classification:

- `divmod-1.c` reaches prepared BIR with expected arithmetic candidates:
  `bir.sext` into `bir.sdiv` for `div1` through `div4`, `bir.sext` into
  `bir.srem` for `mod1` through `mod4`, plus `bir.srem i64` for `mod5` and
  `bir.urem i64` for `mod6`.
- The first concrete blocking facts are not yet a reusable 395 arithmetic
  lowering family. The same-module `i16` calls to `div2`, `div4`, `mod2`, and
  `mod4` still publish frame-slot arguments with `value_bank=gpr` but
  `dest_bank=none`, and their move bundles still use
  `reason=call_arg_stack_to_stack`.
- The old no-bank same-module `i16` argument shape is therefore still visible
  in the fresh prepared dump. This belongs to the producer-side call-plan/ABI
  metadata boundary, not RV64 object-emission inference.
- `20000223-1.c` prepared facts include `bir.and i32 %p.align, %t0` and fixed
  arity calls to `check`/`abort`; all call arguments publish GPR destinations
  and the representative passes, so it does not share the current `divmod-1.c`
  blocker.

## Suggested Next

Do not start a 395 object-emission opcode implementation from this proof.
Route the next packet back to the producer-side same-module frame-slot `i16`
argument publication boundary: repair the existing call-ABI/register-index
authority so these frame-slot `i16` arguments publish target-consumable
`dest_placement=gpr:call_argument#N/w1` and `dest_bank=gpr` instead of
`dest_bank=none`/`call_arg_stack_to_stack`.

After that producer fact is repaired, rerun this 395 reclassification to find
the next true RV64 instruction-fragment family. Likely candidates in
`divmod-1.c` are signed divide/remainder and unsigned remainder, but this run
does not prove they are the first reachable object-lowering gap.

## Watchouts

- Do not reopen 407 unless fresh prepared dumps show the old no-bank
  same-module `i16` call-argument shape again.
- Do not infer scalar call-argument ABI policy inside RV64 object emission.
- Route producer-fact gaps to their owners instead of patching them under 395.

## Proof

Command:

```sh
cmake --build --preset default && mkdir -p build/agent_state/395_reclassify_step1_dumps && printf '%s\n' 'src/divmod-1.c' 'src/20000223-1.c' > build/agent_state/395_reclassify_step1.allowlist.txt && { ALLOWLIST=build/agent_state/395_reclassify_step1.allowlist.txt STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/395_reclassify_step1_probe.log 2>&1 || true; } && build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/divmod-1.c > build/agent_state/395_reclassify_step1_dumps/divmod-1.prepared.txt 2> build/agent_state/395_reclassify_step1_dumps/divmod-1.prepared.err && build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000223-1.c > build/agent_state/395_reclassify_step1_dumps/20000223-1.prepared.txt 2> build/agent_state/395_reclassify_step1_dumps/20000223-1.prepared.err && rg -n 'unsupported_instruction_fragment|unsupported_stack_frame|unsupported_local_memory_access|unsupported_global_data|RV64_BACKEND_RUNTIME_MISMATCH|unsupported RV64 object lowering|\[rv64-gcc-torture\]|pass\t|fail\t|RV64_C4C_OBJ|error:|prepared.func|bir\.(call|sext|zext|trunc|mul|sdiv|udiv|srem|urem|shl|ashr|lshr|and|or|xor|cmp|select)|arg index=.*value_bank|dest_bank=none|missing_frame_slot_arg_publication' build/agent_state/395_reclassify_step1_probe.log build/agent_state/395_reclassify_step1_dumps/*.prepared.txt build/agent_state/395_reclassify_step1_dumps/*.prepared.err && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Results:

- Build completed.
- Probe summary: `total=2 passed=1 failed=1`.
- `src/divmod-1.c` failed with `RV64_C4C_OBJ_COMPILE_FAIL` /
  `unsupported_instruction_fragment`.
- `src/20000223-1.c` passed.
- Prepared dumps were written under
  `build/agent_state/395_reclassify_step1_dumps/`; both `.prepared.err` files
  are empty.
- Backend CTest completed in `test_after.log`: `100% tests passed, 0 tests
  failed out of 326`.
