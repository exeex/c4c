Status: Active
Source Idea Path: ideas/open/10_calls_printing_and_effects_owner_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Retire Misleading Calls Printing Surfaces

# Current Packet

## Just Finished

Completed Step 4 - Retire Misleading Calls Printing Surfaces. Retired the
remaining stale call-family effect/publication surface by moving the
call-preserve effect publication toggle from `calls.*` to the effect owner, and
removed duplicate call machine-node constructor declarations from `calls.hpp`.

Changed files:

- `src/backend/mir/aarch64/codegen/asm_emitter.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/effects.hpp`
- `src/backend/mir/aarch64/codegen/effects.cpp`

Specific outcomes:

- `ScopedPreparedCallPreserveEffectPublication` and the backing thread-local
  publication toggle now live in `effects.*`.
- `asm_emitter.cpp` now includes `effects.hpp` for that effect publication
  guard instead of depending on `calls.hpp`.
- `calls.cpp` queries
  `prepared_call_preserve_effect_publication_enabled()` from the effect owner
  while preserving the previous publication behavior.
- `calls.hpp` no longer redeclares `make_call_instruction`,
  `make_call_boundary_move_instruction`, or
  `make_call_boundary_abi_binding_instruction`; the semantic machine-node
  constructors remain declared by `instruction.hpp` and defined in `calls.cpp`.
- No build metadata needed removal; no stale `calls_printing.cpp` or equivalent
  build entry remains.

## Suggested Next

Delegate Step 5 as validation-only cleanup acceptance: review the Step 2-4 diff
for output preservation, then run the supervisor-selected broader
printer/backend validation before lifecycle closure decisions.

## Watchouts

- `make_call_instruction`, `make_call_boundary_move_instruction`, and
  `make_call_boundary_abi_binding_instruction` are semantic call machine-node
  constructors that must remain; this packet only removed their stale duplicate
  declarations from `calls.hpp`.
- `effects_from_prepared_call_clobbers` and
  `effects_from_prepared_call_preserved_values` remain in `effects.*`; their
  names are call-specific because they convert prepared call facts into generic
  machine effects under the effect owner.
- No test expectations were changed.
- There is an unrelated untracked file under `review/` in the working tree;
  leave it untouched for this packet.

## Proof

Ran the supervisor-selected proof and captured output in `test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_machine_printer|backend_aarch64_target_instruction_records|backend_aarch64_call_boundary_owner|backend_aarch64_instruction_dispatch|backend_call_boundary_effect_plan)$'`

Result: build passed; 5/5 selected tests passed.

Additional AST-backed checks:

- `c4c-clang-tool-ccdb function-callees` on `make_call_instruction` confirms it
  now calls the effect-owner publication query and effect conversion helpers.
- `c4c-clang-tool-ccdb list-symbols` on `effects.cpp` confirms the publication
  toggle and scoped guard definitions now live in the effect owner.
