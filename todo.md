# Current Packet

Status: Active
Source Idea Path: ideas/open/852_hir_canonical_semantic_aggregate_ref_binding.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove complex forms and hand back to 838

## Just Finished

- Completed plan Step 3 coverage packet in `tests/frontend/frontend_hir_tests.cpp`. Added nearby production-path HIR coverage proving populated `QualType::aggregate_ref` for supported complex ordinary free-function signature forms: a typedef alias of a named nested aggregate, a template aggregate instantiation, and nested aggregate typed child structure while preserving the existing named/repeated/pointer-wrapper/fail-closed Step 2 coverage.

## Suggested Next

- Continue plan Step 3 with the downstream forbidden-reconstruction audit plus proportional HIR-to-LIR/backend proof/handoff: confirm ordinary aggregate function-signature consumers do not reconstruct refs from parser pointers, `record_def`, owner keys, tags, rendered text, runtime strings, or `Node*` maps, then record the exact return for 838 Step 2 to consume populated `QualType::aggregate_ref` through the existing HIR-ref-to-LIR-ref intern path.

## Watchouts

- Step 2 intentionally does not migrate LIR/backend producers or consumers. Step 3 should keep the return to 838 bounded to the existing `lir_owned_type_spec` function-signature occurrence producer migration.
- The proportional HIR-to-LIR proof reached the intended downstream boundary: `( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_lir_function_signature_type_ref)$' ) > test_after.log 2>&1` passes `frontend_hir_tests` but aborts `frontend_lir_function_signature_type_ref` with `LIR-owned aggregate function type requires a structured owner key`. Do not repair that inside 852 by reviving legacy reconstruction; hand it to 838's bounded `lir_owned_type_spec` producer migration.
- The helper stores only module-owned `HirAggregateRef` on `QualType`; missing canonical data or absent registered HIR definitions remain fail-closed. Do not add rendered text, tag, owner-key, parser-pointer, `record_def`, runtime-string, or `Node*` downstream recovery.
- Current production sema/HIR evidence did not support adding an anonymous typedef aggregate signature expectation here: the source parses, but the anonymous typedef form did not produce a module-owned HIR aggregate ref under the alias in this path. Local aggregate ordinary free-function signatures are also outside this path because a block-local type cannot be named by a file-scope ordinary function declaration. Template packs and method-specific parameter paths still pass no ordinary free-function `CanonicalFunctionSig` in this packet; keep these as separately scoped limitations instead of widening this packet.

## Proof

- Passed `( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$' ) > test_after.log 2>&1`.
- Proof log: `test_after.log`.
