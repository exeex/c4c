Status: Active
Source Idea Path: ideas/open/842_lir_restricted_first_class_value_unions.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory first-class boundary carriers

# Current Packet

## Just Finished

Lifecycle switched from closed 841 to active 842 after the bounded compact
scalar `LirBinOp` authority slice was accepted.

## Suggested Next

Execute plan.md Step 1: inventory first-class boundary carriers for call
argument/result, PHI, select, and return; select the first bounded migration
target; and record admitted alternatives, wrong-kind exclusions, and proof.

## Watchouts

Do not implement generic 734 Raw-BIR receiver work. Do not add a universal
value bag, generic ID, RTTI/vtable abstraction, or implicit cross-family
adapter. Do not delete boundary `LirTypeRef` fields before all named consumers
for that boundary migrate.

## Proof

Lifecycle-only proof command: `git diff --check`.
