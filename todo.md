Status: Active
Source Idea Path: ideas/open/aarch64-codegen-forward-migration-second-wave-audit.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Second-Wave Candidates

# Current Packet

## Just Finished

Step 2: Classify Second-Wave Candidates completed as an audit-only
classification of the Step 1 surface map. No implementation files were changed.

| Candidate | Bucket | Destination layer | x86/RISC-V reuse value | Risk | Suggested proof |
| --- | --- | --- | --- | --- | --- |
| Same-block producer discovery and select-chain dependency analysis from `dispatch_producers.cpp` and its branch/call/value/store users | Move to shared MIR | Shared MIR query/helper layer, with target-local final emission retaining symbol/register spelling | High: x86/RISC-V need the same local def-use, constant, select-chain, and block lookup facts before final instruction choice | Medium: current helpers may carry AArch64-shaped assumptions around global labels and materialized operand forms | Add shared query API plus AArch64 parity tests for current producer/select cases; then prove one x86 or RISC-V callsite can consume the same query without target-specific branching |
| Scalar value-publication planning into Prepared homes from `dispatch_value_materialization.cpp` and `dispatch_publication.cpp` | Needs target hook | Prealloc or shared MIR planner that computes publication intent; target hooks emit register views, memory operands, immediates, and final instructions | High: publication-to-home decisions, entry publication tracking, clobber checks, and Prepared home interpretation should apply across targets | High: easy to move AArch64 register widths, FP immediates, ADRP/ADD, or load/store mnemonics too early | Introduce a target-neutral publication plan record and keep AArch64 emission byte-for-byte equivalent on stack/register/global/immediate homes; require a small x86/RISC-V consumer sketch or adapter |
| Store-source publication planning, narrow-store to wide-load recovery, and pointer-store writeback from `dispatch_store_sources.cpp` | Move to prealloc | Prealloc store-source planner that resolves logical slots, recovered sources, stack-object identities, and pending publication requirements | Medium-high: x86/RISC-V stores need the same source provenance and stack-object recovery even though address modes differ | High: source recovery is subtle and may accidentally encode AArch64 address legality or store mnemonics | Add planner output inspected by AArch64 lowering; prove existing narrow-store/wide-load and pointer-writeback tests remain supported, plus one non-AArch64 consumer-facing unit or fixture |
| Dynamic-stack helper recognition and operand-home planning from `dispatch_dynamic_stack.cpp` | Needs target hook | Prealloc dynamic-stack op classifier plus target hook for stack-pointer/frame-pointer instruction sequences | Medium: recognizing helper calls and matching `PreparedDynamicStackOp` should be shared; final `sp` adjustment and scratch usage are target-local | Medium: stack alignment, frame-pointer policy, and scratch register availability vary per target | Split recognition/classification from emission; prove AArch64 dynamic alloca cases still lower identically and add hook-level fixture showing x86/RISC-V can choose different stack adjustment spelling |
| Branch-fusion eligibility, support-instruction filtering, selected-operand checks, and constant evaluation from `dispatch_branch_fusion.cpp` | Needs target hook | Shared MIR branch-fusion analysis with target hook for condition set, immediate encodability, compare form, and final branch spelling | High: x86/RISC-V also benefit from local compare/load/cast folding before conditional branch emission | Medium-high: immediate encodability and condition mapping are target-specific, and over-broad sharing could hide target branch semantics | Add reusable fusion-decision object; prove AArch64 fused/unfused branch tests are unchanged and add target-profile fixtures for immediate legality differences |
| Call-boundary move ordering, preservation, and republication mechanics from `dispatch_calls.cpp` and `calls_moves.cpp` | Move to prealloc | Extend prealloc call-plan/move-bundle support to own generic ordering, preservation-home population, source/destination effect resources, and republication intent | High: x86/RISC-V call lowering has the same before/after/return move ordering and preservation problems with different ABI lanes | High: ABI lane/register spelling, variadic details, F128 carriers, byval copies, and actual machine moves must stay target-local | Move only ordering/intent into call plans; prove AArch64 call, byval, preserved-value, indirect-callee, and return-value cases remain equivalent, then wire one x86/RISC-V plan consumer |
| Generic helper APIs collected in `dispatch.hpp` for Prepared lookups, publication/clobber tracking, producer lookup, and move orchestration | Move to shared MIR | Shared MIR/prealloc headers near the owning implementation, leaving `dispatch.hpp` as AArch64 emission declarations only | Medium: clearer ownership reduces duplicated declarations for future x86/RISC-V consumers | Low-medium: declaration moves are mechanical but can create circular dependencies if ownership is not split with the implementation | Extract declarations only after one concrete implementation move above; prove compile/build and ensure `dispatch.hpp` no longer exposes generic helpers without moving target-local APIs |
| Final AArch64 spelling, ABI lane rules, stack-pointer sequences, memory operands, ADRP/ADD, CMP/CSEL/branch forms, and machine instruction construction | Keep target-local | AArch64 codegen final emission layer | Low direct reuse; x86/RISC-V need analogous hooks, not shared AArch64 instructions | High if moved: would collapse target-specific ABI and instruction-selection policy into generic layers | Keep as explicit non-goal; proof is reviewer check that follow-up ideas do not move instruction spelling or target ABI policy |

Most viable Step 3 idea candidates:

- Extract shared same-block producer/select-chain queries first. It has the
  broadest reuse value, the smallest target hook surface, and a clear parity
  proof path.
- Create a target-neutral publication-plan record second, but only if the idea
  explicitly preserves target-local emission hooks.
- Extend prealloc call-plan ownership for generic call-boundary ordering after
  the producer/publication direction is established; it has high value but a
  larger ABI-risk boundary.

## Suggested Next

Step 3 idea-generation packet: create focused `ideas/open/*.md` follow-up ideas
for the top second-wave candidates, starting with shared same-block
producer/select-chain queries. Include intent, scope, non-goals, acceptance,
proof expectations, and reviewer reject signals. Keep each idea small enough
for one execution run and keep AArch64 instruction spelling/ABI policy
explicitly out of scope.

## Watchouts

- This is an audit run; do not implement broad code movement.
- Step 3 is allowed to edit `ideas/open/*.md`; this packet was not.
- Do not create follow-up ideas for target-local final emission. The audit
  explicitly keeps AArch64 register spelling, ABI lane policy, stack-pointer
  sequences, ADRP/ADD, CMP/CSEL/branch forms, memory operands, and machine
  instruction construction in the AArch64 layer.
- The safest first idea is producer/select-chain query extraction. Publication
  planning and call-boundary ordering are valuable but need tighter hook
  boundaries to avoid moving ABI or instruction-selection policy.
- Any Step 3 idea should name one x86 or RISC-V consumer path, not just an
  abstract desire for sharing.

## Proof

Audit-only `todo.md` update. No build or test proof required by the packet, and
no `test_after.log` was created.
