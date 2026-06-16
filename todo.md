Status: Active
Source Idea Path: ideas/open/292_reopen_286_288_match_load_local_memory_admission.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add focused proof and restore CLI subset

# Current Packet

## Just Finished

Step 4 proof packet complete for AArch64 HFA `va_arg` register-save-area
lane-load admission.

Added focused notes-test coverage that constructs the repaired shape without
depending on `00204.c`, `myprintf`, or CLI expected files:

```llvm
%vr.top.ptr = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 2
%vr.top = load ptr, ptr %vr.top.ptr
%lane.ptr = getelementptr i8, ptr %vr.top, i64 16
%lane = load float, ptr %lane.ptr
```

The focused assertion verifies the semantic BIR route: `%vr.top` loads from the
`__va_list_tag_.vr_top` field slot, `%lane.ptr` is materialized as a pointer add
rooted at `%vr.top`, and the `F32` lane load carries pointer-value address
metadata with base `%vr.top`, offset `16`, and size `4`.

## Suggested Next

Supervisor should decide whether idea 292 is ready for closure/review now that
the focused proof and restored 286/288 plus relevant x86 00204 CLI subset are
green.

## Watchouts

- The focused test intentionally proves the semantic lane-load shape directly
  and does not depend on the named `00204.c`/`myprintf` path.
- No CLI expected files, existing CTest filters, lifecycle files, or production
  implementation files were changed in this packet.

## Proof

Delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$'`

Proof log:
`test_after.log`.

Result: passed. `backend_lir_to_bir_notes`, the exact 286/288 AArch64 CLI
subset, and the relevant x86 00204 semantic/prepared CLI coverage are green.
