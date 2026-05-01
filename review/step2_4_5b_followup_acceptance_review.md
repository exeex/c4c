# Step 2.4.4.5B.2 Follow-Up Acceptance Review

Active source idea path: `ideas/open/139_parser_sema_rendered_string_lookup_removal.md`

Chosen base commit: `624d01855` (`Repair Step 2.4.4.5B carrier packet metadata`)

Reason: the supervisor explicitly requested review of the current unstaged
follow-up implementation after `624d01855`. That commit repaired the packet
metadata and left the current implementation entirely uncommitted.

Commit count since base: 0 committed changes; review scope is the unstaged diff.

## Findings

### High: alias-owner fallback recreates rendered/deferred `TypeSpec` authority

`src/frontend/parser/impl/types/base.cpp:2740` through
`src/frontend/parser/impl/types/base.cpp:2776` is a blocking route-quality
regression for idea 139 and Step 2.4.4.5B.

After the direct structured paths fail, the branch constructs a synthetic
`TypeSpec owner_ts` by calling `render_name_in_context(...)`, storing that
rendered spelling in `owner_ts.tag`, setting `owner_ts.tpl_struct_origin` from
the same string, filling template refs through
`set_template_arg_refs_from_parsed_args(...)`, then calling
`lookup_struct_member_typedef_recursive_for_type(...)`.

That is not acceptable structured reuse. It is a new normal-route fallback that
adapts a structured `QualifiedNameKey` back into the legacy/deferred
`TypeSpec` shape that Step 2.4.4.5B is supposed to delete. The active runbook
explicitly says to preserve owner identity as a direct record/template owner,
`QualifiedNameKey`, namespace-aware key, or other reviewed domain key, and to
avoid seeding or recovering the carrier through `tpl_struct_origin`,
`TypeSpec::tag`, rendered owner spelling, or deferred `TypeSpec` fields. This
branch does the opposite at the final resolution point.

The structured member `TextId` argument to
`lookup_struct_member_typedef_recursive_for_type(...)` reduces one part of the
risk, but it does not make the owner route structured. The owner lookup is now
authorized by a locally reconstructed `TypeSpec` carrier whose owner spelling
was rendered out of `ati->member_typedef.owner_key`; this is the same class of
rendered/deferred owner handoff that the deleted projector existed to provide.

Exact issue to fix before acceptance: remove this rendered `owner_ts` fallback
or replace it with a lookup entry point that accepts the existing owner
`QualifiedNameKey` plus structured argument keys/refs and member `TextId`
directly. Do not pass through `render_name_in_context`, `TypeSpec::tag`,
`tpl_struct_origin`, `deferred_member_type_name`, `debug_text`, or split
`Owner::member` spelling to recover the owner.

### Medium: alias-template owner branch is mostly structured, but should not be the accepted proof while the rendered fallback remains

`src/frontend/parser/impl/types/base.cpp:2351` through
`src/frontend/parser/impl/types/base.cpp:2664` follows an alias-template owner
by looking up `find_alias_template_info(...)` with the existing owner key,
substituting carrier arguments, checking
`find_template_instantiation_member_typedef_type(...)`, and selecting
primary/specialization member typedefs by member `TextId`.

That branch is aligned in shape with Step 2.4.4.5B.2 because it uses the
existing alias-template metadata rather than reparsing `Owner::member` text.
However, it cannot be accepted as proving the route while the later rendered
`owner_ts` fallback is still available as a normal success path.

### Low: projector deletion appears real; no expectation downgrade or named-test shortcut found

The unstaged diff deletes `apply_alias_template_member_typedef_compat_type` and
its local `alias_template_arg_debug_text` helper from
`src/frontend/parser/impl/declarations.cpp`, and removes the only call site
that used the projector after `parse_type_name()`. A targeted search found no
remaining definition or call in the changed parser sources.

The diff is limited to:

- `src/frontend/parser/impl/declarations.cpp`
- `src/frontend/parser/impl/types/base.cpp`
- `src/frontend/parser/impl/types/struct.cpp`
- `todo.md`

No test expectation files are changed, no fixture is marked unsupported, and I
did not find a named-test shortcut in the implementation diff. The proof log
records a fresh successful build plus `881/881` passing
`cpp_positive_sema_` tests.

## Judgments

Idea-alignment judgment: `drifting from source idea`

Runbook-transcription judgment: `plan matches idea`

Route-alignment judgment: `route reset needed` for this acceptance slice until
the rendered `owner_ts` fallback is removed or replaced by a direct structured
lookup

Technical-debt judgment: `action needed`

Validation sufficiency: `needs broader proof` after the route-quality issue is
fixed; the current green subset is not acceptance proof while a forbidden
rendered/deferred authority path remains

Reviewer recommendation: `rewrite plan/todo before more execution` only if the
executor cannot replace the fallback within the current packet; otherwise
revise the implementation before supervisor acceptance

