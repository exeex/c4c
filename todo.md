Status: Active
Source Idea Path: ideas/open/167_whole_codebase_string_authority_final_audit.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Covered Structured Domains

# Current Packet

## Just Finished

Step 2 - Classify Covered Structured Domains completed the LinkNameId/LIR/HIR-to-LIR
covered-domain classification packet for `src/codegen/lir/ir.hpp`,
`src/codegen/lir/verify.cpp`, and `src/codegen/lir/hir_to_lir/*.cpp`. No
implementation or test files were edited.

LIR/HIR-to-LIR classification table:

| Owner/domain | File/local symbol or path | Class | Evidence | Bridge boundary / follow-up |
|---|---|---|---|---|
| LIR model link-visible identity | `src/codegen/lir/ir.hpp`: `LirCallOp::direct_callee_link_name_id`, `LirExternDecl::link_name_id`, `LirFunction::link_name_id`, `LirGlobal::link_name_id`, `LirSpecEntry::mangled_link_name_id` | `SA/DO` | Link-visible identity is carried as `LinkNameId`; rendered names remain as final LLVM spelling or display payloads such as `name`, `callee`, `signature_text`, and `mangled_name`. | No `SA` follow-up for the id carriers. Rendered spelling is output/display unless explicitly listed as a compatibility fallback below. |
| LIR extern declaration dedup | `src/codegen/lir/ir.hpp`: `extern_decl_link_name_map`, `extern_decl_name_map`, `record_extern_decl` | `SA/CB` | `record_extern_decl` keys by `LinkNameId` first, upgrades/migrates a prior raw-name entry when a semantic id appears, and uses `extern_decl_name_map` only when the caller has no `LinkNameId`. | `CB` boundary: unresolved external calls or legacy producers with `kInvalidLinkName`. Follow-up: retire `extern_decl_name_map` when all extern-call producers provide `LinkNameId`. |
| LIR aggregate declarations | `src/codegen/lir/ir.hpp`: `LirStructDecl`, `struct_decl_index`, `record_struct_decl`, `find_struct_decl` | `SA` | Structured declarations are indexed by `StructNameId`; rendered `type_decls` are the legacy/printer shadow and verifier parity input. | No `SA` follow-up; the rendered declaration vector is a printer compatibility shadow. |
| LIR aggregate type references | `src/codegen/lir/types.hpp`: `LirTypeRef::struct_type`, `struct_name_id`; `src/codegen/lir/ir.hpp`: `LirTypeRef` fields on globals, externs, calls, signatures, struct fields | `SA/CB` | `LirTypeRef` carries rendered LLVM type text plus optional `StructNameId`; struct references with a known declaration are expected to carry the id mirror. | `CB` boundary: non-aggregate, pointer/array/function fragments, ABI fragments such as `byval(...)`, or legacy rendered aggregate carriers that cannot be normalized to a declared `StructNameId`. |
| LIR verifier struct mirror enforcement | `src/codegen/lir/verify.cpp`: `find_declared_struct_name_id`, `verify_known_struct_type_ref_mirror`, `verify_declared_struct_type_ref_mirror`, call/global/extern/signature mirror checks | `SA` | The verifier rejects known declared struct text without matching `StructNameId`, checks id resolution to a declared struct, and rejects mismatched id/text mirrors. | No bridge is opened after a known structured aggregate miss; text lookup is used to detect and reject missing mirrors. |
| LIR verifier legacy shadows | `src/codegen/lir/verify.cpp`: `legacy_type_decl_name`, `verify_struct_decl_shadows`, `function_signature_line` | `CB` | `legacy_by_name` verifies `struct_decls` still match `type_decls`; `function_signature_line` comment says it only parses final-render payload while semantic signature facts are validated from structured fields. | `CB` boundary: printer shadow/parity checks and final header presence. Follow-up belongs to later bridge retirement once printer no longer needs legacy shadows. |
| HIR-to-LIR aggregate name construction | `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`: `lir_aggregate_structured_name_id`, `lir_field_type_ref`, `lir_global_type_ref`, `lir_signature_type_ref`, `build_type_decls` | `SA/CB` | Owner-key lookup via `typespec_aggregate_owner_key` and `find_struct_def_tag_by_owner` is preferred; compatibility/final spelling is used only when no owner key exists or to normalize rendered text to an existing declared id. | `CB` boundary: legacy `TypeSpec` producers without owner metadata or ABI/final-spelling text that must match a declared `StructNameId`. |
| HIR-to-LIR global/function dedup | `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`: `dedup_globals`, `dedup_functions` | `SA/CB` | Dedup maps prefer `LinkNameId`, then `TextId`, then raw `name` only when no stable ids exist. | `CB` boundary: HIR globals/functions with both `kInvalidLinkName` and `kInvalidText`. Follow-up: eliminate raw-name fallback after HIR materialization guarantees stable ids. |
| HIR-to-LIR call target resolution | `src/codegen/lir/hir_to_lir/call/target.cpp`: `find_local_target_function`, `resolve_call_target_info`, `record_extern_call_decl` | `SA/CB` | Local calls resolve through `mod_.find_function(link_name_id)`; a non-invalid `LinkNameId` miss returns `nullptr` instead of falling back to name; legacy `find_function_by_name_legacy` is reached only when the id is invalid. | `CB` boundary: no-`LinkNameId` call references or builtin alias external names. No complete structured miss reopens rendered lookup. |
| HIR-to-LIR direct call references | `src/codegen/lir/hir_to_lir/call/target.cpp`, `src/codegen/lir/ir.hpp`: `make_lir_call_op_with_return_type_ref`, `LirCallOp::direct_callee_link_name_id` | `SA/DO` | `resolve_call_target_info` propagates `callee_link_name_id` into emitted `LirCallOp`; `callee_val` is the final LLVM symbol spelling. | No `SA` follow-up for direct-call identity; rendered callee text remains printer payload and scan fallback where no id exists. |
| HIR-to-LIR extern finalization | `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`: `finalize_module` | `SA/CB` | Defined functions are filtered by `local_fn_link_names` when an extern decl has `LinkNameId`; fallback filtering through `hir_mod.fn_index.count(fallback_name)` is used only with `kInvalidLinkName`. Return type mirrors are finalized through `extern_return_type_ref`. | `CB` boundary: extern decl records lacking `LinkNameId`. Follow-up: retire fallback-name filtering with `extern_decl_name_map`. |
| HIR-to-LIR global initializer function refs | `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`: `collect_global_init_function_link_name_ids`; `LirGlobal::initializer_function_link_name_ids` | `SA/CB` | Structured initializer references collect `link_name_id`/`fn_link_name_id` and store them on `LirGlobal`; later reachability seeds from these ids. | `CB` boundary: `LirGlobal::init_text` is still scanned for legacy raw LLVM initializer payloads with no structured carrier. |
| HIR-to-LIR discardable function reachability | `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`: `scan_refs`, `collect_inst_refs`, `collect_fn_refs`, `eliminate_dead_internals` | `SA/CB` | Reachability indexes discardable functions by `LinkNameId` first and seeds structured references from `LirCallOp::direct_callee_link_name_id` and `initializer_function_link_name_ids`; comments mark scanned names as compatibility fallback. | `CB` boundary: final LLVM spelling payloads (`signature_text`, call args, operands, `init_text`) without structured reference carriers. Follow-up: add typed symbol-reference carriers for remaining scanned payload producers, then retire `discardable_by_name`/`scan_refs` semantic seeding. |
| HIR-to-LIR aggregate call/vararg/lvalue helpers | `src/codegen/lir/hir_to_lir/call/args.cpp`, `call/target.cpp`, `call/vaarg.cpp`, `lvalue.cpp`, `types.cpp`, `core.cpp`: `*_aggregate_structured_name_id`, `normalize_lir_aggregate_struct_name_id`, `lookup_structured_layout` | `SA/CB` | Helpers derive `StructNameId` through owner keys and normalize only if rendered text matches a declared struct; legacy sites are explicitly named `*-legacy-compat`. | `CB` boundary: aggregate carriers with no owner key, legacy tag-only `TypeSpec`, or ABI layout fragments. No follow-up beyond bridge-retirement planning unless a complete owner-key miss reaches rendered lookup; current inspected paths fail closed or require declared-id normalization. |

Remaining `SA` follow-up recommendation:
- Retire `src/codegen/lir/ir.hpp` `extern_decl_name_map` and the matching
  `src/codegen/lir/hir_to_lir/hir_to_lir.cpp` `finalize_module` fallback once
  extern-call producers reliably pass `LinkNameId`.
- Retire `discardable_by_name`/`scan_refs` reachability seeding after LIR has
  typed symbol-reference carriers for the remaining final-spelling payloads
  currently documented at `signature_text`, call arguments, operands, and
  `init_text`.
- No ordinary rendered string path was found that reopens lookup after a
  complete `LinkNameId` miss or a known declared `StructNameId` miss.

## Suggested Next

Continue Step 2 with the next covered backend/BIR packet: classify BIR and
backend LinkNameId transport, backend symbol registry/lowering, and any
remaining link-visible rendered-name compatibility bridges.

## Watchouts

- `scan_refs` and `discardable_by_name` are still semantic for dead internal
  function retention when LIR references arrive only as final LLVM spelling.
  This is a classified bridge, not display-only text.
- `StructNameId` verifier checks intentionally use rendered text lookup to
  detect missing mirrors; that lookup is rejection logic, not fallback
  authority.
- Do not remove LIR/HIR-to-LIR bridges in this audit step; bridge retirement
  belongs to follow-up planning.

## Proof

Audit replay command run:
`rg -n --glob '!build*/**' --glob '!ideas/closed/**' --glob '!review/**' -e 'LinkNameId' -e 'StructNameId' -e 'link_name_id' -e 'struct_name_id' -e 'extern_decl' -e 'discardable' -e 'rendered_.*compat' -e 'legacy.*compat' -e 'by_name' -e 'signature_text' -e 'initializer_function_link_name_ids' src/codegen/lir/ir.hpp src/codegen/lir/verify.cpp src/codegen/lir/hir_to_lir`

AST-backed inspection used:
`c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/codegen/lir/hir_to_lir/hir_to_lir.cpp build/compile_commands.json`

Result: audit replay and AST function inventory completed. No tests were run
because this packet only recorded classification and did not edit
implementation or tests; `test_after.log` was not updated by this packet.
