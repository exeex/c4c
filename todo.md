Status: Active
Source Idea Path: ideas/open/635_prepared_branch_stack_clobber_safety_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish Or Validate Prepared Safety Evidence

# Current Packet

## Just Finished

Step 3 replaced the `terminator_instruction_index == 0` placeholder in
`prepared_collected_branch_stack_load_clobber_safe` with prepared-layer
intervening-clobber proof for selected pointer `Lhs`/`Rhs` branch stack-load
sources.

The new proof is bound to the selected `BranchStackLoadSource` freshness
candidate, exact stack home/frame slot/object, exact branch block index, and
exact branch terminator instruction index before `plan_prepared_branch_stack_load_authority`
can mark the row available. It fails closed for absent or stale selected
freshness, unknown instruction boundary, same-slot stores, call/helper
instructions, matching store publications, and stack-slot move-bundle writes.
Non-side-effecting intervening `Binary`, `Cast`, `Select`, `Phi`, and load
instructions no longer block proven pointer branch stack-load authority solely
because the terminator index is nonzero.

Focused tests now cover a collected branch with nonzero terminator index and an
exact same-slot intervening store that rejects only the clobbered pointer row as
`MissingStackClobberSafety`.

## Suggested Next

Step 4 should validate the consumer admission boundary for the seven prepared
pointer branch rows: confirm RV64 fused pointer branch admission now observes
`Available` prepared branch stack-load authority with selected freshness and no
longer stops at `MissingStackClobberSafety`. Keep the packet consumer-focused;
do not broaden into no-candidate freshness, select publication, or terminator
lowering.

## Watchouts

The authority still intentionally reports the existing boolean failure as
`MissingStackClobberSafety`; no new public branch clobber-status enum was added
in this packet. Store publications and stack-slot move bundles are rejected
when they target the same object/slot or exact stack offset; unknown same-slot
classification remains fail-closed. Step 4 should not bypass the prepared
authority in RV64.

## Proof

`cmake --build --preset default --target backend_prepare_stack_layout_test -j1 && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_stack_layout$' > test_after.log 2>&1`

Result: passed. Proof log: `test_after.log`.
