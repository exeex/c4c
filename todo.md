Status: Active
Source Idea Path: ideas/open/29_riscv_prepared_edge_publication_pointer_base_policy_broadening.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory RISC-V Pointer-Base Policy Options

# Current Packet

## Just Finished

Step 1 inventoried the RISC-V prepared edge-publication pointer-base policy.
Target helpers are `render_edge_publication_source_operand`,
`consume_edge_publication_move_intent`, and
`append_edge_publication_move_instruction` in
`src/backend/mir/riscv/codegen/emit.cpp`, with intent state in
`src/backend/mir/riscv/codegen/emit.hpp` and focused coverage in
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`.

Current source facts available to codegen:

- Shared authority comes from
  `find_unique_indexed_prepared_edge_publication` over
  `lookups->edge_publications`; clearing that index produces
  `MissingPublication`.
- The pointer source home exposes `pointer_base_value_name` and
  `pointer_byte_delta`; missing base names, unresolved base names, and absent
  deltas currently return `UnsupportedSourceHome`.
- Base names resolve through `lookups->value_homes.value_ids`, then
  `lookups->value_homes.homes_by_id`; the accepted baseline requires the base
  home to be `Register` with `register_name`.
- The current helper accepts only signed-12-bit deltas (`-2048..2047`) and
  renders `mv dst, base` for zero or `addi dst, base, delta` otherwise.
- Destination facts are available after source rendering; register
  destinations expose `destination_home.register_name`, while stack
  destinations remain outside this plan.

Selected first broadened form for Step 2: `PointerBasePlusOffset -> Register`
with a register base and an out-of-`addi`-range delta, only when the destination
register is distinct from the base register so the destination can materialize
the delta without a new scratch-register policy. Candidate rendering is a
target-local multi-instruction materialization such as `li dst, delta` followed
by `add dst, base, dst`.

Fail-closed forms to preserve: missing shared lookup, missing shared
publication, non-move publication, missing source home, missing destination
home, missing pointer base name, unresolved pointer base name, absent pointer
delta, non-register base homes, destination/base register alias for large
deltas until scratch policy exists, and pointer-base sources feeding stack
destinations.

## Suggested Next

Execute Step 2 by extending only the RISC-V prepared edge-publication consumer
for the selected large-delta register-base, distinct-destination form, with
positive coverage for that form and negative coverage for the destination/base
alias fail-closed case.

## Watchouts

Keep shared `edge_publications` and prepared value-home lookup as the only
semantic authorities. Do not fold source-to-`StackSlot` destination policy or
stack-source register-destination policy into this plan.

Large-delta support should not introduce a hidden scratch register. If the
destination aliases the base register, keep the move unsupported unless a
separate scratch policy is explicitly delegated.

## Proof

Passed: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_prepared_edge_publication$' > test_after.log 2>&1`

Proof log: `test_after.log`.
