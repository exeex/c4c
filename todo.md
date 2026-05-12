Status: Active
Source Idea Path: ideas/open/183_lir_bir_backend_freeze_authority_audit.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Map blockers to follow-up ideas

# Current Packet

## Just Finished

Step 3 mapped the Step 2 freeze blockers to existing follow-up ideas 184-188.

Freeze-blocker ownership matrix:

| Classified blocker family | Existing follow-up owner | Coverage decision |
| --- | --- | --- |
| Direct-call rendered signature parsing on generated metadata-rich calls, including return type, parameter type, variadic, byval, and sret facts. | `ideas/open/184_direct_call_signature_metadata_structured_boundary.md` | Covered. Idea 184 explicitly requires generated direct calls to use structured signature facts and fences rendered-signature parsing to no-metadata compatibility. |
| Direct-call aggregate byval/sret/layout lookup through rendered type or layout fragments. | `ideas/open/184_direct_call_signature_metadata_structured_boundary.md` plus shared layout boundary in `ideas/open/185_lir_to_bir_global_typedecl_compatibility_fence.md` | Covered. Idea 184 owns the call-lowering use site; idea 185 owns the shared `LirTypeRef` / `StructNameId` / structured-layout compatibility fence. |
| `GlobalTypes` string-keyed global lookup on generated globals and pointer initializer symbols after `LinkNameId` is available. | `ideas/open/185_lir_to_bir_global_typedecl_compatibility_fence.md` and `ideas/open/186_bir_direct_symbol_identity_validation_closure.md` | Covered. Idea 185 owns the LIR-to-BIR global table fence; idea 186 owns direct symbol identity validation and raw fallback closure. |
| `TypeDeclMap` and rendered aggregate type lookup on generated metadata-rich layout paths. | `ideas/open/185_lir_to_bir_global_typedecl_compatibility_fence.md` | Covered. Idea 185 explicitly owns `TypeDeclMap`, `BackendStructuredLayoutTable`, raw/no-id compatibility separation, and stale/missing structured metadata rejection. |
| BIR direct call/global load/store/function/global declaration raw symbol fallback with present or expected `LinkNameId`. | `ideas/open/186_bir_direct_symbol_identity_validation_closure.md` | Covered. Idea 186 owns `CallInst`, `LoadGlobalInst`, `StoreGlobalInst`, declarations, placeholder compatibility, and generated metadata-rich fail-closed validation. |
| BIR pointer initializer and symbol pointer value identity when final spelling and `LinkNameId` disagree or when metadata is missing. | `ideas/open/186_bir_direct_symbol_identity_validation_closure.md`; related memory provenance cleanup in `ideas/open/187_bir_memory_provenance_global_handle_cleanup.md` | Covered. Idea 186 owns direct symbol references and pointer initializers; idea 187 covers downstream global-provenance handle use. |
| Memory/provenance global maps keyed by final spelling after `LinkNameId` is available. | `ideas/open/187_bir_memory_provenance_global_handle_cleanup.md` | Covered. Idea 187 owns global-provenance maps, separates route-local SSA/slot names from global symbol identity, and depends on 185/186. |
| Prealloc same-module or symbol-backed prepared-address raw spelling fallback. | `ideas/open/186_bir_direct_symbol_identity_validation_closure.md` for upstream direct-symbol identity and `ideas/open/188_lir_bir_freeze_closure_gate.md` for final confirmation | Covered. Step 2 found no separate prealloc-specific blocker; the remaining requirement is to confirm the raw fallback stays compatibility/display-only at the closure gate. |
| Prealloc route-local function, block, value, slot, frame-slot, and join-transfer names. | `ideas/open/188_lir_bir_freeze_closure_gate.md` | Covered as a closure confirmation item, not a repair blocker. These are route-local or output labels unless they cross into global symbol identity. |

Coverage conclusion: ideas 184-188 cover all freeze blockers found by the
Step 1 inventory and Step 2 classification. No additional idea is needed before
backend restart based on the current audit record.

## Suggested Next

Proceed to Step 4: prepare audit closeout in `todo.md`, stating that idea 183
can be handed to lifecycle review after the supervisor confirms whether to
close the audit or activate one of ideas 184-188 next.

## Watchouts

- Do not treat the Step 3 coverage conclusion as backend freeze closure; idea
  188 owns the final milestone gate after ideas 184-187 complete.
- Any later discovery of generated metadata-rich string authority outside the
  families above should become a new open idea before backend restart.
- Prealloc route-local strings remain acceptable only while they stay
  route-local/output labels and do not become global symbol identity.

## Proof

Source-level audit only. Compared the Step 2 classification in `todo.md`
against follow-up scopes in `ideas/open/184_*.md` through
`ideas/open/188_*.md`. Ran `git diff --check -- todo.md`; no build/tests were
run because only `todo.md` changed.
