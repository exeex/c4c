Status: Active
Source Idea Path: ideas/open/644_rv64_object_route_stack_parameter_abi_residual.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement One Prepared ABI/Home Path

# Current Packet

## Just Finished

Step 3 from `plan.md` repaired the RV64 object-route callee-side consumption
path for stack-passed scalar formal homes after callee frame allocation.

- Fused pointer/integer branch operands now validate stack-passed scalar formal
  homes against prepared value-home, frame-slot, stack-object, and formal ABI
  facts before reading from the incoming argument area adjusted by the callee
  frame size.
- Scalar `LoadLocalInst` formal reads gained the same fail-closed
  stack-passed-formal path while ordinary local frame slots continue using their
  prepared local frame offsets.
- Focused object-emission coverage now proves a stack-passed pointer formal
  branch operand reads from `frame_size + incoming_stack_offset`, and a separate
  local-memory fixture proves ordinary local frame slots are not reclassified as
  incoming arguments.
- `src/20001017-1.c` advanced past the callee-side formal-home consumption
  mismatch: `bug` now reads `%p.C` from `ld t3,120(sp)`, i.e. callee frame
  `112` plus incoming formal stack offset `8`, instead of the old local home
  `56(sp)`.

## Suggested Next

Next packet should classify the remaining `src/20001017-1.c` runtime mismatch
as a caller-side RV64 mixed integer/FPR stack-argument production issue: caller
`main` still stores the pointer argument `%p.C` at outgoing stack offset `24`,
while the callee-side ABI/formal path now expects it at incoming offset `8`.

## Watchouts

- The accepted callee-side repair validates local homes but computes the
  incoming stack offset from stack-passed formal ABI facts; do not revert it to
  using the local spill-slot home offset.
- The remaining caller-side residual appears to involve mixed integer/FPR
  argument placement: the caller stores stack arguments at `0`, `8`, `16`,
  `24`, and `32`, while the callee expects `%p.C` at incoming offset `8`.
- Do not weaken the caller-side stack-argument path or rewrite expectations;
  the next slice should repair the prepared caller destination facts/ABI
  classification that placed `%p.C` at offset `24`.

## Proof

Ran the exact supervisor proof command into `test_after.log`:

`rm -f test_after.log && (cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_object_emission|backend_prepare_frame_stack_call_contract|backend_prepared_lookup_helper|backend_prealloc_call_boundary_classification|backend_prepared_object_consumer_contract|backend_call_boundary_effect_plan)$' && ALLOWLIST=build/agent_state/644_step1_20001017_1.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh) > test_after.log 2>&1`

Proof status: build passed; all six focused CTests passed; the one-row torture
probe still failed at runtime mismatch. New result remains
`RV64_BACKEND_RUNTIME_MISMATCH`, `clang_exit=0`, `c4c_exit=Subprocess aborted`,
but the callee-side offset moved from the old incorrect `ld t3,56(sp)` to
`ld t3,120(sp)`. The exact proof log is `test_after.log`.
