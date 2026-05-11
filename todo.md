Status: Active
Source Idea Path: ideas/open/162_link_name_id_backend_symbol_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Tighten HIR Symbol Materialization and Lookup Handoff

# Current Packet

## Just Finished

Completed Step 2 narrow HIR template-function lookup handoff in
`src/frontend/hir/hir_types.cpp`.

`make_function_lookup_decl_ref` now accepts an optional known `LinkNameId` and
uses that as the `DeclRef` authority while preserving the rendered name as the
spelling/fallback payload. Template call result inference now looks up matching
`HirTemplateInstantiation` metadata from `module_->template_defs` and passes
`mangled_link_name_id` into the function lookup `DeclRef` for covered
instantiations. The generic-control-type template call path uses the same
metadata handoff.

## Suggested Next

Tighten the next HIR-to-LIR symbol handoff that still consumes rendered
function/global spelling after structured metadata exists, with special focus
on initializer symbol emission and any direct-call route that can already carry
`DeclRef::link_name_id`.

## Watchouts

- `DeducedTemplateCall` and `TemplateInstance` do not themselves store
  `mangled_link_name_id`; this slice uses canonical `HirTemplateInstantiation`
  metadata in `module_->template_defs` as the ID source.
- The new helper only transfers IDs through specialization-key matches; rendered
  `mangled_name` remains the `DeclRef` spelling/fallback payload, not the ID
  authority.
- `make_global_lookup_decl_ref` still recovers `LinkNameId` from rendered
  spelling; this packet only covered function template lookup handoff.
- A valid `LinkNameId` miss for a covered known symbol must not reopen raw
  string lookup silently in later backend/lowering paths.

## Proof

Passed:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_lookup_tests|cpp_hir_.*_structured_metadata)$'; } > test_after.log 2>&1
```

Proof log: `test_after.log`. Result: build succeeded and 38/38 selected tests
passed.
