Status: Active
Source Idea Path: ideas/open/406_rv64_object_route_residual_local_memory_boundaries.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify Residual Local Memory Rejections

# Current Packet

## Just Finished

Completed Step 1 classification for `src/20020225-2.c` and
`src/ieee/mul-subnormal-single-1.c` using fresh RV64 gcc_torture backend probe
artifacts and prepared dumps under `build/agent_state/406_step1_*`.

`src/20020225-2.c` still fails with
`unsupported_local_memory_access: RV64 object route requires prepared frame-slot
or pointer-value base-plus-offset local memory addressing`. The rejected local
accesses are in `test` on frame slot `#0` for `%lv.a.0`: inst 1 stores
`double %t1` at offset 0, size 8, align 8, `base=frame_slot`,
`base_plus_offset=yes`, `range_verdict=proven_in_bounds`; inst 2 stores `i32 1`
at offset 0, size 4, align 4; inst 3 loads `i32 %t4` at offset 0, size 4,
align 4. The slot is object `#0`, size 8, align 8,
`address_exposed=yes`, `requires_home_slot=yes`, `permanent_home_slot=yes`.
The 8-byte store is admissible; the first 4-byte overlay access is rejected by
`prepared_frame_slot_absolute_offset` in
`src/backend/mir/riscv/codegen/object_emission.cpp` because it requires
`slot_it->align_bytes <= size_bytes`, so an 8-byte-aligned slot fails a 4-byte
access even though the prepared access itself is aligned and in bounds.

`src/ieee/mul-subnormal-single-1.c` still fails with
`unsupported_local_memory_access: RV64 object route supports only 1-, 2-, 4-,
and 8-byte prepared local memory accesses`. The rejected family is 4-byte
`float` local memory on frame slots with coherent prepared facts: `u2f`
slot `#0` `%lv.u.0`, offset 0, size 4, align 4, address-exposed permanent
home, with an `i32` store followed by a `float` load; `f2u` slot `#5`
`%lv.u.0`, offset 0, size 4, align 4, address-exposed permanent home, with a
`float` store followed by an `i32` load; `tstmul` slots `#10` `%lv.x` and
`#11` `%lv.y` have non-address-exposed 4-byte float store/load pairs. These
reach `rv64_local_memory_size_for_type` through
`diagnose_unsupported_prepared_instruction_fragment`, but `F32` is not admitted
there, so the diagnostic is a target-side width/type admission gap, not a
producer fact gap.

Nearby same-family cases found in current backend artifacts: `src_strct-pack-1.c`
and `src_loop-8.c` currently report the same local-memory width diagnostic;
many other current object-route failures report the frame-slot/base-plus-offset
diagnostic, but they were not broadened into this packet.

## Suggested Next

Use separate repair packets. First packet should repair the concrete
target-side fact family for `src/20020225-2.c`: frame-slot overlay/sub-width
access admission where prepared facts prove base `frame_slot`, offset, access
size, access alignment, and in-bounds range, but the containing slot has a
larger alignment than the access width.

## Watchouts

- The two representatives do not share one repair route:
  `20020225-2.c` is a frame-slot sub-width overlay admission issue;
  `mul-subnormal-single-1.c` is an F32 local-memory width/type admission issue.
- No producer-fact gap was found for either representative: prepared base,
  offset, size, alignment, and range facts are explicit and coherent.
- For `20020225-2.c`, do not relax addressing to ignore range or slot bounds;
  the narrow issue is the target helper comparing slot alignment against access
  size instead of validating the prepared access alignment/range contract.
- For the IEEE case, do not treat the 4-byte float overlay as an integer-only
  memory access; it needs semantic F32 local load/store lowering or an explicit
  target-side split.

## Proof

Ran the supervisor-selected proof command exactly:

```sh
cmake --build --preset default && mkdir -p build/agent_state/406_step1_dumps && printf '%s\n' 'src/20020225-2.c' 'src/ieee/mul-subnormal-single-1.c' > build/agent_state/406_step1_local_memory.allowlist.txt && { ALLOWLIST=build/agent_state/406_step1_local_memory.allowlist.txt STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/406_step1_local_memory_probe.log 2>&1 || true; } && rg -n 'unsupported_local_memory_access|unsupported_instruction_fragment|unsupported RV64 object lowering|\[rv64-gcc-torture\]|pass\t|fail\t|RV64_C4C_OBJ|error:' build/agent_state/406_step1_local_memory_probe.log && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Probe result: both representatives fail with the expected current
`unsupported_local_memory_access` diagnostics; `ctest -R '^backend_'` passed
326/326. Canonical proof log: `test_after.log`. Supporting packet artifacts:
`build/agent_state/406_step1_local_memory_probe.log`,
`build/agent_state/406_step1_local_memory.allowlist.txt`, and
`build/agent_state/406_step1_dumps/*.prepared-bir.txt`.
