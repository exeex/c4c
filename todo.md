Status: Active
Source Idea Path: ideas/open/193_hir_constructor_member_owner_structured_lookup_closure.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Constructor And Member Owner Authority

# Current Packet

## Just Finished

Activated idea 193 into `plan.md` with an executor-compatible todo skeleton.

## Suggested Next

Execute Step 1 by inventorying HIR constructor/member owner lookup surfaces in
`src/frontend/hir/impl/expr/object.cpp` and nearby helper APIs, then choose one
metadata-rich route for structured-owner conversion or fencing.

## Watchouts

Do not treat diagnostic/display owner strings as semantic lookup authority. Do
not broaden this into a full object-lowering rewrite. Idea 195 remains blocked
by open idea 193 and still has stale dependency paths involving closed ideas;
repair those paths only when activating or closing that gate requires it.

## Proof

Lifecycle activation only; no code validation run.
