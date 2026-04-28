# Current Packet

Status: Active
Source Idea Path: ideas/open/122_bir_string_legacy_path_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Carry Structured Identity Through LIR-To-BIR

## Just Finished

Completed the first Step 3 packet by threading the LIR-to-BIR global table into
dynamic global scalar-array materialization. The generated `LoadGlobalInst`
entries now set `global_name_id` from the structured global identity when that
identity is available, while genuinely missing table entries still produce the
existing invalid-id boundary. Focused coverage proves dynamic scalar-array
loads keep the semantic `LinkNameId` even when the LIR global display spelling
has drifted.

## Suggested Next

Continue Step 3 with the next bounded LIR-to-BIR identity-threading packet.
Prefer another ordinary authority family that already has structured identity
available at the lowering boundary, then add focused coverage for drifted
display text or missing-identity rejection.

## Watchouts

Keep this idea focused on BIR and the LIR-to-BIR boundary. Do not absorb MIR,
x86 renderer, parser, or HIR cleanup into this plan. Retained display,
selector, compatibility, and intentionally unresolved strings should be
classified rather than deleted. The verifier intentionally allows stale
fallback text with a valid id when that text does not name another declared
symbol; this preserves display compatibility while rejecting known identity
conflicts.

`clang-format` is not installed in this environment, so formatting was not run.
Dynamic global scalar-array materialization now uses `global_types_` for
`LoadGlobalInst::global_name_id`; do not reclassify that path as a compatibility
unresolved-id boundary unless the global table entry is genuinely unavailable.
For Step 3, do not pull parser or HIR cleanup into the packet; if BIR cannot
receive needed structure yet, record that as an upstream gap rather than
expanding this plan.

## Proof

Ran the delegated proof command:
`cmake -S . -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86 -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_cli_dump_bir_is_semantic|backend_cli_dump_bir_focus_function_filters_00204|frontend_lir_(global_type_ref|extern_decl_type_ref|call_type_ref|function_signature_type_ref))$' > test_after.log 2>&1`

Result: passed, 7/7 tests. Log path: `test_after.log`.

Supervisor acceptance also ran the broader backend bucket:
`ctest --test-dir build-x86 -j --output-on-failure -R '^backend_' > test_after.log 2>&1`

Result: passed, 122/122 tests. Log path: `test_after.log`.
