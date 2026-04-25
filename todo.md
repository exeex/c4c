Status: Active
Source Idea Path: ideas/open/98_parser_sema_post_cleanup_structured_identity_leftovers.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Populate Consteval NTTP Binding Mirrors

# Current Packet

## Just Finished

Step 6 - Populate Consteval NTTP Binding Mirrors is complete.

`bind_consteval_call_env` now populates consteval NTTP binding `TextId` and
structured-key mirror maps from parser-owned `template_param_name_text_ids[i]`
when stable metadata exists, beside the existing rendered-name
`out_nttp_bindings` path. `ConstEvalEnv` installs those NTTP mirror maps when
non-empty, so existing `lookup_by_text` and `lookup_by_key` checks can compare
against the rendered-name lookup while preserving rendered-name behavior.

## Suggested Next

Step 7 type-binding text mirror plumbing packet: extend consteval template type
binding mirror population so parser-owned template parameter `TextId` metadata
fills `TypeBindingTextMap` and name-to-`TextId` mirrors beside the existing
structured-key type-binding mirror path.

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
- Step 6 added no parser metadata, HIR/type/codegen identity migration,
  template rendering changes, diagnostics changes, expectation rewrites, or
  broad compile-time engine call-site migration outside the owned sema path.
- The HIR compile-time engine still calls `bind_consteval_call_env` through the
  defaulted mirror parameters; Step 7 should stay focused on type-binding text
  mirrors unless the supervisor delegates that HIR call-site separately.

## Proof

Passed.

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "(frontend_parser_tests|cpp_positive_sema_consteval_nttp_cpp|cpp_positive_sema_deferred_consteval_nttp_cpp|cpp_positive_sema_template_.*nttp|cpp_positive_sema_template_bool_nttp_cpp|cpp_positive_sema_template_char_nttp_cpp|cpp_positive_sema_template_integral_nttp_types_cpp|cpp_positive_sema_template_recursive_nttp_cpp)"' > test_after.log 2>&1`

Proof log: `test_after.log`. The delegated subset passed 19/19 tests.
