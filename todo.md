Status: Active
Source Idea Path: ideas/open/292_reopen_286_288_match_load_local_memory_admission.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair the admitted local-memory shape

# Current Packet

## Just Finished

Step 3 trace-only packet complete for the downstream `myprintf` blocker after
the formal pointer-parameter repair.

Current delegated CLI subset still fails, and both failing tests now report:
semantic lir_to_bir function `myprintf` failed in `load local-memory semantic
family`.

The first rejected shape is not the earlier `match` formal pointer-param load
family. A minimized AArch64 non-HFA aggregate `va_arg` (`struct s7`) lowers
through semantic BIR, including the local aggregate `memcpy` and reload. A
minimized AArch64 HFA `va_arg` (`struct hfa11 { float a; }`) reproduces the
same `load local-memory semantic family` failure.

First rejected generated HFA load shape:

```llvm
%t10 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 2
%t11 = load ptr, ptr %t10
%t12 = getelementptr i8, ptr %t11, i32 %t2
%t13 = getelementptr i8, ptr %t12, i64 0
%t14 = load float, ptr %t13
```

In full `myprintf`, this is the `%hfa11` branch equivalent after the `%7s` and
`%9s` aggregate va_arg paths. The pointer comes from the AArch64 `va_list`
`vr_top` register-save-area field, is advanced through `i8` GEPs, and reaches
`try_lower_addressed_pointer_load` without typed HFA lane/load authority.

## Suggested Next

Implement the smallest semantic repair for AArch64 HFA `va_arg` register-save
loads. First target: the AArch64 va_arg/register-save-area pointer publication
path around `emit_aarch64_vaarg_hfa` and the semantic local-memory
`PointerAddress`/addressed-load admission route, so byte-GEP-derived HFA lane
loads carry structured lane type authority instead of looking like opaque
`i8`/`Void` pointer loads.

## Watchouts

- This is a separate AArch64 `va_list`/HFA register-save-area rule, not another
  formal pointer parameter metadata-propagation fix.
- Do not add named `00204.c`, `myprintf`, or format-string matching logic.
- The non-HFA aggregate `va_arg` path already lowers in a minimized probe; avoid
  disturbing its raw pointer `memcpy`/aggregate-reload behavior.
- `llvm.va_start.p0` is currently accepted as a compatibility call; the failing
  HFA register lane load needs typed source-address authority before or during
  the `load float`, not expectation weakening.

## Proof

Delegated proof command:
`cmake --build --preset default && ctest --test-dir build -R '^(backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$' --output-on-failure`

Proof log:
`test_after.log`.

Result: build passed; both delegated CLI tests still fail with latest function
`myprintf` in `load local-memory semantic family`.

Additional trace-only probes, both outside the repo root:

- Minimal AArch64 `struct s7` aggregate `va_arg`: `./build/c4cll --dump-bir --target aarch64-linux-gnu /tmp/c4c-vaarg-s7-*.c` passed.
- Minimal AArch64 `struct hfa11 { float a; }` `va_arg`: `./build/c4cll --dump-bir --target aarch64-linux-gnu /tmp/c4c-vaarg-hfa-*.c` failed with `load local-memory semantic family`.

The delegated proof is sufficient for this trace-only packet because it
reproduces the current red subset and preserves the canonical log path.
