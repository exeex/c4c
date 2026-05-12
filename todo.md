Status: Active
Source Idea Path: ideas/open/200_hir_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Fence Consteval Replay And Pending Identity Names

# Current Packet

## Just Finished

Completed `plan.md` Step 2. The final HIR template value-arg owner-recovery
slice fenced `resolve_member_lookup_owner_tag` so complete tag owner-key misses
are rejected before rendered `tag_text_id`/legacy `struct_defs` recovery when no
template-origin metadata can legitimately realize a new owner, and template
static-member value-arg evaluation now stops before rendered primary recovery
for complete qualified owner-key misses.

Retained rendered compatibility is fenced to no-owner/incomplete-owner paths:
template-origin realization can still proceed, and no-metadata template
static-member value args still use rendered primary lookup.

## Suggested Next

Execute `plan.md` Step 3: fence consteval replay and pending expression identity
so rendered names cannot act as semantic fallback when structured identity is
complete. Start by inventorying the remaining rendered consteval lookup surfaces
after idea 196, including `PendingConstevalExpr::fn_name` or any successor
display/no-metadata state, then pick a focused stale-name replay or pending
consteval proof case.

## Watchouts

- The template static-member fence is intentionally limited to complete
  qualified owner keys with a namespace context; current positive fixtures still
  rely on rendered primary compatibility for parser-generated owner refs that
  do not carry that full owner context.
- Template-origin metadata is not treated as a stale rendered recovery path;
  it may still materialize a fresh owner before returning a tag.

## Proof

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_lookup_tests|frontend_hir_tests|cpp_hir_value_args_residual_structured_metadata|cpp_hir_template_value_arg_static_member_trait|cpp_hir_template_deferred_nttp_(expr|arith_expr|paren_expr|bool_expr|logic_expr|true_expr|number_expr|static_member_expr|cast_static_member_expr|sizeof_pack_expr)|cpp_hir_template_alias_deferred_nttp_static_member)$"' > test_after.log 2>&1`

Result: passed. `test_after.log` is the proof log.
