Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 1 `Name The First Prepared Guard Lane` by classifying
the next x86 prepared-handoff family as straight-line `i32` equality guards
with immediate-coded early returns over already-prepared operands. The core
proving cluster is `00047`, `00048`, `00054`, and `00055`; `00049` remains an
adjacent pointer-indirect variant that fails at the same emitter boundary but
is not required to name the first lane.

## Suggested Next

Start `plan.md` Step 2 by widening the prepared x86 guard path from the current
compare-against-zero route to straight-line equality-against-immediate guard
chains with immediate return leaves. Prove against the core cluster
`00047`/`00048`/`00054`/`00055`, and only pull in `00049` if the shared operand
rendering covers pointer-indirect prepared values without separate global-data
work.

## Watchouts

- Do not weaken `x86_backend` expectations to accept fallback LLVM IR.
- Do not add testcase-named shortcuts or rendered-text recognizers.
- Keep this route at the prepared x86 handoff boundary; do not reopen semantic
  local-memory work as routine follow-up.
- `00045` is still the bootstrap global-data family, `00037` is still a
  scalar-cast family, and `00051` is still multi-block control-flow.
- `00049` shares the same minimal return/guard failure note, but it adds a
  pointer-indirect prepared input and should stay adjacent until the core
  equality-guard lane lands honestly.

## Proof

Executed the delegated Step 1 proof command:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|backend_lir_to_bir_notes|c_testsuite_x86_backend_src_00047_c|c_testsuite_x86_backend_src_00048_c|c_testsuite_x86_backend_src_00049_c|c_testsuite_x86_backend_src_00054_c|c_testsuite_x86_backend_src_00055_c)$' >> test_after.log 2>&1`.
`backend_lir_to_bir_notes` and `backend_x86_handoff_boundary` passed. The
sampled c-tests `00047`, `00048`, `00049`, `00054`, and `00055` all failed
with the same x86 emitter note stating that only the minimal single-block
return terminator or bounded compare-against-zero branch family is currently
supported. Proof log: `test_after.log`.
