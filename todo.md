Status: Active
Source Idea Path: ideas/open/202_hir_generated_member_payload_structured_miss.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Trace Generated-Member Authority

# Current Packet

## Just Finished

Step 1 - Trace Generated-Member Authority completed as a trace-only packet.
In `src/frontend/hir/impl/expr/scalar_control.cpp`, qualified nameref static
member handling enters the structured branch through
`structured_owner_key_from_qualified_ref` at lines 207-209, builds
`owner_lookup_keys` with an optional `structured_owner_key_from_qualified_segments`
secondary key at lines 210-218, then recovers rendered/compat owner identities
through `owner_tag`, `owner_name`, and `owner_template_origin_name` at lines
219-267.

Generated-member candidate extraction happens in two passes. The first pass
fills the primary `member_name`/`member_text_id` from rendered `n->name` only
when the normal unqualified member is absent, using
`try_generated_member_payload` over `owner_name`, `owner_tag`, and
`owner_template_origin_name` at lines 288-315. The second pass always captures a
separate `generated_member_name`/`generated_member_text_id` from the same owner
prefix set at lines 317-350. Candidate lookup then adds `member_text_id`,
`unqualified_member_text_id`, and `generated_member_text_id` to
`member_candidates` at lines 430-448.

The structured authoritative lookup is in the local `find_static_member_decl`
and `find_static_member_const_value` lambdas. For each candidate with a valid
text id, each `owner_lookup_keys` entry is converted to a
`HirStructMemberLookupKey` with `make_struct_member_lookup_key`, then looked up
through `find_struct_static_member_decl(*key, nullptr, nullptr)` at lines
454-461 or `find_struct_static_member_const_value(*key, nullptr, nullptr)` at
lines 480-487. Those key overloads intentionally receive null rendered
tag/member pointers, so they should fail closed on complete structured misses.

The exact fallback Step 2 must remove, guard, or fence is the rendered owner /
member fallback inside those same lambdas after each structured miss:
`find_struct_static_member_decl(lookup_owner, candidate.name)` at lines 466-469
and `find_struct_static_member_const_value(lookup_owner, candidate.name)` at
lines 492-495. On generated-member paths, `lookup_owner` is
`realized_owner_tag ? *realized_owner_tag : (owner_tag ? *owner_tag :
owner_name)`, while `candidate.name` can be the generated payload extracted
from rendered `n->name`; this reopens the rendered compatibility maps after a
metadata-rich `HirStructMemberLookupKey` miss.

## Suggested Next

Implement Step 2 by fencing generated-member/static-member paths so complete
structured owner/member lookup misses do not fall through to rendered
`find_struct_static_member_decl(lookup_owner, candidate.name)` or
`find_struct_static_member_const_value(lookup_owner, candidate.name)`, except
for explicitly no-metadata compatibility cases.

## Watchouts

- Idea 195 remains open but inactive/blocked until ideas 201 and 202 are
  resolved or explicitly fenced.
- Do not close idea 195, run backend restart work, or advance milestone
  validation from this blocker runbook.
- Treat a rendered owner/member fallback on metadata-rich generated-member
  paths as a blocker unless it is fenced as explicit no-metadata compatibility.
- Do not claim progress through expectation changes, unsupported markings, or
  helper renames that preserve rendered owner/member authority.
- The rendered overloads in `hir_types.cpp` are documented as no-owner
  compatibility mirrors; the issue traced here is that `scalar_control.cpp`
  calls those rendered overloads after constructing complete structured keys.

## Proof

Trace/todo-only. No build or test validation was run because no code changed.
