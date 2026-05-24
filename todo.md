Status: Active
Source Idea Path: ideas/open/aarch64-codegen-forward-migration-second-wave-audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Remaining Codegen Surfaces

# Current Packet

## Just Finished

Step 1: Map Remaining Codegen Surfaces completed as an audit-only inventory.
Reference model used:
`ref/claudes-c-compiler/src/backend/arm/codegen/README.md` treats target
codegen as final AArch64 assembly emission: ABI/register/frame policy, call
marshalling, memory addressing, ALU/compare/float/cast/global/return/intrinsic
lowering, inline asm, and peephole/printing.

File-to-responsibility map:

- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`:
  direct target codegen includes AArch64 FP immediate materialization, GP/FP
  register spelling, load/store mnemonics, ADRP/ADD symbol loads, compare/ALU
  spellings, and final materialization lines. Broad mechanics include same-block
  producer chasing, recursive value-publication planning, PreparedValueHome
  interpretation, select-chain materialization, stack-vs-register publication
  decisions, and publication of cast/FP-binary results into Prepared homes.
- `src/backend/mir/aarch64/codegen/dispatch_store_sources.cpp`: direct target
  codegen includes scalar conversion/cast emission, global-address emission,
  pointer-base-plus-offset address spelling, store mnemonic choice, and final
  memory instructions. Broad mechanics include store-source discovery,
  logical-slot matching, narrow-store to wide-load recovery, byval/frame-slot
  source recognition, stack-homed pointer-store writeback, and pending
  store-global stack publication scheduling.
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`: direct target
  codegen includes register view/mnemonic mapping, frame/global/va-list address
  spelling, and scalar type width helpers tied to AArch64 register views. Broad
  mechanics include Prepared frame-slot/object lookup, value-home lookup,
  current-block entry publication tracking, clobber checks, and generic
  instruction-result/value-publication bookkeeping.
- `src/backend/mir/aarch64/codegen/dispatch_dynamic_stack.cpp`: direct target
  codegen includes `sp` adjustment/save/restore sequences, dynamic alloca
  assembly, frame-pointer address spelling, and AArch64 scratch/register-home
  names. Broad mechanics include recognizing dynamic-stack helper calls,
  matching them to `PreparedDynamicStackOp`, validating Prepared value homes,
  and converting unsupported helper cases into machine rejection records.
- `src/backend/mir/aarch64/codegen/dispatch_branch_fusion.cpp`: direct target
  codegen includes AArch64 condition suffixes, CMP immediate encodability,
  register/address spelling, compare-branch assembly construction, and final
  block-label branch instructions. Broad mechanics include same-block
  cast/load/binary producer discovery, constant evaluation, branch-fusion
  eligibility, support-instruction filtering, selected-operand use checks, and
  Prepared frame-slot load-address lookup.
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`: mostly broad
  BIR/MIR pipeline mechanics: same-block producer lookup, select-chain
  discovery, integer constant evaluation, load-global target lookup, BIR block
  lookup, and join parallel-copy source detection. Target-local content is
  limited to global symbol label spelling assumptions.
- `src/backend/mir/aarch64/codegen/dispatch_calls.cpp`: direct target codegen
  includes call instruction construction, indirect callee register
  materialization with AArch64 scratch registers and `csel`/`cmp`, call result
  source-register recording, and variadic/dynamic-stack handoff. Broad
  mechanics include scalar call-argument producer materialization,
  local-aggregate address publication, local-load/store alias recovery for
  indirect callees, call-boundary source materialization, move reload detection,
  and preserved-value publication/republication orchestration.
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`: direct target codegen
  includes ABI lane/register spelling, outgoing stack base setup,
  frame-slot-address materialization, immediate-to-register/stack move
  emission, F128 carrier-specific moves, byval stack/register lane copies, and
  before/after/return move machine instruction construction. Broad mechanics
  include Prepared move-bundle traversal, move classification, ordering around
  call boundaries, preservation-home population/republication, value stack
  moves, aggregate copy planning, and source/destination effect-resource
  bookkeeping.
- `src/backend/mir/aarch64/codegen/dispatch.hpp`: mixed declaration hub.
  Target-local declarations cover branch conditions, scalar mnemonics/register
  views, AArch64 memory/address helpers, dynamic-stack lowering, and call
  boundary emission entry points. Broad mechanics declarations expose producer
  lookup, Prepared home/value lookup, publication/clobber tracking, move
  orchestration, and helper APIs that are not inherently final target printing.

Summary split:

- Direct AArch64 instruction selection/printing remains in final emission
  helpers: register/view parsing, mnemonic choice, address spelling,
  ADRP/ADD/loads/stores, CMP/CSEL/branch forms, stack-pointer adjustment,
  call/return ABI register spellings, and assembler record construction.
- Broad Prepared/MIR pipeline mechanics still present in target codegen include
  producer discovery, select-chain/value-publication planning, Prepared home
  interpretation, logical-slot and stack-object matching, dynamic-stack helper
  recognition, branch-fusion eligibility analysis, call-boundary move ordering,
  preserved-value scheduling, and publication/republication bookkeeping.

## Suggested Next

Step 2 classification packet: convert the map above into a compact candidate
table with bucket, destination layer, x86/RISC-V reuse value, risk, and proof
expectation. Suggested first-pass rows:

- same-block producer/select-chain discovery (`dispatch_producers.cpp`,
  branch/call/value/store users)
- value publication planning into Prepared homes
  (`dispatch_value_materialization.cpp`, `dispatch_publication.cpp`)
- store-source publication and narrow-store recovery
  (`dispatch_store_sources.cpp`)
- dynamic-stack helper recognition versus target-local stack-pointer emission
  (`dispatch_dynamic_stack.cpp`)
- branch-fusion eligibility/support filtering versus final AArch64 branch
  spelling (`dispatch_branch_fusion.cpp`)
- call-boundary move ordering and preservation/republication mechanics
  (`dispatch_calls.cpp`, `calls_moves.cpp`)
- generic helper declarations still centralized in `dispatch.hpp`

## Watchouts

- This is an audit run; do not implement broad code movement.
- Do not regenerate first-wave ideas unless a concrete second slice is visible.
- Keep source-idea edits rare; routine audit notes should stay in this file
  until a durable follow-up idea is created.
- Several files intentionally combine target-local emission and reusable
  planning; Step 2 should avoid proposing any move that hides AArch64 ABI lane
  rules, register spelling, stack-pointer rules, or instruction sequences in a
  generic layer.
- The most reusable-looking areas are producer discovery, publication planning,
  Prepared home lookup, and call-boundary orchestration; each still needs an
  explicit x86/RISC-V consumer before becoming a follow-up idea.

## Proof

Audit-only `todo.md` update. No build or test proof required by the packet, and
no `test_after.log` was created.
