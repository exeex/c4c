Status: Active
Source Idea Path: ideas/open/58_aarch64_prepared_authority_regression_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Confirm The Returned Failure Shape

# Current Packet

## Just Finished

Completed idea 58 Step 1 confirmation after idea 60 closure. The current
focused four-test proof in `test_after.log` remains `3/4`:
`backend_aarch64_instruction_dispatch`,
`backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`, and
`c_testsuite_aarch64_backend_src_00207_c` are green.
`c_testsuite_aarch64_backend_src_00196_c` is still red with the known
short-circuit runtime mismatch: the `joe() && fred()` and
`joe() && (0 + fred())` cases call `fred`, then print `1` where the expected
result is `0`.

## Suggested Next

Next packet: execute Step 2 for idea 58 by re-inspecting the `78730af2f`
boundary for the remaining `00196` family. Candidate same-feature guard targets
for that investigation are `c_testsuite_aarch64_backend_src_00033_c`
(short-circuit side effects in control flow),
`c_testsuite_aarch64_backend_src_00164_c` (boolean operator result
composition), `c_testsuite_aarch64_backend_src_00207_c` (constant
short-circuit and conditional control-flow behavior, already green), and
`c_testsuite_aarch64_backend_src_00208_c` (short-circuit `||` in an `if` with
calls and local stack objects).

## Watchouts

- Do not reopen closed idea 60 seams unless the `00196` investigation proves a
  shared semantic owner.
- Do not key behavior to `00196`, exact temporary names, literal labels, or
  fixed stack offsets.
- Keep dispatch, dynamic-stack fixed-slot FP anchoring, and `00207` green while
  repairing the remaining family.
- The remaining observed shape is specifically an `&&` false-result
  materialization problem when the left operand is true and the right operand
  calls a function returning `0`; do not repair only the named testcase without
  checking nearby short-circuit/control-flow probes.
- `c_testsuite_aarch64_backend_src_00207_c` is both a must-stay-green target
  and a same-feature guard for constant short-circuit behavior.

## Proof

No fresh build or test was required for this documentation-only confirmation
packet. Existing proof cited from `test_after.log`:

```sh
(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor|c_testsuite_aarch64_backend_src_00196_c|c_testsuite_aarch64_backend_src_00207_c)$') > test_after.log 2>&1
```

Recorded result: build passed. `backend_aarch64_instruction_dispatch`,
`backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`, and
`c_testsuite_aarch64_backend_src_00207_c` passed.
`c_testsuite_aarch64_backend_src_00196_c` failed with the known runtime
mismatch recorded above. `test_after.log` remains preserved as the cited proof
log.
