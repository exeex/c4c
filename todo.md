Status: Active
Source Idea Path: ideas/open/69_aarch64_call_publication_prepared_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Consume Prepared Call Plans And Boundary Move Facts

# Current Packet

## Just Finished

Completed the Step 2 residue audit after the prepared-effect
register-destination branch work. The in-scope explicit-move
register-destination migration is complete: remaining `BeforeCall` /
`CallArgumentAbi` checks in `calls.cpp` are not uncovered destination
construction branches.

## Suggested Next

Advance to `plan.md` Step 3, unless the supervisor explicitly approves a
separate Step 2 stack-slot destination-copy packet. No remaining in-scope
Step 2 register-destination explicit-move packet was found.

## Watchouts

- Fallthrough guards: `call_boundary_move_selection_status`, the final
  register argument unsupported diagnostic, `is_aarch64_byval_register_lane_move`,
  and `lower_before_call_immediate_binding` still contain raw
  `BeforeCall` / `CallArgumentAbi` shape checks, but they classify or guard
  existing records rather than choosing destination authority.
- Stack-slot destination copies: the remaining raw `BeforeCall` /
  `CallArgumentAbi` stack-slot branches lower byval stack lanes, aggregate
  stack copies, binary128 stack stores, and scalar frame-slot stores. These are
  stack destination-copy paths and should stay out of Step 2 register-dest
  cleanup unless delegated as a distinct stack-slot packet.
- Publication-ordering / out-of-scope: the immediate-cast call-argument
  publication reload check and symbol-address materialization skip operate
  after a destination has already been selected or after a publication was
  already materialized. Treat these as publication/ordering residue, not a
  remaining register-destination migration.
- Concrete next migration: none found for Step 2's in-scope
  register-destination explicit-move branch family.

## Proof

Command: `git diff --check -- todo.md`

Result: passed.
