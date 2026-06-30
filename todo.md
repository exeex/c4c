Status: Active
Source Idea Path: ideas/open/435_rv64_coherent_aggregate_sret_call_storage_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Prepared SRet Contract Coverage

# Current Packet

## Just Finished

Completed Step 1: probed the coherent aggregate `sret` representatives and
classified the current residual shape.

Evidence summary:

- `20000917-1.c` raw/prepared BIR has coherent `ptr sret(size=12, align=4)`
  calls to same-module `one` and `zero`.
- `20020206-1.c` raw/prepared BIR has coherent `ptr sret(size=12, align=4)`
  call to same-module `bar`.
- Prepared call plans carry `memory_return`, `memory_encoding=frame_slot`,
  `sret_arg_index=0`, `memory_size=12`, `memory_align=4`, and aggregate-address
  arg0 selected by `local_frame_address_materialization`.
- Prepared frame-slot/addressing facts cover field-copy offsets `0`, `4`, and
  `8` for callee sret stores and caller post-call aggregate copies.
- `--codegen obj` still reports the generic RV64 object diagnostic
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64
  object lowering`.
- `--codegen asm` resolves the first non-sret residual to unsupported runtime
  external `abort`: `20000917-1` in `main`, `20020206-1` in `baz`.
- Probe artifacts and classification are under
  `build/agent_state/435_step1_sret_residual_probe/`.

## Suggested Next

Execute Step 2: Add Focused Prepared SRet Contract Coverage.

First bounded packet:

- Primary file: `tests/backend/mir/backend_riscv_object_emission_test.cpp`.
- Validate the existing same-module sret object fixture against the current
  representative shape: frame-slot `memory_return`, `sret_arg_index=0`,
  aggregate-address argument, local frame-slot address materialization, and
  fail-closed missing/incoherent prepared facts.
- Add narrow coverage only if the existing fixture cannot express the
  representative 12-byte/three-field-copy aggregate shape without inferring
  layout in RV64 lowering.
- Implementation files should remain unchanged unless this coverage exposes a
  real semantic defect in the existing prepared sret path.

Future implementation proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not include `src/pr88904.c`; it belongs to idea 436 until producer facts
  are reconciled.
- Do not include scalar metadata defect rows `src/20010224-1.c` or
  `src/20020506-1.c` as aggregate lowering proof.
- Do not infer aggregate layout, size, alignment, field offsets, return slots,
  or storage homes in RV64.
- Do not route unsupported runtime external `abort`/`exit` calls into this
  aggregate sret plan; they are the first non-sret residuals after the coherent
  facts in the representatives.
- Keep select, local-publication, pointer, inline-asm, F128, and expectation
  work out of this plan unless Step 4 routes residuals separately.

## Proof

Step 1 classification-only proof:

```sh
git diff --check
```
