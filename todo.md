Status: Active
Source Idea Path: ideas/open/41_riscv_prepared_edge_publication_dynamic_stack_source_policy.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate Dynamic Stack-Source Policy

# Current Packet

## Just Finished

Step 4 validated the RISC-V dynamic-address `StackSlot -> Register`
edge-publication route as a documented fail-closed prepared-authority blocker.

The backend bucket is green after the fail-closed policy tests from Step 2.
Focused searches of `src/backend/mir/riscv/codegen/emit.cpp` show that no
dynamic stack-source load implementation was introduced:
- `StackSlot` source loads are still gated on `source_stack_offset_bytes`, which
  is recorded only when the source home has concrete `offset_bytes` and size 4
  or 8.
- `PointerBasePlusOffset` still records `source_pointer_byte_delta` and lowers
  through `mv`, `addi`, or `li; add` address-value materialization, not
  `lw`/`ld`/`flw`/`fld`.
- The only `0(t6)` large-offset load sequence remains in the concrete
  `source_stack_offset_bytes` branch.
- The only `lw t0` stack-destination path remains the existing concrete,
  non-aliasing I32 `StackSlot -> StackSlot` path.

Focused searches also show no local edge-copy rediscovery was introduced in
RISC-V edge-publication lowering. `consume_edge_publication_move_intent` still
uses `find_unique_indexed_prepared_edge_publication`; `emit_prepared_module`
still builds shared prepared function lookups and iterates published
publications. There is no new predecessor/successor block scan for edge-copy
facts in the lowering path.

Closure-ready: yes. The source idea is complete as a policy decision and proof
that dynamic-address `StackSlot -> Register` publication remains fail closed
until shared prepared authority provides dynamic address/base anchor, offset
expression/source memory access, load width, and scratch-register facts.

## Suggested Next

Supervisor should hand this active lifecycle state to the plan owner for
closure review of idea 41.

## Watchouts

Do not infer dynamic stack-source addresses from block shape, predecessor or
successor scans, fixture labels, value ids, stack slot ids, offsets, or test
names. Preserve existing concrete 4-byte, 8-byte, and large-offset behavior.
Do not treat `PointerBasePlusOffset` as a dynamic `StackSlot` load; it is
currently an address-value materialization path.

## Proof

Proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log`

Result: passed, 163/163 tests. Proof log: `test_after.log`.

Focused searches:
- `rg -n "\b(l[bhwd]|flw|fld)\b.*\(|source_pointer_byte_delta.*\b(lw|ld|flw|fld)\b|pointer_base.*\b(lw|ld|flw|fld)\b|0\(t6\).*source_pointer|source_pointer.*0\(t6\)" src/backend/mir/riscv/codegen/emit.cpp || true`
  returned only the existing concrete stack-destination `lw t0` path.
- `rg -n "for \(|while \(|blocks|predecessor|successor|insts|find_unique_indexed_prepared_edge_publication|publications_by_edge_destination|make_prepared_function_lookups|value_homes" src/backend/mir/riscv/codegen/emit.cpp`
  showed only the existing tiny LIR path, prepared value-home lookups, shared
  edge-publication lookup consumption, module/publication iteration, and shared
  prepared-function lookup construction.
- `rg -n "StackSlot|PointerBasePlusOffset|source_stack_offset_bytes|source_pointer_byte_delta|\blw\b|\bld\b|0\(t6\)|find_unique_indexed_prepared_edge_publication" src/backend/mir/riscv/codegen/emit.cpp`
  confirmed stack loads are tied to `source_stack_offset_bytes` while
  pointer-base materialization stays separate.

`git diff --check -- todo.md` passed.
