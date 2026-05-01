# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.4.5C
Current Step Title: Add Dependent/Template Record Member-Typedef Carrier

## Just Finished

Step 2.4.4.5B bridge deletion was attempted and then fully reverted by the
executor. The attempt is blocked and incomplete, with no code progress claimed.

The failed attempt showed that deleting the dependent/template member-typedef
bridge still regresses an alias-of-alias dependent member-typedef route such as
`remove_ref_t<B&>` inside another alias template. The missing piece is a
structured carrier for that alias-of-alias dependent member-typedef case, not
another rendered/deferred `TypeSpec` composition path.

The bridge targets remain live and not deleted:
`apply_alias_template_member_typedef_compat_type`, the dependent
rendered/deferred `TypeSpec` projection, and fallback
`find_alias_template_info_in_context`.

## Suggested Next

Next coherent packet is Step 2.4.4.5C, extended only within the existing
runbook scope: add or thread the missing structured carrier for alias-of-alias
dependent member-typedef availability before another Step 2.4.4.5B bridge
deletion attempt.

Do not retry deleting the bridge until the carrier covers
`cpp_positive_sema_template_alias_member_typedef_dependent_ref_runtime_cpp`
without materializing through rendered/deferred `TypeSpec` fields or timing out.

## Watchouts

- Step 2.4.4.5B remains incomplete. Its bridge-deletion targets are preserved
  until the missing structured carrier exists and passes proof.
- Step 2.4.4.5C already covers the dependent/template record member-typedef
  carrier route; use it for the newly exposed alias-of-alias dependent
  member-typedef carrier instead of rewriting `plan.md`.
- Keep bridge removal semantic and carrier-driven. Do not replace the failed
  local composition with named-test matching, expectation downgrades, rendered
  owner/member splitting, debug-text argument parsing, or another
  rendered/deferred `TypeSpec` relay.
- HIR member-typedef resolver cleanup remains out of scope for idea 139 and
  belongs to idea 140.

## Proof

Failed Step 2.4.4.5B proof on disk:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' >> test_after.log 2>&1`

Result: failed. Regression guard comparison against `test_before.log` failed:
before 881/881, after 880/881. New failing test:
`cpp_positive_sema_template_alias_member_typedef_dependent_ref_runtime_cpp`.

`test_after.log` is the canonical failed proof log path for the reverted
Step 2.4.4.5B attempt. No code progress should be claimed from that attempt.
