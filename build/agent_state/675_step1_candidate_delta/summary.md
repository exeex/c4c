# Step 1 Candidate Delta And First Owners

## Inputs

- Accepted baseline: `test_baseline.log`, `11/3397` failed, captured
  2026-07-10 07:53.
- Candidate baseline: `test_baseline.new.log`, `9/3397` failed, captured
  2026-07-10 12:25.
- Matching history log:
  `log/baseline_5fef23bfa8b4eaf7f4cd2b897c25ff074d35209e.log`, `9/3397`
  failed.

The candidate and history logs match by stable failing test name.

## Name-Based Delta

### Candidate-Only Failures

- `backend_cli_failure_riscv64_pointer_global_local_publication_live_load_rejection`
- `backend_cli_riscv64_call_arg_local_frame_address_materialization`

### Accepted-Only Failures

- `backend_dump_riscv64_byval_aggregate_fixed_call`
- `backend_dump_riscv64_byval_preserved_pointer_args`
- `backend_riscv_object_emission`
- `backend_aarch64_instruction_dispatch`

### Common Failures

- `backend_dump_riscv64_stack_passed_parameter_home_publication`
- `backend_dump_riscv64_scalar_compare_frame_slot_destination`
- `backend_dump_riscv64_prepared_fused_compare_call_result_predicate`
- `backend_dump_riscv64_function_pointer_return_chain`
- `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`
- `llvm_gcc_c_torture_src_20040709_2_c`
- `llvm_gcc_c_torture_src_20040709_3_c`

## Candidate-Only First-Owner Evidence

### `backend_cli_failure_riscv64_pointer_global_local_publication_live_load_rejection`

Fresh focused proof in `test_after.log` no longer shows an RV64 object-route
diagnostic for this row. It fails because the expected-failure wrapper reports:

```text
[BACKEND_OBJ_EXPECTED_FAIL]
/workspaces/c4c/tests/backend/case/riscv64_pointer_global_local_publication_live_load_rejection.c
unexpectedly succeeded
```

The generated object exists at
`build/tests/backend/riscv64_pointer_global_local_publication_live_load_rejection.o`.
`llvm-objdump -d -r` shows successful direct-global address publication into
stack slots and a live reload:

```text
8:  auipc t1, 0x0
    R_RISCV_PCREL_HI20 rv64_pointer_global_local_live_sink
c:  mv t1, t1
    R_RISCV_PCREL_LO12_I .Lpcrel_hi_global_pointer_local_publication_1_1_0
10: sd t1, 0x0(sp)
14: ld s1, 0x0(sp)
18: auipc t1, 0x0
    R_RISCV_PCREL_HI20 rv64_pointer_global_local_live_sink
1c: mv t1, t1
    R_RISCV_PCREL_LO12_I .Lpcrel_hi_global_pointer_local_publication_1_1_2
20: sd t1, 0x8(sp)
24: ld s1, 0x8(sp)
28: lh t1, 0x0(s1)
```

Prior owner evidence in
`build/agent_state/649_step1_pointer_global_local_evidence/summary.md`
classified the old owner as RV64 object-route local memory admission for a
global address published through a local pointer slot. The fresh proof shows
that this owner has moved: the current first owner is the stale expected-failure
test contract or baseline accounting for this now-succeeding CLI row.

Exact missing probe before any expectation or baseline change: run this object
under the RV64 execution harness or compare against a known-good clang object to
prove whether the emitted live-load sequence returns the global short value.
Without that runtime/semantic proof, the supported direction is to route this
as a test-contract review, not as an implementation repair.

### `backend_cli_riscv64_call_arg_local_frame_address_materialization`

Fresh focused proof in `test_after.log` fails in the object wrapper:

```text
[BACKEND_OBJ_MISSING_BYTES]
/workspaces/c4c/build/tests/backend/riscv64_call_arg_local_frame_address_materialization.o
did not contain 13050100
```

The text route still emits the desired direct frame-address materialization in
`build/tests/backend/riscv64_call_arg_local_frame_address_materialization.s`:

```text
addi a0, sp, 0
call read_local_address
```

The generated object exists at
`build/tests/backend/riscv64_call_arg_local_frame_address_materialization.o`.
`llvm-objdump -d -r` shows the object route instead using a source-register
publication plus ABI copy:

```text
38: mv s2, t0
3c: mv s1, sp
40: mv a0, s1
44: auipc ra, 0x0
    R_RISCV_CALL_PLT read_local_address
```

This matches prior evidence in
`build/agent_state/648_post656_call_evidence/summary.md`: prepared BIR marks
`arg.source_selection=local_frame_address_materialization`, while RV64 object
emission consumes that route by materializing the frame address into the
prepared source register and then copying to the ABI register. The first owner
is still the RV64 object-route consumption boundary for
`LocalFrameAddressMaterialization`: align object emission with the text-route
direct `addi a0, sp, offset` contract, or explicitly prove the object contract
should allow the two-step `mv s1, sp; mv a0, s1` shape.

## Proof

Command:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_cli_failure_riscv64_pointer_global_local_publication_live_load_rejection|backend_cli_riscv64_call_arg_local_frame_address_materialization' > test_after.log 2>&1
```

Result: build passed (`ninja: no work to do`); focused CTest failed with both
candidate-only rows still red. `test_after.log` is the proof log.
