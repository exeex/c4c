Status: Active
Source Idea Path: ideas/open/380_aarch64_function_pointer_table_relocation_dispatch.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Table Relocation First Bad Fact

# Current Packet

## Just Finished

Step 1 localized the `test_multi_relocs` failure without implementation edits.
HIR records `table` as `{sys_one, sys_two, sys_three}`, semantic and prepared
BIR load `@table` at offsets `0`, `8`, and `16`, and emitted global data keeps
three distinct `.xword sys_one/sys_two/sys_three` entries. The first bad fact is
in generated AArch64 dispatch from the BIR selected callee value: the block
loads all three table entries, spills the third, but emits `blr x21`
unconditionally, so every iteration calls the value loaded from `table+8`.

## Suggested Next

Delegate Step 2: add focused AArch64 backend coverage for a selected pointer
value feeding an indirect call callee. The smallest shape is a three-entry
global function-pointer table with distinct targets and a dynamic index, where
the expected assembly/runtime contract requires the selected callee value, not
the middle entry's register, to feed `blr`.

## Watchouts

Do not chase range-designator overwrite order or relocation/global-data
publication for this packet's observed failure: current HIR, BIR, prepared BIR,
and `.s` data evidence all preserve `table[0..2]` as distinct
`sys_one/sys_two/sys_three`. The repair owner appears to be AArch64 lowering of
`bir.select`/select-materialization when the selected result is the prepared
indirect-call callee.

## Proof

Ran the supervisor-selected proof exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00216_c$'; } > test_after.log 2>&1`

`test_after.log` is the canonical proof log for this packet. The build was
up-to-date and the focused CTest failed at the known mismatch: expected
`one/two/three`, actual `two/two/two`.
