# Current Packet

Status: Active
Source Idea Path: ideas/open/147_rendered_qualified_compatibility_bridge_removal.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Delete or Isolate Compatibility Bridge Helpers

## Just Finished

Step 4 fixed the blocking `review/147_step4_route_review.md` HIR finding in
template value-arg constant evaluation. `try_eval_template_value_arg_expr()` no
longer splits rendered qualified `NK_VAR::name` spelling into owner/member
authority before static-member lookup; member lookup now comes from
`unqualified_*` metadata or an unqualified spelling, and owner authority comes
from structured `Node` qualifier metadata before static-member const lookup is
allowed.

Focused HIR lookup tests now cover rendered-only stale owner/member rejection
and retained structured owner/member static-const evaluation.

## Suggested Next

Next packet: supervisor should decide whether Step 4 needs another route review
or can proceed toward Step 5 closure validation.

## Watchouts

- Structured instantiated owners such as generated `Count_N_3` still evaluate
  through the `Node` qualifier carrier; rendered-only `StaleOwner::value`
  without qualifier metadata now fails closed.
- Member suffix fallback from rendered spelling is only used after a structured
  owner carrier exists, so it does not reauthorize rendered owner splitting.
- `test_after.log` is the canonical proof log for this packet.

## Proof

Delegated proof command ran exactly:

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'cpp_positive_sema_namespaced_out_of_class_method_context_frontend_cpp|frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_.*template|cpp_hir_.*qualified|cpp_hir_.*member_typedef|frontend_hir_tests|cpp_positive_sema_.*(template|alias|dependent_typename)'; } > test_after.log 2>&1`

Result: passed. `test_after.log` records 357/357 tests passing.
