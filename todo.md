Status: Active
Source Idea Path: ideas/open/167_whole_codebase_string_authority_final_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish Audit Method and Initial Inventory

# Current Packet

## Just Finished

Step 1 - Establish Audit Method and Initial Inventory completed the
recording-only audit setup for idea 167.

Audit search method:
- Primary broad inventory command:
  `rg -n --glob '!build*/**' --glob '!ideas/closed/**' --glob '!review/**' -e 'unordered_map\s*<\s*(std::)?string' -e 'map\s*<\s*(std::)?string' -e 'find\s*\(\s*name\s*\)' -e 'lookup\s*\(\s*name\s*\)' -e '[A-Za-z0-9_]*_by_name\b' -e '[A-Za-z0-9_]*_name_map\b' -e 'mangled_name\b' -e 'source_name\b' -e 'qualified_name\b' -e 'rendered' -e 'fallback' src tests CMakeLists.txt docs`
- Primary source-file inventory command:
  `rg -l --glob '!build*/**' --glob '!ideas/closed/**' --glob '!review/**' -e 'unordered_map\s*<\s*(std::)?string' -e 'map\s*<\s*(std::)?string' -e 'find\s*\(\s*name\s*\)' -e 'lookup\s*\(\s*name\s*\)' -e '[A-Za-z0-9_]*_by_name\b' -e '[A-Za-z0-9_]*_name_map\b' -e 'mangled_name\b' -e 'source_name\b' -e 'qualified_name\b' -e 'rendered' -e 'fallback' src tests`
- Bridge-boundary refinement command:
  `rg -n --glob '!build*/**' --glob '!ideas/closed/**' --glob '!review/**' -e 'rendered_compat' -e 'complete.*miss' -e 'no-metadata' -e 'invalid-id' -e 'legacy.*fallback' src/frontend src/codegen src/backend`
- Structured-domain refinement command:
  `rg -n --glob '!build*/**' --glob '!ideas/closed/**' --glob '!review/**' -e 'LinkNameId' -e 'TextId' -e 'StructNameId' -e 'BlockLabelId' -e 'ValueNameId' src/frontend src/codegen src/backend`

Classification legend:
- `SA`: semantic authority; rendered/raw string controls source, semantic, or
  link-visible identity where a structured id/key should own the decision.
- `CB`: compatibility bridge; rendered/raw spelling retained only behind an
  explicit no-metadata, invalid-id, raw-only, or legacy producer boundary.
- `DO`: display/output; dumps, printers, final assembly/IR/object text, or
  user diagnostics not used as lookup authority.
- `DD`: diagnostic/debug; error text, tracing, debug labels, and test messages.
- `RL`: route-local identity; backend-local SSA values, labels, stack slots,
  registers, prepared-route names, linker-private section/symbol staging.
- `GT`: generated temporary name; compiler-created temps, labels, synthetic
  static/global/local names, string-pool labels.
- `FP`: false positive; comments, docs, test harness helpers, spelling
  renderers, or variable names that do not establish lookup authority.

Known false-positive rules:
- Do not count `render_qualified_name`/qualified-name table rendering as
  semantic authority unless the rendered spelling is fed back into a covered
  lookup after a complete structured miss.
- Do not count frontend/backend diagnostics, dump printers, or final emitted
  text as authority without a call chain back into lookup or validation.
- Do not count tests that intentionally create stale rendered names as bugs;
  they are regression witnesses unless they downgrade supported behavior.
- Do not count preprocessor macro/include maps as part of the string-authority
  cleanup unless they cross into parser/sema/HIR identity domains.
- Do not count route-local BIR/LIR/MIR SSA names, labels, stack slots, register
  names, or object/linker staging symbols as source/link semantic authority
  until a cross-domain lookup or structured miss fallback is proven.
- Do not count `.cpp.md` subsystem notes as implementation hits; keep them as
  orientation only.

Initial candidate inventory by owner/domain:
- Shared qualified-name rendering: `src/shared/qualified_name_table.hpp`
  contains render helpers and legacy rendered-spelling helpers. Initial class:
  `DO/CB`; inspect only if a caller uses rendered output as lookup authority.
- Parser support and parser state: `src/frontend/parser/parser_support.hpp`,
  `src/frontend/parser/impl/support.cpp`, `src/frontend/parser/impl/parser_state.hpp`,
  and parser type/expression files carry `compatibility_tag_map`,
  `compatibility_named_consts`, rendered typedef helpers, `qualified_name`,
  and source spelling fields. Initial class: mixed `CB/SA`; Step 2 should
  verify TextId overloads fail closed and string overloads are fenced as
  compatibility.
- Sema validate/consteval/type-utils: `src/frontend/sema/validate.cpp`,
  `src/frontend/sema/consteval.*`, `src/frontend/sema/type_utils.*`, and
  `src/frontend/sema/canonical_symbol.cpp` contain rendered maps,
  `*_by_name` helper maps, and structured key mirrors. Initial class:
  mixed `CB/SA`; inspect complete-miss behavior around globals, enum consts,
  functions, overloads, local scopes, type bindings, and consteval maps.
- HIR lowerer and compile-time engine: `src/frontend/hir/impl/lowerer.hpp`,
  `src/frontend/hir/hir_ir.hpp`, `src/frontend/hir/hir_types.cpp`,
  `src/frontend/hir/compile_time_engine.hpp`, and HIR template/materialization
  files contain local/param/static maps, `rendered_compat_*` fences,
  template definition maps, and stale-rendered tests nearby. Initial class:
  mixed `CB/RL/SA`; inspect call chains for complete structured misses.
- LIR and HIR-to-LIR: `src/codegen/lir/ir.hpp`, `src/codegen/lir/verify.cpp`,
  `src/codegen/lir/hir_to_lir/*.cpp`, and call/lvalue helpers contain
  `extern_decl_name_map`, `mangled_name`, `best_by_name` fallbacks,
  `discardable_by_name`, `llvm_struct_field_slot_by_name`, rendered aggregate
  type refs, and verifier legacy name checks. Initial class: mixed `CB/DO/SA`;
  inspect LinkNameId/StructNameId boundaries before classifying.
- BIR lowering and validation: `src/backend/bir/lir_to_bir.cpp`,
  `src/backend/bir/lir_to_bir/types.cpp`, `src/backend/bir/lir_to_bir/lowering.hpp`,
  memory maps, and `src/backend/bir/bir_validate.cpp` contain raw fallback
  function lookups, type-decl fallback, global/function find-by-name helpers,
  and many local value maps. Initial class: mixed `CB/RL/SA`; inspect whether
  BIR module validation is intentionally link-visible by rendered symbol.
- Backend prealloc/stack layout: `src/backend/prealloc/*.cpp`,
  `src/backend/prealloc/prealloc.hpp`, and stack-layout files contain
  `defs_by_name`, `blocks_by_label`, slot/name maps, `ValueNameId`,
  `BlockLabelId`, and prepared name tables. Initial class: mostly `RL/CB`;
  inspect where prepared ids exist but raw string lookup remains a fallback.
- MIR/linker/assembler/codegen: RISC-V and AArch64 linker maps plus x86/AArch64
  codegen contain symbol tables, source object names, text offsets, label
  defs, and rendered assembly helpers. Initial class: mostly `RL/DO`, with
  link-visible `SA` possible in linker symbol resolution; inspect separately
  from frontend semantic identity.
- Tests and fixtures: dense hits in `tests/frontend/frontend_parser_tests.cpp`,
  `tests/frontend/frontend_parser_lookup_authority_tests.cpp`,
  `tests/frontend/frontend_hir_lookup_tests.cpp`, structured metadata tests,
  LIR extern/type-ref tests, and backend prepared-route tests. Initial class:
  `DD/FP` as witnesses; only classify as action items if expectations weaken
  source/link authority or hide fallback after complete miss.

Regions needing AST-backed or local call-chain inspection:
- `src/frontend/sema/validate.cpp` lookup helpers and maps around
  `lookup_function_by_name`, overload lookup, `lookup_local_symbol_by_name`,
  globals/enums, and structured key mirrors.
- `src/frontend/sema/consteval.*` rendered compatibility paths for type
  bindings, NTTPs, enum/local/named consts, and no-metadata recursive calls.
- `src/frontend/hir/hir_types.cpp` and `src/frontend/hir/impl/lowerer.hpp`
  around `rendered_compat_*` local/param/static-global fences.
- `src/frontend/hir/compile_time_engine.hpp` template/consteval/enum/local
  maps and complete structured miss comments.
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp` fallback selection for globals,
  functions, extern declarations, discardable functions, and aggregate type
  refs.
- `src/codegen/lir/verify.cpp` legacy-by-name checks and StructNameId mirror
  verification.
- `src/backend/bir/lir_to_bir.*` raw-only function fallback, type-decl legacy
  fallback, `raw_symbol_link_name_ids`, and local/global pointer maps.
- `src/backend/prealloc/out_of_ssa.cpp`, `src/backend/prealloc/regalloc.cpp`,
  and stack-layout files where route-local string maps coexist with
  `ValueNameId` or `BlockLabelId`.

## Suggested Next

Begin Step 2 by classifying the covered frontend structured domains first:
parser support, sema validate/consteval/type-utils, and HIR
`rendered_compat_*` lookup paths. Keep the packet audit-only unless a tiny
semantic-authority bug is obvious and separately approved by the supervisor.

## Watchouts

- Do not treat all strings as equally bad.
- Do not start idea 168, 169, or 170 implementation work during the initial
  audit.
- Do not weaken tests, remove diagnostics/output text, or reclassify semantic
  lookup as display without evidence.
- The initial `rg` result set is intentionally noisy; broad output included
  thousands of `rendered`/`fallback` hits, so later packets should use the
  owner/domain inventory above instead of trying to triage every raw line at
  once.
- Linker symbol tables and route-local backend maps may legitimately be string
  keyed, but they still need separation from covered source/HIR/LIR identity.

## Proof

Audit-only Step 1. No build or test proof was required, and no
`test_after.log` was produced for this recording-only packet.

Suggested proof commands for the first classification packet:
- Audit replay:
  `rg -n --glob '!build*/**' --glob '!ideas/closed/**' --glob '!review/**' -e 'rendered_compat' -e 'complete.*miss' -e 'no-metadata' -e 'invalid-id' -e 'legacy.*fallback' src/frontend src/codegen src/backend`
- Frontend structured-domain tests:
  `ctest --test-dir build -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|frontend_hir_lookup_tests|cpp_hir_parser_support_residual_structured_metadata|cpp_hir_parser_type_helper_residual_structured_metadata|cpp_hir_sema_canonical_symbol_structured_metadata|cpp_hir_sema_consteval_type_utils_structured_metadata)$' --output-on-failure`
