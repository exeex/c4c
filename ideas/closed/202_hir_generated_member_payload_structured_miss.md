# HIR Generated Member Payload Structured Miss

Status: Closed
Created: 2026-05-12
Closed: 2026-05-12

Depends On:
- `ideas/open/195_frontend_to_bir_legacy_string_lookup_closure_gate.md`

Unblocked:
- `ideas/open/195_frontend_to_bir_legacy_string_lookup_closure_gate.md`

## Goal

Retire or hard-fence the HIR generated-member payload path that extracts owner
and member candidates from rendered qualified spelling and then falls through
to rendered owner/member lookup after structured misses.

Generated member lookup should use structured owner/member authority when
metadata exists and fail closed when that structured authority misses
completely.

## Why This Idea Exists

Idea 195 Step 3 found that `src/frontend/hir/impl/expr/scalar_control.cpp`
enters the generated-member path only after
`structured_owner_key_from_qualified_ref` succeeds, but then extracts
`member_name` and `generated_member_name` from rendered qualified spelling.

The lookup first tries structured `HirStructMemberLookupKey`, but falls through
to rendered `find_struct_static_member_decl(lookup_owner, candidate.name)` and
`find_struct_static_member_const_value(lookup_owner, candidate.name)`. That
means a complete structured owner/member miss can still be repaired by
rendered owner/member spelling.

## In Scope

- Audit the generated-member route in
  `src/frontend/hir/impl/expr/scalar_control.cpp`.
- Ensure metadata-rich generated-member lookup does not recover through
  rendered owner/member spelling after complete structured misses.
- Preserve rendered names only where they are diagnostics, final spelling,
  display text, or explicit no-metadata compatibility.
- Add or update focused tests showing stale rendered owner/member spelling
  cannot repair a structured generated-member miss.
- Document any retained generated-member compatibility fallback with owner,
  limitation, and removal condition.

## Out Of Scope

- Rewriting the full HIR member lookup model.
- Removing diagnostic, final-output, or display-only member spelling.
- Parser, sema, LIR, BIR, or backend compatibility retirement.
- Weakening generated-member support, marking supported cases unsupported, or
  changing expectations only to hide the fallback.

## Acceptance Criteria

- Metadata-rich generated-member payload extraction no longer uses rendered
  owner/member lookup as semantic authority after a complete structured miss.
- Any retained rendered owner/member fallback is fenced as explicit
  no-metadata compatibility with a concrete owner and removal condition.
- Focused tests cover stale rendered owner/member spelling and prove the
  structured generated-member path fails closed or resolves by structured
  identity.
- The closure notes state whether idea 195's generated-member payload blocker
  is cleared.

## Closure Notes

Idea 195 may treat its generated-member payload blocker as resolved. The
metadata-rich generated-member path is fenced so a complete structured
owner/member lookup miss cannot recover semantic authority through stale
rendered owner/member spelling.

Retained compatibility owner: no-metadata/generated-member callers that cannot
form complete structured owner/member lookup keys.

Retained compatibility limitation: rendered owner/member lookup remains only
for explicit no-metadata compatibility or non-semantic text/display paths; it
is not semantic authority after a complete structured miss.

Retained compatibility removal condition: remove the rendered compatibility
fallback once all generated-member callers can provide complete structured
owner/member metadata.

Close-time proof used matching `test_before.log` and `test_after.log` for
`frontend_hir_lookup_tests`; `c4c-regression-guard` passed with
`--allow-non-decreasing-passed`.

Broader supporting proof recorded before closure:
`frontend_hir_tests` passed 1/1, and the accepted full-suite baseline at commit
`4bbc48abc` had 3137/3137 passing.

## Reviewer Reject Signals

- A slice claims progress while `scalar_control.cpp` can still repair a
  metadata-rich generated-member miss through rendered
  `find_struct_static_member_decl` or `find_struct_static_member_const_value`.
- The fix only renames candidate extraction or lookup helpers while preserving
  rendered owner/member authority after complete structured misses.
- Tests only update dump text, diagnostic spelling, or rendered-name
  expectations.
- A supported generated-member route is downgraded, skipped, or marked
  unsupported without explicit user approval.
- The route expands into broad HIR object/member rewrites or backend restart
  work instead of the narrow generated-member blocker.
- The exact old rendered owner/member recovery behavior remains reachable
  behind a new abstraction name.
