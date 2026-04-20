# Execution State

Status: Active
Source Idea Path: ideas/open/58_semantic_lir_to_bir_gap_closure_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The First Semantic/BIR Seam
Plan Review Counter: 2 / 10
# Current Packet

## Just Finished

Step 2 packet repaired the still-owned `load local-memory` seam behind
`c_testsuite_x86_backend_src_00050_c` by teaching semantic `lir_to_bir`
global-load lowering to accept scalar loads from tracked byte-offset addresses
inside linear-addressed globals, even when the addressed subobject is a nested
aggregate/union view rather than an already-scalar leaf. The slice stays
upstream and general rather than `00050`-shaped: nested global aggregate GEPs
now carry enough provenance for scalar `LoadGlobalInst` lowering to continue
through the semantic layer. Under the delegated proof, `00050` no longer fails
in the idea-58 `load local-memory semantic family` and instead graduates into
the downstream x86 emitter restriction family.

## Suggested Next

Treat `00050` as graduated out of idea 58 and route its new x86 emitter
restriction failure to the downstream scalar-expression/terminator ownership,
likely idea 60 rather than this upstream semantic bucket. For idea 58 itself,
the next packet should stay on a still-owned seam from the remaining semantic
family, such as another load/local-memory or broader semantic case like
`c_testsuite_x86_backend_src_00089_c`, instead of reopening emitter work here.

## Watchouts

- Keep this runbook on upstream semantic/BIR gaps; route stack/addressing
  ownership to idea 62 when that is the more precise failure family.
- Do not add x86 emitter special cases for cases that still fail before
  prepared emission exists.
- Preserve downstream routing when a case graduates into ideas 59, 60, or 61
  instead of silently keeping it under idea 58.
- The new global-load handling is still bounded to tracked linear-addressed
  globals and byte-offset scalar accesses; do not widen it into untracked
  arbitrary integer-address or opaque-pointer claims without a real semantic
  owner for those routes.
- The current blocker for `00050` is outside this runbook’s semantic bucket:
  it now reaches x86 emission and stops in the minimal single-block
  terminator/bounded guard family, so do not spend more idea-58 budget inside
  x86 prepared-emitter code from this packet.

## Proof

Ran the delegated proof command and preserved `/workspaces/c4c/test_after.log`:
`cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_.*|c_testsuite_x86_backend_src_00050_c)$' | tee test_after.log`.
Current result: all `74` `backend_*` tests pass, and
`c_testsuite_x86_backend_src_00050_c` no longer fails with the old idea-58
`load local-memory semantic family` diagnostic. The proof still ends nonzero
overall because `00050` now graduates into a downstream `[FRONTEND_FAIL]`:
`error: x86 backend emitter only supports a minimal single-block i32 return
terminator, a bounded equality-against-immediate guard family with immediate
return leaves including fixed-offset same-module global i32 loads and
pointer-backed same-module global roots, or one bounded compare-against-zero
branch family through the canonical prepared-module handoff`. For idea-58
acceptance, that route change is valid progress because the testcase has left
the owned semantic-lowering bucket. This slice is commit-ready for idea-58
runbook progress even though the delegated proof remains red on the downstream
x86-emitter failure.
