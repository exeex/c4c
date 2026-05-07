# Current Packet

Status: Active
Source Idea Path: ideas/open/147_rendered_qualified_compatibility_bridge_removal.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Delete or Isolate Compatibility Bridge Helpers

## Just Finished

Step 4 audited the remaining parser display fallback helpers and compatibility
spelling consumers without code churn. The remaining `compatibility_spelling`
and `visible_name_spelling()` paths are display/handoff materialization from
structured lookup results; tests cover fail-closed behavior for rendered
qualified TextIds, stale rendered owners, qualified typedef re-entry, and
current-record/member-key authority.

The remaining parser support compatibility bridges are isolated to
TextId-less/final-spelling compatibility outside qualified-name semantic
authority: constant-evaluation legacy/HIR callers, parser-local no-carrier
record layout probes, and no-metadata template/base/member fallback comments.
Current-owner consumers are already fenced to structured unqualified
`current_struct_tag_text_id` authority before string equality or owner lookup is
allowed.

## Suggested Next

Next packet: supervisor should decide whether Step 4 is acceptance-ready or
needs independent route review before lifecycle closure.

## Watchouts

- No remaining audited parser reference uses rendered qualified spelling as
  qualified-name semantic authority.
- Display spelling remains expected for AST names, typedef/cast handoff text,
  diagnostics, and legacy compatibility surfaces that do not carry metadata.
- `test_after.log` is the canonical proof log for this packet.

## Proof

Delegated proof command ran exactly:

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'cpp_positive_sema_namespaced_out_of_class_method_context_frontend_cpp|frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_.*template|cpp_hir_.*qualified|cpp_hir_.*member_typedef|frontend_hir_tests|cpp_positive_sema_.*(template|alias|dependent_typename)'; } > test_after.log 2>&1`

Result: passed. `test_after.log` records 357/357 tests passing.
