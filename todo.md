Status: Active
Source Idea Path: ideas/open/124_hir_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory HIR String Lookup Surfaces

# Current Packet

## Just Finished

Completed Step 1 inventory of HIR legacy string lookup surfaces under
`src/frontend/hir` and named the first bounded conversion packet.

## Suggested Next

First bounded conversion packet: demote legacy rendered-name authority in
module declaration resolution.

Owned implementation files:
- `src/frontend/hir/hir_ir.hpp`
- `tests/frontend/frontend_hir_lookup_tests.cpp`

Target:
- `Module::resolve_function_decl()` and `Module::resolve_global_decl()` should
  prefer `LinkNameId`, concrete IDs, and `ModuleDeclLookupKey` structured
  results over `find_*_by_name_legacy()` when both structured and legacy
  rendered-name hits exist but disagree.
- Keep `fn_index`, `global_index`, `find_*_by_name_legacy()`, and
  `LegacyRendered` hit reporting as compatibility/fallback surfaces for missing
  structured metadata.
- Add or update a focused HIR lookup test proving stale rendered names do not
  override structured function/global declaration lookup.

Suggested focused proof command:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_lookup_tests$' > test_after.log 2>&1
```

## Watchouts

Step 1 inventory classification:

- Pure text lookup: per-function lowering maps in `Lowerer::FunctionCtx`
  (`locals`, `params`, `label_blocks`, local function-pointer signatures,
  `local_const_bindings`, `pack_params`, and static-local name bridges) are
  name-to-local/block/parameter scratch state. They are candidates only where
  AST/HIR text IDs are already carried through the local declaration/reference
  path; otherwise classify as unresolved local-scope metadata gaps.
- Semantic lookup: `Module::fn_index` / `global_index` and
  `find_*_by_name_legacy()` are rendered-name compatibility indexes beside
  existing `fn_structured_index` / `global_structured_index`. The current
  disagreement branch returns the legacy hit, so this is the first precise
  conversion packet.
- Semantic lookup: `CompileTimeState` template/consteval definition maps retain
  rendered-name maps but already have `CompileTimeRegistryKey` structured
  mirrors and comments that classify rendered lookup as fallback. Treat as a
  later demotion/classification packet, not the first conversion.
- Semantic lookup: template argument/type binding scratch state
  (`TypeBindings`, `NttpBindings`, `HirTemplateArgMaterializer`,
  `DeferredNttpExprEnv`, `Lowerer::deduced_template_calls_`) still uses
  parameter spellings and encoded debug refs. This is a larger template packet
  and may need `TextId` propagation for template params before conversion.
- Semantic lookup with structured mirrors: struct definitions, template struct
  primaries/specializations, struct static members, member symbols, methods,
  method link-name IDs, and method return types already have owner/member
  structured-key mirrors plus parity counters. These should be later packets
  that make structured lookup win or visibly demote rendered maps.
- Diagnostics/display: `CompileTimeDiagnostic`, inline rejection labels,
  source spans, context strings, parity mismatch records, and HIR summary
  strings are retained display/debug data, not lookup authority.
- Dump/final spelling: HIR printer output, `format_hir()`,
  `canonical_type_str()`, template/debug ref encoders, mangled template names,
  link-visible names, asm templates, data layout, and symbol sanitization are
  final spelling or dump surfaces and should not be converted as lookup state.
- Compatibility bridge: public map accessors and callback boundaries for
  consteval/template evaluation, `struct_defs`, `template_defs`, link-name
  tables, and codegen-facing HIR records still carry strings for downstream or
  serialized compatibility.
- Unresolved boundary: type tags in `TypeSpec::tag`, struct/base/member owner
  discovery that must search rendered owner tags, and local-scope AST name
  maps need separate metadata analysis before conversion.

Do not weaken tests or add named-case shortcuts. Stale rendered-name tests are
appropriate only when they prove structured identity wins over legacy lookup
authority while preserving explicit fallback behavior.

## Proof

Inventory-only lifecycle scratchpad update; no build or test proof required and
no `test_after.log` was produced for this packet.
