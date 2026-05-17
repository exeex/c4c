Status: Active
Source Idea Path: ideas/open/267_aarch64_inline_asm_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Create The Inline-Asm Owner Boundary

# Current Packet

## Just Finished

Completed Step 2 from `plan.md`: created the compiled AArch64 inline-asm owner
boundary in `src/backend/mir/aarch64/codegen/inline_asm.cpp` and
`src/backend/mir/aarch64/codegen/inline_asm.hpp`.

Moved the existing accepted prepared route into that owner:

- prepared inline-asm carrier lookup and the dispatch visibility check
  `has_prepared_inline_asm_carrier(...)`
- complete-carrier-to-selected-assembler-machine-node lowering through
  `lower_inline_asm_instruction(...)`
- clobber/effect validation, operand materialization, tied-home/coallocation
  checks, immediate handling, and prepared memory/address operand selection
- inline-asm template substitution and operand formatting through
  `substitute_inline_asm_template(...)`

`dispatch.cpp` now keeps only the high-level route decision and call into the
inline-asm owner. `machine_printer.cpp` keeps only assembler-family print
handoff and uses the inline-asm owner for template substitution. Public support
was not broadened; unsupported/deferred inline-asm facts still flow through the
existing diagnostics and fail-closed paths.

## Suggested Next

Execute Step 3 from `plan.md`: update or remove the markdown shard/index
references so the documented owner points at the compiled `inline_asm.cpp` and
`inline_asm.hpp` boundary.

## Watchouts

- Step 2 was behavior-preserving: do not claim new public AArch64 inline-asm
  support in docs. Public cases remain blocked unless a later packet proves
  them.
- `inline_asm.cpp` intentionally owns only the current prepared-carrier route,
  not the legacy broad static substitution surface from the markdown shard.
- Atomic helper behavior was not moved or reimplemented.

## Proof

Ran the delegated proof:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Result: PASS, backend subset `139/139` passed. Proof log: `test_after.log`.
