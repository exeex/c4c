Status: Active
Source Idea Path: ideas/open/380_aarch64_function_pointer_table_relocation_dispatch.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Table Relocation Dispatch

# Current Packet

## Just Finished

Step 3 repaired AArch64 indirect-callee lowering for pointer-typed selected
values from global function-pointer tables. The dispatch path now resolves an
indirect callee that reloads a same-block local pointer back to the latest
stored selected value, materializes the select chain into the prepared callee
GPR with `cmp`/`csel`, records that register as the emitted callee value, and
does this before ordinary call-argument moves can clobber the selector input.

The focused CTest
`backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call`
now passes because `call_index` selects the dynamic table entry into the same
register used by `blr`.

## Suggested Next

Supervisor can review and commit the completed Step 3 slice, then decide
whether this plan is exhausted or needs a follow-up lifecycle decision.

## Watchouts

The repair is semantic rather than filename- or symbol-shaped: it follows the
prepared indirect callee value, same-block local store/load relationship, and
selected direct-global-load chain. Existing store-local lowering still emits an
older stack publication for the selected pointer before the callee repair; the
new pre-call callee materialization overrides the prepared `blr` register.

## Proof

Ran the supervisor-selected proof exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

`test_after.log` is the canonical proof log for this packet. The build
completed and all 147 backend tests passed, including
`backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call`.
