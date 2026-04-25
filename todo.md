Status: Active
Source Idea Path: ideas/open/105_hir_to_lir_text_id_bridge_inventory_and_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Prefer LinkNameId And TextId For Symbol Identity Handoffs

# Current Packet

## Just Finished

Completed `plan.md` Step 2 cleanup for the HIR-to-LIR function/global dedup
seam in `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`. `dedup_globals` and
`dedup_functions` now prefer valid `LinkNameId` maps, then valid
`name_text_id` maps for invalid-LinkNameId entries, and use raw rendered `name`
maps only when both stable ids are invalid. Existing selection behavior is
preserved: globals still prefer initialized definitions, functions still prefer
definitions over declarations, and lowering still copies the original
spelling/link id into emitted LIR fields without changing printed symbol
spelling.

## Suggested Next

Continue Step 2 with the next low-risk identity handoff only if the supervisor
selects one. The function/global dedup fallback now has the intended stable-id
ordering; avoid crossing into printer output, type refs, const-init payload
bytes, or dead-internal rendered operand scanning without a separate packet.

## Watchouts

- Do not remove rendered fallback or change emitted spelling while this bridge
  cleanup is in progress.
- Do not absorb HIR-internal legacy lookup cleanup already covered by idea 104.
- Keep printer-only, diagnostic, ABI/link spelling, and literal-byte strings
  separate from semantic identity handoffs.
- Do not downgrade tests or expectations to claim bridge cleanup progress.
- Dead-internal elimination still scans rendered `@name` strings and indexes
  functions by `LirFunction::name`; treat it as a later cleanup because it
  crosses printed operand scanning and reachability behavior.
- Type string cleanup is higher risk than symbol dedup because `LirTypeRef`,
  `type_decls`, `signature_text`, and const-init aggregate text currently feed
  printer/backend compatibility.

## Proof

Ran the supervisor-selected proof exactly and saved full output to
`test_after.log`:
`cmake --build build --target c4cll && ctest --test-dir build -R '^(frontend_hir_tests|positive_sema_ok_call_target_resolution_runtime_c|positive_sema_inline_call_discovery_c|cpp_positive_sema_namespace_global_var_runtime_cpp|cpp_positive_sema_namespace_function_call_runtime_cpp|cpp_positive_sema_anon_namespace_fn_lookup_cpp|cpp_llvm_spec_key_metadata|cpp_llvm_forward_pick_specialization_metadata)$' --output-on-failure`.

Result: green. `c4cll` rebuilt successfully and all 7 selected tests passed.
