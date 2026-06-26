Status: Active
Source Idea Path: ideas/open/395_rv64_object_route_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reclassify Current Instruction-Fragment Residuals

# Current Packet

## Just Finished

Step 1 reclassified the current representatives after the corrected 407
close.

Representative results:

- `src/divmod-1.c`: still fails object codegen with
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64
  object lowering`.
- `src/20000223-1.c`: passes in the two-case allowlist probe.
- Backend CTest subset remains green: `100% tests passed, 0 tests failed out
  of 326`.

Corrected 407 caller-side facts remain complete in the fresh
`divmod-1.c` prepared dump. Same-module frame-slot `i16` call arguments now
publish `value_bank=gpr`, `dest_placement=gpr:call_argument#N/w1`,
`dest_reg=aN`, `dest_bank=gpr`, and
`missing_frame_slot_arg_publication=yes`. The old blocking caller-side shapes
do not reappear: no `source_encoding=frame_slot ... dest_bank=none`, no
`call_arg_stack_to_stack`, and no `placement=none:call_argument`.

First concrete residual family:

- Prepared opcode/fact shape: `bir.sext i16 %p.x to i32` in `div2`
  reaches RV64 object emission with the source formal published as
  `storage %p.x ... encoding=register bank=none reg=a0 width=1 units=a0`.
- The RV64 object route already has a scalar `SExt` fragment path, but source
  materialization goes through `gpr_register_number_for_value()` /
  `gpr_register_number_for_home()`, which rejects homes whose target register
  identity is present and not `PreparedRegisterBank::Gpr`. The `bank=none`
  formal therefore prevents the existing `SExt` lowering from reading `a0`.
- This is the first live unsupported instruction after `div1`'s supported
  `i8` path; later `divmod-1.c` instructions include `bir.sdiv`, `bir.srem`,
  and `bir.urem`, but those are not yet the first confirmed boundary.

Route sharing:

- `divmod-1.c` and `20000223-1.c` do not share the current first route.
  `20000223-1.c` passes with its `bir.and i32 %p.align, %t0` and call shape.
  The live residual is specific to `divmod-1.c`'s narrow `i16` callee formal
  register-bank publication feeding `bir.sext`.

## Suggested Next

Step 2 should repair the producer-side prepared storage/ABI metadata for
callee `i16` formals in argument registers so `%p.x` / `%p.y` publish GPR bank
facts when they are already physically in `aN`. Do not add RV64 object-route
inference for `bank=none`; once this producer fact is complete, rerun the
probe to expose the next true 395 instruction-fragment family if one remains.

## Watchouts

- Do not reopen 407 unless fresh prepared dumps again show the old
  same-module `i16` producer regression:
  `source_encoding=frame_slot ... dest_bank=none`,
  `call_arg_stack_to_stack`, or `placement=none:call_argument`.
- Do not infer scalar call-argument ABI policy inside RV64 object emission.
- Route producer-fact gaps to their owners instead of patching them under 395.

## Proof

Exact command run:

```sh
cmake --build --preset default && mkdir -p build/agent_state/395_step1_reclassify_after407_dumps && printf '%s\n' 'src/divmod-1.c' 'src/20000223-1.c' > build/agent_state/395_step1_reclassify_after407.allowlist.txt && { ALLOWLIST=build/agent_state/395_step1_reclassify_after407.allowlist.txt STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/395_step1_reclassify_after407_probe.log 2>&1 || true; } && build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/divmod-1.c > build/agent_state/395_step1_reclassify_after407_dumps/divmod-1.prepared.txt 2> build/agent_state/395_step1_reclassify_after407_dumps/divmod-1.prepared.err && build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000223-1.c > build/agent_state/395_step1_reclassify_after407_dumps/20000223-1.prepared.txt 2> build/agent_state/395_step1_reclassify_after407_dumps/20000223-1.prepared.err && rg -n 'unsupported_instruction_fragment|unsupported_stack_frame|unsupported_local_memory_access|unsupported_global_data|RV64_BACKEND_RUNTIME_MISMATCH|unsupported RV64 object lowering|\[rv64-gcc-torture\]|pass\t|fail\t|RV64_C4C_OBJ|error:|prepared.func|bir\.(call|sext|zext|trunc|mul|sdiv|udiv|srem|urem|shl|ashr|lshr|and|or|xor|cmp|select)|source_encoding=frame_slot.*dest_bank=none|dest_placement=gpr:call_argument|missing_frame_slot_arg_publication|call_arg_stack_to_stack|placement=none:call_argument' build/agent_state/395_step1_reclassify_after407_probe.log build/agent_state/395_step1_reclassify_after407_dumps/*.prepared.txt build/agent_state/395_step1_reclassify_after407_dumps/*.prepared.err && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result: command completed successfully. The allowlist probe reported
`fail	src/divmod-1.c`, `pass	src/20000223-1.c`, and
`[rv64-gcc-torture] total=2 passed=1 failed=1`. `test_after.log` was
preserved from the backend subset and is green.
