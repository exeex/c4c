# Current Packet

Status: Active
Source Idea Path: ideas/open/122_bir_string_legacy_path_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Demote Local BIR Semantic String Authority

## Just Finished

Completed Step 2 first symbol-identity packet for idea 122. BIR now carries
`LinkNameId` beside raw display spelling for functions, globals, extern
declarations, direct calls, and lowered global load/store references where LIR
provides link-name identity. LIR-to-BIR no longer rewrites BIR function/global
display names from `LinkNameId`; semantic maps and BIR validation use the id
when it is present. Focused BIR lowering coverage now proves drifted raw
function/global/callee spelling cannot override valid `LinkNameId` identity.

## Suggested Next

Step 2 follow-up packet: tighten or document the remaining symbol-string
boundaries that intentionally still have invalid `LinkNameId`, especially
global initializer symbol names, string constants, runtime/intrinsic callees,
and dynamic global references produced from compatibility-only text paths.
Keep this scoped to classification plus focused verifier/BIR tests; do not
start struct/type layout cleanup.

## Watchouts

Keep this idea focused on BIR and the LIR-to-BIR boundary. Do not absorb MIR,
x86 renderer, parser, or HIR cleanup into this plan. Retained display,
selector, compatibility, and intentionally unresolved strings should be
classified rather than deleted.

`clang-format` is not installed in this environment, so formatting was not run.
Dynamic global scalar-array materialization still has no structured id because
that static helper receives only compatibility-derived access text. Treat it as
a retained unresolved-id boundary unless a later packet deliberately threads
identity into that access record.

## Proof

Ran the delegated proof command:
`cmake -S . -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86 -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_lir_to_bir_notes|frontend_lir_(global_type_ref|extern_decl_type_ref|call_type_ref|function_signature_type_ref))$' > test_after.log 2>&1`

Result: passed, 5/5 tests. Log path: `test_after.log`.

Supervisor acceptance also ran the broader backend bucket:
`ctest --test-dir build-x86 -j --output-on-failure -R '^backend_' > test_after.log 2>&1`

Result: passed, 122/122 tests. Log path: `test_after.log`.
