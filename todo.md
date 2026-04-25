Status: Active
Source Idea Path: ideas/open/98_parser_sema_post_cleanup_structured_identity_leftovers.md
Source Plan Path: plan.md
Current Step ID: 4B
Current Step Title: Mirror Template NTTP Validation Bindings

# Current Packet

## Just Finished

Step 4B - Mirror Template NTTP Validation Bindings is complete.

Template NTTP validation placeholders now pass parser-owned
`template_param_name_text_ids[i]` into sema structured local binding mirrors
when the metadata is valid. The rendered local binding remains unchanged, and
missing metadata or `kInvalidText` safely keeps the existing name-only fallback.

## Suggested Next

Step 5 template type-parameter validation mirror packet: consume
`Node::template_param_name_text_ids` for sema-owned template type-parameter
checks, adding structured or `TextId` mirrors where stable metadata is
available while preserving rendered-name fallback behavior.

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
- Step 4B added sema structured local mirrors only for NTTP placeholders in
  `validate.cpp`; it did not alter parser metadata, HIR/type/codegen identity,
  template rendering, diagnostics, expectations, or consteval NTTP call
  binding maps.
- Keep Step 5 sema-only unless the supervisor explicitly delegates consteval
  mirror plumbing; no HIR/type/codegen migration, template rendered-name
  cleanup, or expectation rewrites.
- Enum mirror population now depends on parser-owned `enum_name_text_ids`; keep
  any future enum work on that definition-time metadata and do not re-derive
  stable identity from rendered strings.

## Proof

Passed.

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "(frontend_parser_tests|cpp_parse_template_|cpp_positive_sema_template_.*nttp|cpp_positive_sema_template_type_context_nttp_parse_cpp|cpp_positive_sema_template_variadic_|cpp_positive_sema_consteval_nttp_cpp)"' > test_after.log 2>&1`

Proof log: `test_after.log`. The delegated subset passed 25/25 tests.
