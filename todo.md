Status: Active
Source Idea Path: ideas/open/267_aarch64_inline_asm_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Focused Backend Behavior

# Current Packet

## Just Finished

Completed Step 3 from `plan.md`: deleted the stale
`src/backend/mir/aarch64/codegen/inline_asm.md` markdown shard after the
compiled inline-asm owner boundary was created.

Cleaned the required non-lifecycle references:

- removed the obsolete `codegen/inline_asm.md` row from
  `src/backend/mir/aarch64/CLASSIFICATION_INDEX.md`
- updated the inline-asm bring-up row in
  `src/backend/mir/aarch64/BACKEND_CASE_BRINGUP_MATRIX.md` to point at
  `src/backend/mir/aarch64/codegen/inline_asm.cpp` and
  `src/backend/mir/aarch64/codegen/inline_asm.hpp`

Confirmed no non-lifecycle repository reference still expects
`inline_asm.md`. Remaining mentions are lifecycle context in `plan.md`,
`todo.md`, and the source idea, which this packet left untouched.

## Suggested Next

Execute Step 4 from `plan.md`: record the focused backend proof for preserved
inline-asm redistribution behavior and explicit unsupported behavior.

## Watchouts

- The bring-up matrix still marks public inline-asm cases blocked; this packet
  did not broaden public AArch64 inline-asm support.
- Lifecycle references to `inline_asm.md` remain intentionally as plan/source
  context until the plan is closed by the plan owner.

## Proof

Ran the delegated proof:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Result: PASS, backend subset `139/139` passed. Proof log: `test_after.log`.
