Status: Active
Source Idea Path: ideas/open/197_bir_backend_compatibility_string_retirement.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory BIR And Backend Compatibility Strings

# Current Packet

## Just Finished

Step 1 - Inventory BIR And Backend Compatibility Strings completed as an
inventory/classification pass.

Focused evidence inspected with:
`rg -n "compatibility|legacy|no-id|rendered|display|StringConstant|TextId|LinkNameId|GlobalTypes|TypeDeclMap|prepared|prealloc" src/backend src/codegen/lir src/codegen/shared src/shared`
plus narrower follow-up `rg` passes over BIR, LIR, shared, prealloc, and
backend-prepared name/id helpers.

Inventory summary:

- `bir::StringConstant::name`: semantic compatibility. Evidence:
  `src/backend/bir/bir.hpp` keeps `StringConstant::name` as a compatibility
  name with no `LinkNameId`; `src/backend/bir/lir_to_bir.cpp` already builds a
  `TextTable` and `bytes_by_name` keyed by `TextId`, while preserving
  `StringConstant::name` as compatibility/display spelling. This is the first
  metadata-rich conversion/fencing target.
- LIR string pool and string aliases: semantic compatibility plus route-local
  handles. Evidence: `src/codegen/lir/ir.hpp` has `string_pool`; `src/backend/bir/lir_to_bir.cpp`
  maps route-local SSA GEP/cast spellings to string-pool `TextId` aliases and
  then rewrites direct BIR call args. The SSA key is route-local; the string
  identity is the `TextId` payload.
- `LirFunction::signature_text`: final/output spelling and unresolved
  producer-boundary compatibility. Evidence: `src/codegen/lir/ir.hpp` says
  structured signature facts should be preferred, with function references in
  `signature_text` still an unresolved compatibility fallback; `src/codegen/lir/lir_printer.cpp`
  renders through `LinkNameId` when available.
- `LirGlobal::init_text` and `initializer_function_link_name_ids`:
  final/output spelling with explicit semantic carrier. Evidence:
  `src/codegen/lir/ir.hpp` records function reference ids separately and says
  scanning `init_text` is retained only for legacy producers.
- LIR extern declarations: semantic compatibility fenced by `LinkNameId`.
  Evidence: `src/codegen/lir/ir.hpp` uses `extern_decl_link_name_map` first and
  `extern_decl_name_map` only as raw/rendered no-id compatibility/output
  boundary.
- `GlobalTypes`, `TypeDeclMap`, `FunctionSymbolSet`: explicit no-id
  compatibility at the LIR-to-BIR boundary. Evidence:
  `src/backend/bir/lir_to_bir/lowering.hpp` labels `GlobalTypes` and
  `TypeDeclMap` as producer-final-spelling compatibility tables; converted BIR
  instructions carry `LinkNameId`; `FunctionSymbolSet::raw_symbol_link_name_ids`
  is raw-import/no-id compatibility and must not recover from a present
  structured-id miss.
- BIR global/function fields and pointer initializers: mixed final spelling,
  display, and semantic identity. Evidence: `src/backend/bir/bir.hpp` gives
  `Global::link_name_id`, `Function::link_name_id`, and pointer-initializer
  `initializer_symbol_name_id`; raw `name`/`initializer_symbol_name` remain
  final/display compatibility. `src/backend/bir/bir_validate.cpp` checks
  initializer ids against globals/functions when present.
- Backend aggregate/type layout lookups: semantic compatibility with structured
  preference. Evidence: `src/backend/bir/lir_to_bir/types.cpp` builds
  `BackendStructuredLayoutTable` from `StructNameId`-backed declarations,
  records legacy parity, and `lookup_backend_aggregate_type_ref_layout_result`
  fails mismatch instead of falling back; raw `TypeDeclMap` lookup remains
  legacy no-id fallback.
- BIR local values, labels, slots, phi labels, and local memory: route-local
  handles. Evidence: `src/backend/bir/bir.hpp` comments label raw spellings as
  display/compatibility and pair them with `BlockLabelId`, `SlotNameId`, or
  `ValueNameId` where present.
- Prepared/prealloc names: route-local handles with structured prepared ids.
  Evidence: `src/backend/prealloc/prealloc.hpp` owns `PreparedNameTables`;
  `resolve_prepared_block_label_id`, `resolve_prepared_function_name_id`, and
  `resolve_prepared_value_name_id` resolve through prepared id tables;
  `src/backend/prealloc/liveness.cpp` and `src/backend/prealloc/prealloc.cpp`
  prefer BIR ids before raw labels/names when building prepared records.
- Prepared same-module global refs: metadata-rich link-visible path with
  display fallback. Evidence: `src/backend/prealloc/prealloc.hpp`
  `resolve_prepared_bir_link_name_ref` validates `LinkNameId` spelling and
  only returns raw display-name refs when the id is invalid; materialized
  compare-join resolution checks prepared link-name spelling before selecting
  globals.
- Backend x86 data emission: final/output spelling plus compatibility
  fallback. Evidence: `src/backend/mir/x86/module/data.cpp` renders assembler
  symbols from BIR global names and still finds same-module globals by raw
  logical name; this is output spelling, but same-module lookup should not grow
  new semantic authority without `LinkNameId`.

## Suggested Next

Execute Step 2 against `bir::StringConstant::name`: add a focused stale/missing
`TextId` proof around LIR string-pool to BIR direct-call string pointer
rewriting, then either convert the retained name path to carry/use a structured
`TextId` in BIR or fence the raw name as explicit no-id compatibility with an
owner/removal condition.

## Watchouts

- Do not start AArch64 backend implementation work.
- Do not downgrade supported tests or replace semantic fixes with expectation
  rewrites.
- Do not treat printer, assembler, linker, diagnostic, debug-focus, or
  route-local spelling as semantic identity.
- If parser, Sema, HIR, or LIR source intent blocks this work, record a
  separate open idea instead of expanding this plan.
- `src/backend/bir/lir_to_bir.cpp` already has the richest metadata for the
  first slice: string-pool names are interned as `TextId`, while BIR still
  publishes only `StringConstant::name`. Avoid widening immediately into
  `GlobalTypes`/`TypeDeclMap`; those are broader LIR-boundary compatibility
  tables and better Step 3/Step 4 candidates.
- x86 `module/data.cpp` raw same-module global lookup is currently final
  assembler/output spelling territory. Do not convert that before the upstream
  BIR/prepared link-name carriers are fenced.

## Proof

Inventory-only lifecycle scratchpad update; no build or code validation
required by the packet.

Focused evidence commands run:

- `rg -n "compatibility|legacy|no-id|rendered|display|StringConstant|TextId|LinkNameId|GlobalTypes|TypeDeclMap|prepared|prealloc" src/backend src/codegen/lir src/codegen/shared src/shared`
- `rg -n "StringConstant|TextId|LinkNameId|GlobalTypes|TypeDeclMap|type_decls|struct_names|signature_text|init_text|extern_decl|initializer_function|legacy_|compatibility|no-metadata|no-id|raw/rendered|rendered-name" src/codegen/lir src/codegen/shared src/shared`
- `rg -n "StringConstant|TextId|LinkNameId|prepared|prealloc|compatibility|legacy|no-id|rendered|display|name_id|function_name|symbol_name|block_label|value_name|global" src/backend/prealloc src/backend/bir src/backend/prepare src/backend/mir/x86/module 2>/dev/null`
- `rg -n "string_pool|StringConstant|StringConst|initializer_symbol_name|initializer_function_link_name|GlobalTypes|TypeDeclMap|record_extern_decl|FunctionSymbolSet|raw_symbol|link_name_id_for_global|lookup_backend_aggregate|structured_layout|compatibility_tag" src/backend/bir src/codegen/lir src/codegen/shared src/shared`
- `rg -n "prepared_named_value_id|find_prepared_.*_by_.*name|resolve_prepared|prepared_.*name\\(|intern_prepared_slot_name|FunctionNameId|BlockLabelId|ValueNameId|SlotNameId|LinkNameId" src/backend/prealloc src/backend/mir/x86/module src/backend/mir/x86/prepared`

No `test_after.log` was produced because the delegated proof was grep evidence
only and explicitly required no build.
