Status: Active
Source Idea Path: ideas/open/98_parser_sema_post_cleanup_structured_identity_leftovers.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Prefer Structured Parser Helper Inputs

# Current Packet

## Just Finished

Step 2 - Prefer Structured Parser Helper Inputs is complete.

Updated `src/frontend/parser/parser_support.hpp` and
`src/frontend/parser/impl/support.cpp` so the parser-owned `eval_const_int`
named-constant boundary explicitly treats the `TextId` overload as the primary
structured surface. The string named-constant overload is now labeled as a
compatibility bridge for legacy/HIR proof paths that only carry rendered names.
No parser-owned string named-constant callers were introduced or migrated, and
the evaluator behavior is unchanged.

## Suggested Next

Step 3 sema enum mirror packet: populate the existing enum-constant structured
mirror tables in sema alongside the legacy rendered-name maps, then prove that
enum lookup behavior stays unchanged.

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
- Enum variant mirrors look like the clean first sema implementation after the
  parser helper packet. Template NTTP/type-parameter and consteval NTTP mirror
  work needs a parameter-identity strategy first because current AST template
  parameter arrays do not carry `TextId`s.

## Proof

Passed:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_parser_tests|cpp_parse_.*|llvm_gcc_c_torture_src_builtin_types_compatible_p_c)$"' > test_after.log 2>&1`

Proof log: `test_after.log`.
