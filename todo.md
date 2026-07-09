Status: Active
Source Idea Path: ideas/open/644_rv64_object_route_stack_parameter_abi_residual.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement One Prepared ABI/Home Path

# Current Packet

## Just Finished

Step 3 from `plan.md` repaired RV64 object-emission call-argument scheduling
for scratch-based FPR immediate materialization versus pending scalar stack
arguments.

- `fragment_for_prepared_call` now flushes pending scalar stack arguments that
  use the FPR-immediate helper scratch GPR before materializing that FPR
  immediate, instead of letting `fmv.[wd].x` setup clobber a still-needed stack
  argument source.
- The flush is local to the proven scratch path and leaves sret, byval/aggregate
  stack-copy sequencing, ordinary register moves, and ABI lane placement
  unchanged.
- Focused object-emission coverage now builds an F64 immediate register
  argument followed by an I64 stack argument sourced from `t0`, and proves the
  stack source is stored before `t0` is reused for `fmv.d.x`.
- `src/20001017-1.c` advanced past the prior `%p.C` clobber: `main` now stores
  `%p.C` from `t0` to outgoing stack offset `8` before reusing `t0` for the
  `b` F64 immediate.

## Suggested Next

Next packet should repair the remaining RV64 local frame-address argument source
materialization gap for `src/20001017-1.c`: `main` consumes pointer argument
source registers `t0`, `s1`, and `s2` for `C`, `A`, and `B` without visible
materialization of those local array frame addresses before the call.

## Watchouts

- The prior scratch-clobber residual is gone. In the fresh disassembly, `main`
  emits `mv t3,t0; sd t3,8(sp)` before the `fmv.d.x fa1,t0` materialization for
  `b`.
- The remaining runtime mismatch is not ABI lane placement or the FPR-immediate
  stack-source clobber. The fresh `main` still starts the call setup with
  `mv a0,t0`, later uses `mv a5,s1`, `mv a7,s2`, and stores `t0` to `8(sp)`,
  but there is no visible setup making `t0`/`s1`/`s2` point at the local arrays
  `C`/`A`/`B` before those uses.
- Keep the next fix semantic: repair local frame-address argument source
  materialization/publication for prepared register-source call arguments. Do
  not hard-code `src/20001017-1.c`, `bug`, register names, or argument indexes.

## Proof

Ran the exact supervisor proof command into `test_after.log`:

`rm -f test_after.log && (cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_object_emission|backend_prepare_frame_stack_call_contract|backend_prepared_lookup_helper|backend_prealloc_call_boundary_classification|backend_prepared_object_consumer_contract|backend_call_boundary_effect_plan)$' && ALLOWLIST=build/agent_state/644_step1_20001017_1.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh) > test_after.log 2>&1`

Proof status: build passed; all six focused CTests passed; the one-row torture
probe still failed with `RV64_BACKEND_RUNTIME_MISMATCH`, `clang_exit=0`, and
`c4c_exit=Subprocess aborted`. The fresh residual owner is RV64 local
frame-address call-argument source materialization/publication for the pointer
arguments, not ABI lane placement or scratch-based FPR immediate clobbering.
The exact proof log is `test_after.log`.
