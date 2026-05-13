Status: Active
Source Idea Path: ideas/open/215_aarch64_first_machine_node_selection_slice.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Select Scalar And Branch Nodes

# Current Packet

## Just Finished

Completed `plan.md` Step 3: Select Scalar And Branch Nodes.

`records.cpp` now tightens scalar and branch machine-node selection over the
accepted target-MIR record subset:

- supported integer scalar ALU and simple cast records stay selected with
  concrete `MachineOpcode` values, typed operands, prepared value provenance,
  and def/use metadata
- unsupported scalar ALU and cast records now fail closed as
  `DeferredUnsupported` machine nodes with explicit diagnostics instead of
  looking selected with an unspecified opcode
- materialized-bool conditional branches now surface the condition value as a
  structured node operand when no physical condition operand is already present
- fused integer compare branches now surface condition plus compare operands as
  structured node operands, preserve predicates and compare widths in the
  payload, and fail closed when required compare facts or fusable-candidate
  authority are missing

Focused backend tests now inspect structured scalar, materialized-bool branch,
and fused compare-branch machine-node fields directly, including operand order,
prepared value ids, register uses, opcode identity, selection status, and
unsupported-case diagnostics without relying on assembly text.

## Suggested Next

Implement Step 4 by selecting the narrow spill/reload and memory operand subset
into structured machine-node fields, preserving prepared slot/scratch/memory
facts and keeping unsupported forms fail-closed.

## Watchouts

- Do not emit or parse assembly text under this plan.
- Select only from accepted target-MIR records; do not bypass through rendered
  BIR, old examples, or fixture names.
- Branch compare records still preserve predicate and compare width in the
  payload; there is no condition-code spelling or final compare instruction
  encoding in this slice.
- Step 4 should keep call/return/prologue/variadic/global/linker behavior out
  of scope even though placeholder record variants and statuses exist.
- The spill/reload pseudo vocabulary is present, but the module spill/reload
  bridge still belongs to the later spill/reload selection packet.

## Proof

Ran delegated proof:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed. The enabled `^backend_` subset reported 100% passing for 131
enabled backend tests.

Log path: `test_after.log`.

Also ran `git diff --check`: passed. `clang-format` is not installed in this
environment.
