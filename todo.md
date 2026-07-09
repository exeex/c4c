Status: Active
Source Idea Path: ideas/open/645_rv64_branch_residual_terminator_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate Boundaries And Residual Owners

# Current Packet

## Just Finished

Step 4 of `plan.md` validated the new prepared fused pointer branch boundary
and classified the remaining residual owners.

Focused object coverage now includes the condition-plus-LHS mirror for the
Step 3 condition-plus-RHS positive path. The new mirror proves the already
admitted symmetric shape consumes selected `role=condition` plus `role=lhs`
branch-stack-load authority, loads the LHS pointer operand from its selected
stack slot, and does not reload the folded condition bool.

Probe evidence is under
`build/agent_state/645_step4_boundary_validation/`, including `summary.md`,
per-row BIR/prepared dumps, object diagnostics, and disassembly for object
emission successes.

Row outcomes:

- `src/20140828-1.c`: advanced past `unsupported_terminator_fragment` and
  still fails at runtime. The emitted branch consumes explicit condition/RHS
  branch-stack-load authority and loads RHS from the selected `%t6` stack slot
  at `8(sp)`. The remaining owner is stale or unmaterialized stack-carried
  pointer source publication/materialization for `%t6`, not branch admission.
- `src/loop-2e.c`: now also advances past `unsupported_terminator_fragment`
  and fails at runtime. The block_6 pointer branch consumes explicit
  condition/RHS authority and loads RHS from the selected `%t23` stack slot at
  `336(sp)`. The remaining owner is the same stack-carried pointer source
  publication/materialization family, not branch admission.
- `src/20000314-3.c`: remains at `unsupported_terminator_fragment`. Its first
  pointer branch compares register `%p.varg0` with direct-global `@arg0`, and
  prepared evidence publishes RHS branch-stack-load authority for stack-backed
  `@arg0` only. This is outside the accepted condition-plus-one-stack-operand
  path and remains a separate direct-global stack-backed pointer branch
  boundary.

## Suggested Next

Proceed to Step 5 lifecycle review. Idea 645 has a semantic same-family
terminator shape advanced through RV64 lowering, mirror coverage for the
symmetric admitted shape, and precise residual owners for the probed nearby
rows. If the plan owner closes the idea, split follow-up work should target
stack-carried pointer source publication/materialization for `%t6`/`%t23` and
the separate direct-global stack-backed pointer branch boundary.

## Watchouts

- Step 4 did not publish new branch freshness or clobber-safety authority.
- Do not treat the `src/20140828-1.c` or `src/loop-2e.c` runtime aborts as
  terminator-fragment failures unless fresh evidence shows the emitted branches
  are wrong.
- Do not accept stack branch operands from apparent stack offsets, frame homes,
  final assembly shape, or assumed no-clobber behavior.
- `src/20050125-1.c`, `src/pr39100.c`, `src/20080519-1.c`,
  `src/930930-1.c`, and `src/20060910-1.c` expose select/join carrier,
  `missing_policy`, or `missing_stack_clobber_safety` boundaries on secondary
  branches. Do not paper over those owners inside RV64 lowering.
- `src/20000314-3.c` and `src/loop-2e.c` are useful same-family secondary
  probes for Step 4 route-quality validation.

## Proof

Step 4 delegated proof command:

```bash
rm -f test_after.log && (cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_object_emission|backend_prepare_frame_stack_call_contract|backend_prepared_lookup_helper|backend_prealloc_formal_publications|backend_prealloc_call_boundary_classification|backend_prepared_object_consumer_contract|backend_call_boundary_effect_plan|backend_x86_prepared_decoded_home_storage)$' && printf '%s\n' 'src/20140828-1.c' 'src/20000314-3.c' 'src/loop-2e.c' > build/agent_state/645_step4_boundary_validation.allowlist && ALLOWLIST=build/agent_state/645_step4_boundary_validation.allowlist BUILD_DIR=build VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh) > test_after.log 2>&1
```

`test_after.log` is preserved. Result: build passed; 8/8 focused CTests
passed; the three-row RV64 torture probe exited nonzero with classified
residuals: `src/20140828-1.c` runtime mismatch, `src/20000314-3.c`
`unsupported_terminator_fragment`, and `src/loop-2e.c` runtime mismatch.

Additional focused check before the delegated proof:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'
```

Result: passed.
