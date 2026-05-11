Status: Active
Source Idea Path: ideas/open/162_link_name_id_backend_symbol_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Propagate LinkNameId Through HIR to LIR

# Current Packet

## Just Finished

Completed Step 3 audit and fix for the HIR-to-LIR constant-initializer
function-designator route in
`src/codegen/lir/hir_to_lir/const_init_emitter.cpp`.

The scoped audit found that `resolve_function_decl_name` still fell back to the
raw rendered `DeclRef::name` when a constant initializer carried a valid
`LinkNameId` whose function lookup missed. That covered metadata path now uses
the HIR link-name table spelling instead of raw text. The focused test mutates
a global function-pointer initializer to carry a semantic external
`LinkNameId` while the raw initializer spelling remains `rendered_shadow`, then
proves the LIR initializer emits `@semantic_external_helper` and not the raw
collision.

## Suggested Next

Move to Step 4 on the LIR-to-BIR symbol authority target, starting with the
backend route that consumes LIR function/global/extern `LinkNameId` metadata
and must not reselect symbols by rendered text.

## Watchouts

- Existing constant-initializer global-address selection already routes through
  `select_global_object(DeclRef)` or `select_global_object(GlobalId)`.
- `LirModule::record_extern_decl` already dedups by `LinkNameId`, and the LIR
  printer resolves extern declaration spellings from `LinkNameId`; no scoped
  `ir.hpp` change was needed for this packet.
- A valid `LinkNameId` miss for a covered symbol must not reopen raw string
  lookup silently in Step 4 backend/lowering paths.

## Proof

Passed:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_lookup_tests|frontend_lir_.*)$'; } > test_after.log 2>&1
```

Proof log: `test_after.log`. Result: build succeeded and 5/5 selected tests
passed.
