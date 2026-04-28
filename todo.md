# Current Packet

Status: Active
Source Idea Path: ideas/open/122_bir_string_legacy_path_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Document Retained String Boundaries

## Just Finished

Completed Step 4's retained-string documentation/classification packet. Updated
the BIR and LIR-to-BIR comments that still described pointer initializer
`initializer_symbol_name_id` as future-only: pointer initializer spelling is now
documented as retained display text, while known global/function targets gain
structured `LinkNameId` authority during module lowering. Also audited nearby
dynamic global load/store and string-pool comments so known dynamic globals are
classified as LinkNameId-backed and unresolved string-pool names remain an
intentional compatibility-only boundary.

## Suggested Next

Have the supervisor decide whether Step 4 needs a plan-owner/reviewer
completion check or can advance to the next lifecycle packet. No additional
code or test-expectation tightening was needed for this documentation slice.

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
`LoadGlobalInst::global_name_id`, and pointer global initializers now resolve
`initializer_symbol_name_id` from the same structured global identity plus the
function symbol table. Do not reclassify either path as a compatibility
unresolved-id boundary unless the structured table entry is genuinely
unavailable. The current reason to advance is documentation drift, not an
identified remaining Step 3 implementation family. For Step 4, do not pull
parser or HIR cleanup into the packet; if BIR cannot receive needed structure
yet, record that as an upstream gap rather than expanding this plan.

This packet intentionally left `tests/backend/backend_lir_to_bir_notes_test.cpp`
unchanged because the existing focused coverage already observes the retained
display spelling versus structured `LinkNameId` authority split for direct
globals, dynamic scalar-array loads, pointer initializers, and unresolved
compatibility-only initializer symbols.

## Proof

Ran the delegated proof command:
`cmake -S . -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86 -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_cli_dump_bir_is_semantic|backend_cli_dump_bir_focus_function_filters_00204)$' > test_after.log 2>&1`

Result: passed, 3/3 tests. Log path: `test_after.log`.
