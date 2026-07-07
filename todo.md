Status: Active
Source Idea Path: ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Step 2 follow-up / Step 3 remaining rows - Scalar-Control-Flow RV64 Same-Family Repair

# Current Packet

## Just Finished

Step 2 follow-up / Step 3 remaining rows - Scalar-Control-Flow RV64
Same-Family Repair: repaired the next generic CFG PHI admission blocker exposed
by `src/930614-1.c` / `main`. The scalar-control-flow PHI planner now admits
all scalar/function-pointer BIR value types through the existing
`lower_scalar_or_function_pointer_type` carrier instead of only integer PHIs.
This admits the row's `phi double` join while preserving existing pointer PHI
behavior such as `@g`/`null` joins.

Added focused `backend_lir_to_bir_notes` coverage for a generic F64 PHI join.
The fixture intentionally uses two PHIs so it exercises the generic CFG PHI
planner rather than the canonical-select fast path.

The current RV64 representatives now show the target row and several nearby
same-family rows advanced beyond their original scalar-control-flow admission
diagnostics:

Per-row classification:

- `src/20000314-3.c`: advanced beyond `attr_eq` /
  `scalar-control-flow semantic family`; now fails downstream in RV64 object
  lowering with `unsupported_terminator_fragment: BIR terminator requires
  unsupported RV64 object lowering`.
- `src/20080502-1.c`: advanced beyond `foo` /
  `scalar-control-flow semantic family`; now fails later at `main` in
  `scalar-binop semantic family`.
- `src/930614-1.c`: advanced beyond `main` /
  `scalar-control-flow semantic family`; now fails downstream in RV64 object
  lowering with `unsupported_terminator_fragment`.
- `src/980604-1.c`: advanced beyond `main` /
  `scalar-control-flow semantic family`; now fails downstream in RV64 object
  lowering with `unsupported_move_bundle_target_shape` for select publication
  moves.
- `src/ieee/fp-cmp-8.c`: advanced beyond `test_isunordered` /
  `scalar-control-flow semantic family`; now fails in
  `scalar/local-memory semantic family`.
- `src/ieee/fp-cmp-8f.c`: advanced beyond `test_isunordered` /
  `scalar-control-flow semantic family`; now fails in
  `scalar/local-memory semantic family`.
- `src/ieee/fp-cmp-8l.c`: advanced beyond `test_isunordered` /
  `scalar-control-flow semantic family`; now fails in
  `scalar/local-memory semantic family`.
- `src/ieee/pr38016.c`: advanced beyond `test_isunordered` /
  `scalar-control-flow semantic family`; now fails in
  `scalar/local-memory semantic family`.
- `src/pr35456.c`: advanced beyond `not_fabs` /
  `scalar-control-flow semantic family`; now fails downstream in RV64 object
  lowering with `unsupported_terminator_fragment`.
- `src/pr39501.c`: advanced beyond semantic admission and now fails downstream
  in RV64 object lowering with `unsupported_instruction_fragment` for a BIR
  `SelectInst` in `float_min1`.

## Suggested Next

Delegate a follow-up scalar-control-flow implementation packet for the
remaining active scalar-control-flow work only after supervisor review of the
new row map. This packet moved all currently allowlisted rows beyond the
original scalar-control-flow diagnostic; remaining failures now sit in
downstream RV64 object lowering, scalar-binop, or scalar/local-memory families.

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
- `src/20000314-3.c`, `src/930614-1.c`, and `src/pr35456.c` are no longer BIR
  scalar-control-flow semantic admission blockers. Their remaining root cause
  is downstream RV64 object support for prepared BIR terminators, which is
  outside this packet's Do Not Touch boundaries.
- `src/980604-1.c` is now downstream RV64 prepared move-bundle/select
  publication work, also outside this packet.
- `src/20080502-1.c` is now scalar-binop work. The IEEE rows are now
  scalar/local-memory work. Keep those families in their own packets/plans.
- No allowlisted row currently remains at the original
  `scalar-control-flow semantic family` diagnostic after this packet's proof.

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

Current packet proof ran `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_notes$' && ALLOWLIST=build/agent_state/560_step3_scalar_control_flow.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh; } 2>&1 | tee test_after.log`.
Build completed, `backend_lir_to_bir_notes` passed, and the RV64 progress
harness exited 0 while reporting `total=10 passed=0 failed=10`. Per-row proof
is recorded above; prior advancements for `src/20000314-3.c`,
`src/20080502-1.c`, and `src/pr39501.c` were preserved.

Supervisor acceptance reran
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log`:
passed, 346/346 backend tests. Current `test_after.log` contains this final
backend acceptance run.
