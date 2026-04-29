Status: Active
Source Idea Path: ideas/open/124_hir_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Demote Rendered-Name Semantic Lookup For Functions And Globals

# Current Packet

## Just Finished

Completed Step 3 nested `operator_arrow` and range-for `operator_deref`
method return lookup demotion packet. `Lowerer::lower_member_expr()` now uses
`Module::resolve_operator_callee()` for nested `operator_arrow` return
metadata after constructing the `DeclRef` with its `link_name_id`, and
`Lowerer::lower_range_for_stmt()` now uses
`Module::resolve_range_for_method_callee()` for iterator `operator_deref`
return metadata instead of calling `find_function_by_name_legacy()` directly.
The focused HIR lookup test now pins that stale rendered names cannot override
resolver-authoritative return metadata while preserving explicit rendered-name
fallback.

## Suggested Next

Next coherent Step 3 packet: inspect the remaining `find_function_by_name_legacy()`
callers and decide whether the range-for `begin()` return-type lookup has
enough declaration metadata for resolver demotion or should remain classified
as a compatibility fallback.

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
  existing `fn_structured_index` / `global_structured_index`. The module
  declaration resolver now makes structured/link/concrete authorities win on
  disagreement and keeps legacy rendered-name lookup as fallback.
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
- `fn_index`, `global_index`, and `find_*_by_name_legacy()` remain live
  compatibility/fallback surfaces for missing or incomplete structured
  declaration metadata. The direct-call packet removed direct-call uses of the
  function legacy helper in `call.cpp`; this packet demoted the main
  overloaded-operator helper and bool-conversion helper in `operator.cpp`, and
  the range-for prefix-increment method return lookup in `range_for.cpp`; the
  nested `operator_arrow` helper in `operator.cpp` and range-for dereference
  method helper in `range_for.cpp` are now also routed through resolver
  authority. The range-for `begin()` return-type lookup still calls the legacy
  helper and needs separate classification before demotion.
- Direct-call link-carrier discovery now records declaration lookup hits through
  the shared resolver. The hit/mismatch recorders deduplicate exact repeats, but
  future packets should keep an eye on noisy lookup telemetry if more call-site
  helpers are routed through the same boundary.
- The focused HIR lookup tests cover module structured function/global,
  function and global `LinkNameId`, concrete `GlobalId`, direct-call
  structured/LinkNameId stale-rendered-name disagreement cases, and
  operator/range-for resolver return metadata without weakening legacy fallback
  coverage.

Do not weaken tests or add named-case shortcuts. Stale rendered-name tests are
appropriate only when they prove structured identity wins over legacy lookup
authority while preserving explicit fallback behavior.

## Proof

Supervisor-selected proof ran exactly:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_lookup_tests$' > test_after.log 2>&1
```

Result: passed. `test_after.log` contains `frontend_hir_lookup_tests`: 1 passed
/ 0 failed.
