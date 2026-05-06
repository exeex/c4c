Status: Active
Source Idea Path: ideas/open/145_move_record_tag_authority_from_parser_to_sema.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate Parser Record Lookup Families Without Parser Identity Authority

# Current Packet

## Just Finished

Handled `review/record_tag_step4_route_review.md` by repairing the active
Step 4 route in `plan.md`. The review found route drift in
`resolve_record_type_spec`: a context-defaulted unique same-`TextId`
parser-map candidate was accepted as record identity, and namespace-context
matches did not consistently check full structured metadata.

## Suggested Next

Execute Step 4 before any Step 6 work. The next packet must repair
`resolve_record_type_spec` by either deleting/narrowing the unique same-`TextId`
parser-map path to full structured carrier matches, or replacing the final
record decision with a structured Sema-owned lookup. Do not widen parser-side
identity authority or add new parser-map fallback success conditions.

## Watchouts

`TextId` is spelling identity only. A lookup must not succeed merely because
`struct_tag_def_map` currently has exactly one completed same-`TextId` record.
When namespace context is present, global qualification and qualifier metadata
must still be validated unless Sema owns a canonical key that proves equivalence.

## Proof

Lifecycle-only repair. No implementation validation run.
