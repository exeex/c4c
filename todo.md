Status: Active
Source Idea Path: ideas/open/435_rv64_coherent_aggregate_sret_call_storage_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Lower Caller-Side Aggregate SRet Memory Return

# Current Packet

## Just Finished

Completed Step 2: added focused prepared aggregate `sret` contract coverage for
the representative same-module frame-slot memory-return shape.

Coverage summary:

- Added a representative RV64 same-module `sret` object fixture derived from
  the existing same-module sret test.
- The fixture uses one aggregate-address argument, frame-slot
  `memory_return`, `sret_arg_index=0`, `memory_size=12`, `memory_align=4`, and
  local frame-slot address materialization with `source_size=12` /
  `source_align=4`.
- The focused test verifies the prepared facts before object emission and then
  checks RV64 emits the frame-slot address into `a0`, records the call
  relocation, restores the stack, and returns.
- Existing fail-closed same-module sret coverage remains in place for
  wrapper-kind, memory-return encoding, wrong `sret_arg_index`, missing slot,
  missing/incoherent source selection, and dynamic-stack rejection.
- No implementation files changed.
- Step 2 artifact: `build/agent_state/435_step2_sret_contract_coverage/summary.txt`.

## Suggested Next

Execute Step 3: Lower Caller-Side Aggregate SRet Memory Return.

Recommended scope: use the now-covered prepared facts as authority and re-probe
`20000917-1.c` / `20020206-1.c` to decide whether the remaining in-scope work
is post-call aggregate frame-slot value publication or whether the source idea
is already blocked only by out-of-scope runtime external/select/local
publication residuals.

Implementation proof command:

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
- The focused object fixture validates the call-storage contract; the
  representative field-copy facts remain a prepared-addressing/post-call-copy
  validation concern and should not be rederived in RV64.
- Keep select, local-publication, pointer, inline-asm, F128, and expectation
  work out of this plan unless Step 4 routes residuals separately.

## Proof

Focused pre-proof:

```sh
cmake --build build -j2 --target backend_riscv_object_emission_test && ctest --test-dir build -j2 --output-on-failure -R '^backend_riscv_object_emission$'
```

Delegated proof passed:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Proof log: `test_after.log`.
