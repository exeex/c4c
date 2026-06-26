Status: Active
Source Idea Path: ideas/open/406_rv64_object_route_residual_local_memory_boundaries.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Repair Valid Frame-Slot Or Pointer-Value Base-Plus-Offset Lowering

# Current Packet

## Just Finished

Completed the Step 2 first-family repair for `src/20020225-2.c`: valid
prepared frame-slot overlay/sub-width local-memory accesses are no longer
rejected solely because the containing frame slot has stricter alignment than
the narrower access width. `prepared_frame_slot_absolute_offset` still requires
prepared frame-slot base-plus-offset facts, exact access size, access alignment
no larger than the access size, non-negative immediate offset, in-slot range,
in-frame range, and signed-12-bit stack offset.

Focused backend coverage now makes the existing scalar local subobject fixture
use a 4-byte access inside an 8-byte, 8-byte-aligned containing frame slot.

Representative outcome: `src/20020225-2.c` advanced from the prior
`unsupported_local_memory_access: RV64 object route requires prepared
frame-slot or pointer-value base-plus-offset local memory addressing` boundary
to `unsupported_instruction_fragment: BIR instruction requires unsupported
RV64 object lowering`.

`src/ieee/mul-subnormal-single-1.c` remains on the separate F32 local-memory
width/type boundary:
`unsupported_local_memory_access: RV64 object route supports only 1-, 2-, 4-,
and 8-byte prepared local memory accesses`.

## Suggested Next

Use a separate Step 3 repair packet for the F32 local-memory width/type
admission family represented by `src/ieee/mul-subnormal-single-1.c`, without
treating this Step 2 frame-slot overlay admission as proof of that family.

## Watchouts

- The `20020225-2.c` local-memory boundary is repaired, but the file still does
  not object-compile because it now reaches a later generic unsupported
  instruction-fragment boundary.
- The IEEE representative was intentionally not repaired in this packet; avoid
  claiming incidental progress for F32 local-memory width/type admission.
- The accepted overlay route still depends on prepared facts; do not backfill
  missing frame-slot IDs, offsets, sizes, alignment, or range facts in RV64
  object emission.

## Proof

Ran the supervisor-selected proof command exactly:

```sh
cmake --build --preset default && mkdir -p build/agent_state/406_step2_dumps && printf '%s\n' 'src/20020225-2.c' 'src/ieee/mul-subnormal-single-1.c' > build/agent_state/406_step2_frame_slot_overlay.allowlist.txt && { ALLOWLIST=build/agent_state/406_step2_frame_slot_overlay.allowlist.txt STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/406_step2_frame_slot_overlay_probe.log 2>&1 || true; } && rg -n 'unsupported_local_memory_access|unsupported_instruction_fragment|unsupported RV64 object lowering|\[rv64-gcc-torture\]|pass\t|fail\t|RV64_C4C_OBJ|error:' build/agent_state/406_step2_frame_slot_overlay_probe.log && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result: build passed; RV64 gcc_torture probe completed with both allowlisted
representatives still failing but on the expected split boundaries
(`20020225-2.c` advanced to `unsupported_instruction_fragment`,
`mul-subnormal-single-1.c` remained on the F32 local-memory width diagnostic);
backend CTest passed. Canonical proof log: `test_after.log`. Supporting packet
artifacts: `build/agent_state/406_step2_frame_slot_overlay_probe.log`,
`build/agent_state/406_step2_frame_slot_overlay.allowlist.txt`, and
`build/agent_state/406_step2_dumps/`.
