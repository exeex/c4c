# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 narrowed the AST dependency helper route for NTTP template-argument
expressions in `src/frontend/parser/ast.hpp`. When
`Node::template_arg_exprs[arg_index]` is present, dependency detection now walks
the expression AST and uses `NK_VAR::unqualified_text_id` before considering
rendered `template_arg_nttp_names`; the rendered-name fallback remains only for
true no-expression-carrier forwarded NTTP args.

Focused coverage in
`tests/frontend/frontend_parser_lookup_authority_tests.cpp` now proves a stale
rendered NTTP expression string cannot recover dependency when the expression
carrier points at a different `TextId`, while matching expression-carrier
`TextId` metadata and no-carrier forwarded-name compatibility still work.

## Suggested Next

Continue Step 4 review after the next review trigger. A likely next narrow
inspection route is another remaining rendered `tag`,
`deferred_member_type_name`, or `debug_text` fallback in `base.cpp`; verify
whether an existing `TextId`, `record_def`, origin key, or expression carrier
can narrow the fallback before preserving it as no-carrier compatibility.

## Watchouts

- This packet intentionally did not edit HIR, LIR, BIR, backend, `plan.md`, or
  `ideas/open`.
- The patched AST dependency route intentionally preserves rendered
  `template_arg_nttp_names` lookup only when no `template_arg_exprs` carrier and
  no `template_arg_nttp_text_ids` carrier are present.
- Rendered `tag`, `deferred_member_type_name`, and `debug_text` fallback still
  exists for no-carrier compatibility. Do not reclassify those fallback paths
  as semantic authority in later packets.
- The expression walker covers the AST links used by parsed NTTP expressions
  (`left`, `right`, `cond`, `then_`, `else_`, and `children`) and uses rendered
  expression-node names only when an individual `NK_VAR` has no
  `unqualified_text_id`.
- The no-carrier compatibility routes remain separate Step 4 watch items. Do
  not count this packet alone as source-idea closure.
- Existing untracked `review/step4_*.md` artifacts were left untouched.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp)$' | tee test_after.log`.

Result: build completed successfully; CTest passed `886/886` matched tests.
Final proof log path: `test_after.log`.
