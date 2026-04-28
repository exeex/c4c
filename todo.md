# Current Packet

Status: Active
Source Idea Path: ideas/open/122_bir_string_legacy_path_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Document Retained String Boundaries

## Just Finished

Completed the Step 3 identity-threading sequence far enough to advance to Step
4. The committed Step 3 packets thread structured identity into dynamic global
scalar loads and BIR pointer initializer symbol IDs, after earlier packets had
already threaded ordinary link-name identity through BIR declarations and direct
symbol references. Pointer initializer display text is now retained spelling,
while `initializer_symbol_name_id` is populated when the target resolves through
the existing global or function symbol tables. Focused coverage proves drifted
global/function display spelling cannot override the structured initializer
target identity, and unknown compatibility-only initializer symbols remain valid
only with an invalid `LinkNameId`.

## Suggested Next

Start Step 4 with a bounded retained-string documentation/classification
packet. Update the retained-boundary comments/map for pointer initializer
symbols, especially the stale comments that still say
`initializer_symbol_name_id` remains invalid until identity is threaded, and
audit nearby dynamic global load/store comments so they distinguish retained
display or compatibility spelling from structured `LinkNameId` authority. Keep
the packet documentation/classification-focused; only add or tighten focused
tests if needed to make the retained-string boundary observable.

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

## Proof

Ran the delegated proof command:
`cmake -S . -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86 -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_cli_dump_bir_is_semantic|backend_cli_dump_bir_focus_function_filters_00204|frontend_lir_(global_type_ref|extern_decl_type_ref|call_type_ref|function_signature_type_ref))$' > test_after.log 2>&1`

Result: passed, 7/7 tests. Log path: `test_after.log`.

Supervisor acceptance also ran the broader backend bucket:
`ctest --test-dir build-x86 -j --output-on-failure -R '^backend_' > test_after.log 2>&1`

Result: passed, 122/122 tests. Log path: `test_after.log`.
