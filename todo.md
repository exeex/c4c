Status: Active
Source Idea Path: ideas/open/162_link_name_id_backend_symbol_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Propagate LinkNameId Through HIR to LIR

# Current Packet

## Just Finished

Completed Step 3 narrow HIR-to-LIR global lvalue store handoff in
`src/codegen/lir/hir_to_lir/lvalue.cpp`.

Global lvalue lowering now resolves a `DeclRef` with a concrete `GlobalId`
through `select_global_object(*r)`, preserving the original global's
`LinkNameId` authority instead of reselecting by the rendered `GlobalVar::name`.
The focused test corrupts both the HIR global spelling and the lvalue
`DeclRef` spelling, then proves the generated LIR store still targets the
semantic global symbol rather than a rendered-name collision.

## Suggested Next

Continue Step 3 on the next HIR-to-LIR route that still treats raw rendered
symbol text as authority after structured metadata exists, with special focus
on constant-initializer function designator spelling and any remaining
extern-declaration bridge not already covered by `LinkNameId` dedup.

## Watchouts

- The sibling runtime global rvalue path in
  `src/codegen/lir/hir_to_lir/expr/coordinator.cpp` has the same rendered-name
  reselection shape, but it was outside this packet's owned files.
- Existing constant-initializer global-address selection already routes through
  `select_global_object(DeclRef)` or `select_global_object(GlobalId)`.
- A valid `LinkNameId` miss for a covered known symbol must not reopen raw
  string lookup silently in later backend/lowering paths.

## Proof

Passed:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_lookup_tests|frontend_lir_.*)$'; } > test_after.log 2>&1
```

Proof log: `test_after.log`. Result: build succeeded and 5/5 selected tests
passed.
