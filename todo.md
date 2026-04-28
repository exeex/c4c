# Current Packet

Status: Active
Source Idea Path: ideas/open/122_bir_string_legacy_path_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Demote Local BIR Semantic String Authority

## Just Finished

Completed Step 2 follow-up symbol-boundary packet for idea 122. BIR verifier
now rejects ordinary global load/store, direct-call, and pointer-initializer
symbol paths when a present `LinkNameId` is paired with fallback text that names
a different declared global/function. The remaining intentionally unresolved
boundaries are documented in code: pointer initializer symbols parsed from
legacy LIR text, string-pool compatibility globals, runtime/intrinsic
placeholder callees, and dynamic scalar-array materialization paths.

## Suggested Next

Step 2 appears ready for supervisor review/acceptance. Suggested next packet:
have the supervisor decide whether to accept this Step 2 follow-up as complete
or route a reviewer pass over the symbol-string boundary classification before
moving to the next plan step.

## Watchouts

Keep this idea focused on BIR and the LIR-to-BIR boundary. Do not absorb MIR,
x86 renderer, parser, or HIR cleanup into this plan. Retained display,
selector, compatibility, and intentionally unresolved strings should be
classified rather than deleted. The verifier intentionally allows stale
fallback text with a valid id when that text does not name another declared
symbol; this preserves display compatibility while rejecting known identity
conflicts.

`clang-format` is not installed in this environment, so formatting was not run.
Dynamic global scalar-array materialization still has no structured id because
that static helper receives only compatibility-derived access text. Treat it as
a retained unresolved-id boundary unless a later packet deliberately threads
identity into that access record.

## Proof

Ran the delegated proof command:
`cmake -S . -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86 -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_cli_dump_bir_is_semantic|backend_cli_dump_bir_focus_function_filters_00204|frontend_lir_(global_type_ref|extern_decl_type_ref|call_type_ref|function_signature_type_ref))$' > test_after.log 2>&1`

Result: passed, 7/7 tests. Log path: `test_after.log`.

Supervisor acceptance also ran the broader backend bucket after the verifier
and test-target wiring changes:
`ctest --test-dir build-x86 -j --output-on-failure -R '^backend_' > test_after.log 2>&1`

Result: passed, 122/122 tests. Log path: `test_after.log`.
