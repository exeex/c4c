# Execution State

Status: Active
Source Idea Path: ideas/open/58_semantic_lir_to_bir_gap_closure_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The First Semantic/BIR Seam
Plan Review Counter: 6 / 10
# Current Packet

## Just Finished

Step 2 packet repaired the live scalar-cast semantic seam in normal backend
`lir_to_bir` lowering by admitting the remaining scalar cast opcodes in BIR
and by letting the memory-path cast lane fall back to generic cast lowering
when special i64 pointer-provenance recovery is unavailable. That upstream
repair is general rather than testcase-shaped: the nearest notes coverage now
proves memory-path `sitofp`/`fptosi`/`fpext`/`fptrunc` plus ptr-int round-trips
lower successfully without re-entering the scalar-cast family. Under the
delegated proof, `00144`, `00175`, and `00214` all leave the old idea-58
scalar-cast failure family and graduate into downstream prepared-emitter
families, while `00113` no longer fails in scalar-cast and instead exposes a
narrower remaining idea-58 `scalar/local-memory semantic family` seam.

## Suggested Next

Treat the scalar-cast seam as repaired for this packet and route the graduated
downstream cases explicitly: `00144` now belongs with idea 59's authoritative
prepared short-circuit handoff, while `00175` and `00214` now belong with idea
60's minimal-return prepared-emitter restriction family. For idea 58 itself,
the next packet should isolate the still-owned `scalar/local-memory semantic
family` seam now surfaced by `00113` rather than reopening scalar-cast work.

## Watchouts

- Keep this runbook on upstream semantic/BIR gaps; route stack/addressing
  ownership to idea 62 when that is the more precise failure family.
- Do not add x86 emitter special cases for cases that still fail before
  prepared emission exists.
- Preserve downstream routing when a case graduates into ideas 59, 60, or 61
  instead of silently keeping it under idea 58.
- The new cast admission is intentionally limited to semantic lowering plus BIR
  representation; it does not claim downstream prepared-emitter support for
  float-heavy or pointer-cast-heavy routes.
- `backend_lir_to_bir_notes` is now the nearest protective backend coverage for
  this seam because it exercises the memory-path admitted scalar casts without
  depending on one named c-testsuite case.
- `00113` still fails in idea 58, but the remaining blocker is no longer the
  scalar-cast family. The next packet should inspect why the local float lane
  still lands in `scalar/local-memory semantic family` instead of assuming more
  cast work is needed.
- `00144`, `00175`, and `00214` should stay out of this runbook unless a later
  audit shows they still fail before prepared x86 consumption.

## Proof

Ran the delegated proof command and preserved `/workspaces/c4c/test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|c_testsuite_x86_backend_src_00113_c|c_testsuite_x86_backend_src_00144_c|c_testsuite_x86_backend_src_00175_c|c_testsuite_x86_backend_src_00214_c)$' | tee test_after.log`.
Current result: `backend_lir_to_bir_notes` passes with the new admitted
memory-path scalar-cast lane covered. `00144` now fails downstream with the
authoritative prepared short-circuit handoff diagnostic, and `00175` plus
`00214` now fail downstream with idea-60's minimal-return prepared-emitter
restriction. `00113` still ends in a semantic diagnostic, but it has left the
old scalar-cast family and now reports `scalar/local-memory semantic family`.
The delegated proof remains nonzero overall because one case is still idea-58
owned and three cases now fail in downstream prepared-emitter families. For
this packet, that route change is still valid progress because the targeted
scalar-cast seam shrank and the remaining owned blocker is now a narrower
different semantic family. This slice is commit-ready for idea-58 runbook
progress even though the delegated proof remains red overall.
