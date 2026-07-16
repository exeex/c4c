Status: Active
Source Idea Path: ideas/open/841_lir_compact_scalar_abi_leaf_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory scalar and ABI-leaf authority users

# Current Packet

## Just Finished

Closed idea 866 as lifecycle-only remaining-authority triage and activated the
first ordered executable successor, idea 841.

## Suggested Next

Execute Step 1 by inventorying scalar and ABI-leaf authority users, separating
true scalar rows from vector, aggregate, function, opaque, pointer, and void
ABI-leaf cases, and selecting the first bounded migration target.

## Watchouts

Do not implement 734 Raw-BIR receiver work in this idea. Do not assume opaque is
scalar, do not parse rendered text as scalar authority, and do not reopen
accepted receiver rows such as `LirAbsOp` selected-global/i32.

## Proof

Lifecycle activation proof: `git diff --check`. Step 1 may remain
documentation/inventory-only unless it selects and delegates a code slice.
