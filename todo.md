# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.4.2
Current Step Title: Add Template-Instantiation Member-Typedef Carrier

## Just Finished

Lifecycle decision for Step 2.4.4.2: keep the blocker inside the active
parser/Sema lookup-removal plan and widen this step to own the parser metadata
carrier/API needed by the template-instantiation member-typedef writer.

The blocked executor inspection found that the writer has the required
non-rendered concrete-instantiation ingredients at the call site:

- primary template owner: `QualifiedNameKey template_key`
- concrete arguments: `std::vector<TemplateInstantiationKey::Argument>`
- member carrier: `inst->member_typedef_names[ti]` plus a parser `TextId`

The missing piece is parser-owned storage/lookup keyed by
`TemplateInstantiationKey concrete_owner + TextId member_text_id`.
`binding_state_.struct_typedefs` is keyed by `QualifiedNameKey`, which cannot
encode the concrete template argument vector. Writing there by rendering
`mangled` keeps rendered instantiation text as owner identity; keying only the
primary template aliases different instantiations of the same primary.

`plan.md` now makes Step 2.4.4.2 explicitly own the minimal parser carrier/API,
writer conversion, reader plumbing for concrete template owner plus member
`TextId`, and focused disagreement proof. No implementation progress is
accepted from the blocked packet, and no separate idea was opened because this
is parser-owned metadata required to finish the active source idea.

## Suggested Next

Next bounded packet: execute widened Step 2.4.4.2. Add the parser-owned
structured member-typedef storage/API for
`TemplateInstantiationKey concrete_owner + TextId member_text_id`, convert the
template-instantiation writer in `src/frontend/parser/impl/types/base.cpp` to
use it, add only the reader plumbing that starts from the same concrete owner
carrier, and produce focused parser proof plus fresh `test_after.log`.

## Watchouts

Do not convert this writer by setting `inst->unqualified_text_id` from
`mangled` and calling `record_member_typedef_key_in_context`; that still uses
rendered concrete instantiation text as owner identity and loses the structured
argument vector. Do not key on the primary template alone; that would collide
`Template<int>::member` with `Template<float>::member`.

Do not add a generic helper that takes rendered `Template<Args>::member`,
`owner::member`, `std::string`, or `std::string_view` and parses it back into
semantic owner/member identity. If implementation discovers that the required
reader crosses HIR, LIR, BIR, or backend metadata, stop and route that boundary
as a separate open idea.

## Proof

Lifecycle-only plan/todo rewrite. No implementation files or tests were
changed, no code proof is claimed, and `test_after.log` was intentionally left
unchanged.
