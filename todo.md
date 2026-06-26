Status: Active
Source Idea Path: ideas/open/400_rv64_object_route_local_memory_addressing_edges.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify Local-Memory Addressing Forms

# Current Packet

## Just Finished

Step 1 classification-only packet completed for the local-memory
representatives. The fixed proof probe still reports
`unsupported_local_memory_access` for all three allowlisted cases; no
implementation or expectation files were changed.

Representative facts:

- `src/20000722-1.c`: current diagnostic is
  `requires prepared frame-slot or pointer-value base-plus-offset local memory
  addressing`. Prepared dump shows the earlier `bar` blocker is a string
  constant address load, not the later `foo` pointer-field access:
  `bar` inst 7 has base `string_constant`, result `@.str0`, symbol `.str0`,
  offset 0, size 8, align 8, no frame slot, range
  `proven_out_of_bounds`; prepared notes also say the address materialization
  for `@.str0` is missing a structured symbol `LinkNameId`. The following
  local stores use frame slots #0/#1/#2-#5 with in-bounds 8/4/1-byte facts, and
  `foo` has valid pointer-value accesses through `%p.p` offset 8, size 4,
  align 4, range `unknown_compatible`, source home `%p.p` in `a0`.
- `src/20030910-1.c`: current diagnostic is
  `supports only 1-, 2-, 4-, and 8-byte prepared local memory accesses` because
  the RV64 object scalar memory-width helper rejects BIR `double` before address
  lowering. Prepared facts are coherent for target lowering: `main` frame size
  32/align 8; `%lv.dc.0` object slot #0 offset 0 size 8 align 8; `%lv.dp`
  scratch/home slot #2 offset 16 size 8 align 8; inst 0/1 access slot #2
  offset 0 size 8 align 8 range `proven_in_bounds`; inst 2 stores `double`
  through pointer value `%t1` offset 0 size 8 align 8 range
  `unknown_compatible`; inst 3 loads `double` from slot #0 offset 0 size 8
  align 8 range `proven_in_bounds`. Source/value homes: `%lv.dc.0` in `t0`,
  `%t1` in `s1`, `%t4` in FPR `ft0`.
- `src/20020225-2.c`: current diagnostic is also the `supports only 1-, 2-,
  4-, and 8-byte` local-memory message on the 8-byte `double` store, but the
  prepared local storage facts are contradictory. `test` frame size is 16/align
  8; union storage is published as eight separate one-byte fixed slots
  `%lv.a.0` through `%lv.a.7` with slot #0 offset 0 size 1 align 8; accesses
  against slot #0 are size 8 for stored `%t1` and size 4 for the later int
  store/load, all with range `proven_out_of_bounds`. Source home `%t1` is FPR
  `ft0`; `%t4` is GPR `s1`. This is a producer/layout fact split, not a valid
  target-emission lowering.

## Suggested Next

First implementation packet: target-lower valid RV64 object-route `F64`
prepared local-memory load/store forms for complete in-bounds prepared facts,
centered on `src/20030910-1.c`. The packet should add the missing F64 memory
width and FPR/immediate movement path only when the prepared access is coherent
under the existing contract; it must keep `src/20020225-2.c` on a diagnostic
boundary instead of emitting from its out-of-bounds one-byte union slots.

Suggested proof command for that packet:

```bash
cmake --build --preset default && mkdir -p build/agent_state/400_step1_dumps && printf '%s\n' 'src/20000722-1.c' 'src/20030910-1.c' 'src/20020225-2.c' > build/agent_state/400_step1_local_memory.allowlist.txt && { ALLOWLIST=build/agent_state/400_step1_local_memory.allowlist.txt STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/400_step1_local_memory_probe.log 2>&1 || true; } && rg -n 'unsupported_local_memory_access|unsupported_instruction_fragment|unsupported RV64 object lowering|\[rv64-gcc-torture\]|pass\t|fail\t|RV64_C4C_OBJ|error:' build/agent_state/400_step1_local_memory_probe.log && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

## Watchouts

- Do not reopen idea 401's scalar compare/trunc or floating-cast work here.
- Treat `src/20020225-2.c` as local-memory/addressing evidence only after the
  floating cast producer has already lowered.
- Do not make F64 width admission alone sufficient for emission: the
  `20020225-2.c` union case proves that 8-byte size plus frame-slot base can
  still be invalid when the prepared range verdict and slot shape disagree.
- `src/20000722-1.c` is not blocked first on its valid `%p.p + 8` pointer-value
  facts. Its current boundary is a string-constant local address materialization
  with missing structured symbol identity; route that as a producer/global
  address-materialization split unless a prepared fact already exposes a target
  authority for RV64 object emission.
- Existing same-bucket logs still show width-neighbor cases
  `src/loop-8.c`, `src/strct-pack-1.c`, and
  `src/ieee_mul-subnormal-single-1.c`; use them as follow-up probes only when
  the supervisor broadens beyond this packet's three-case proof.
- Route wide rematerialized-immediate producer admission to
  `ideas/open/404_prepared_wide_rematerialized_immediate_admission.md`.
- Route scalar `ashr`/bitfield-global lowering to the existing instruction
  fragment idea, `ideas/open/395_rv64_object_route_instruction_fragment_lowering.md`.

## Proof

Step 1 classification proof:

```bash
cmake --build --preset default && mkdir -p build/agent_state/400_step1_dumps && printf '%s\n' 'src/20000722-1.c' 'src/20030910-1.c' 'src/20020225-2.c' > build/agent_state/400_step1_local_memory.allowlist.txt && { ALLOWLIST=build/agent_state/400_step1_local_memory.allowlist.txt STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/400_step1_local_memory_probe.log 2>&1 || true; } && rg -n 'unsupported_local_memory_access|unsupported_instruction_fragment|unsupported RV64 object lowering|\[rv64-gcc-torture\]|pass\t|fail\t|RV64_C4C_OBJ|error:' build/agent_state/400_step1_local_memory_probe.log && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result: build was up to date; the allowlist probe reported all three
representatives failing with `unsupported_local_memory_access`; backend CTest
passed 326/326. Logs and dumps:
`build/agent_state/400_step1_local_memory_probe.log`,
`build/agent_state/400_step1_local_memory.allowlist.txt`,
`build/agent_state/400_step1_dumps/*.semantic-bir.txt`,
`build/agent_state/400_step1_dumps/*.prepared-bir.txt`, and `test_after.log`.
