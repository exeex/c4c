# Execution State

Status: Active
Source Idea Path: ideas/open/58_semantic_lir_to_bir_gap_closure_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The First Semantic/BIR Seam
Plan Review Counter: 5 / 10
# Current Packet

## Just Finished

Step 2 packet repaired the still-owned dynamic-alloca seam behind
`c_testsuite_x86_backend_src_00207_c` by letting normal backend
`lir_to_bir` lowering preserve dynamic `alloca` instead of admitting it only
when printing semantic BIR. That route repair is upstream and general rather
than testcase-shaped: the same `accumulate_vla` lane already proven by
`backend_codegen_route_x86_64_vla_goto_stackrestore_observe_semantic_bir` now
survives the real x86 handoff path too. Under the delegated proof, `00207` no
longer fails in the idea-58 `alloca local-memory semantic family` and instead
graduates into the downstream x86 emitter restriction family with
`error: x86 backend emitter only supports a minimal i32 return function
through the canonical prepared-module handoff`.

## Suggested Next

Treat `00207` as graduated out of idea 58 and route its new x86 emitter
restriction to idea 60 rather than reopening prepared-emitter work in this
semantic runbook. For idea 58 itself, the next packet should isolate another
still-owned semantic-lowering failure such as `00217` or a remaining backend
route case that still reports the `semantic lir_to_bir` handoff diagnostic.

## Watchouts

- Keep this runbook on upstream semantic/BIR gaps; route stack/addressing
  ownership to idea 62 when that is the more precise failure family.
- Do not add x86 emitter special cases for cases that still fail before
  prepared emission exists.
- Preserve downstream routing when a case graduates into ideas 59, 60, or 61
  instead of silently keeping it under idea 58.
- The dynamic-alloca repair is intentionally limited to preserving a semantic
  route the backend already knows how to print as BIR; if the next failure is
  really about stack/addressing ownership or prepared-emitter scalar/terminator
  selection, move it to idea 62 or 60 instead of widening idea 58.
- `backend_codegen_route_x86_64_vla_goto_stackrestore_observe_semantic_bir`
  remains the nearest protective backend coverage for this seam; keep any
  follow-up coverage centered on durable VLA routing rather than testcase-only
  `00207` matching.
- The new blocker for `00207` is outside this runbook’s semantic bucket: it
  now reaches x86 emission and stops in the downstream minimal-return emitter
  restriction family.

## Proof

Ran the delegated proof command and preserved `/workspaces/c4c/test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_x86_64_vla_goto_stackrestore_observe_semantic_bir|backend_lir_to_bir_notes|c_testsuite_x86_backend_src_00207_c)$' | tee test_after.log`.
Current result: `backend_lir_to_bir_notes` and
`backend_codegen_route_x86_64_vla_goto_stackrestore_observe_semantic_bir`
pass, and `00207` no longer fails with the old idea-58
`alloca local-memory semantic family` diagnostic. The proof still ends nonzero
overall because `00207` now graduates into a downstream `[FRONTEND_FAIL]`:
`error: x86 backend emitter only supports a minimal i32 return function
through the canonical prepared-module handoff`. For idea-58 acceptance, that
route change is valid progress because `00207` has left the owned
semantic-lowering bucket. This slice is commit-ready for idea-58 runbook
progress even though the delegated proof remains red on the downstream x86
emitter failure.
