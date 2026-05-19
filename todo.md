Status: Active
Source Idea Path: ideas/open/325_aarch64_variadic_local_value_home_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Focused Local Publication Coverage

# Current Packet

## Just Finished

Step 3 stabilized the current Step 2 publication repairs with focused local
coverage, small cleanup, and the supervisor-reported broader backend regression
fixes. The dirty implementation route remains the same:
branch/control values are published before branch use, zero-offset global GEP
pointer provenance reaches call operands as symbol addresses, frame-slot
address call operands materialize the frame address instead of loading the slot,
mutable pointer-local loads publish the loaded runtime pointer as provenance,
and predecessor/join source publication can materialize the edge compare before
the branch.

Focused coverage added:
- `backend_aarch64_instruction_dispatch` now covers a materialized branch
  condition read from a prepared stack home before `cbnz`.
- `backend_aarch64_instruction_dispatch` now covers frame-slot address call
  operands by asserting the selected call-boundary move is marked as an address
  materialization and prints `add x1, sp, #48`, not an `ldr` from the slot.
- `backend_aarch64_instruction_dispatch` now covers predecessor/join source
  publication by forcing an out-of-SSA edge compare source to materialize before
  the branch.
- `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`
  now covers AArch64 prepared handoff facts for `%t22`/`%t24`/`%t34` mutable
  pointer-local provenance, `@.str30` symbol-address call provenance, later
  prepared branch conditions, the `%t15` join incoming, and `%lv.t7.0`
  frame-slot address materialization.

Small cleanup: `lower_predecessor_join_source_publication` no longer accepts an
unused diagnostics parameter.

Broader regression cleanup:
- String-pointer call rewriting now falls back to LIR/BIR result-name pairing
  when direct-call counts drift, preserving `%t5`/`@.str30` provenance through
  TextId-backed string constants without raw spelling recovery.
- Non-AArch64 non-indirect pointer-slot loads may forward the tracked current
  base and avoid dead computed-address temporaries, while indirect/exposed
  pointer slots keep explicit loaded-pointer provenance. This preserves the x86
  local-slot publication contract.
- Regalloc active assignment expiry now allows same-boundary register reuse
  only inside dynamic-stack functions and not at call points, removing the
  VLA-only spill slot while preserving local-arg-call and weighted post-call
  pressure contracts.

The publication repair/coverage slice is commit-ready as a Step 2/Step 3
stabilization slice. The representative c-testsuite case still fails with the
same out-of-scope HFA/floating residual: after the fixed-size string argument
cases, `fa_hfa11(hfa11)` prints `0.0` instead of `11.1`, followed by corrupted
floating/HFA output and a later segmentation fault. No fresh generated-code
evidence ties that residual back to local/value-home publication.

## Suggested Next

Hand the remaining `00204.c` residual to lifecycle classification or the
HFA/floating argument owner. Do not keep extending Step 2 unless fresh
generated-code evidence again shows an unpublished local/value home, branch
condition, call operand, or predecessor/join source.

## Watchouts

- Preserve prior repairs: large stack offsets, large frame adjustments,
  `va_start` helper lowering, scalar ALU immediates, HFA argument lanes, F128
  transport, aggregate helper text lowering, `va_start` destination
  materialization, aggregate/floating `va_arg` source/progression, frame-size
  coverage, and fixed-formal publication.
- Branch publication for the `for.cond.70` fused compare, literal pointer
  provenance for `%t5`/`@.str30`, frame-slot address call operands for
  `%lv.t7.0`, stale mutable pointer-local provenance, later branch exits, and
  `%t15` predecessor/join source publication are fixed and now have focused
  local coverage.
- The remaining runtime representative failure is no longer a timeout. The
  CTest case exits with `RUNTIME_NONZERO`/segmentation fault after printing
  substantial output. `match` latch provenance now uses `%t21`/`%t23`/`%t25`/
  `%t31`, branch exits are emitted, and `%t15` predecessor compare/source
  publication is now executable. The next first bad fact is the first HFA float
  argument output: `fa_hfa11(hfa11)` prints `0.0` instead of `11.1`.
- Do not special-case `00204.c`, `stdarg`, `myprintf`, the format loop, one
  local, one stack slot, one register, one offset, or one emitted instruction
  sequence.
- Do not weaken expectations, unsupported classifications, CTest registration,
  runner behavior, timeout policy, proof-log policy, or prior guardrails.
- Reopen frame/formal publication only if fresh generated output again shows
  uncovered stack references or fixed-formal clobber before publication.

## Proof

Delegated proof ran:
`cmake --build --preset default && bash -o pipefail -c "ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_(machine_printer|instruction_dispatch|target_instruction_records|prepared_branch_records|branch_compare_records)|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|c_testsuite_aarch64_backend_src_00204_c)$' | tee test_after.log"`

Result: build passed; 6/7 delegated tests passed. The AArch64 internal machine
printer, instruction dispatch, target-record, prepared-branch, branch-compare,
and prepared-BIR tests passed. `c_testsuite_aarch64_backend_src_00204_c` still
exits with `RUNTIME_NONZERO`/segmentation fault after substantial output; the
first remaining bad fact is still `fa_hfa11(hfa11)` printing `0.0` instead of
`11.1`, outside this publication owner. Proof log: `test_after.log`.

Additional focused test added outside the delegated regex and run separately:
`ctest --test-dir build -j --output-on-failure -R '^backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication$'`

Additional result: passed.

Supervisor-reported broader regression proof ran:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_cli_dump_prepared_bir_vla_goto_stackrestore_cfg|backend_codegen_route_x86_64_pointer_param_loaded_char_deref_observe_semantic_bir)$'`

Broader regression result: passed 3/3. `backend_lir_to_bir_notes`,
`backend_cli_dump_prepared_bir_vla_goto_stackrestore_cfg`, and
`backend_codegen_route_x86_64_pointer_param_loaded_char_deref_observe_semantic_bir`
all pass again.

Follow-up backend regression proof ran:
`ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_cli_dump_prepared_bir_local_arg_call_contract)$'`

Follow-up result: passed 2/2. `backend_prepare_liveness` and
`backend_cli_dump_prepared_bir_local_arg_call_contract` both pass again after
the targeted regalloc expiry repair.

Full backend proof ran:
`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Full backend result: passed 140/140.

Slice status: complete for Step 2/Step 3 publication repair coverage and the
reported broader backend regressions. Commit readiness: ready for the
publication repair/coverage slice; remaining HFA/floating residual should be
classified or routed separately.
