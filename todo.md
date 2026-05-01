# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.4.5C
Current Step Title: Add Dependent/Template Record Member-Typedef Carrier

## Just Finished

Step 2.4.4.5C added the missing structured carrier thread for alias-of-alias
dependent member typedefs. A using-alias RHS now clears stale carrier state
before parsing, captures a dependent alias-template member-typedef carrier
emitted by alias-template application, and stores that carrier on the outer
`ParserAliasTemplateInfo` when the RHS is an alias application such as
`remove_ref_t<B&>` rather than a direct `typename Owner<...>::member` spelling.

The carrier remains structured: owner `QualifiedNameKey`, substituted owner
args as parsed template args, and member `TextId`. The target runtime case now
lowers `ns::alias_of_alias_t<int, short>` as `short`/`i16` instead of a
deferred `remove_ref_B_ref` placeholder.

## Suggested Next

Next coherent packet is to let the supervisor decide whether to retry the
Step 2.4.4.5B bridge-deletion slice now that the alias-of-alias dependent
member-typedef carrier exists.

## Watchouts

- Step 2.4.4.5B remains incomplete; this packet intentionally did not delete
  `apply_alias_template_member_typedef_compat_type`, the dependent
  rendered/deferred `TypeSpec` projection, or fallback
  `find_alias_template_info_in_context`.
- The new carrier is intentionally transient through
  `last_using_alias_member_typedef`; keep clearing it before parsing a using
  alias RHS so unrelated alias parses cannot inherit stale member-typedef
  metadata.
- Future bridge deletion should remain semantic and carrier-driven. Do not
  replace this with named-test matching, expectation downgrades, rendered
  owner/member splitting, debug-text argument parsing, or another
  rendered/deferred `TypeSpec` relay.
- HIR member-typedef resolver cleanup remains out of scope for idea 139 and
  belongs to idea 140.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' >> test_after.log 2>&1`

Result: passed. Build succeeded and `cpp_positive_sema_` passed 881/881.

`test_after.log` is the canonical proof log for Step 2.4.4.5C.
