Status: Active
Source Idea Path: ideas/open/58_aarch64_prepared_authority_regression_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair The First Boundary Failure Family

# Current Packet

## Just Finished

Completed Step 3 repair for the first `c92708627` boundary family in
`src/backend/mir/aarch64/codegen/alu.cpp`.

Repair:

- Prepared scalar frame-slot load-source operands now carry the function
  fixed-slot frame-pointer-base policy through `uses_frame_pointer_base`.
- Scalar fallback and prepared scalar ALU operand override still consume the
  prepared same-block load-local source lookup, but only after checking whether
  a current emitted scalar register already exists for the value. This preserves
  current scalar state without adding a duplicate BIR scan or testcase-shaped
  match.

Focused proof result:

- `backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`
  passes. The generated reloads use `x29` after dynamic stack movement, and the
  increment form is restored through current emitted scalar state.
- `c_testsuite_aarch64_backend_src_00207_c` now passes in the focused subset.
- `backend_aarch64_instruction_dispatch` still fails with unsupported prepared
  BIR instruction family for the store-owned stack select before global store.
- `c_testsuite_aarch64_backend_src_00196_c` still has the same runtime mismatch
  and remains the later `78730af2f` boundary unless Step 4 proves otherwise.

## Suggested Next

Delegate the next packet to compare the `78730af2f` boundary for `00196`, unless
the supervisor chooses to split off the remaining instruction-dispatch selected
store/global-store publication failure first.

## Watchouts

- The remaining dispatch failure did not change after the ALU prepared-source
  repair. Its owner still appears to be selected-value publication into the
  store/global-store path, not dynamic-stack frame-slot addressing.
- The `00196` mismatch did not change and should remain out of the Step 3
  success criteria.
- Do not weaken the dynamic-stack route test snippets: they caught both the
  frame-pointer base policy gap and the redundant prepared-memory preference
  over already emitted scalar state.

## Proof

```sh
bash -o pipefail -c "cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor|c_testsuite_aarch64_backend_src_00196_c|c_testsuite_aarch64_backend_src_00207_c)$' | tee test_after.log"
```

Build succeeded. Focused ctest result was `2/4` passing:
`backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor` and
`c_testsuite_aarch64_backend_src_00207_c` passed; `backend_aarch64_instruction_dispatch`
and `c_testsuite_aarch64_backend_src_00196_c` failed as above. Proof log:
`test_after.log`.

Supervisor accepted the hook-produced full-suite baseline candidate with
`scripts/plan_review_state.py accept-baseline` after checking for stale test
processes. Accepted baseline: `test_baseline.log` at commit `a6058f648`, full
suite `3413/3415` passing with only the known remaining failures
`backend_aarch64_instruction_dispatch` and
`c_testsuite_aarch64_backend_src_00196_c`.
