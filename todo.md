Status: Active
Source Idea Path: ideas/open/395_rv64_object_route_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Iterate Or Split Remaining Families

# Current Packet

## Just Finished

Step 4 - Iterate Or Split Remaining Families classified the
`src/20000403-1.c` parameter-home blocker as producer-side rather than an RV64
object-emitter lowering gap. The failing prepared shape is the `%p.win` formal
in both `seqgt` and `seqgt2`: `i16 %p.win` is assigned to prepared stack-slot
homes (`slot#3+stack0` and `slot#5+stack8`) backed by
`source_kind=regalloc.spill_slot`, but the function parameter has no usable
prepared scalar integer ABI fact. The shared legalizer's direct ABI repair
covers `I1`, `I8`, `I32`, `I64`, and `Ptr`, but not `I16`, so the emitter
would have to reconstruct the missing incoming-argument register (`a1`) from
parameter order and type to seed the stack home. This packet intentionally did
not add that producer-side reconstruction in `object_emission.cpp`.

## Suggested Next

Split a producer-side packet to repair scalar `I16` formal ABI publication in
the shared prepared-BIR legalize/regalloc path, then re-run
`src/20000403-1.c` through the RV64 object route. The expected repair point is
`src/backend/prealloc/legalize.cpp` `direct_bir_call_arg_abi_repair()`, with
nearby prepared-BIR/regalloc tests proving `I16` formals publish an integer
register ABI/home instead of reaching the object emitter as ABI-less stack-slot
parameter homes.

## Watchouts

- Do not repair this by teaching the RV64 object emitter to infer missing ABI
  from `i16` parameter order; that would duplicate producer ABI policy inside
  the consumer.
- The exact prepared homes from
  `build/agent_state/395_step4_20000403-1.prepared-bir.txt` are:
  `seqgt home %p.win value_id=19 kind=stack_slot slot_id=3 offset=0` and
  `seqgt2 home %p.win value_id=26 kind=stack_slot slot_id=5 offset=8`, with
  stack objects `#3` and `#5` both `source_kind=regalloc.spill_slot type=i16
  size=2 align=2`.
- Existing object-emitter scalar formal stack-home support is still useful for
  fully prepared scalar GPR stack-slot facts; this blocker is missing producer
  ABI publication for the `I16` formal, not a need for testcase-shaped emitter
  matching.

## Proof

Ran the delegated RV64 proof exactly. Result: `[rv64-gcc-torture] total=1
passed=0 failed=1`, with `fail	src/20000403-1.c`. This is sufficient for
the classification/split packet because the proof reproduces the producer-side
`unsupported_param_home` blocker and no object-emitter reconstruction was added.

Delegated command:
`cmake --build --preset default --target c4cll && mkdir -p build/agent_state && printf '%s\n' 'src/20000403-1.c' > build/agent_state/395_step4_param_home.allowlist.txt && { ALLOWLIST=build/agent_state/395_step4_param_home.allowlist.txt STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1 || true; } && rg -n 'unsupported_param_home|unsupported_instruction_fragment|unsupported RV64 object lowering|\[rv64-gcc-torture\]|pass\t|fail\t|RV64_C4C_OBJ' test_after.log`

Proof log: `test_after.log`. Per-case log:
`build/rv64_gcc_c_torture_backend/src_20000403-1.c/case.log` reports
`unsupported_param_home: RV64 object route requires all parameters in supported
GPR or prepared FPR register homes`.
