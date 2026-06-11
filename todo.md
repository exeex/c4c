Status: Active
Source Idea Path: ideas/open/195_cross_target_route_view_reuse_beyond_x86_route6.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Define proof routes for ready or blocked candidates

# Current Packet

## Just Finished

Completed Step 3 of `plan.md`: assigned proof routes for the Step 2
classification without promoting any boundary beyond the already-proven x86
Route 6 scalar call-argument source helper to ready status.

### Ready proof routes

| Candidate | Proof command | Adjacent same-feature sanity cases | Readiness decision |
| --- | --- | --- | --- |
| x86 `find_consumed_scalar_i32_call_argument_source` over `Route6CallUseSourceIndex` | `ctest --test-dir build -R '^backend_x86_handoff_boundary$' --output-on-failure` | The existing `backend_x86_handoff_boundary_direct_extern_call_test.cpp` coverage must continue to prove source-id agreement, missing Route 6 fail-closed behavior, preserved prepared call-argument fallback, and unchanged emitted asm. Adjacent same-feature sanity should include `backend_prepare_frame_stack_call_contract` for prepared call-source shape/source-producer contracts and `backend_aarch64_instruction_dispatch` for the AArch64 Route 6 source-producer consumer oracle before accepting any code-changing expansion. | Ready only for the existing x86 Route 6 scalar argument-source sub-boundary. This is not evidence for whole `ConsumedPlans`, whole call plans, riscv wrappers, or other route families. |

### Blocked and unknown proof routes

| Candidate | Current blocker | Future proof route if unblocked | Future implementation idea scope |
| --- | --- | --- | --- |
| x86 whole `ConsumedPlans` / call-plan reuse outside scalar Route 6 source | Missing AArch64 route view for whole call plans, plus ABI/register/stack/result/move/wrapper policy ambiguity. | First prove one additional semantic Route 6 argument/result class with source-id agreement and prepared fallback; then run `backend_aarch64_instruction_dispatch`, `backend_prepare_frame_stack_call_contract`, and `backend_x86_handoff_boundary` with adjacent ABI-sensitive fallback cases. | Yes, but only as one wrapper-boundary idea per selected call argument/result semantic source class. |
| x86 `find_consumed_call_plan`, `find_consumed_call_argument_plan`, and `find_consumed_call_result_plan` whole-plan substitution | Missing target-neutral route view for ABI-bound call plans and result lanes; prepared call-plan ownership remains policy-sensitive. | No clear proof route for whole-plan substitution. A narrower proof route must target one semantic source read and reject ABI/result-lane policy migration. | Yes, if scoped to a single semantic source read; no broad whole-plan idea is ready. |
| x86 direct extern and same-module call wrapper paths beyond scalar source lookup | Missing wrapper adapter for using any Route 6 fact other than the scalar source helper; wrapper construction and move facts remain prepared-owned. | Reuse the x86 helper proof only as an input, then prove wrapper output remains unchanged under missing/mismatched Route 6 facts in `backend_x86_handoff_boundary`. | Yes, only for a single wrapper boundary that consumes an already-proven helper result. |
| x86 edge publication move helpers | Missing x86 adapter tying Route 4 publication identity or Route 5 edge/join-source identity to prepared edge publication without taking over move scheduling or value homes. | Need AArch64 Route 4/5 oracle coverage plus a new x86 boundary test that checks source/destination agreement, duplicate/ambiguous rejection, and prepared fallback before any implementation can be accepted. | Yes, likely future idea: x86 edge-publication semantic source adapter at the edge-publication wrapper boundary. |
| x86 compare/join, countdown, and short-circuit helpers | Missing wrapper proof for Route 5 join-source or Route 7 comparison provenance; branch spelling, parallel-copy order, and compare instruction policy remain target-local. | Need a candidate-specific x86 adapter and tests that pair Route 5/7 agreement with adjacent branch/parallel-copy fallback cases. | Possible future idea, but only after the boundary is narrowed to one compare/join semantic provenance read. |
| x86 `x86::prepared::Query` facade | Target wrapper adapter ambiguity: the facade mixes route-view candidates with prepared policy and has no single route owner. | No facade-wide proof route. Each method would need its own source-id/route-id/fallback proof before migration. | No broad facade idea; split method-level ideas only. |
| x86 local-slot and compare-load helpers | Missing proof that Route 3 memory/source identity can be consumed without moving frame-slot, offset, operand rendering, or addressing legality into the route view. | Need an AArch64 Route 3 oracle-backed semantic memory-source case, then an x86 local-slot adapter test with addressing-sensitive fallback. | Possible future idea: x86 local-slot semantic memory-source adapter, scoped to one load/compare helper boundary. |
| riscv edge-publication move reuse | Missing riscv wrapper adapter for Route 4/5 and no riscv imported reuse proof; move scheduling, homes, register naming, stack/immediate handling, and formatting are target policy. | Need a riscv boundary test that proves Route 4/5 source identity agreement, duplicate/ambiguous rejection, and prepared edge-publication fallback while preserving emitted riscv policy. | Yes, strongest future candidate: riscv edge-publication semantic source adapter at one edge-publication wrapper boundary. It is blocked, not ready. |
| riscv operand rendering and stack-source publication sub-reads | Missing separation between semantic source identity and operand/stack/register/width/extension policy. | No clear proof route until a narrower source-id-only adapter is designed and backed by AArch64 Route 4/5 oracle coverage. | Possible future idea only after narrowing to a source-id read that excludes operand rendering and stack/register policy. |

### Step 3 conclusion

No boundary beyond x86 Route 6 has a clear proof route yet. The most plausible
future implementation ideas are one-boundary adapters for x86 edge-publication
semantic source reuse, riscv edge-publication semantic source reuse, and
possibly x86 local-slot semantic memory-source reuse, but each remains blocked
until the missing target wrapper adapter and oracle-backed fallback proof are
defined. The audit should not claim riscv readiness or broad x86 readiness from
the existing Route 6 scalar helper.

## Suggested Next

Start Step 4 by producing the durable cross-target reuse table from the Step 1
inventory, Step 2 classification, and Step 3 proof-route decisions. The table
should preserve the Step 3 conclusion that no additional boundary beyond x86
Route 6 is ready yet.

## Watchouts

- Do not treat the x86 Route 6 helper as proof for whole call plans,
  `ConsumedPlans`, `x86::prepared::Query`, or any riscv wrapper.
- Future implementation ideas should be one wrapper boundary at a time:
  riscv edge-publication semantic source adapter, x86 edge-publication semantic
  source adapter, or x86 local-slot semantic memory-source adapter.
- Blocked means missing adapter/oracle/policy separation, not permission to add
  target-specific BIR records.
- Any code-changing follow-up must include a build, the named narrow CTest, and
  adjacent same-feature sanity cases; this Step 3 packet is lifecycle/docs-only.

## Proof

Lifecycle/docs-only packet. Used the Step 2 classification in `todo.md` as the
primary input, with targeted confirmation of existing test names and Route 6
coverage in `tests/backend/bir/CMakeLists.txt`,
`tests/backend/mir/CMakeLists.txt`, and the relevant test sources. No build or
test proof was required by the delegated packet, and no `test_after.log` was
generated.
