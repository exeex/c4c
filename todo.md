# Current Packet

Status: Active
Source Idea Path: ideas/open/147_rendered_qualified_compatibility_bridge_removal.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove Bridge Removal Across Parser, Sema, and HIR

## Just Finished

Step 5 repaired the full-suite regression in non-template structured
static-member lookup. Scalar lowering now lets non-template references use the
structured owner/member key directly, and falls back from stale qualifier
TextIds to the structured qualifier segment carrier when the primary key misses.

`left::Box::value` and `right::Box::value` now fold to `11` and `23` without
splitting rendered `expr->name` or restoring rendered owner suffix recovery.
The focused HIR lookup fixture now covers this stale-TextId, valid-segment
owner-key fallback.

## Suggested Next

Next packet: supervisor or reviewer can treat Step 5 validation as green for
this regression and decide whether any additional full-suite closure proof is
needed.

## Watchouts

- The segment fallback uses parser-carried qualifier segments after a structured
  TextId owner key misses; it does not split rendered `A::B::member` spelling.
- Template static-member paths still require generated member payload,
  owner-tag, or template-origin evidence before template owner realization.
- `rg` still finds no rendered owner suffix recovery pattern in the scalar or
  global-init HIR routes.

## Proof

Delegated proof command ran exactly:

`cd /workspaces/c4c && { cmake --build --preset default && ./build/tests/frontend/frontend_hir_lookup_tests && ctest --test-dir build -j --output-on-failure -R 'cpp_positive_sema_structured_static_member_lookup_runtime_cpp|cpp_positive_sema_namespaced_out_of_class_method_context_frontend_cpp|frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_.*template|cpp_hir_.*qualified|cpp_hir_.*member_typedef|frontend_hir_tests|cpp_positive_sema_.*(template|alias|dependent_typename)'; } > test_after.log 2>&1`

Result: passed. `./build/tests/frontend/frontend_hir_lookup_tests` passed and
CTest records 358/358 passing. `test_after.log` is the preserved proof log.

Supervisor full-suite proof also passed after this repair:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure`

Result: passed. `test_after.log` records 3023/3023 CTest tests passing.
