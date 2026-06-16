Status: Active
Source Idea Path: ideas/open/292_reopen_286_288_match_load_local_memory_admission.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair the admitted local-memory shape

# Current Packet

## Just Finished

Step 3 implementation packet complete for AArch64 HFA `va_arg`
register-save-area lane-load admission.

The semantic memory route now publishes AArch64 `__va_list_tag_.vr_top` loads
as FP register-save-area runtime pointers, preserves that authority through
`i8` byte GEPs, skips conflicting dynamic-local-aggregate handling for that
marked pointer family, and admits `F32`/`F64`/`F128` lane loads through the
addressed pointer-load path.

The repaired generated shape is the HFA register path:

```llvm
%vr_top_ptr = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 2
%vr_top = load ptr, ptr %vr_top_ptr
%reg_base = getelementptr i8, ptr %vr_top, i32 %offs
%lane_src = getelementptr i8, ptr %reg_base, i64 <lane-offset>
%lane = load <fp-lane-type>, ptr %lane_src
```

No named `00204.c`, `myprintf`, or format-string logic was introduced, and no
expected outputs were weakened.

## Suggested Next

Supervisor should decide whether Step 3 is complete enough to move to the next
plan step or request a focused review of the AArch64 `va_list` pointer-fact
publication route before commit.

## Watchouts

- The repair is intentionally scoped to AArch64 `__va_list_tag_.vr_top` FP
  register-save-area pointers and HFA lane scalar loads.
- The full delegated 286/288 CLI subset is now green; this removes the known
  downstream blocker that kept idea 291 on lifecycle hold.
- `PointerAddress` now carries route-local AArch64 FP register-save-area facts;
  any future pointer-phi propagation for this fact should be reviewed before
  widening beyond the current direct load/GEP path.

## Proof

Delegated proof command:
`cmake --build --preset default && ctest --test-dir build -R '^(backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$' --output-on-failure`

Proof log:
`test_after.log`.

Result: passed. Both delegated CLI tests are green.

Additional temporary probes outside the repo root:

- Minimal AArch64 `struct hfa11 { float a; }` aggregate `va_arg` now lowers
  through semantic BIR.
- Minimal AArch64 `struct hfa12 { float a; float b; }` aggregate `va_arg` now
  lowers through semantic BIR, including the 16-byte register-save lane stride.

Supervisor broader guard:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: regression guard passed. Before/after comparison moved from 175
passed, 5 failed to 177 passed, 3 failed, resolving the two target 286/288 CLI
tests with no new failures. The remaining failures are the pre-existing
AArch64 byval stack-overflow route cases and pointer-value named scalar
writeback route case.
