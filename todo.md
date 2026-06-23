Status: Active
Source Idea Path: ideas/open/311_rv64_ordinary_control_expression_completion.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Nearby Runtime Candidates

# Current Packet

## Just Finished

Step 3: Repair The First Semantic Emission Boundary is implemented for RV64
prepared fused-compare `cond_branch` emission. The RV64 prepared function
emitter now looks up the prepared control-flow branch condition for the current
block, verifies it matches the terminator condition, and lowers
`kind=fused_compare` by materializing the prepared compare operands through the
existing prepared value-home helpers before emitting the true branch plus false
fallthrough jump. The prepared path resolves and validates true/false branch
targets through `resolve_prepared_compare_branch_target_labels`, then formats
the RV64 assembly targets from those prepared label IDs. The fallback
simple-compare path remains unchanged.

The repair is semantic rather than filename-shaped. It supports prepared
register, rematerializable immediate, simple immediate, and supported i32 stack
slot homes through the same `emit_move_to_register` path used by nearby RV64
prepared scalar emission. Added emitted-assembly route coverage for both the
local-loaded loop predicate and same-module call-result predicate cases; the
existing prepared-BIR dump coverage remains in place.

## Suggested Next

Execute Step 4: probe nearby RV64 c-testsuite runtime candidates, including
`src/00008.c`, `src/00030.c`, and `src/00105.c`, and classify moved versus
still-failing outcomes by mechanism.

## Watchouts

- Do not implement filename-shaped logic or c-testsuite-specific shortcuts.
- Do not downgrade supported-path expectations or mark cases unsupported.
- Keep stack/local address materialization and external-stub policy separate
  unless they directly block ordinary control/expression completion proof.
- The prepared fused-compare emitter currently relies on the existing RV64
  prepared value-home movement support; unsupported non-i32 or unsupported
  stack/address homes should remain separate blockers rather than being hidden.
- `backend_riscv_prepared_edge_publication` remains the existing out-of-scope
  backend failure in the delegated `^backend_` subset.

## Proof

Delegated proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: build succeeded and `test_after.log` was produced. CTest reported
`217/218` passing. The only failing test is the known out-of-scope
`backend_riscv_prepared_edge_publication` failure:
`RISC-V prepared module should emit a register edge move`.

Supervisor regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Result: PASS. Before `passed=215 failed=1 total=216`, after
`passed=217 failed=1 total=218`, delta `passed=2 failed=0`, and no new failing
tests.

Focused check run before the full subset:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route)_riscv64_prepared_fused_compare'`

Result: `4/4` tests passed, covering the two prepared-BIR dump tests and the
two emitted-assembly route tests for fused-compare branch emission.
Supervisor generated-assembly spot-check for the two focused route cases shows
the expected `bne`/`beq` branches, false jumps, and returns.
