Status: Active
Source Idea Path: ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Step 2 follow-up / Step 3 remaining rows - Scalar-Control-Flow RV64 Same-Family Repair

# Current Packet

## Just Finished

Step 2 follow-up / Step 3 remaining rows - Scalar-Control-Flow RV64
Same-Family Repair: repaired the next canonical-select producer shape exposed
by `src/20080502-1.c` / `foo`. The scalar-control-flow canonical select path
now admits the F128 signbit-style prelude used by the row: `fp128` to `i128`
bitcast, `i128` shift, trunc/compare condition, an `fpext` false arm, and a
raw `0xL...` F128 literal true arm. The generic PHI planner gate was removed
from the already-recognized canonical select route because that route consumes
its single select PHI itself and the generic planner is still an
integer/aggregate CFG planner.

Touched adjacent `src/backend/bir/lir_to_bir/scalar.cpp` because investigation
proved the canonical select scalar helper was the true owner for entry-prelude
casts/binops and chain arm casts. Kept F128 literal parsing local to canonical
select so closed scalar-binop F128 constant behavior remains unchanged.

The current RV64 representatives now show two rows advanced beyond the original
scalar-control-flow admission diagnostic:

Per-row classification:

- `src/20000314-3.c`: advanced beyond `attr_eq` /
  `scalar-control-flow semantic family`; now fails downstream in RV64 object
  lowering with `unsupported_terminator_fragment: BIR terminator requires
  unsupported RV64 object lowering`.
- `src/20080502-1.c`: advanced beyond `foo` /
  `scalar-control-flow semantic family`; now fails later at `main` in
  `scalar-binop semantic family`.
- `src/930614-1.c`: still original diagnostic; latest function failure is
  `main` in `scalar-control-flow semantic family`.
- `src/980604-1.c`: still original diagnostic; latest function failure is
  `main` in `scalar-control-flow semantic family`.
- `src/ieee/fp-cmp-8.c`: still original diagnostic; latest function failure is
  `test_isunordered` in `scalar-control-flow semantic family`.
- `src/ieee/fp-cmp-8f.c`: still original diagnostic; latest function failure is
  `test_isunordered` in `scalar-control-flow semantic family`.
- `src/ieee/fp-cmp-8l.c`: still original diagnostic; latest function failure is
  `test_isunordered` in `scalar-control-flow semantic family`.
- `src/ieee/pr38016.c`: still original diagnostic; latest function failure is
  `test_isunordered` in `scalar-control-flow semantic family`.
- `src/pr35456.c`: still original diagnostic; latest function failure is
  `not_fabs` in `scalar-control-flow semantic family`.
- `src/pr39501.c`: advanced beyond semantic admission and now fails downstream
  in RV64 object lowering with `unsupported_instruction_fragment` for a BIR
  `SelectInst` in `float_min1`.

## Suggested Next

Delegate a follow-up scalar-control-flow implementation packet for the
remaining same-family rows, starting with `src/930614-1.c` / `main` because
`src/20000314-3.c` and `src/20080502-1.c` have moved out of their original BIR
scalar-control-flow admission blockers.

## Watchouts

- Keep function-signature work in
  `ideas/open/561_bir_function_signature_semantic_producer_admission.md`.
- Keep scalar-binop work in
  `ideas/open/562_bir_scalar_binop_semantic_producer_admission.md`.
- Do not claim scalar-control-flow progress through expectation rewrites,
  unsupported downgrades, allowlist edits, or named-case shortcuts.
- The delegated proof command pipeline exits 0, but the progress harness
  summary still reports `total=10 passed=0 failed=10`; treat the subset as not
  acceptance green.
- The five supervisor-reported aggregate/sret regressions now pass under the
  required targeted proof.
- `src/20000314-3.c` is no longer a BIR scalar-control-flow semantic admission
  blocker. Its remaining root cause is downstream RV64 object support for the
  prepared BIR terminator fragment, which is outside this packet's Do Not Touch
  boundaries.
- The remaining same-family rows still reporting original
  `scalar-control-flow semantic family` diagnostics are `src/930614-1.c`,
  `src/980604-1.c`, `src/ieee/fp-cmp-8.c`, `src/ieee/fp-cmp-8f.c`,
  `src/ieee/fp-cmp-8l.c`, `src/ieee/pr38016.c`, and `src/pr35456.c`. Do not
  route those through either the void-return or F128 signbit canonical-select
  fixes unless generated LIR proves the same carrier.

## Proof

Ran `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_notes$' && ALLOWLIST=build/agent_state/560_step3_scalar_control_flow.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh; } 2>&1 | tee test_after.log`.
Build completed, `backend_lir_to_bir_notes` passed, and the RV64 progress
harness reported `total=10 passed=0 failed=10`. Per-case logs live under
`build/rv64_gcc_c_torture_backend/`; summary:
`build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`.

Follow-up proof ran `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_codegen_route_aarch64_sret_global_scalar_source_publication|backend_cli_dump_bir_layout_sensitive_aggregate|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|backend_codegen_route_x86_64_aggregate_return_pair_observe_semantic_bir|backend_codegen_route_x86_64_aggregate_param_return_pair_observe_semantic_bir)$' && ALLOWLIST=build/agent_state/560_step3_scalar_control_flow.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh; } 2>&1 | tee test_after.log`.
Build completed; all six targeted backend tests passed; the RV64 progress
harness still reported `total=10 passed=0 failed=10`, with `src/20000314-3.c`
remaining beyond the original scalar-control-flow admission diagnostic at the
downstream `unsupported_terminator_fragment` blocker.

Supervisor acceptance reran
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log`:
passed, 346/346 backend tests. Current `test_after.log` contains this final
backend acceptance run.

Current packet proof ran `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_notes$' && ALLOWLIST=build/agent_state/560_step3_scalar_control_flow.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh; } 2>&1 | tee test_after.log`.
Build completed, `backend_lir_to_bir_notes` passed, and the RV64 progress
harness exited 0 while reporting `total=10 passed=0 failed=10`. Per-row proof:
`src/20000314-3.c` remains downstream at `unsupported_terminator_fragment`;
`src/20080502-1.c` advanced beyond `foo` / `scalar-control-flow semantic
family` to `main` / `scalar-binop semantic family`; `src/pr39501.c` remains
downstream at RV64 `unsupported_instruction_fragment` for a BIR `SelectInst`;
the seven rows listed in Watchouts remain original scalar-control-flow
admission blockers.

Supervisor acceptance reran
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log`:
passed, 346/346 backend tests. Current `test_after.log` contains this final
backend acceptance run.
