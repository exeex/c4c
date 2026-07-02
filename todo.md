Status: Active
Source Idea Path: ideas/open/543_rv64_object_data_symbol_fixup_module_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Late Object Assembly Ownership

# Current Packet

## Just Finished

Activation created `plan.md` and this executor-compatible `todo.md` skeleton
for Step 1.

## Suggested Next

Begin Step 1 by mapping late object assembly ownership in
`src/backend/mir/riscv/codegen/object_emission.cpp` and
`src/backend/mir/riscv/codegen/object_emission.hpp`.

## Watchouts

- Preserve object bytes, relocations, symbol bindings, section layout, ELF
  flags, public entrypoints, and zero-fill behavior.
- Do not touch tests, expectations, unsupported markers, or runtime contracts.
- Keep `test_baseline.new.log` treated as a rejected full-suite candidate, not
  an accepted baseline.
- Leave `review/global_address_helper_cleanup_review.md` untouched.

## Proof

No validation run for activation-only lifecycle work.
