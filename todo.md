Status: Active
Source Idea Path: ideas/open/239_phase_e5_prepared_bir_module_demotion_or_retirement_gate.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Create Safe Follow-Up Implementation Ideas

# Current Packet

## Just Finished

Step 4 created only narrow E5 follow-up implementation ideas permitted by the
gate artifact. No implementation files, `plan.md`, draft 155, expectation
files, baseline files, or the E5 gate document were changed.

Opened follow-up ideas:

- `ideas/open/240_phase_e5_route3_memory_source_identity_adapter_follow_up.md`
  for one Route 3 memory/source identity adapter inside `memory_accesses`;
- `ideas/open/241_phase_e5_route45_edge_publication_identity_adapter_follow_up.md`
  for one Route 4 or Route 5 publication, edge, or join-source identity
  adapter inside `edge_publications` or
  `edge_publication_source_producers`;
- `ideas/open/242_phase_e5_route6_scalar_call_use_source_identity_row_follow_up.md`
  for one Route 6 scalar call-use source identity row or reader.

Each idea keeps the scope to one adapter or row, requires acceptance criteria
and proof at that exact boundary, preserves prepared fallback, diagnostics,
wrapper output, expected strings, and baselines, and includes reviewer reject
signals for named-case shortcuts, expectation downgrades, facade-only changes,
broad rewrites, retained old failure modes, and aggregate-retirement claims.

Draft 155 remains blocked and was not opened as-is. No broad aggregate
retirement idea was created. Idea 238 remains narrow evidence only for x86
Route 6 scalar `i32` route-debug / `ConsumedPlans` compatibility.

## Suggested Next

Execute Step 5: validate the docs/lifecycle slice and prepare closure notes
for source idea 239. The recommended Step 5 packet should confirm that no
implementation, expectation, baseline, `plan.md`, draft 155, or E5 gate
document changes are present, then record closure-ready validation and the
final follow-up idea list.

## Watchouts

- Keep idea 238 narrow: closed, prerequisite-complete, but only for x86 Route 6
  scalar `i32` route-debug / `ConsumedPlans` compatibility. It is not broad
  x86 call-wrapper, riscv, route-wide wrapper, or aggregate retirement proof.
- Step 4 follow-ups are not deletion permission; each future executor packet
  must first name exactly one selected reader, adapter, or row and prove that
  boundary without widening to a whole field group.
- Keep target policy out of target-neutral BIR: ABI, frame, registers, stack,
  addressing, move scheduling, call/helper protocols, carrier/runtime-helper
  protocol, branch/output spelling, final assembler, emission order, and
  wrapper output remain target/prepared-owned unless a later source proves a
  narrower owner.
- Preserve helper-oracle names/status labels, expected strings,
  supported-path status, prepared printer/debug output, route-debug output,
  wrapper output, fallback behavior, and baselines. Expectation rewrites,
  helper renames, unsupported downgrades, baseline refreshes, facade-only
  moves, and named-case-only shortcuts remain reject signals.

## Proof

Docs/lifecycle-only Step 4 follow-up idea packet. No implementation files
changed, so no build or CTest proof is required and no `test_after.log` is
required for this packet. Local proof should be limited to source/lifecycle
inspection, `git status --short`, `git diff -- ideas/open todo.md`, and
`git diff --check`.
