# Execution State

Status: Active
Source Idea Path: ideas/open/58_semantic_lir_to_bir_gap_closure_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit The Active Semantic-Lowering Family
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Step 1 packet audited `c_testsuite_x86_backend_src_00051_c` and confirmed the
first still-owned idea-58 seam was not phi planning after all but plain
`LirSwitch` lowering in `src/backend/bir/lir_to_bir_module.cpp`. The prior
failure was real upstream semantic/BIR ownership: `lower_block_terminator()`
only admitted `ret`, `br`, and `cond_br`, so switch-heavy cases never reached
prepared x86. The packet added integer `LirSwitch` lowering as a compare-ladder
of BIR `Eq` + `CondBranch` blocks, which moves `00051` past the old
`semantic lir_to_bir ... scalar-control-flow semantic family` diagnostic.
Under the delegated proof, that testcase now fails later with duplicate asm
labels in emitted x86 output, so the packet stops at honest Step 1 audit-plus-
graduation rather than widening into downstream emitter work outside the owned
files.

## Suggested Next

Treat `00051` as graduated out of idea 58 and hand the new duplicate-label
backend failure to the supervisor for downstream routing, likely under the
x86-side scalar/control-flow leaf rather than this upstream semantic bucket.
For this runbook, the next idea-58 packet should stay on a still-owned
semantic family such as the `00037` scalar-cast seam or the `00050` load
local-memory seam instead of reopening `00051` here.

## Watchouts

- Keep this runbook on upstream semantic/BIR gaps; route stack/addressing
  ownership to idea 62 when that is the more precise failure family.
- Do not add x86 emitter special cases for cases that still fail before
  prepared emission exists.
- Preserve downstream routing when a case graduates into ideas 59, 60, or 61
  instead of silently keeping it under idea 58.
- The new switch lowering is intentionally generic integer-switch lowering, not
  a `00051`-shaped shortcut, but it is only proven so far on a non-phi switch
  testcase. Switch successors with phi-sensitive predecessor identity may need
  their own later packet if they appear.
- The current blocker is outside the owned files: `00051` now reaches x86 asm
  emission and fails with duplicate labels such as `.Lmain_block_13` and
  `.Lmain_block_16`, so do not spend more idea-58 budget inside
  `src/backend/mir/x86/**` from this packet.

## Proof

Ran the delegated proof command and preserved `/workspaces/c4c/test_after.log`:
`cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_.*|c_testsuite_x86_backend_src_00051_c)$' | tee test_after.log`.
Current result: all `74` `backend_*` tests pass, and
`c_testsuite_x86_backend_src_00051_c` no longer fails with the old idea-58
`semantic lir_to_bir` scalar-control-flow diagnostic. The proof still ends
nonzero overall because `00051` has graduated into a downstream `[BACKEND_FAIL]`
during x86 asm emission with duplicate labels in
`build/c_testsuite_x86_backend/src/00051.c.s`. For idea-58 acceptance, that
route change is acceptable progress because the testcase has left the owned
semantic-lowering bucket. This slice is therefore commit-ready for idea-58
runbook progress even though the delegated proof remains red on the downstream
backend failure.
