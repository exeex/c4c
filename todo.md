Status: Active
Source Idea Path: ideas/open/318_rv64_byval_aggregate_call_abi.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Reprobe And Close Classification

# Current Packet

## Just Finished

Completed the focused Step 5 RV64 prepared callee incoming formal publication
repair exposed after byval payload staging.

Implemented repair:

- RV64 prepared function entry emission now publishes incoming non-byval GPR
  fixed formals from ABI argument registers into their prepared homes.
- Register homes get an entry `mv` when the prepared home differs from the ABI
  register; stack-slot homes get a typed GPR store through the same prologue
  boundary.
- The existing byval pointer-home publication remains in the same entry helper,
  so byval payload staging still records the incoming byval address before
  aggregate lane loads.

New focused fixture:

- `tests/backend/case/riscv64_byval_formal_gpr_publication.c`

New focused tests:

- `backend_dump_riscv64_byval_formal_gpr_publication`
- `backend_codegen_route_riscv64_byval_formal_gpr_publication`
- `backend_rv64_runtime_riscv64_byval_formal_gpr_publication`

Focused coverage result:

- The dump test proves a byval callee whose non-byval pointer formal homes in
  `t0` while arriving in `a1`, and whose non-byval integer formal homes in
  `s1` while arriving in `a2`.
- The codegen test proves entry publication with `mv t0, a1` and `mv s1, a2`
  before pointer-field loads and integer use.
- The runtime test passes under qemu for the focused case.
- The focused fixture keeps a variadic declaration only because the currently
  admitted fixed-arity shapes either fail before prepared BIR for byval plus
  trailing scalar formals, or home the non-byval formals directly in `a1`/`a2`
  and therefore do not expose the repaired `a1 -> t0` / `a2 -> s1` publication
  boundary. The fixture does not use `va_arg` and does not claim broader
  variadic ABI behavior.
- Existing byval fixed-call dump/codegen/runtime tests remain positive.
- Existing byval float-lane dump/codegen tests remain positive.
- Existing preserved-pointer byval dump/codegen/runtime tests remain positive.

`src/00140.c` reprobe artifacts:

- `build/rv64_c_testsuite_probe_latest/triage_318_step5/probe_results.tsv`
- `build/rv64_c_testsuite_probe_latest/triage_318_step5/summary.md`
- BIR, prepared-BIR, emitted RV64 assembly, clang, and qemu logs under the same
  triage directory

`src/00140.c` status after this repair:

- `--dump-bir`: 0
- `--dump-prepared-bir`: 0
- RV64 asm emit: 0
- clang link: 0
- qemu: 0

Current classification:

- The callee incoming formal publication residual is repaired. Emitted `f1`
  now records the byval address in its prepared stack home, publishes incoming
  `%p.p` with `mv t0, a1`, and publishes incoming `%p.n` with `mv s1, a2`
  before using the prepared homes.
- `src/00140.c` now emits, links, and exits successfully under qemu after the
  byval payload, F32 lane, caller pointer-materialization, and callee
  formal-publication repairs.
- No further first-bad mechanism is exposed by this candidate in the fresh Step
  5 reprobe.

## Suggested Next

Plan-owner or supervisor review can decide whether idea 318 is closure-ready
based on the final Step 5 reprobe and focused coverage.

## Watchouts

- Do not fold broad variadic ABI, runtime F-extension behavior, or unrelated
  aggregate ABI rewrites into this closure decision.
- Treat the `riscv64_byval_formal_gpr_publication` variadic spelling as a
  fixture-admission shape for fixed formal publication only; it is not proof of
  vararg payload transport or `va_arg` lowering.
- Do not special-case `src/00140.c`, `%p.f`, `%lv.f`, or fixed stack offsets.
- The byval pointer home, i8 lane, float/FPR lane, and caller pointer argument
  materialization boundaries have focused positive coverage; this packet adds
  positive focused coverage for callee GPR formal publication.

## Proof

Focused proof passed:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route|rv64_runtime)_riscv64_byval_(aggregate_fixed_call|aggregate_float_lane|preserved_pointer_args|formal_gpr_publication)'`.

Delegated proof ran:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: build passed; backend CTest returned nonzero because the existing
unrelated `backend_riscv_prepared_edge_publication` test still fails. The new
formal-publication byval tests, existing preserved-pointer byval tests,
existing fixed-call byval tests, and existing float-lane byval tests pass in
the final proof log. Fresh log: `test_after.log`.
