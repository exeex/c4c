Status: Active
Source Idea Path: ideas/open/239_phase_e5_prepared_bir_module_demotion_or_retirement_gate.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Validate and Prepare Closure Notes

# Current Packet

## Just Finished

Step 5 validated the E5 docs/lifecycle gate and prepared closure-ready notes
for source idea 239. The committed Step 1 through Step 4 touched-file set
after plan activation is limited to:

- `docs/bir_prealloc_fusion/phase_e5_prepared_bir_module_demotion_or_retirement_gate.md`;
- `ideas/open/240_phase_e5_route3_memory_source_identity_adapter_follow_up.md`;
- `ideas/open/241_phase_e5_route45_edge_publication_identity_adapter_follow_up.md`;
- `ideas/open/242_phase_e5_route6_scalar_call_use_source_identity_row_follow_up.md`;
- `todo.md`.

No implementation files, expectation files, baseline files, draft 155 files,
or `plan.md` changes are present in the Step 1 through Step 4 delta.

Final draft 155 disposition: draft 155 remains blocked and must not be opened
as broad aggregate-retirement work. A later lifecycle packet may only rewrite
or supersede it as one-field-group or one-adapter successor ideas that retain
prepared fallback, diagnostics/oracles, wrapper compatibility, expected
strings, and baseline authority.

Idea 238 status: closed and prerequisite-complete only for the x86 Route 6
scalar `i32` route-debug / `ConsumedPlans` compatibility boundary. It is not
broad x86 call-wrapper, riscv, route-wide wrapper, cross-target wrapper, or
aggregate retirement evidence.

Field-group outcomes:

- no `PreparedBirModule` field group is ready for whole-field deletion,
  whole-field privatization, or aggregate retirement;
- no `PreparedFunctionLookups` group is ready for whole-group deletion,
  whole-group privatization, or aggregate replacement;
- only narrow later adapter candidates are supported: one selected direct
  BIR/route semantic identity reader, one Route 3 memory/source identity
  read, one Route 4/5 publication or edge/join identity read, one Route 6
  scalar call-use source identity row or reader, one diagnostic/oracle row,
  or one isolated identity-only reader that excludes target policy;
- target policy, prepared private context, public fallback/oracle,
  diagnostic/string authority, wrappers, expected strings, route-debug output,
  and baseline behavior remain retained until a future narrow proof matrix
  says otherwise.

Blocker summary: whole aggregate retirement, broad prepared lookup
replacement, broad x86 wrapper migration, riscv readiness, and cross-target
wrapper convergence remain blocked. Expectation rewrites, baseline refreshes,
helper renames, unsupported downgrades, wrapper-output relabeling,
facade-only moves, aggregate reshuffles, and named-case-only shortcuts remain
reject signals.

Accepted baseline evidence for closure: the E5 gate inherits the full-suite
baseline at commit `8cebab4beba219e6a8cdef998bc970c8658ce28b`, recorded as
3428/3428 passing in
`log/baseline_8cebab4beba219e6a8cdef998bc970c8658ce28b.log`, plus the later
narrow idea 238 proofs for x86 Route 6 scalar `i32` route-debug /
`ConsumedPlans` compatibility. No new regression guard was generated because
this plan changed only docs/lifecycle artifacts.

Follow-up ideas opened by Step 4:

- `ideas/open/240_phase_e5_route3_memory_source_identity_adapter_follow_up.md`
  for one Route 3 memory/source identity adapter inside `memory_accesses`;
- `ideas/open/241_phase_e5_route45_edge_publication_identity_adapter_follow_up.md`
  for one Route 4 or Route 5 publication, edge, or join-source identity
  adapter inside `edge_publications` or
  `edge_publication_source_producers`;
- `ideas/open/242_phase_e5_route6_scalar_call_use_source_identity_row_follow_up.md`
  for one Route 6 scalar call-use source identity row or reader.

## Suggested Next

Plan owner should decide whether source idea 239 is complete and close or
otherwise transition the active lifecycle state. Do not close or move the
source idea from an executor packet.

## Watchouts

- Step 4 follow-ups are not deletion permission; each future implementation
  packet must first name exactly one selected reader, adapter, or row and
  prove that boundary without widening to a whole field group.
- Keep idea 238 narrow: closed, prerequisite-complete, but only for x86 Route
  6 scalar `i32` route-debug / `ConsumedPlans` compatibility. It is not broad
  x86 call-wrapper, riscv, route-wide wrapper, cross-target wrapper, or
  aggregate retirement proof.
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

Docs/lifecycle-only Step 5 validation packet. Commands run:

- `git status --short`
- `git diff --check`
- `git log --name-status --oneline -12 -- plan.md todo.md ideas/open docs/bir_prealloc_fusion`
- `git diff --name-status 7b093f55c..HEAD`
- `git diff --name-only 7b093f55c..HEAD`
- `git diff --name-only 7b093f55c..HEAD -- '*.cpp' '*.cc' '*.cxx' '*.c' '*.hpp' '*.hh' '*.h' '*.ll' '*.s' '*.asm' '*.exp' '*expect*' '*baseline*'`
- `git diff --name-only 7b093f55c..HEAD | rg -n '(^|/)(draft|.*155.*)|plan\.md$|expect|baseline|\.cpp$|\.cc$|\.cxx$|\.c$|\.hpp$|\.hh$|\.h$|\.ll$|\.s$|\.asm$'`
- `rg -n '3428|8cebab4|full-suite|full suite|baseline' docs/bir_prealloc_fusion/phase_e5_prepared_bir_module_demotion_or_retirement_gate.md log/baseline_8cebab4beba219e6a8cdef998bc970c8658ce28b.log ideas/closed/238_phase_e4_x86_route6_scalar_i32_route_debug_consumedplans_compatibility_follow_up.md`

`git diff --check` passed. The implementation/expectation/baseline/draft
155/`plan.md` file-set filters returned no matches for the Step 1 through
Step 4 delta after plan activation. No build or CTest was run because no
implementation files changed. No new regression guard or `test_after.log` was
generated for this docs/lifecycle-only packet.
