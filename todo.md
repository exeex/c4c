Status: Active
Source Idea Path: ideas/open/113_bir_prealloc_aggregate_call_boundary_contract_audit.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Current Contract Ownership

# Current Packet

## Just Finished

Step 1 `Reconstruct Idea 112 Repair Boundaries` reconstructed the durable
repair lessons from `ideas/closed/112_aarch64_00216_00204_post_closure_regression.md`
and commits `5054b6999`, `cdfc33cac`, `1362a467b`, and `9e78e96e7`.

Idea 112 closure summary: `00216` and `00204` were both call
aggregate/call-boundary regressions, but not one identical root cause. `00216`
was repaired by direct aggregate address materialization for frame-slot-backed
local aggregate pointer operands. `00204` additionally required outgoing
AArch64 stack argument reservation before `x16`-relative stores and stable
callee-side `va_list` home preservation for aggregate `va_arg`. The accepted
close proof was matching AArch64-labelled CTest before/after logs:
`test_before.log` had 270/272 passing, `test_after.log` had 272/272 passing,
and `c4c-regression-guard` reported no new failures.

Step 1 boundary findings:

| Source | Observed failure | Repaired layer | Owner-boundary classification | Credible proof route |
| --- | --- | --- | --- | --- |
| `5054b6999` local aggregate call address selection | `00216` segfaulted because a direct frame-slot-backed local aggregate pointer operand no longer found its explicit frame-slot materialization when aggregate lane/base names differed, such as `x` vs `x.0`. | `src/backend/prealloc/call_plans.cpp` source selection. The repair recognizes a direct local-frame-address pointer operand only when source value/home identity matches and the destination is a GPR, then lets explicit materialization lookup match that aggregate base/lane case. | Primary owner: shared prealloc/planning fact. Target codegen should consume a prepared `FrameSlotAddress`/local-frame-address selection; it should not rediscover aggregate pointer identity from AArch64 emission shape. ABI detail remains only the eventual register or stack placement. | Focused two-test proof after the patch showed `c_testsuite_aarch64_backend_src_00216_c` passing while `00204` narrowed to the remaining variadic path, proving the repair was not a filename-shaped fix for both tests. |
| `cdfc33cac` outgoing stack argument reservation | `00204` still crashed in aggregate/byval call traffic because outgoing stack stores were addressed relative to an unreserved caller frame shape; `x16` previously represented `sp - outgoing_bytes` while the real `sp` was adjusted only around the printed call. | AArch64 call lowering in `calls.cpp`/`machine_printer.cpp`. The repair emits real `sub sp, sp, #N`, publishes `x16` from the adjusted `sp`, adjusts caller-frame source memory after reservation, stores through the outgoing base, and restores `sp` after the call. | Shared side: prealloc should expose outgoing stack argument area size, destination offsets, and source memory facts. Target ABI placement decision: AAPCS64 decides which args spill to outgoing stack. Target codegen emission detail: AArch64 uses `x16` as the scratch outgoing-stack base and chooses the exact `sub`/`mov`/store/`add` sequence. | The same focused runtime proof passed both `00216` and `00204` after the full `cdfc33cac` repair; later internal and route tests protected the exact reservation/base-publication order. |
| `cdfc33cac` aggregate `va_arg` va-list home preservation | `00204` reached `myprintf` and crashed while fetching the first stack-backed variadic HFA long-double aggregate because inline aggregate `va_arg` lowering could use a clobbered va-list base register after generic helper operand materialization. | AArch64 variadic lowering in `instruction.hpp`/`variadic.cpp`, with scalar producer suppression for inline variadic helper calls in `calls.cpp`. The repair carries the stable va-list object home from the matching `va_start` helper and rematerializes the base before reading/updating AAPCS64 va-list fields. | Shared side: prepared variadic metadata must preserve payload shape and the relation between `va_start`, the va-list object, and aggregate `va_arg` source homes. Target ABI detail: AArch64 owns register-save/overflow field layout, offsets, progression, and rematerialization instructions. | Focused two-test proof passed both original regressions after the repair; assembly evidence in the historical `todo.md` noted the va-list base was rematerialized from the stable stack home before `fp_offset`/register-save reads. |
| `1362a467b` recovered target protection | Internal AArch64 dispatch expectations still described the old outgoing-stack convention and would fail after the accepted reservation semantics, risking regression of idea-110 recovered stack-call targets. | Test contract layer in `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`; no implementation repair. The tests now assert `sub sp`, `mov x16, sp`, stores through `x16`, call, then `add sp`, and update byval/f128 source-offset expectations after reservation. | Owner-boundary classification: proof/contract visibility around target codegen emission detail. It protects the AArch64 convention while still checking shared prepared source/destination facts; it is not a shared BIR rule. | Recovered-target subset proof passed `backend_aarch64_instruction_dispatch` plus `00172`, `00180`, `00216`, `00220`, and `00204`, confirming the new contract preserved prior recovered tests. |
| `9e78e96e7` byval stack route contract refresh | Backend route probes for byval stack-overflow aggregate payloads still expected the old `sub x16, sp, #32` pseudo-base and stale source offsets. | Test route contract layer in `tests/backend/CMakeLists.txt`; no implementation repair. Snippets now require `sub sp`, `mov x16, sp`, adjusted caller-frame source loads, stores through `x16`, call, and `add sp`. | Owner-boundary classification: target codegen emission proof for AArch64 byval stack routes. Shared facts under test are aggregate payload source bytes and outgoing destination offsets; AArch64-specific details are `x16` and concrete instruction order. | Broad AArch64-labelled proof passed all 272 tests, including `00216`, `00204`, the idea-110 recovered C tests, and the two byval stack-overflow route probes. |

## Suggested Next

Proceed to Step 2 by auditing the current BIR, prealloc/planning, and AArch64
lowering surfaces for the target list in `plan.md`, then classify the current
owner layer for each aggregate call-boundary behavior.

## Watchouts

- Do not collapse AArch64's `x16` scratch convention into a shared contract;
  the shared candidate is the outgoing stack area/reservation fact, not the
  register used to address it.
- Do not treat the byval route expectation updates as capability repairs.
  They are proof-route refreshes for the already-accepted reservation contract.
- The likely Step 2 split is: aggregate address source selection belongs to
  shared prealloc/planning; variadic aggregate payload/home relations need
  shared prepared facts plus target ABI field layout; outgoing stack area size
  and offsets need shared planning facts plus target-specific emission.

## Proof

Analysis-only; no build/test run. History/source-note evidence came from
`ideas/closed/112_aarch64_00216_00204_post_closure_regression.md` and
`git show` inspection of `5054b6999`, `cdfc33cac`, `1362a467b`, and
`9e78e96e7`. No new `test_after.log` was produced for this packet.
