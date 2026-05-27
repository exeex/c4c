Status: Active
Source Idea Path: ideas/open/43_shared_prepared_typed_stack_source_authority.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Consume Shared Typed Authority In RISC-V

# Current Packet

## Just Finished

Completed Step 3 by consuming
`prepare_same_width_i32_stack_source_publication()` in the RISC-V
StackSlot-to-Register edge-publication load path.

The RISC-V intent now copies the shared same-width `I32` source/destination
types, stack slot/offset/size/alignment, `SameWidthNoExtension`, destination
register bank, and destination register placement before rendering `lw`.
The selected same-width `I32` path proves raw destination register spelling is
not authoritative, while existing concrete same-width `I64` and large-offset
`I64` StackSlot-to-Register paths continue to emit `ld`.

## Suggested Next

Supervisor-side validation/acceptance for the Step 3 slice. A coherent next
check would be the matching regression guard around the same delegated command,
or a broader backend prepared-publication subset if the supervisor wants more
confidence before commit.

## Watchouts

This slice intentionally requires shared typed authority for
StackSlot(size=4)->Register same-width `I32` loads, but it preserves the
pre-existing concrete same-width `I64` StackSlot-to-Register load path.
Subword, widening, F32, dynamic, and aggregate stack-source forms remain
fail-closed. Existing stack-destination source materialization is still guarded
by the older destination-specific path.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_prepared_edge_publication$'`

Result: passed. The accepted proof log was rolled forward to `test_before.log`.
