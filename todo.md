Status: Active
Source Idea Path: ideas/open/215_aarch64_first_machine_node_selection_slice.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Select Spill Reload And Memory Operand Nodes

# Current Packet

## Just Finished

Completed `plan.md` Step 4: Select Spill Reload And Memory Operand Nodes.

`records.cpp` now selects the accepted memory load/store subset only when
prepared memory operands publish supported bases and required value identities,
and unsupported or missing-fact forms fail closed with explicit
`MachineNodeStatusRecord` diagnostics. It also adds structured spill/reload
pseudo-node records that preserve prepared value id/name/type, scratch
register authority, occupied scratch registers, spill slot id, prepared offset
snapshot, memory operand facts, source spill/reload provenance, opcode/pseudo
identity, def/use resources, and memory side effects.

`module.cpp` now derives selected spill/reload machine nodes from existing
prepared spill/reload target-MIR records while retaining the original
`SpillReloadRecord` surface. Focused backend tests inspect selected memory and
spill/reload node fields directly and cover fail-closed memory and incomplete
spill pseudo forms without assembly text.

## Suggested Next

Implement Step 5 by updating the AArch64 markdown consumers to document that
selected machine nodes are structured internal output layered after target-MIR
records, not assembly text or encoder/object output.

## Watchouts

- Do not emit or parse assembly text under this plan.
- Selected spill/reload nodes are derived only from prepared spill/reload
  target-MIR records; rematerialize remains outside the selected subset.
- Memory selection still requires prepared memory access facts and supported
  base identities; register-base, global/string, or missing prepared forms
  remain fail-closed.
- Call/return/prologue/variadic/global/linker selection remains out of scope
  even though placeholder record variants and statuses exist.

## Proof

Ran delegated proof:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed. The enabled `^backend_` subset reported 100% passing for 131
enabled backend tests.

Log path: `test_after.log`.
