Status: Active
Source Idea Path: ideas/open/195_frontend_to_bir_legacy_string_lookup_closure_gate.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Audit High-Risk Generated Paths

# Current Packet

## Just Finished

Step 3 - Audit High-Risk Generated Paths: audited the three Step 2 ledger blockers and split the two remaining HIR blockers into separate follow-up ideas before this gate can close.

Source-path evidence:

- HIR rendered template registry calls remain unproven and are now tracked by `ideas/open/201_hir_template_registry_structured_generated_paths.md`. `CompileTimeState::find_template_def(const Node*, rendered_name)` in `src/frontend/hir/compile_time_engine.hpp` fails closed on complete structured misses through `find_structured_node_entry`, but representative generated call paths still use string-only `find_template_def(name)`/`has_template_def(name)`: `src/frontend/hir/impl/expr/call.cpp` template-call lowering, `src/frontend/hir/impl/templates/deduction.cpp` deduction result inference, `src/frontend/hir/hir_build.cpp` collection/seed/deferred instantiation, and `src/frontend/hir/impl/compile_time/engine.cpp` deferred retry bookkeeping. `Module::find_template_def_by_rendered_preservation_name` is preservation-only in the retry path, but the preceding string-only `ct_state_` checks are metadata-capable and not proven no-metadata-only.
- HIR generated member payload extraction remains unproven and is now tracked by `ideas/open/202_hir_generated_member_payload_structured_miss.md`. In `src/frontend/hir/impl/expr/scalar_control.cpp`, the generated-member path enters only after `structured_owner_key_from_qualified_ref` succeeds, then extracts `member_name`/`generated_member_name` from rendered qualified spelling and adds those candidates. Lookup tries structured `HirStructMemberLookupKey` first, but then falls through to rendered `find_struct_static_member_decl(lookup_owner, candidate.name)` and `find_struct_static_member_const_value(lookup_owner, candidate.name)`. That means a complete structured owner/member miss can still be repaired by rendered owner/member spelling.
- Representative BIR present-id/global-id miss paths are closed for this audit. `FunctionSymbolSet` in `src/backend/bir/lir_to_bir/lowering.hpp` explicitly separates `contains_link_name_id` from raw symbol fallback and documents that present `LinkNameId` metadata must not fall through raw maps. Direct calls in `src/backend/bir/lir_to_bir/calling.cpp` fail the semantic family when metadata-rich direct calls lack `callee_signature` or when a signature-bearing call's `direct_callee_link_name_id` is absent from `function_symbols`. BIR validation in `src/backend/bir/bir_validate.cpp` resolves globals/functions by id when `LinkNameId`/`global_name_id` is present and only uses raw names when the id is invalid, including global load/store and named pointer values.

Lifecycle state:

- Idea 195 remains the active plan as a blocked closure gate. Do not advance it
  to closure or backend restart readiness until ideas 201 and 202 are resolved
  or the supervisor explicitly switches active work to one blocker idea.

## Suggested Next

Before `plan.md` Step 4 closure, supervisor should choose whether to switch
active lifecycle state to `ideas/open/201_hir_template_registry_structured_generated_paths.md`
or `ideas/open/202_hir_generated_member_payload_structured_miss.md`. Idea 195
should remain active only as the blocked gate if no switch is requested.

## Watchouts

- Do not advance this gate as closure-complete until the two HIR blockers have separate open ideas or equivalent supervisor-approved lifecycle handling.
- The two blocker ideas now exist, but idea 195 is still not closure-ready
  while their underlying HIR blockers remain unresolved.
- BIR raw maps remain classified as explicit raw/no-id compatibility; the representative present-id paths audited here did not expose a raw-map fallthrough blocker.
- Backend restart work remains out of scope until Step 4 records validation and a restart-readiness decision.

## Proof

Audit/todo-only packet. No validation was run because no code changed, and no `test_after.log` was produced for this no-build proof.
