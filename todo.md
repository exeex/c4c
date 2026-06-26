Status: Active
Source Idea Path: ideas/open/401_rv64_object_route_scalar_and_floating_edge_lowering.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair Or Split Floating-Cast Lowering

# Current Packet

## Just Finished

Step 3 - Repair Or Split Floating-Cast Lowering repaired the
`unsupported_floating_cast` representative through reusable RV64 object
integer-to-floating cast lowering.

Implemented semantic prepared cast lowering for `SIToFP`/`UIToFP` from `I32`
and `I64` sources to `F32`/`F64` FPR destinations. The object emitter now moves
integer sources through an existing GPR value path when needed, then emits the
appropriate `fcvt.{s,d}.{w,wu,l,lu}` instruction directly into the prepared FPR
destination. Focused backend coverage now includes `SIToFP i32 0 -> double`
homed in `ft0`, encoded as `addi t0, zero, 0; fcvt.d.w ft0, t0, rne; ret`.

The delegated representative no longer fails with `unsupported_floating_cast`.
The Step 3 probe now advances to a separate local-memory/addressing boundary:
`src/20020225-2.c` fails with
`unsupported_local_memory_access: RV64 object route supports only 1-, 2-, 4-, and 8-byte prepared local memory accesses`.
Concrete evidence from the prepared dump: `%t1 = bir.sitofp i32 0 to double`
is homed in FPR `ft0`, and `store_source function=test block=entry inst=1`
publishes `%t1` as a cast producer for `bir.store_local %lv.a.0, double %t1`.
The prepared addressing fact for that store is `size=8 align=8` on frame slot
`#0`, while the stack layout models the union storage as fixed one-byte slots
and marks the access `range_verdict=proven_out_of_bounds`.

## Suggested Next

Next coherent packet should split from floating-cast lowering into the local
aggregate/union memory route: decide whether RV64 object local memory should
support FPR `F32`/`F64` stores/loads to prepared frame-slot accesses, and whether
the prepared stack/addressing facts for fixed byte-slot union storage need a
producer-side repair before object emission can safely accept the 8-byte store.

## Watchouts

- Do not special-case `src/20020225-2.c`, `test`, constants, or the union shape.
- The current probe failure is no longer `unsupported_floating_cast`; it is a
  local-memory/addressing split after the cast producer has been lowered.
- Even after the local-memory boundary is repaired, this representative contains
  `%t5 = bir.ashr i32 %p.x, %t4`; scalar `ashr` is likely a later separate
  object-lowering gap, matching the Step 2 split evidence for shift lowering.
- Keep any follow-up ABI-coherent: integer-to-FP conversion should remain GPR
  source to FPR destination, not bit-moving through FPR move helpers.

## Proof

Ran the supervisor-selected Step 3 proof exactly:

```bash
cmake --build --preset default && mkdir -p build/agent_state && printf '%s\n' 'src/20020225-2.c' > build/agent_state/401_step3_floating_cast.allowlist.txt && { ALLOWLIST=build/agent_state/401_step3_floating_cast.allowlist.txt STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/401_step3_floating_cast_probe.log 2>&1 || true; } && rg -n 'unsupported_floating_cast|unsupported_instruction_fragment|unsupported RV64 object lowering|\[rv64-gcc-torture\]|pass\t|fail\t|RV64_C4C_OBJ|error:' build/agent_state/401_step3_floating_cast_probe.log && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result: build succeeded; focused backend coverage is included in backend CTest;
backend CTest passed and is recorded in `test_after.log`. Probe log
`build/agent_state/401_step3_floating_cast_probe.log` reports total=1,
passed=0, failed=1 for `src/20020225-2.c`, but the failure is now
`unsupported_local_memory_access`, not `unsupported_floating_cast`.
