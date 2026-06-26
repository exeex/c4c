Status: Active
Source Idea Path: ideas/open/387_rv64_object_route_same_module_sret_calls.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Focused Backend Coverage

# Current Packet

## Just Finished

Step 3 added the narrow RV64 object-emission prerequisite for stack-homed
`sret_param` pointer-value local memory stores.

The new object-emission route recognizes a prepared `StoreLocalInst` memory
access with `base=pointer_value` when the pointer value is stack-homed and its
matching stack object is an address-exposed permanent `source_kind=sret_param`
object. The emitted sequence loads the sret pointer value from its stack home
into a temporary GPR, materializes the scalar store value into another GPR, and
stores through the loaded pointer plus the prepared byte offset.

Focused backend coverage now proves the accepted shape with a fixture that
stores an `i32` through a stack-homed `%ret.sret` pointer and asserts the RV64
sequence `ld` from the pointer home followed by an indirect `sw`. Fail-closed
coverage preserves rejection for unsupported source kinds, non-permanent homes,
unsupported home/register shapes, mismatched frame-slot metadata, non-default or
volatile accesses, non-base-plus-offset access, out-of-range offset, and
over-aligned scalar accesses.

The same-module `memory_return` gate was not deleted or weakened in this
packet.

Additional representative evidence:
`build/agent_state/387_step3_920908-1.run.log` shows the
`920908-1.c` object route now advances past the prior
`unsupported_local_memory_access` boundary and reaches
`unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering`,
which is consistent with returning to the same-module call/memory-return
emission boundary.

## Suggested Next

Execute the next idea 387 packet against the now-exposed same-module
`memory_return=%t8` call-emission boundary. Start from the Step 1 prepared call
plan and the Step 3 representative log; do not revisit local-memory admission
unless the new evidence contradicts this repair.

## Watchouts

- The sret-param helper is intentionally separate from the existing byval
  stack-slot pointer helper. Byval still treats stack-homed byval payload as
  direct storage; sret loads the pointer and stores through it.
- The next boundary should be call emission for prepared same-module
  `memory_return` plans, not another local-memory route.
- Do not hard-code `920908-1.c`, callee `f`, stack offsets, or the generic
  unsupported-instruction diagnostic.

## Proof

Delegated proof command run:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.

Result: passed; `test_after.log` reports `100% tests passed, 0 tests failed
out of 326`.

Focused pre-proof also passed:
`cmake --build --preset default --target backend_riscv_object_emission_test && build/tests/backend/mir/backend_riscv_object_emission_test`.

Representative evidence log:
`build/agent_state/387_step3_920908-1.run.log`.
