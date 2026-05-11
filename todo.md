Status: Active
Source Idea Path: ideas/open/167_whole_codebase_string_authority_final_audit.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Classify Compatibility Bridges and Retirement Candidates

# Current Packet

## Just Finished

Step 4 - Classify Compatibility Bridges and Retirement Candidates is complete.
No implementation, test, plan, source-idea, review, or closed-idea files were
edited. The focused audit classified retained rendered-string bridges and
retirement candidates from parser/sema support, HIR declaration/record indexes,
template/specialization mirrors, final-output scan boundaries, and LIR/BIR/
prealloc bridge candidates carried forward from Step 2.

Compatibility bridge inventory:

| Owner/domain | Bridge / files | Caller class | Exact compatibility boundary | Can complete structured miss reach it? | Retirement condition | Feed idea 168? |
| --- | --- | --- | --- | --- | --- | --- |
| Parser record/type support | `src/frontend/parser/parser_support.hpp`, `src/frontend/parser/impl/support.cpp`: `resolve_record_type_spec`, `eval_const_int` record-layout tag maps | Parser-local record layout probes, HIR proof paths, tests | Direct `record_def` and structured record metadata are preferred; rendered `compatibility_tag_map` is retained for callers without `record_def` or owner keys. | No for structured record carriers: direct `record_def`/metadata paths fail closed before rendered recovery. Yes only for no-metadata compatibility callers. | All parser/HIR callers carry `record_def`, owner key, or typed layout carrier into the helper. | Yes: production parser/HIR compatibility bridge, plus tests proving closed misses. |
| Parser typedef/type equality support | `parser_support.hpp`, `support.cpp`: `resolve_typedef_chain` and `types_compatible_p` `TextId` vs rendered overloads | Parser-owned builtin type compatibility, HIR/template proof callers, tests | `TextId` overloads are authoritative; rendered `std::string` typedef overloads are explicitly legacy/HIR proof bridges. | No for `TextId` callers: header states structured typedef miss must not recover through rendered fallback. Rendered overload remains reachable only when caller chooses compatibility map. | Legacy/HIR proof paths pass typedef `TextId`s or typed HIR bindings. | Yes: production compatibility overloads should be narrowed/retired separately from parser diagnostics. |
| Sema static integer enum lookup | `src/frontend/sema/type_utils.hpp`, `type_utils.cpp`: `StaticEvalIntEnumLookupInput::rendered_enum_consts`, `static_eval_int` | Static-eval enum constants from parser/HIR scalar evaluation | Lookup tries structured key, then `TextId`, then rendered map only when those carriers are absent; rendered map is global and has no local/block enum lifetime. | No: a structured-key or text miss returns `Miss` and does not continue into rendered lookup. | All static-eval enum callers provide scoped structured/text carriers where scope matters; remaining global-only wrapper is deleted or test-only. | Yes, if production callers still pass only rendered enum maps; otherwise regression-guard lane only. |
| Sema canonical/template argument mirrors | `src/frontend/sema/canonical_symbol.cpp`: `fallback_name_bindings`, `canonical_leaf_display_spelling`, `format_canonical_type`, ABI mangling | Sema canonical symbol builder and ABI text renderer | Owner/index/text metadata is authority for template params; fallback name bindings and leaf display spellings cover incomplete metadata and display/ABI text. | No for complete template metadata; fallback is keyed by display spelling only when structured binding lookup is unavailable. | Canonical template args always carry owner/index/text metadata; ABI renderer remains output-only. | Partial: only `fallback_name_bindings` is a bridge candidate; display/ABI text is not. |
| Sema/consteval template binding mirrors | `src/frontend/sema/consteval.cpp`: `lookup_type_binding_by_text`, `lookup_type_binding_by_key`, `*_bindings_by_text`, `*_binding_keys_by_name`, `bind_consteval_call_env` | Consteval call-env binding/substitution | Rendered parameter spelling selects a text/key mirror; authority lives in `TextId` or structured binding maps. Invalid text/key maps return `Miss`. | No: present mirrors with invalid/missing text/key return `Miss`, and complete metadata does not recover through rendered binding names. | Typedef/template TypeSpecs carry binding text/key metadata directly through consteval, so rendered-name mirrors are unnecessary. | Yes: production bridge, but likely grouped with template-binding cleanup rather than route-local idea 169. |
| HIR module function/global declaration indexes | `src/frontend/hir/hir_ir.hpp`: `fn_index`, `global_index`, `find_*_by_name_legacy`, `classify_*_decl_lookup`, `resolve_*_decl` | HIR expression resolution, calls/operators/range-for methods, inspect printer | `LinkNameId`, `ConcreteGlobalId`, and structured `ModuleDeclLookupKey` win; rendered `fn_index`/`global_index` is the legacy fallback. | Mostly no: when a structured key exists, same declaration text is known, and rendered compatibility is not explicitly allowed, lookup returns null. Yes for self-consistent rendered names and rendered qualified import compatibility. | All module decl refs carry `LinkNameId`, concrete id, or complete qualifier/text metadata; import/raw rendered qualified compatibility is isolated or deleted. | Yes: central HIR rendered-registry bridge-retirement candidate. |
| HIR record owner/definition indexes | `hir_ir.hpp`: `struct_defs`, `struct_def_owner_index`; `src/frontend/hir/hir_types.cpp`: `find_base_struct_def_for_layout`, `find_struct_def_by_layout_compatibility_tag`, `lookup_record_layout` | HIR layout, member/method lookup, consteval record layout, codegen/dumps | `HirRecordOwnerKey` and owner index are structured authority; `struct_defs` by rendered tag is retained for codegen/dumps and no-owner-index handoffs. | No where owner index exists: consteval layout returns null after owner-index miss; fallback rendered lookup is only for absent owner index or incomplete metadata. | Every layout/member/method path provides complete owner keys and the HIR module always has owner index at these call sites. | Yes: production record/layout bridge; keep dump/codegen ordering separate. |
| HIR template definition and static-member rendered maps | `src/frontend/hir/impl/lowerer.hpp`, `templates/*`, `compile_time_engine.hpp`: `template_struct_defs_`, `template_defs`, `template_struct_defs_by_owner_`, deferred NTTP lookup maps | Template lowering, deferred instantiation, compile-time engine | Structured owner maps exist beside rendered template/struct maps; rendered maps support legacy template origin names and no-owner metadata. | No for complete owner-key paths that check `template_struct_defs_by_owner_`; yes for legacy origin/name paths and deferred static-member lookup that only carries rendered template names. | Template origins, deferred NTTP references, and static-member lookups always carry owner key plus parameter binding metadata. | Yes: production template bridge, probably same lane as idea 168/structured template retirement. |
| HIR specialization display keys | `src/frontend/hir/hir_ir.hpp`: `SpecializationKey::canonical`, `SpecializationOwnerIdentity::display_name`, `make_specialization_key`, `canonical_type_str`; `struct_instantiation.cpp` owner identity mirrors | Template specialization identity/display | Structured owner/argument identity is present; `canonical` string and display name remain compatibility/display payloads until all record owners carry full structured specialization identity. | No for structured specialization identity comparisons; yes only where code still indexes or serializes by `canonical`/display key. | Specialization lookup and owner records use structured owner/argument identity end-to-end; canonical string remains dump-only. | Yes for any production lookup keyed by `canonical`; no for metadata/dump output. |
| Final-output signature/global initializer payloads | `src/codegen/lir/ir.hpp`: `LirFunction::signature_text`, `LirGlobal::init_text`; `src/codegen/lir/lir_printer.cpp`; `src/codegen/lir/verify.cpp`; `src/backend/bir/lir_to_bir/{call_abi,aggregate,global_initializers}.cpp` | LIR printer/verifier, LIR-to-BIR ABI and initializer parsers | `LinkNameId`, structured signature params/type refs, and initializer function ids are authority when available; scans of final LLVM text are producer-boundary compatibility. | Not a structured semantic miss path; scans remain reachable because final LLVM text is still the payload for unstructured producer pieces. | Function headers, ABI params, aggregate types, and initializers are represented structurally through LIR-to-BIR; verifier only parity-checks text mirrors. | Yes: bridge-retirement lane should fence or retire production scans; output printing itself is not a bug. |
| LIR struct declaration/type mirror verifier | `src/codegen/lir/verify.cpp`: `verify_struct_decl_shadows`, `legacy_type_decl_name`, signature type-ref mirror checks | LIR verifier/parity checks | Structured `LirStructDecl`/`StructNameId` must match legacy `type_decls` and signature text mirrors; mismatch is verifier failure. | Not a recovery path; it detects disagreement rather than falling back. | Legacy `type_decls` shadow is removed after structured struct declarations fully drive rendering. | Maybe: feed idea 168 only if retiring rendered registry mirrors includes verifier shadow removal. |
| LIR extern/function identity fallback | `src/codegen/lir/ir.hpp`: `extern_decl_name_map`; `src/backend/bir/lir_to_bir.cpp`: `LirFunctionIdentityLookup::fallback_by_name` | Extern declaration dedup and post-lowering call/string rewrite matching | Dedup/matching uses `LinkNameId` first; raw name fallback exists only when the producer has not populated `LinkNameId`. | No if `LinkNameId` is present; yes for raw-only LIR functions/extern decls. | HIR-to-LIR and imported/raw LIR producers populate `LinkNameId` for every link-visible function/extern declaration. | Yes: production backend bridge from Step 2. |
| BIR link-visible symbol fallback | `src/backend/bir/bir.hpp`, `src/backend/bir/bir_validate.cpp`: `Global/Function/CallInst` names plus `*_link_name_id`, `find_*_by_name`, initializer symbol validation | BIR validator and global/call lowering | Known symbols carry `LinkNameId`; raw names are retained for unresolved compatibility/display and for parity checks that name and id agree. | No when id is present and mismatched: validation fails. Yes when id is invalid by design for runtime/intrinsic/raw compatibility calls. | All user/extern/global symbol references have `LinkNameId`; runtime/intrinsic placeholders are separated into typed builtin/runtime ids or remain explicitly display-only. | Yes for user/extern/global fallback; no for runtime/intrinsic display tokens. |
| BIR string constants and direct-call string rewrite | `src/backend/bir/lir_to_bir.cpp`: `LoweredStringConstantMetadata`, `bytes_by_name`, string alias collection; `src/backend/bir/bir.hpp`: `StringConstant::name` | LIR-to-BIR generated string-pool lowering and call argument rewrite | String-pool names are generated/private and intentionally not `LinkNameId`; TextId table narrows byte lookup, but addressable BIR values still use generated string names. | Not a source/link structured miss path; generated data labels remain route-local compatibility names. | Introduce typed/generated data-label ids or structured string-constant refs across LIR-to-BIR/prealloc. | No for idea 168 if scoped to rendered source-name bridges; feed idea 169 or generated-data cleanup. |
| Prepared stack/value/slot bridges from Step 2 | `src/backend/prealloc/prealloc.hpp`, `prealloc.cpp`, `stack_layout/coordinator.cpp`, `alloca_coalescing.cpp`: `ValueNameId`, `SlotNameId`, `find_regalloc_value_by_name`, `find_direct_frame_slot`, symbol fallback resolver | Prealloc storage, regalloc, stack-layout, addressing, alloca coalescing | Prepared ids are authority after interning; helpers still resolve display/raw BIR names or generated slot slice strings at boundaries. Link symbols prefer `LinkNameId`; raw display names are accepted only with invalid ids. | No for link symbols with ids: mismatches fail. Yes for local value/slot/generated names because no complete typed local object id exists across all structures yet. | Prepared storage/regalloc/stack-layout APIs pass typed `ValueNameId`, `SlotNameId`, object ids, and link ids end-to-end; generated slot slices become typed offset records. | No for idea 168 except link-symbol fallback; main work feeds idea 169. |
| Diagnostics, debug, parity, printers, dumps | Parser debug/render helpers, HIR `CompileTimeInfo::diagnostics`, parity mismatch vectors, HIR/LIR/BIR printers | Diagnostics, debug inspection, final output | Strings describe decisions or render final artifacts; they should not be parsed back into lookup authority. | Not applicable unless a future caller feeds diagnostic/printer text back into semantic lookup. | Keep output-only; add guard only if future audit finds feedback into authority. | No. |

## Suggested Next

Start Step 5 by consolidating the audit artifact and follow-up recommendations
from Steps 1-4. Preserve these primary lanes:

1. idea 168: production rendered-name compatibility bridge retirement for
   parser support overloads, HIR module decl/record indexes, sema/consteval
   template mirrors, HIR template/specialization rendered maps, LIR final-output
   scans, and BIR link-symbol fallback.
2. idea 169: route-local/generated identity cleanup for LIR/BIR/prealloc local
   values, labels, slots, generated string constants, HIR locals/labels, and
   consteval interpreter local `by_name`.
3. regression guard lane: tests that prove complete structured misses fail
   closed for parser typedef/record helpers, sema enum static-eval, HIR module
   declaration lookup, record owner lookup, and BIR/LIR link-name parity.

## Watchouts

- A complete structured miss is fenced in the audited core paths, but exceptions
  are intentional compatibility cases: HIR self-consistent rendered names,
  rendered qualified imports, raw-only LIR functions/externs, invalid-id BIR
  runtime/intrinsic placeholders, and absent-owner-index record layout handoffs.
- Keep final-output text separate from semantic lookup: `signature_text`,
  `init_text`, ABI/mangled strings, printer output, diagnostics, and parity
  labels are only bridge candidates when production code scans them to recover
  semantic facts.
- Do not merge idea 168 and 169. Bridge retirement is about rendered source/
  link compatibility boundaries; route-local cleanup is about local SSA/label/
  slot/generated-name ids.
- The untracked `review/166_compile_time_registry_fencing_route_review.md`
  pre-existed this packet and was not touched.

## Proof

Focused audit replay only. No tests were required because this packet made no
implementation or test changes and found no tiny behavior-sensitive issue.
`test_after.log` was not updated.

Commands run:

`sed -n '1,240p' /workspaces/c4c/.codex/skills/c4c-executor/SKILL.md`

`sed -n '1,220p' /workspaces/c4c/.codex/skills/c4c-clang-tools/SKILL.md`

`git status --short`

`sed -n '1,260p' todo.md`

`sed -n '1,220p' plan.md`

`which c4c-clang-tool c4c-clang-tool-ccdb`

`rg -n --glob '!build*/**' --glob '!ideas/closed/**' --glob '!review/**' -e 'legacy|compat|compatibility|rendered|fallback|no-metadata|invalid-id|structured miss|fail closed|find_.*_by_name|classify_.*lookup|struct_defs|fn_index|global_index|template_defs|struct_def_owner_index' src/frontend/parser src/frontend/sema src/frontend/hir src/codegen/lir src/backend/bir src/backend/prealloc`

`rg -n --glob '!build*/**' -e 'signature_text|init_text|mangled_name|canonical|display_name|SpecializationKey|format_type_for_specialization|TypeBindings|NttpBindings|NttpTextBindings|rendered_enum_consts|by_name|by_text|by_key|find_.*legacy|classify_.*decl_lookup' src/frontend src/codegen/lir src/backend/bir src/backend/prealloc`

`rg -n --glob '!build*/**' -e 'resolve_record_type_spec\\(' -e 'eval_const_int\\(' -e 'resolve_typedef_chain\\(' -e 'types_compatible_p\\(' src tests`

`rg -n --glob '!build*/**' -e 'find_.*_by_name_legacy|classify_.*decl_lookup|fn_index|global_index|struct_defs|template_defs|struct_def_owner_index' src/frontend/hir`

`rg -n --glob '!build*/**' -e 'fallback_name_bindings|TemplateArgBindings|canonical_leaf_display_spelling|format_canonical_type|mangled|display' src/frontend/sema/canonical_symbol.* src/frontend/sema/canonical_symbol.cpp`

Targeted `nl -ba ... | sed -n ...` inspections were used for the audited
sections of `parser_support.hpp`, `parser/impl/support.cpp`, `type_utils.hpp`,
`type_utils.cpp`, `hir_ir.hpp`, `hir_types.cpp`, `sema/consteval.cpp`,
`codegen/lir/ir.hpp`, `lir_printer.cpp`, `lir/verify.cpp`,
`backend/bir/lir_to_bir.cpp`, `backend/bir/bir.hpp`,
`backend/bir/bir_validate.cpp`, `backend/prealloc/prealloc.hpp`,
`backend/prealloc/prealloc.cpp`, `stack_layout/coordinator.cpp`, and
`stack_layout/alloca_coalescing.cpp`.

AST-backed inspections run:

`c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/frontend/parser/impl/support.cpp resolve_record_type_spec build/compile_commands.json`

`c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/frontend/parser/impl/support.cpp types_compatible_p build/compile_commands.json`

Both caller queries returned `target not called in this translation unit`, so
they were not used as positive proof; direct `rg` caller inspection was used
instead.

`c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/frontend/hir/hir_types.cpp build/compile_commands.json`

`c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/frontend/sema/consteval.cpp build/compile_commands.json`

`c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/bir/lir_to_bir.cpp build/compile_commands.json`

`c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/prealloc/stack_layout/coordinator.cpp build/compile_commands.json`

The focused searches and successful compile-database symbol inspections
completed. No root-level proof log was created because no tests were delegated
or required for this docs/audit-only packet.
