Status: Active
Source Idea Path: ideas/open/644_rv64_object_route_stack_parameter_abi_residual.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement One Prepared ABI/Home Path

# Current Packet

## Just Finished

Step 3 from `plan.md` implemented the RV64 object-route prepared ordinary
same-module call path for explicit scalar stack-destination call arguments.

- `fragment_for_prepared_call` now consumes explicit
  `outgoing_stack_argument_area`, `destination_stack_offset_bytes`, and
  `destination_stack_size_bytes` for scalar stack arguments.
- Supported sources in this slice are GPR register, GPR/null/integer
  immediate, scalar frame-slot GPR sources with the active outgoing-area
  offset accounted for, FPR register, and F32/F64 immediate sources.
- The path emits one outgoing stack-area adjustment, stores each scalar value
  to the prepared stack offset, lowers the direct call, then restores `sp`.
- Missing outgoing stack area, missing/malformed stack destination facts,
  out-of-area destinations, unsupported banks, aggregate transports, and
  mismatched FPR stack sizes remain fail-closed.
- Focused object-emission coverage now includes a positive same-module call
  with scalar stack GPR register, GPR immediate, and F64 immediate sources,
  plus negative malformed/missing authority cases.
- `src/20001017-1.c` moved past the prior `unsupported_call_abi` boundary.
  The new first residual is runtime mismatch: clang exits `0`, while the c4c
  binary aborts. Disassembly shows caller `main` now allocates the prepared
  `40`-byte outgoing area and stores stack args at offsets `0`, `8`, `16`,
  `24`, and `32` before calling `bug`; `bug` then aborts while reading a
  stack-passed formal from its post-prologue frame-relative offset
  (`ld t3,56(sp)`) instead of the incoming argument area adjusted by the callee
  frame size.

## Suggested Next

Next packet should classify and repair the callee-side RV64 object-route
stack-passed formal home consumption for ordinary same-module calls. The likely
owner is formal stack-slot entry materialization using explicit prepared callee
homes plus callee frame size, not caller stack-argument production.

## Watchouts

- The caller-side scalar stack-argument path is general over prepared stack
  destination facts and must not be replaced with ABI formula reconstruction.
- The new residual should not be fixed by changing expectations or by
  weakening the caller path; the caller emits the prepared outgoing stack area
  and direct call now.
- The callee-side stack formal offsets seen in the prepared facts are explicit
  homes, but the object route must distinguish callee local frame offsets from
  incoming caller argument area addresses after the callee prologue.

## Proof

Ran the exact supervisor proof command into `test_after.log`:

`rm -f test_after.log && (cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_object_emission|backend_prepare_frame_stack_call_contract|backend_prepared_lookup_helper|backend_prealloc_call_boundary_classification|backend_prepared_object_consumer_contract|backend_call_boundary_effect_plan)$' && ALLOWLIST=build/agent_state/644_step1_20001017_1.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh) > test_after.log 2>&1`

Proof status: build passed; all six focused CTests passed; the one-row torture
probe still failed, but no longer at `unsupported_call_abi`. New result:
`RV64_BACKEND_RUNTIME_MISMATCH`, `clang_exit=0`, `c4c_exit=Subprocess aborted`.
