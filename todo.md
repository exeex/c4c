Status: Active
Source Idea Path: ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Step 2 follow-up / Step 3 blocker - Scalar-Control-Flow RV64 Still Original Diagnostic

# Current Packet

## Just Finished

Step 2 follow-up / Step 3 blocker - Scalar-Control-Flow RV64 Still Original
Diagnostic: repaired the scalar-control-flow terminator producer path for
legacy LIR `void` returns that still carry a dummy payload such as `ret void 0`.
BIR lowering now emits a plain void return for non-sret void functions instead
of trying to lower the dummy payload as a scalar value. Added focused
`backend_lir_to_bir_notes` coverage for that producer boundary.

Follow-up regression repair: narrowed the void-return payload guard so sret
aggregate returns still consume their return payload and copy aggregate lanes
back to `%ret.sret`. This preserves the scalar-control-flow advancement while
restoring aggregate/sret BIR dump and backend route behavior.

The representative RV64 row advanced beyond the original semantic admission
diagnostic:

Per-row classification:

- `src/20000314-3.c`: advanced beyond `attr_eq` /
  `scalar-control-flow semantic family`; now fails downstream in RV64 object
  lowering with `unsupported_terminator_fragment: BIR terminator requires
  unsupported RV64 object lowering`.
- `src/20080502-1.c`: still original diagnostic; latest function failure is
  `foo` in `scalar-control-flow semantic family`.
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
- `src/pr39501.c`: still original diagnostic; latest function failure is
  `float_min1` in `scalar-control-flow semantic family`.

## Suggested Next

Delegate a follow-up scalar-control-flow implementation packet for the
remaining same-family rows, starting with `src/20080502-1.c` / `foo` because
`src/20000314-3.c` has moved out of BIR semantic admission and now exposes a
separate RV64 terminator-lowering gap.

## Watchouts

- Keep function-signature work in
  `ideas/open/561_bir_function_signature_semantic_producer_admission.md`.
- Keep scalar-binop work in
  `ideas/open/562_bir_scalar_binop_semantic_producer_admission.md`.
- Do not claim scalar-control-flow progress through expectation rewrites,
  unsupported downgrades, allowlist edits, or named-case shortcuts.
- The delegated proof command pipeline exited 0, but the progress harness
  summary still reports `total=10 passed=0 failed=10`; treat the subset as not
  acceptance green.
- The five supervisor-reported aggregate/sret regressions now pass under the
  required targeted proof.
- `src/20000314-3.c` is no longer a BIR scalar-control-flow semantic admission
  blocker. Its remaining root cause is downstream RV64 object support for the
  prepared BIR terminator fragment, which is outside this packet's Do Not Touch
  boundaries.
- The other nine allowlisted rows still report the original
  `scalar-control-flow semantic family` diagnostic and need separate producer
  inspection. Do not route those through the `void` return payload fix unless
  their generated LIR proves the same carrier.

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
