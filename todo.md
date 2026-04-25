Status: Active
Source Idea Path: ideas/open/98_parser_sema_post_cleanup_structured_identity_leftovers.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Mirror Template Type-Parameter Validation

# Current Packet

## Just Finished

Step 5 - Mirror Template Type-Parameter Validation is complete.

Sema now records parser-owned `template_param_name_text_ids[i]` for template
type parameters in a `TextId` mirror alongside the rendered-name set when the
metadata is valid. Cast validation consults mirror-aware helpers and compares
rendered-name and mirrored decisions when both inputs exist, while preserving
rendered-name fallback behavior and HIR-facing `TypeSpec::tag` output.

## Suggested Next

Step 6 consteval NTTP binding mirror packet: populate consteval call NTTP
binding text/key maps from parser-owned template parameter metadata where
stable IDs exist, and dual-read or compare those mirrors beside the existing
rendered NTTP binding path.

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
- Step 5 added sema-only template type-parameter `TextId` mirrors in
  `validate.cpp`; it did not alter parser metadata, HIR/type/codegen identity,
  template rendering, diagnostics, expectations, or consteval NTTP call
  binding maps.
- Keep Step 6 limited to consteval NTTP binding mirrors unless the supervisor
  explicitly delegates broader type-binding mirror plumbing; no HIR/type/codegen
  migration, template rendered-name cleanup, or expectation rewrites.
- Enum mirror population now depends on parser-owned `enum_name_text_ids`; keep
  any future enum work on that definition-time metadata and do not re-derive
  stable identity from rendered strings.

## Proof

Passed.

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "(template_(type|typename|unresolved|visible|specialization|member_type|typedef)|c_style_cast_.*template|frontend_parser_tests)"' > test_after.log 2>&1`

Proof log: `test_after.log`. The delegated subset passed 37/37 tests.
