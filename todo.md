Status: Active
Source Idea Path: ideas/open/195_frontend_to_bir_legacy_string_lookup_closure_gate.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reconstruct Closure Inputs

# Current Packet

## Just Finished

Step 1 - Reconstruct Closure Inputs: reviewed closed dependency evidence from ideas 188, 190-194, and 196 and inspected the frontend-to-BIR string lookup surfaces named by idea 195.

Dependency evidence reviewed:

- `ideas/closed/188_lir_bir_freeze_closure_gate.md`: LIR/BIR freeze ledger closed after classifying retained strings as presentation/output, diagnostics, route-local handles, ABI/final spelling, runtime/intrinsic placeholders, or explicit raw/no-id compatibility. Close notes say no high-risk generated-path string authority remained unclassified and full-suite regression guard passed `3137/3137` before and after.
- `ideas/closed/190_lir_call_argument_structured_payload_boundary.md`: `LirCallOp` carries generated `structured_args`; LIR-to-BIR call lowering uses non-empty structured payloads for operand/type/byval facts, with empty payloads retained as explicit raw/no-metadata compatibility. Focused coverage covered stale rendered call text for direct scalar, direct byval, indirect, and no-signature structured calls.
- `ideas/closed/191_bir_function_signature_byval_metadata_text_retirement.md`: `LirSignatureParam::is_byval` is the structured explicit-byval carrier; generated signatures populate structured byval/type-ref metadata; BIR aggregate parameter collection consumes structured signature metadata and leaves `signature_text` parsing only for raw/no-metadata fixtures.
- `ideas/closed/192_hir_compile_time_rendered_registry_api_retirement_audit.md`: direct consteval call lowering now requires structured callee identity before rendered consteval registry compatibility; remaining rendered registry APIs are compatibility/follow-up surfaces. The highest-value remaining consteval route was split to idea 196 and is now closed.
- `ideas/closed/193_hir_constructor_member_owner_structured_lookup_closure.md`: direct struct constructor lowering uses structured `record_def` or tag/namespace owner metadata; complete structured owner misses fail closed instead of recovering through stale rendered callee spelling. Remaining rendered-owner surfaces were recorded as no-owner/no-metadata compatibility or broader cleanup, not acceptance blockers for the selected route.
- `ideas/closed/194_bir_global_memory_provenance_linknameid_expansion.md`: dynamic global scalar-array materialization carries structured `LinkNameId` provenance into `bir::LoadGlobalInst::global_name_id`; raw spelling lookup remains at the explicit `kInvalidLinkName` compatibility boundary. Tests covered display-spelling drift, unresolved carried-id fail-closed behavior, and no-id compatibility.
- `ideas/closed/196_hir_pending_consteval_structured_identity.md`: pending consteval lowering now carries structured callee identity through delayed materialization and no longer depends on rendered function names when identity metadata is complete. The retained rendered-name path is closed evidence for explicit incomplete-identity compatibility, not a live Step 2/3 blocker.

Inspected frontend-to-BIR surfaces:

- Parser token/name surface: `src/frontend/parser/parser_types.hpp`, `src/frontend/parser/parser_support.hpp`, and `src/frontend/parser/impl/support.cpp` show parser `TextId`/symbol carriers, parser-local record compatibility APIs, TextId const-int/typedef lookup, and explicit rendered named-const/tag compatibility bridges.
- Parser type spelling surface: `src/frontend/parser/impl/types/base.cpp` retains final spelling/rendered template arg helpers and explicit `resolve_record_type_spec_with_parser_tag_map_compatibility` call sites for parser-local compatibility.
- Sema consteval/type utility surface: `src/frontend/sema/type_utils.cpp`, `src/frontend/sema/type_utils.hpp`, `src/frontend/sema/consteval.cpp`, and `src/frontend/sema/consteval.hpp` show enum/const/NTTP lookup by structured key or TextId first, rendered maps fenced as no-metadata compatibility, and rendered type-name equality blocked when structured metadata exists.
- HIR compile-time state surface: `src/frontend/hir/compile_time_engine.hpp` defines `CompileTimeRegistryKey`; `src/frontend/hir/impl/compile_time/engine.cpp` uses structured pending consteval identity when complete and falls back to rendered `fn_name` only when incomplete.
- HIR consteval lowering surface: `src/frontend/hir/impl/expr/call.cpp` uses `ct_state_->find_consteval_def(n->left, n->left->name)`, records `PendingConstevalExpr::callee_identity`, and disables rendered NTTP fallback when a forwarded TextId carrier exists.
- HIR object/member owner surface: `src/frontend/hir/impl/expr/object.cpp`, `src/frontend/hir/impl/stmt/decl.cpp`, `src/frontend/hir/impl/expr/operator.cpp`, `src/frontend/hir/impl/expr/builtin.cpp`, and `src/frontend/hir/hir_types.cpp` contain structured owner-key paths plus explicit no-owner rendered compatibility for legacy aggregate/member/operator/builtin cases.
- HIR-to-LIR call/type metadata surface: `src/codegen/lir/hir_to_lir/call/*.cpp`, `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`, `src/codegen/lir/ir.hpp`, `src/codegen/lir/call_args.hpp`, `src/codegen/lir/call_args_ops.hpp`, and `src/codegen/lir/verify.cpp` show `LirCallSignature`, `LirCallArg`, `arg_type_refs`, `direct_callee_link_name_id`, signature params/type refs, and verifier mirror checks.
- LIR call payload surface: `src/backend/bir/lir_to_bir/calling.cpp` prefers `structured_args`/`callee_signature` in `parse_typed_call` and `parse_direct_global_typed_call`; rendered `callee_type_suffix`/`args_str` parsing remains for raw/no-structured payload compatibility and for selected validation/mirror checks.
- BIR signature/ABI surface: `src/backend/bir/lir_to_bir/aggregate.cpp` and `src/backend/bir/lir_to_bir/call_abi.cpp` use structured signature params when available and reserve `signature_text` parsing for hand-built legacy LIR without structured metadata.
- BIR global provenance/lowering surface: `src/backend/bir/lir_to_bir/globals.cpp`, `src/backend/bir/lir_to_bir/memory/addressing.cpp`, `src/backend/bir/lir_to_bir/memory/provenance.cpp`, `src/backend/bir/lir_to_bir/memory/local_slots.cpp`, `src/backend/bir/bir.hpp`, and `src/backend/prealloc/prealloc.hpp` retain raw global spelling maps alongside `LinkNameId` carriers and no-id compatibility boundaries.

## Suggested Next

Execute `plan.md` Step 2: produce the closure ledger that classifies each retained frontend-to-BIR string surface as structured-authority mirror, display/output text, diagnostic text, route-local handle, ABI/final spelling, or explicit no-metadata compatibility. Include owner/removal-condition rows for every explicit compatibility path.

## Watchouts

- Do not start backend restart work inside this gate.
- Candidate Step 2/3 deeper-audit blockers: remaining `ct_state_->find_template_def(call->left->name)` / rendered template registry calls in HIR template and call lowering; no-owner rendered compatibility in aggregate/member/operator/builtin owner lookup; parser-local `resolve_record_type_spec_with_parser_tag_map_compatibility` and rendered named-const compatibility; BIR global memory/provenance routes that still call `global_types.find(global_name)` with `kInvalidLinkName` or no carried id.

## Proof

Audit/todo-only packet. No validation was run because no code changed, and no `test_after.log` was produced for this no-build proof.
