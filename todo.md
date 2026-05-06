Status: Active
Source Idea Path: ideas/open/145_move_record_tag_authority_from_parser_to_sema.md
Source Plan Path: plan.md
Current Step ID: 4C
Current Step Title: Migrate Remaining Parser Record Lookup Families

# Current Packet

## Just Finished

Step 4B completed the fallback deletion follow-up and was committed as
`0c1abb4b2 Repair structured template constructor handoff`.

## Suggested Next

Continue with plan Step 4C: migrate the remaining parser `sizeof`, `alignof`,
`offsetof`, and support-helper lookup families one executor packet at a time.

## Watchouts

Do not restore parser-map uniqueness, rendered-key matching, or
context-defaulted `TextId` lookup. Step 4C should remove parser-owned semantic
authority from each remaining lookup family while preserving parser-local
grammar support and any explicitly bounded compatibility handoff.

## Proof

Step 4B proof was green before the supervisor rolled `test_after.log` forward
to `test_before.log`. Step 4C still needs fresh build or compile proof plus the
supervisor-selected narrow tests after implementation.
