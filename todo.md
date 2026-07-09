Status: Active
Source Idea Path: ideas/open/644_rv64_object_route_stack_parameter_abi_residual.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement One Prepared ABI/Home Path

# Current Packet

## Just Finished

Step 3 from `plan.md` repaired RV64 fixed-arity scalar ABI lane assignment for
mixed integer/FPR ordinary calls and matching formal homes.

- RV64 ordinary C stack-pressure assignment now tracks independent GPR and FPR
  scalar lanes, so hard-float F32/F64 arguments do not consume GPR register or
  outgoing stack lanes.
- Prepared call destination and fixed formal register-index helpers now use the
  same independent RV64 GPR/FPR lane rule for non-variadic scalar ABI facts.
- Focused contract coverage now proves a mixed RV64 scalar call places the
  eighth GPR argument in `a7`, keeps FPR scalars in `fa0`/`fa1`, publishes GPR
  stack arguments at compact offsets `0` and `8`, and leaves a malformed
  missing-ABI stack argument fail-closed.
- `src/20001017-1.c` advanced past the caller/callee ABI placement mismatch:
  prepared facts now place `%p.B` in `a7`, `fdB` at outgoing stack offset `0`,
  `b` in `fa1`, `%p.C` at outgoing stack offset `8`, and `fdC` at offset `16`.

## Suggested Next

Next packet should repair RV64 object-emission call-argument materialization so
FPR immediate construction does not clobber a later stack argument source
register before that source is stored to the outgoing argument area.

## Watchouts

- The ABI placement is now correct for the 20001017-1 call: `main` allocates a
  24-byte outgoing area and `bug` reads `%p.C` from callee frame size `96` plus
  incoming offset `8`, i.e. `ld t3,88(sp)`.
- The remaining runtime mismatch is not an offset disagreement. Current `main`
  materializes `b` into `t0` for `fmv.d.x fa1,t0`, then stores `t0` to
  `8(sp)` for `%p.C`; that writes the double bit-pattern instead of the `%p.C`
  pointer.
- Keep the next fix semantic: preserve or schedule call argument source
  registers across scratch-based immediate/FPR materialization. Do not hard-code
  `src/20001017-1.c`, `bug`, or argument indexes.

## Proof

Ran the exact supervisor proof command into `test_after.log`:

`rm -f test_after.log && (cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_object_emission|backend_prepare_frame_stack_call_contract|backend_prepared_lookup_helper|backend_prealloc_call_boundary_classification|backend_prepared_object_consumer_contract|backend_call_boundary_effect_plan)$' && ALLOWLIST=build/agent_state/644_step1_20001017_1.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh) > test_after.log 2>&1`

Proof status: build passed; all six focused CTests passed; the one-row torture
probe still failed with `RV64_BACKEND_RUNTIME_MISMATCH`, `clang_exit=0`, and
`c4c_exit=Subprocess aborted`. The fresh residual owner is RV64 object-emission
call-argument source preservation during scratch-based FPR immediate
materialization, not ABI lane placement. The exact proof log is
`test_after.log`.
