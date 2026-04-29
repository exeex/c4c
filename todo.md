Status: Active
Source Idea Path: ideas/open/124_hir_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Demote Rendered-Name Semantic Lookup For Functions And Globals

# Current Packet

## Just Finished

Completed Step 3 `hir_functions.cpp` previous-declaration alignment carryover
packet. `Lowerer::lower_function()` now builds a `DeclRef` carrier from the
function being lowered (`name`, `name_text_id`, `link_name_id`, and
`ns_qual`) and resolves the previous declaration through
`Module::resolve_function_decl()`. This routes structured and link-name
authority through the shared resolver while preserving rendered-name fallback
inside that resolver when declaration metadata is incomplete.

## Suggested Next

Next coherent Step 3 packet: inspect the remaining
`find_function_by_name_legacy()` callers in `hir_types.cpp`, then route each
through resolver authority where a `DeclRef`/`LinkNameId` carrier exists or
classify the exact metadata gap.

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
  overloaded-operator helper and bool-conversion helper in `operator.cpp`, the
  range-for prefix-increment, dereference, and `begin()` method return lookups
  in `range_for.cpp`, and the nested `operator_arrow` helper in `operator.cpp`
  through resolver authority. The small builtin/scalar-control packet demoted
  `builtin.cpp` and `scalar_control.cpp` direct function lookups through
  `Module::resolve_function_decl()`. The `hir_functions.cpp`
  previous-declaration alignment carryover now uses a `DeclRef` built from the
  function's existing declaration metadata and resolves through the same shared
  function declaration resolver. Remaining direct function legacy helper callers
  are in `hir_types.cpp` and need separate carrier-metadata inspection.
- Direct-call link-carrier discovery now records declaration lookup hits through
  the shared resolver. The hit/mismatch recorders deduplicate exact repeats, but
  future packets should keep an eye on noisy lookup telemetry if more call-site
  helpers are routed through the same boundary.
- The focused HIR lookup tests cover module structured function/global,
  function and global `LinkNameId`, concrete `GlobalId`, direct-call
  structured/LinkNameId stale-rendered-name disagreement cases, and
  operator/range-for resolver return metadata without weakening legacy fallback
  coverage. The builtin/scalar-control packet relies on the same function-decl
  resolver coverage because both sites now delegate to that shared authority.

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
