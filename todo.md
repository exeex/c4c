# Execution State

Status: Active
Source Idea Path: ideas/open/58_semantic_lir_to_bir_gap_closure_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The First Semantic/BIR Seam
Plan Review Counter: 4 / 10
# Current Packet

## Just Finished

Step 2 packet repaired the shared still-owned `scalar-control-flow` seam behind
`c_testsuite_x86_backend_src_00089_c` and
`c_testsuite_x86_backend_src_00095_c` by teaching semantic `lir_to_bir`
scalar value lowering to admit direct global operands when the expected value
is a pointer. That upstream repair is general rather than testcase-shaped: the
scalar-control-flow path can now carry pointer-valued global symbols through
phi/return lowering instead of falling out of idea 58 before prepared x86
consumption. Under the delegated proof, both `00089` and `00095` no longer
fail with the idea-58 semantic `lir_to_bir` diagnostic and instead graduate
into the downstream x86 emitter restriction family for non-minimal pointer
returns/calls. The packet also added a backend-notes regression proving the
global-pointer half of this seam now lowers successfully.

## Suggested Next

Treat `00089` and `00095` as graduated out of idea 58 and route their new x86
emitter restriction failure to the downstream scalar-expression/terminator
ownership, likely idea 60 rather than this upstream semantic bucket. For idea
58 itself, the next packet should stay on a remaining owned seam such as the
`alloca local-memory` case `00207`, instead of reopening pointer-return emitter
work here.

## Watchouts

- Keep this runbook on upstream semantic/BIR gaps; route stack/addressing
  ownership to idea 62 when that is the more precise failure family.
- Do not add x86 emitter special cases for cases that still fail before
  prepared emission exists.
- Preserve downstream routing when a case graduates into ideas 59, 60, or 61
  instead of silently keeping it under idea 58.
- The new scalar fix is intentionally bounded to direct global operands when
  the semantic layer already expects a pointer value; do not widen it into
  arbitrary non-pointer globals or unrelated named-value exceptions without a
  clearer semantic owner.
- The current blockers for `00089` and `00095` are outside this runbook’s
  semantic bucket: both now reach x86 emission and stop in the downstream
  emitter restriction family, so do not spend more idea-58 budget inside x86
  prepared-emitter code from this packet.
- The backend-notes regression covers the global-pointer half of this seam; the
  function-symbol half is proven by `00095` itself in the delegated c-tests, so
  keep future follow-up coverage aligned with real route shapes rather than
  fragile synthetic pointer-return forms.

## Proof

Ran the delegated proof command and preserved `/workspaces/c4c/test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_.*|c_testsuite_x86_backend_src_00089_c|c_testsuite_x86_backend_src_00095_c)$' | tee test_after.log`.
Current result: all `74` `backend_*` tests pass, including the new
`backend_lir_to_bir_notes` regression, and `00089` plus `00095` no longer fail
with the old idea-58 `scalar-control-flow semantic family` diagnostic. The
proof still ends nonzero overall because both c-tests now graduate into a
downstream `[FRONTEND_FAIL]`: `error: x86 backend emitter only supports a
minimal i32 return function through the canonical prepared-module handoff`.
For idea-58 acceptance, that route change is valid progress because both cases
have left the owned semantic-lowering bucket. This slice is commit-ready for
idea-58 runbook progress even though the delegated proof remains red on the
downstream x86-emitter failure.
