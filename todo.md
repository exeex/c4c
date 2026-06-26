Status: Active
Source Idea Path: ideas/open/400_rv64_object_route_local_memory_addressing_edges.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Lower Valid Prepared Local-Memory Forms

# Current Packet

## Just Finished

Step 2 implementation packet completed for valid prepared RV64 object-route
`F64` local-memory lowering. The object route now lowers coherent F64
frame-slot and pointer-value local accesses through `fld`/`fsd`, uses prepared
FPR homes for named F64 values, materializes immediate F64 bit patterns through
GPR-to-FPR movement for local stores, and rejects frame-slot accesses whose
selected prepared slot cannot contain the requested access size.

Diagnostic movement in the fixed three-case probe:

- `src/20030910-1.c` moved off the F64 local-memory width diagnostic to
  `unsupported_terminator_fragment: BIR terminator requires unsupported RV64
  object lowering`, proving the coherent F64 local store/load forms are no
  longer the blocker.
- `src/20020225-2.c` remains rejected at
  `unsupported_local_memory_access: RV64 object route requires prepared
  frame-slot or pointer-value base-plus-offset local memory addressing` because
  the prepared slot facts still publish one-byte union slots for 8-byte and
  4-byte accesses.
- `src/20000722-1.c` remains rejected at the existing string-constant/local
  address-materialization local-memory boundary.

## Suggested Next

Next coherent packet: route the new `src/20030910-1.c`
`unsupported_terminator_fragment` boundary to the existing terminator/compare
work instead of expanding local-memory lowering. Keep `src/20020225-2.c`
blocked on producer/layout facts until the contradictory one-byte union slots
are repaired upstream.

## Watchouts

- The frame-slot offset helper now checks selected slot containment, not only
  whole-frame containment. This is what keeps `src/20020225-2.c` fail-closed.
- Do not treat the new F64 local-memory width admission as permission for F32,
  global F64 memory, byval F64, or sret pointer F64 forms.
- Do not reopen idea 401's scalar compare/trunc or floating-cast work from this
  packet; `src/20030910-1.c` has moved to a later terminator boundary.
- Treat `src/20020225-2.c` as local-memory/addressing evidence only after the
  floating cast producer and union slot layout facts have been repaired.
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

Step 2 implementation proof:

```bash
cmake --build --preset default && mkdir -p build/agent_state/400_step1_dumps && printf '%s\n' 'src/20000722-1.c' 'src/20030910-1.c' 'src/20020225-2.c' > build/agent_state/400_step1_local_memory.allowlist.txt && { ALLOWLIST=build/agent_state/400_step1_local_memory.allowlist.txt STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/400_step1_local_memory_probe.log 2>&1 || true; } && rg -n 'unsupported_local_memory_access|unsupported_instruction_fragment|unsupported RV64 object lowering|\[rv64-gcc-torture\]|pass\t|fail\t|RV64_C4C_OBJ|error:' build/agent_state/400_step1_local_memory_probe.log && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result: build succeeded; the allowlist probe reported all three representatives
still failing, with `src/20030910-1.c` moved to
`unsupported_terminator_fragment`, `src/20020225-2.c` retained at
`unsupported_local_memory_access`, and `src/20000722-1.c` retained at
`unsupported_local_memory_access`. Backend CTest passed and wrote
`test_after.log`. Logs and dumps:
`build/agent_state/400_step1_local_memory_probe.log`,
`build/agent_state/400_step1_local_memory.allowlist.txt`,
`build/agent_state/400_step1_dumps/*.semantic-bir.txt`,
`build/agent_state/400_step1_dumps/*.prepared-bir.txt`, and `test_after.log`.
