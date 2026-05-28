Status: Active
Source Idea Path: ideas/open/58_aarch64_prepared_authority_regression_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Confirm The Returned Failure Shape

# Current Packet

## Just Finished

Idea 60 decomposition is closed and the active lifecycle returned to idea 58.
The accepted Step 6-10 seam commits split and proved store-local selected
publication, store-global stack publication, fused-compare selected operand
order, call/outgoing stack argument materialization, and direct-edge
`LoadLocal` prepared source-memory consumption.

The post-Step-10 review accepted those seams as aligned with idea 60 and found
no route reset or further decomposition requirement. The focused four-test
proof remains `3/4`: dispatch, dynamic-stack fixed-slot FP anchoring, and
`00207` pass; `00196` still fails with the known `joe() && fred()` runtime
mismatch.

## Suggested Next

Next packet: execute Step 1 for idea 58 by confirming the returned failure
shape from `test_after.log`, then select nearby same-feature probes before
re-inspecting the `78730af2f` boundary for the remaining `00196` family.

## Watchouts

- Do not reopen closed idea 60 seams unless the `00196` investigation proves a
  shared semantic owner.
- Do not key behavior to `00196`, exact temporary names, literal labels, or
  fixed stack offsets.
- Keep dispatch, dynamic-stack fixed-slot FP anchoring, and `00207` green while
  repairing the remaining family.
- `c_testsuite_aarch64_backend_src_00196_c` still fails with the existing
  runtime mismatch (`joe() && fred()` cases print `1` instead of `0`).

## Proof

Ran the delegated proof command and preserved `test_after.log`:

```sh
(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor|c_testsuite_aarch64_backend_src_00196_c|c_testsuite_aarch64_backend_src_00207_c)$') > test_after.log 2>&1
```

Result: build passed. `backend_aarch64_instruction_dispatch`,
`backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`, and
`c_testsuite_aarch64_backend_src_00207_c` passed.
`c_testsuite_aarch64_backend_src_00196_c` failed with the known baseline
runtime mismatch recorded above. `test_after.log` is preserved with the exact
delegated proof output.

Plan-owner close gate for idea 60 used the preserved matching four-test logs.
The strict monotonic check reported no pass-count increase (`3/4` before and
after), while the documented non-decreasing regression guard passed with no new
failures:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py \
  --before test_before.log \
  --after test_after.log \
  --allow-non-decreasing-passed
```
