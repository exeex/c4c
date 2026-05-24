Status: Active
Source Idea Path: ideas/open/aarch64-codegen-forward-migration-candidate-audit.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Produce the Audit Table and Priority List

# Current Packet

## Just Finished

Step 4 - Produce the Audit Table and Priority List completed by condensing
Steps 1-3 into a reviewable audit table and forward-migration priority list.

Audit table:

| Current AArch64 codegen files/helper group | Bucket | Reference-model comparison | Destination / reuse note |
| --- | --- | --- | --- |
| `instruction.*`, `machine_printer.*`, `asm_emitter.*`, family `print_*` helpers | Keep target-local | Reference ARM codegen owns mnemonic, register, condition-code, operand, and assembly syntax spelling. | Keep in AArch64; x86/RISC-V need their own spelling and target printers. |
| `alu.*`, `memory.*`, `comparison.*`, `cast_ops.*`, `float_ops.*`, `globals.*`, `atomics.*`, `intrinsics.*`, `inline_asm.*`, `f128.*`, `i128_ops.*`, `returns.*`, `prologue.*`, `variadic.*`, `peephole.*` target-emission portions | Keep target-local | Maps to the reference inventory when selecting AArch64 instructions, applying AAPCS64 policy, enforcing target addressing/immediate limits, or doing target cleanup. | Keep target-local; only split out generic Prepared operand recovery or diagnostics when separable. |
| `codegen.hpp`, `module_compile.*`, `traversal.*` prepared-module handoff and traversal | Move to shared MIR / Needs target hook | No reference analogue for prepared BIR handoff, compiled target module product, or shared machine-module traversal. | Use shared MIR for `MachineModule`/function/block stream contracts; keep AArch64 BuildResult and target handoff local. x86 already exposes similar public prepared/module boundaries. |
| Prepared fact indexes now associated with AArch64 module/lowering context: call-plan, address-materialization, move-bundle, and value-home indexes | Move to prealloc | Reference codegen assumes existing state; it does not describe per-target rebuilding of Prepared lookup indexes. | High x86 reuse via `ConsumedPlans`; RISC-V can later consume a common index if it adopts PreparedBirModule. Keep helpers read-only over prealloc-owned facts. |
| `operands.*` generic value-home and storage decoding | Move to prealloc / Needs target hook | Prepared storage/value-home interpretation is outside the reference ARM codegen model; target register/immediate spelling still maps to codegen. | Prealloc owns `PreparedValueHome`, storage plans, assignments, frame slots, immediates, symbols, and labels. Targets should map decoded decisions to target operands/register banks. |
| `dispatch_publication.cpp`, `dispatch_store_sources.cpp`, `dispatch_value_materialization.cpp`, generic pieces of `dispatch_lookup.cpp` | Move to prealloc / shared prepared-consumer helper | Reference model has no Prepared machine-value publication layer. | Reuse value is high for x86 because prepared lowering already consumes value-location and move records; AArch64 should only emit target instructions for the already-classified publications. |
| `dispatch_entry_formals.cpp` entry-formal publication planning | Move to prealloc / Needs target hook | Reference AAPCS64 entry handling is target-local, but generic formal publication from prepared homes is not a target-codegen responsibility. | Prealloc should plan formal homes/publications; targets keep ABI register names and concrete entry copies. x86 can consume the same formal publication plan. |
| `dispatch_calls.cpp`, `calls_common.cpp`, `calls_argument_sources.cpp`, `calls_moves.cpp`, `calls_preservation.cpp` generic call-boundary interpretation | Move to prealloc / Needs target hook | Reference `calls.rs` covers target ABI emission, but not generic prepared call-plan lookup, move-bundle classification, or preservation bookkeeping. | Prealloc call helpers already own argument/result placement, caller/callee-save policy, and move records. AAPCS64 sret, variadic lanes, clobber spellings, and printed call records stay AArch64-local. |
| `dispatch_edge_copies.cpp` and block-entry publication bookkeeping | Move to prealloc / shared MIR | Reference model does not describe Prepared edge-copy classification or block-entry machine-value bookkeeping. | High reuse for any target lowering SSA-edge moves from prepared bundles; target hook may be needed only for concrete move emission. |
| `dispatch_diagnostics.cpp` and missing Prepared-fact diagnostics in AArch64 module context | Move to prealloc or shared MIR | Reference diagnostics are not a Prepared consumer diagnostic layer. Current messages name missing function context, value authority, typed register authority, storage plan, call plan, and block/instruction mappings. | Share diagnostic builders near the owner of the missing facts (`prealloc`) or the consumer stream context (`mir`). Exact bucket can be deferred, but duplicating messages in x86 is low value. |
| `compatibility_projection.*` | Unknown / defer, likely shared MIR or retirement | Reference codegen emits final assembly and has no legacy flat-view projection from target MIR. | Shared MIR already has compatibility flattening concepts. Audit callers/tests before deciding whether to share the adapter or delete it. |
| `dispatch_branch_fusion.cpp`, `dispatch_dynamic_stack.cpp`, `dispatch_producers.cpp`, mixed `make_prepared_*` family entrypoints | Needs target hook / Unknown split | Mixed surfaces: some generic Prepared lookup or producer discovery, plus target control-flow, stack, or instruction emission decisions. | Do not move as a batch. Split only after identifying a read-only helper or policy-free record with x86 reuse. |
| `calls_printing.cpp`, target ABI details in call helpers | Keep target-local | Matches reference target call-record and assembly spelling responsibilities. | Keep in AArch64; generic call-boundary move classification is the separable migration candidate. |

Reference-model summary:
- Target instruction selection, AAPCS64 call/frame/return effects, target
  immediate/addressing constraints, register spelling, instruction records, and
  assembly printing remain AArch64 codegen responsibilities.
- The migration-worthy areas are outside the reference model because they
  rebuild or interpret `PreparedBirModule` facts, create target-independent
  indexes, publish machine values, classify move bundles, and diagnose missing
  prepared authority.
- `src/backend/bir` is not the direct destination for current
  Prepared-to-machine consumer logic. Use BIR only when a future audit finds an
  earlier semantic fact should be produced during LIR-to-BIR lowering.
- `src/backend/prealloc` is the strongest owner for prepared facts, storage and
  value-home interpretation, call/move records, and reusable lookup indexes.
  Shared MIR is the stronger owner for machine stream shape, traversal,
  compatibility flattening, and possibly consumer-facing diagnostics.
- x86 is the concrete reuse precedent because it already consumes prepared
  plans through `ConsumedPlans`, `prepared/*`, `lowering/*`, and `module/*`.
  RISC-V is currently a reference-model contrast, not a near-term destination,
  because it still exposes a Rust-style LIR text-emitter route.

Prioritized forward-migration candidates:

1. Prepared move/publication indexing.
   - Current AArch64 responsibility: target-local construction/use of
     call-plan, address-materialization, move-bundle, and value-home indexes.
   - Destination: `src/backend/prealloc` or a read-only prepared-consumer helper
     adjacent to it.
   - Reuse value/risk: highest reuse and low behavior risk if introduced as
     shared lookup helpers with no emission changes; x86 has matching
     `ConsumedPlans` pressure.
2. Value-home and storage-encoding interpretation.
   - Current AArch64 responsibility: `operands.*` generic decoding of prepared
     homes, storage-plan values, assignments, frame slots, immediates, symbols,
     and labels before target operand spelling.
   - Destination: `src/backend/prealloc` helper APIs with target hooks for
     register-bank and final operand mapping.
   - Reuse value/risk: high reuse for x86 prepared operands; medium risk
     because target immediates, condition use, and register views must remain
     target-local.
3. Call-boundary move classification.
   - Current AArch64 responsibility: generic portions of `dispatch_calls.cpp`
     and `calls_*` that interpret prepared call plans, argument/result sources,
     call moves, clobbers, and preservation records.
   - Destination: `src/backend/prealloc/calls.*` plus `regalloc/call_moves.*`
     and related call-return ABI helpers.
   - Reuse value/risk: high reuse because all prepared targets need the same
     call-boundary classifications; medium risk from interleaving with AAPCS64
     sret, variadic, and printable call-record details.
4. Entry-formal publication planning.
   - Current AArch64 responsibility: `dispatch_entry_formals.cpp` planning of
     formal publications from prepared homes into machine values.
   - Destination: `src/backend/prealloc` value-location/publication planning
     with target hooks for concrete ABI register/stack copy emission.
   - Reuse value/risk: high reuse for x86 function-entry lowering; medium risk
     because the boundary with target ABI entry effects must stay explicit.
5. Edge-copy and block-entry bookkeeping.
   - Current AArch64 responsibility: `dispatch_edge_copies.cpp` and adjacent
     block-entry publication bookkeeping.
   - Destination: `src/backend/prealloc` move-bundle consumption helpers or
     shared MIR block-entry utilities.
   - Reuse value/risk: medium-high reuse for all PreparedBirModule consumers;
     medium risk because concrete parallel-copy emission remains target-owned.
6. Shared diagnostics for missing Prepared facts.
   - Current AArch64 responsibility: `dispatch_diagnostics.cpp` and diagnostic
     enum/message patterns for missing prepared authority.
   - Destination: shared prepared-consumer diagnostics under `prealloc` or
     `mir`.
   - Reuse value/risk: medium reuse and low behavior risk if added as message
     builders; exact ownership should be decided after choosing whether the
     diagnostic is fact-owner-facing or machine-consumer-facing.
7. Prepared-module public handoff and traversal shape.
   - Current AArch64 responsibility: `codegen.hpp`, `module_compile.*`, and
     `traversal.*` coordinating prepared-module lowering into compiled MIR.
   - Destination: shared MIR stream contracts plus target API/module boundary
     cleanup.
   - Reuse value/risk: medium reuse, higher design risk; do after narrower
     prepared lookup/publication helpers prove the split.
8. Compatibility projection.
   - Current AArch64 responsibility: `compatibility_projection.*` and legacy
     flat-record production.
   - Destination: shared MIR compatibility adapter or retirement.
   - Reuse value/risk: low-medium reuse and unknown risk until callers/tests are
     audited; defer behind the higher-value prepared-consumer candidates.

## Suggested Next

Start Step 5 through lifecycle authority by drafting focused follow-up
`ideas/open/*.md` entries for the top candidates, beginning with Prepared
move/publication indexing and value-home/storage interpretation. Each idea
should name the current AArch64-owned helper group, destination layer, x86 reuse
path, proof requirements, and reject signals for testcase-overfit or accidental
movement of AArch64 ABI/spelling policy.

## Watchouts

Do not turn Step 5 into one broad migration idea. The first implementation idea
should be read-only or record/helper oriented, leave AArch64 output unchanged,
and explicitly reject movement of instruction spelling, AAPCS64 policy,
register names, immediate/addressing legality, or assembly printing.

`prealloc` is the best default destination for fact authority and lookup; shared
`mir` is better for machine stream shape, traversal, compatibility, and possibly
consumer diagnostics. BIR is not the destination unless the responsibility is
really semantic fact production before preallocation.

## Proof

Documentation/audit-only packet. No build, ctest, clang-tools, or
`test_after.log` was required by the delegated proof because the owned change
was limited to canonical `todo.md` audit notes. Steps 1-3 were synthesized from
existing `todo.md` history and current plan context; no implementation files,
`plan.md`, `ideas/open/*`, or `ref/` files were touched.
