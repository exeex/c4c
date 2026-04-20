# Execution State

Status: Active
Source Idea Path: ideas/open/58_semantic_lir_to_bir_gap_closure_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The First Semantic/BIR Seam
Plan Review Counter: 3 / 10
# Current Packet

## Just Finished

Step 2 packet repaired the still-owned `gep local-memory` seam behind
`c_testsuite_x86_backend_src_00217_c` by teaching semantic `lir_to_bir`
memory lowering to reuse direct local-slot immediate values only as GEP index
hints and to accept scalar stores to tracked byte-offset addresses inside
linear-addressed globals. The slice stays upstream and general rather than
`00217`-shaped: global-backed byte-address pointers can now resolve constant
offset GEPs and lower matching scalar `StoreGlobalInst` writes without falling
out of the semantic layer. Under the delegated proof, `00217` no longer fails
in the idea-58 `gep local-memory semantic family` and instead graduates into a
downstream x86 emitter restriction family.

## Suggested Next

Treat `00217` as graduated out of idea 58 and route its new x86 emitter
restriction failure to the downstream scalar-expression/terminator ownership,
likely idea 60 rather than this upstream semantic bucket. For idea 58 itself,
the next packet should stay on a still-owned seam from the remaining semantic
family, such as the `scalar-control-flow` cases `00089` or `00095`, or the
remaining `alloca local-memory` case `00207`, instead of reopening emitter
work here.

## Watchouts

- Keep this runbook on upstream semantic/BIR gaps; route stack/addressing
  ownership to idea 62 when that is the more precise failure family.
- Do not add x86 emitter special cases for cases that still fail before
  prepared emission exists.
- Preserve downstream routing when a case graduates into ideas 59, 60, or 61
  instead of silently keeping it under idea 58.
- The new local-slot immediate tracking is intentionally narrow: it is used
  only as a GEP index hint, not as a general load-elision pass, because the
  backend route coverage still expects ordinary local loads to remain visible
  in many semantic BIR cases.
- The new global-store handling is still bounded to tracked linear-addressed
  globals and byte-offset scalar accesses; do not widen it into untracked
  arbitrary integer-address or opaque-pointer claims without a real semantic
  owner for those routes.
- The current blocker for `00217` is outside this runbook’s semantic bucket:
  it now reaches x86 emission and stops in the downstream emitter restriction
  family, so do not spend more idea-58 budget inside x86 prepared-emitter code
  from this packet.

## Proof

Ran the delegated proof command and preserved `/workspaces/c4c/test_after.log`:
`cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_.*|c_testsuite_x86_backend_src_00217_c)$' | tee test_after.log`.
Current result: all `74` `backend_*` tests pass, and
`c_testsuite_x86_backend_src_00217_c` no longer fails with the old idea-58
`gep local-memory semantic family` diagnostic. The proof still ends nonzero
overall because `00217` now graduates into a downstream `[FRONTEND_FAIL]`:
`error: x86 backend emitter only supports direct immediate i32 returns,
constant-evaluable straight-line no-parameter i32 return expressions, direct
single-parameter i32 passthrough returns, single-parameter i32
add-immediate/sub-immediate/mul-immediate/and-immediate/or-immediate/
xor-immediate/shl-immediate/lshr-immediate/ashr-immediate returns, a bounded
equality-against-immediate guard family with immediate return leaves, or one
bounded compare-against-zero branch family through the canonical prepared-
module handoff`. For idea-58 acceptance, that route change is valid progress
because the testcase has left the owned semantic-lowering bucket. This slice is
commit-ready for idea-58 runbook progress even though the delegated proof
remains red on the downstream x86-emitter failure.
