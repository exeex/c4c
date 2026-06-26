Status: Active
Source Idea Path: ideas/open/407_prepared_i16_same_module_call_arg_abi_publication.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prove Divmod And Route Residuals

# Current Packet

## Just Finished

Completed Step 3 / Prove Divmod And Route Residuals.

Representative result:

- `src/divmod-1.c` still fails the RV64 GCC torture backend probe with
  `RV64_C4C_OBJ_COMPILE_FAIL`.
- Fresh diagnostic family:
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64
  object lowering`.

407 ownership classification:

- The producer-side same-module frame-slot `i16` call-argument facts remain
  complete in the fresh Step 3 prepared dump.
- `div2`, `div4`, `mod2`, and `mod4` frame-slot `i16` arguments publish
  `value_bank=gpr`, `dest_placement=gpr:call_argument#N/w1`, `dest_reg=aN`,
  `dest_bank=gpr`, and `missing_frame_slot_arg_publication=yes`.
- The old 407 blocker is absent: no `source_encoding=frame_slot ... dest_bank=none`,
  no `call_arg_stack_to_stack`, and no `placement=none:call_argument` matches
  in the Step 3 prepared dump.

Residual owner:

- This no longer belongs to 407. The residual routes back to
  `ideas/open/395_rv64_object_route_instruction_fragment_lowering.md`.
- The prepared module reaching object emission contains arithmetic/cast
  instruction fragments including `bir.sext`, `bir.sdiv`, `bir.srem`,
  `bir.urem`, `bir.shl`, `bir.trunc`, and calls. Step 3 did not implement or
  further isolate the exact first object-lowering opcode; that belongs to the
  395 route.

## Suggested Next

Hand this back to the supervisor/plan owner to close or retire reopened 407
and reactivate the 395 instruction-fragment lowering route. The next executor
packet should be a 395 classification/implementation packet for the first
concrete RV64 object-route instruction-fragment family now that `divmod-1.c`
is past the `i16` call-argument producer blocker.

## Watchouts

- Do not implement 395 RV64 opcode lowering while this producer blocker
  remains.
- Do not infer scalar call-argument ABI policy inside RV64 object emission.
- Do not regress the repaired producer facts: 407 Step 2 deliberately avoided
  a raw argument-index fallback in call-plans and repaired the existing
  target-aware call-ABI/register-index authority instead.
- The common target-register placement helper still has a bankless `I16`
  surface outside this packet's owned scope; the Step 2 producer sites
  normalize the repaired scalar `i16` placement to GPR locally.

## Proof

Command:

```sh
cmake --build --preset default && mkdir -p build/agent_state/407_reopen_step3_dumps && printf '%s\n' 'src/divmod-1.c' > build/agent_state/407_reopen_step3_divmod.allowlist.txt && { ALLOWLIST=build/agent_state/407_reopen_step3_divmod.allowlist.txt STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/407_reopen_step3_divmod_probe.log 2>&1 || true; } && build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/divmod-1.c > build/agent_state/407_reopen_step3_dumps/divmod-1.prepared.txt 2> build/agent_state/407_reopen_step3_dumps/divmod-1.prepared.err && rg -n 'unsupported_instruction_fragment|unsupported_stack_frame|unsupported_local_memory_access|unsupported_global_data|RV64_BACKEND_RUNTIME_MISMATCH|unsupported RV64 object lowering|\[rv64-gcc-torture\]|pass\t|fail\t|RV64_C4C_OBJ|error:|arg index=.*value_bank|source_encoding=frame_slot.*dest_bank=none|dest_placement=gpr:call_argument|missing_frame_slot_arg_publication|call_arg_stack_to_stack|placement=none:call_argument' build/agent_state/407_reopen_step3_divmod_probe.log build/agent_state/407_reopen_step3_dumps/divmod-1.prepared.txt build/agent_state/407_reopen_step3_dumps/divmod-1.prepared.err && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Results:

- Build completed with no work to do.
- Probe summary: `total=1 passed=0 failed=1`.
- `src/divmod-1.c` failed with `RV64_C4C_OBJ_COMPILE_FAIL` /
  `unsupported_instruction_fragment`.
- Prepared dump written to
  `build/agent_state/407_reopen_step3_dumps/divmod-1.prepared.txt`;
  `.prepared.err` is empty.
- The proof `rg` confirmed complete frame-slot `i16` call-plan destinations
  and `missing_frame_slot_arg_publication=yes`; it found no old
  `source_encoding=frame_slot ... dest_bank=none`, `call_arg_stack_to_stack`,
  or `placement=none:call_argument` producer regression.
- Backend CTest completed in `test_after.log`: `100% tests passed, 0 tests
  failed out of 326`.
