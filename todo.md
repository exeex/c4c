Status: Active
Source Idea Path: ideas/open/167_whole_codebase_string_authority_final_audit.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Covered Structured Domains

# Current Packet

## Just Finished

Step 2 - Classify Covered Structured Domains completed the backend/BIR
LinkNameId transport and link-visible rendered-name bridge classification
packet for `src/backend/bir/**`, backend symbol registry/lowering paths, and
the inspected prepared/x86 link-facing bridges. No implementation or test files
were edited.

Backend/BIR classification table:

| Owner/domain | File/local symbol or path | Class | Evidence | Bridge boundary / follow-up |
|---|---|---|---|---|
| BIR model link-visible identity | `src/backend/bir/bir.hpp`: `Global::link_name_id`, `Function::link_name_id`, `CallInst::callee_link_name_id`, `LoadGlobalInst::global_name_id`, `StoreGlobalInst::global_name_id`, `Value::pointer_symbol_link_name_id`, `MemoryAddress::base_link_name_id`, `Global::initializer_symbol_name_id` | `SA/DO` | BIR stores link-visible globals, functions, direct calls, global memory references, symbol pointers, and initializer targets as `LinkNameId` when known; adjacent string fields are documented as final/display or compatibility spelling. | No `SA` follow-up for the id carriers. Rendered `name`, `callee`, `global_name`, `base_name`, and `initializer_symbol_name` remain final spelling or no-id bridge payloads. |
| BIR validation fences | `src/backend/bir/bir_validate.cpp`: `validate_link_name_id`, `find_global`, `find_function`, `validate_named_value`, `validate_call`, `validate_load_global`, `validate_store_global`, `validate_initializer_symbol_link_name` | `SA/CB` | If a `LinkNameId` is present, lookup resolves by id and mismatched rendered names are rejected; by-name lookup is used only when the corresponding id is invalid or to reject an id/name mismatch. | `CB` boundary: raw-only BIR payloads with `kInvalidLinkName`. No complete id miss reopens rendered lookup; unknown id fails validation. |
| BIR module name tables | `src/backend/bir/bir.hpp`: `NameTables::import_link_names`; `src/backend/bir/lir_to_bir/module.cpp`: module setup | `SA` | BIR imports LIR link-name tables before lowering, so ids crossing from LIR to BIR preserve their table authority rather than being rederived from rendered strings. | No `SA` follow-up. |
| LIR-to-BIR function/global/extern registry | `src/backend/bir/lir_to_bir/module.cpp`: `function_symbols`, `global_types`, `function_name_for_reporting`, `global_name_for_identity`, `extern_decl_name_for_identity`; `src/backend/bir/lir_to_bir/lowering.hpp`: `FunctionSymbolSet` | `SA/CB` | Function and extern registry stores a `LinkNameId` set plus a raw-symbol map; globals are keyed by resolved identity spelling while each `GlobalInfo` carries `link_name_id`. | `CB` boundary: LIR declarations with `kInvalidLinkName` and textual pointer initializers not yet normalized at parse boundary. Follow-up: retire `FunctionSymbolSet::raw_symbol_link_name_ids` once all pointer-initializer/function-symbol producers carry ids. |
| LIR-to-BIR direct call lowering | `src/backend/bir/lir_to_bir/calling.cpp`: `lower_call`, `lower_extern_decl`, `lower_decl_function`, `parse_direct_global_typed_call` | `SA/CB` | Lowering copies `LirCallOp::direct_callee_link_name_id` into `bir::CallInst::callee_link_name_id` and copies function/extern declaration ids into BIR declarations; runtime/intrinsic placeholders intentionally keep invalid ids. | `CB` boundary: runtime/intrinsic synthesized calls and direct-call text when LIR did not provide `LinkNameId`. No complete id miss is repaired through text. |
| LIR-to-BIR global and pointer initializer lowering | `src/backend/bir/lir_to_bir/globals.cpp`, `module.cpp`: `lower_minimal_global_impl`, `resolve_initializer_symbol_link_name_id`, `resolve_pointer_initializer_offsets`, `apply_resolved_pointer_initializer_value_ids` | `SA/CB` | Global definitions copy `global.link_name_id`; initializer function ids flow through `initializer_function_link_name_ids` to `initializer_symbol_name_id` and `Value::pointer_symbol_link_name_id`. Textual initializer parsing is retained only to import legacy LIR final spelling. | `CB` boundary: LIR `init_text` and aggregate pointer fields whose symbol names are parsed before full normalization. Follow-up: add typed initializer symbol carriers earlier and retire raw initializer symbol lookup. |
| BIR memory/provenance lowering | `src/backend/bir/lir_to_bir/memory/*.cpp`: `GlobalAddress::link_name_id`, `link_name_id_for_global`, `try_lower_*global*`, `named_symbol_pointer`, `global_name_id` stores | `SA/CB` | Provenance and memory helpers propagate known global/function ids into `MemoryAddress`, `Value`, `LoadGlobalInst`, and `StoreGlobalInst`; raw-name probes are used only when resolving legacy textual operands or no-id global addresses. | `CB` boundary: LIR memory operands and parsed pointer text with no structured id at the boundary. Follow-up aligns with typed initializer/global-address carriers. |
| LIR-to-BIR string constants | `src/backend/bir/lir_to_bir.cpp`: `collect_lowered_string_constants`, `rewrite_direct_call_string_pointer_args`; `src/backend/bir/lir_to_bir/globals.cpp`: `lower_string_constant_global` | `CB/DO` | String-pool names are tracked by `TextId`/display spelling and explicitly do not carry `LinkNameId`; rewrite logic maps string pointer args to private data labels, not source/link semantic identity. | `CB` boundary: string-pool constants are addressable data labels. Potential follow-up belongs to route-local/string-pool identity cleanup, not Step 2 source/link authority. |
| BIR printer and dumps | `src/backend/bir/bir_printer.cpp`: `render_value`, `render_call_target`, `render_block_label`, `render_memory_address`, `render_function` | `DO/CB` | Printer renders already-lowered BIR payloads and prefers structured block labels when available; call-target and address text is dump/final spelling after semantic lookup. | `CB` boundary: raw-only BIR payloads lacking ids. No lookup authority is created by printing. |
| Prepared-stage LinkNameId transfer | `src/backend/prealloc/prealloc.cpp`: `resolve_symbol_pointer_name`, `resolve_direct_callee`; `src/backend/prealloc/prealloc.hpp`: `PreparedNameTables::link_names`, `resolve_prepared_bir_link_name_ref`, `bir_link_name_or_raw`, materialized global ref helpers; `src/backend/prealloc/stack_layout/coordinator.cpp`: `build_direct_symbol_backed_address` | `SA/CB` | Prepared lowering resolves BIR ids through BIR link-name tables, interns authoritative names into prepared link tables, and rejects id/name drift; raw names are accepted only when the BIR id is invalid. | `CB` boundary: BIR payloads with `kInvalidLinkName` and string-pool/private-label paths. No complete id miss falls back to raw spelling. |
| x86 link-visible final rendering | `src/backend/mir/x86/abi/abi.cpp`: `render_asm_symbol_name`; `src/backend/mir/x86/module/data.cpp`, `module.cpp`: `.globl`, `.type`, labels, direct calls, data initializers | `DO/CB` | x86 emission receives logical names already chosen by BIR/prepared layers and applies target ABI spelling, e.g. Darwin underscore decoration; same-module checks use rendered names only as final backend contract checks. | `CB` boundary: final object/assembly spelling and same-module compatibility checks. Follow-up: keep `LinkNameId` available up to prepared/module contracts where supported; do not treat ABI rendering as semantic source authority. |

Remaining `SA` follow-up recommendation:
- Retire `FunctionSymbolSet::raw_symbol_link_name_ids` and the matching raw
  function-symbol probes in `src/backend/bir/lir_to_bir/**` after LIR/BIR
  pointer-initializer and global-address producers provide typed `LinkNameId`
  carriers at the parse/lowering boundary.
- Add typed initializer/global-address symbol carriers before retiring
  `GlobalInfo::initializer_symbol_name`, `Global::initializer_symbol_name`,
  and raw parsed pointer-initializer lookups.
- Consider a later string-pool/private-label identity follow-up for BIR string
  constants; it is separate from source/link-visible semantic authority.
- No backend/BIR path inspected here reopens rendered-name lookup after a
  complete `LinkNameId` miss; id/name mismatch paths fail closed or return no
  prepared/lowered result.

## Suggested Next

Continue Step 2 with the next covered-domain audit packet chosen by the
supervisor, likely any remaining parser/sema/HIR surfaces not yet recorded in
the Step 2 classification or a consolidation pass before Step 3.

## Watchouts

- `FunctionSymbolSet::raw_symbol_link_name_ids` is a real compatibility bridge,
  not display-only text; it remains semantic for raw LIR pointer/function
  symbol payloads that lack `LinkNameId`.
- BIR validators intentionally use rendered by-name lookup to reject
  id/display mismatches and support raw-only payloads. That lookup should not
  be counted as fallback after a complete id miss.
- x86 `render_asm_symbol_name` is final ABI spelling, not source identity.
  Same-module rendering checks should remain classified as final backend
  contract checks unless they feed new lookup authority.
- Do not edit implementation/tests for this audit-only packet; bridge
  retirement belongs to follow-up planning.

## Proof

Audit replay command run:
`rg -n --glob '!build*/**' --glob '!ideas/closed/**' --glob '!review/**' -e 'LinkNameId' -e 'link_name_id' -e 'pointer_symbol_link_name_id' -e 'callee_link_name_id' -e 'global_name_id' -e 'initializer_symbol_name_id' -e 'FunctionSymbolSet' -e 'raw_symbol_link_name_ids' -e 'resolve_direct_callee' -e 'resolve_prepared_bir_link_name_ref' -e 'render_asm_symbol_name' -e 'find_.*by_name' -e 'fallback_by_name' src/backend/bir src/backend/prealloc src/backend/mir/x86`

AST-backed inspection used:
`c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/bir/lir_to_bir.cpp build/compile_commands.json`
`c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/bir/lir_to_bir/module.cpp build/compile_commands.json`
`c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/bir/lir_to_bir/globals.cpp build/compile_commands.json`

Result: audit replay and AST function inventory completed. No tests were run
because this packet only recorded classification and did not edit
implementation or tests; `test_after.log` was not updated by this packet.
