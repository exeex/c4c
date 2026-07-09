Status: Active
Source Idea Path: ideas/open/645_rv64_branch_residual_terminator_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Lower One Supported Same-Family Shape

# Current Packet

## Just Finished

Step 3 of `plan.md` lowered the selected prepared fused pointer compare branch
shape in the RV64 object route.

The RV64 fused pointer branch consumer now:

- checks explicit `branch_stack_load_authority role=condition` when the fused
  condition value is stack-backed;
- continues checking `role=lhs` and `role=rhs` authority for stack-backed
  pointer operands;
- allows the selected same-family shape only when the condition value and
  exactly one compared pointer operand are stack-backed with selected
  branch-stack-load authority;
- keeps unrelated/missing/ambiguous authority paths fail-closed through the
  existing structured branch-stack-load diagnostics.

Focused object coverage added a generic condition-plus-rhs-stack fused pointer
branch fixture, positive object emission coverage, and missing/ambiguous
condition authority diagnostics. Existing LHS/RHS authority status tests remain
the operand negative coverage.

The representative row `src/20140828-1.c` advanced past
`unsupported_terminator_fragment`; it now reaches RV64 runtime and aborts.
The new observed residual is stale or unmaterialized stack-carried pointer
source `%t6` before the first branch: the object branch loads RHS from the
prepared stack slot at `8(sp)`, but the slot does not hold the expected
`&a[1]` value when compared with the call result.

## Suggested Next

Execute Step 4: validate the new terminator lowering boundary and classify the
remaining `src/20140828-1.c` runtime residual. The next packet should inspect
whether `%t6` local-frame-address materialization or call-preserved stack-slot
publication is missing/stale before the first branch, without weakening branch
authority admission.

## Watchouts

- The Step 3 route consumed existing prepared condition/RHS authority; it did
  not publish new branch freshness or clobber-safety authority.
- Do not treat the new runtime abort as a terminator-fragment failure unless
  fresh evidence shows the emitted branch itself is wrong.
- Do not accept stack branch operands from apparent stack offsets, frame homes,
  final assembly shape, or assumed no-clobber behavior.
- `src/20050125-1.c`, `src/pr39100.c`, `src/20080519-1.c`,
  `src/930930-1.c`, and `src/20060910-1.c` expose select/join carrier,
  `missing_policy`, or `missing_stack_clobber_safety` boundaries on secondary
  branches. Do not paper over those owners inside RV64 lowering.
- `src/20000314-3.c` and `src/loop-2e.c` are useful same-family secondary
  probes for Step 4 route-quality validation.

## Proof

Step 3 delegated proof command:

```bash
rm -f test_after.log && (cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_object_emission|backend_prepare_frame_stack_call_contract|backend_prepared_lookup_helper|backend_prealloc_formal_publications|backend_prealloc_call_boundary_classification|backend_prepared_object_consumer_contract|backend_call_boundary_effect_plan|backend_x86_prepared_decoded_home_storage)$' && printf '%s\n' 'src/20140828-1.c' > build/agent_state/645_step3_selected.allowlist && ALLOWLIST=build/agent_state/645_step3_selected.allowlist BUILD_DIR=build VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh) > test_after.log 2>&1
```

`test_after.log` is preserved. Result: build passed; 8/8 focused CTests
passed; the selected RV64 torture row no longer fails at
`unsupported_terminator_fragment` and now fails with
`RV64_BACKEND_RUNTIME_MISMATCH` / `c4c_exit=Subprocess aborted`.

Additional focused check before the delegated proof:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'
```

Result: passed.
