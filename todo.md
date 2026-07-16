Status: Active
Source Idea Path: ideas/open/855_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select The Next Body-Parameter Authority Row

# Current Packet

## Just Finished

Lifecycle switched from exhausted 734 Step 7.43 after accepted receiver commit
`617a8fae9` to the separate producer-handoff blocker for the next
body-parameter row.

## Suggested Next

Execute Step 1 from `plan.md`: identify exactly one next valid
function-body parameter-use row after the accepted Step 7.43 unary-`fneg`
receipt, without touching Raw-BIR/importer receiver code.

## Watchouts

Preserve 734's accepted progress through Step 7.43. Do not reopen accepted
DirectPointer or DirectScalar body-parameter rows, select from presentation
fields, or absorb non-parameter families.

## Proof

Lifecycle-only switch. Run `git diff --check` before returning.
