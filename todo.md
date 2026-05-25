Status: Active
Source Idea Path: ideas/open/14_c_aggregate_initializer_compound_literal_layout.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair The Narrow Owner

# Current Packet

## Just Finished

Step 2 proved the missing local aggregate frame-address call-argument rule with
focused backend coverage for
`materialize_missing_frame_slot_call_arguments`. Step 3 repaired both live
AArch64 call-boundary routes that could otherwise choose a prior preserved
spill slot for a pointer-typed local aggregate/array argument.

The repaired rule is semantic rather than testcase-shaped: when a frame-slot
call argument is a pointer operand whose prepared value name has a matching
local object lane such as `<aggregate>.0`, lowering materializes the local
object frame address into the call ABI register. It no longer loads the
preserved pointer spill slot for `00216`'s `print(ls2)` shape.

## Suggested Next

Proceed to Step 4 validation for the `00216` family. Suggested packet: run the
supervisor-selected broader nearby aggregate/compound-literal proof needed to
confirm no adjacent same-feature case regressed.

## Watchouts

The local lane object does not need `address_exposed=yes`; the aggregate
pointer operand is the address-bearing value. The rule is guarded by the BIR
call operand type when BIR context is available, and explicit non-address
selection coverage still rejects rederiving a frame-slot address for ordinary
value loads.

## Proof

Ran exactly:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00216_c)$') > test_after.log 2>&1`

Result: build succeeded; `backend_aarch64_instruction_dispatch` passed;
`c_testsuite_aarch64_backend_src_00216_c` passed. Proof log:
`test_after.log`.
