Status: Active
Source Idea Path: ideas/open/58_aarch64_prepared_authority_regression_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reproduce And Classify The Four Failures

# Current Packet

## Just Finished

Completed Step 1 reproduction and classification for the four active AArch64
regressions. The focused command rebuilt successfully, then the ctest subset
failed as expected with all four target tests still red:

- `backend_aarch64_instruction_dispatch`: dispatch emits 6 instructions with 3
  diagnostics instead of the expected 7 instructions with 2 diagnostics. First
  diagnostic is `AArch64 block dispatch visited unsupported prepared BIR
  instruction family`. The failing assertion is the stack-homed i32 select
  publication before a global store.
- `backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`: route
  assembly is missing `add w13, w13, #1`; generated code reloads `fixed` and
  `total` from `sp` offsets after moving `sp` for the VLA, despite the test
  contract requiring fixed slots to use the frame-pointer anchor.
- `c_testsuite_aarch64_backend_src_00196_c`: runtime mismatch in short-circuit
  evaluation. The `joe() && fred()` cases print `fred` and yield `1`, where the
  expected result is `0`, so the selected false value or call-side effect
  publication is being consumed incorrectly.
- `c_testsuite_aarch64_backend_src_00207_c`: runtime timeout. The source mixes a
  VLA/backward `goto`, constant non-VLA arrays guarded by logical/conditional
  expressions, and short-circuit/ternary side-effect suppression.

Preliminary family mapping:

- instruction dispatch: prepared stack select publication before selected
  stores, likely shared prepared-fact consumption rather than a standalone test
  expectation issue.
- dynamic stack: fixed-slot frame-pointer anchoring after dynamic `sp`
  adjustment.
- `00196`: short-circuit selected value and side-effect control around calls.
- `00207`: dynamic stack/VLA and backward-goto behavior with additional
  short-circuit/conditional-expression pressure; classify with the dynamic-stack
  family first because it times out rather than producing a simple value
  mismatch.

## Suggested Next

Delegate Step 2 boundary comparison for the `c92708627` family. Start from the
prepared load-local/register-home authority change and explain why stack-homed
select publication, dynamic-stack fixed-slot anchoring, and `00207` regressed
together before choosing a repair surface.

## Watchouts

Nearby same-feature probes for the next repair packet:

- Dispatch publication: in `backend_aarch64_instruction_dispatch`, compare the
  failing `block_dispatch_publishes_stack_homed_i32_select_before_global_store`
  with the adjacent passing/related stack-select paths
  `block_dispatch_publishes_f64_select_before_global_store`,
  `block_dispatch_publishes_stack_homed_global_load_select_before_local_store`,
  `short_circuit_join_consumes_edge_published_result`, and
  `stack_home_fused_compare_rejects_same_block_i8_extension_home`.
- Dynamic stack anchoring:
  `backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor` and
  `tests/backend/case/vla_goto_stackrestore.c` should be enough to guard
  against anchoring only the exact `fixed` local.
- Short-circuit runtime behavior: use `c_testsuite_aarch64_backend_src_00196_c`
  plus nearby allowlisted c-testsuite logical/control-flow cases
  `00194`, `00197`, and `00207`; avoid treating `00196` as a print-order-only
  problem.
- `00207`: split observations by `f1` VLA/backward goto, `f2` constant
  logical/conditional array bounds with `goto`, and `f3` short-circuit/ternary
  side-effect suppression before tying it to a single repair.

Do not accept testcase-name matching, expectation downgrades, unsupported-path
rewrites, or broad duplicate local scans as capability progress.

## Proof

Ran exactly:

```sh
bash -o pipefail -c "cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor|c_testsuite_aarch64_backend_src_00196_c|c_testsuite_aarch64_backend_src_00207_c)$' | tee test_after.log"
```

Result: build succeeded, ctest failed as expected with 0/4 tests passing.
Canonical proof log: `test_after.log`.
