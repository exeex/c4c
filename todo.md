Status: Active
Source Idea Path: ideas/open/98_parser_sema_post_cleanup_structured_identity_leftovers.md
Source Plan Path: plan.md
Current Step ID: 3A
Current Step Title: Add AST/Parser Enumerator TextId Metadata

# Current Packet

## Just Finished

Step 3 - Populate Sema Enum Variant Mirrors was blocked before code changes and
has been split into Step 3A then Step 3B in `plan.md`.

`src/frontend/sema/validate.cpp` has the target mirror containers
(`structured_enum_consts_`, `enum_const_vals_global_by_text_`,
`enum_const_vals_global_by_key_`, `enum_const_vals_scopes_by_text_`, and
`enum_const_vals_scopes_by_key_`) and the global/local enum binders, but
`NK_ENUM_DEF` exposes only rendered `enum_names` plus `enum_vals`. There is no
parallel per-variant `TextId` or structured identity in the AST, and sema has no
`TextTable` handle to derive a valid parser `TextId` from those rendered names.
Populating the mirrors in sema-only code would require inventing IDs or deriving
bindings from later references, which would not be stable binding metadata.

## Suggested Next

Step 3A AST/parser enum variant identity packet: add stable per-enumerator
`TextId` metadata to `NK_ENUM_DEF` during enum parsing while preserving rendered
`enum_names`/`enum_vals` bridge behavior. After Step 3A is proven, run Step 3B
so sema can dual-write enum variant type and const-value text/key mirrors from
definition metadata without changing lookup behavior.

## Watchouts

- Keep this plan limited to parser/sema cleanup; HIR module lookup migration
  belongs to idea 99.
- Preserve rendered-string bridges required by AST, HIR, consteval, diagnostics,
  codegen, and link-name output.
- Do not touch parser struct/tag maps, template rendered names, `TypeSpec::tag`
  outputs, or HIR/type/codegen identity surfaces.
- Do not downgrade expectations or add testcase-shaped exceptions.
- Do not treat the parser `struct_tag_def_map` argument to `eval_const_int` as a
  removable string leftover; it is still the rendered tag bridge used by
  `sizeof`, `alignof`, and `offsetof`.
- Enum variant mirrors should be populated from definition-time enumerator
  identity, not from lookup-time reference nodes or rendered-string hashing.
- Template NTTP/type-parameter and consteval NTTP mirror work needs a
  parameter-identity strategy first because current AST template parameter
  arrays do not carry `TextId`s.

## Proof

Not run; the original Step 3 sema-only packet was blocked before code changes
by missing per-enumerator stable AST identity.

Delegated proof, not attempted:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "(enum|frontend_parser_tests|positive_sema_ok_enum_scope_|negative_tests_bad_enum_)"' > test_after.log 2>&1`

Proof log: no fresh `test_after.log` for this blocked packet.
