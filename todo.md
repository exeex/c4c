Status: Active
Source Idea Path: ideas/open/68_aarch64_large_owner_residue_audit.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Compare Against Reference And Shared Authority

# Current Packet

## Just Finished

Step 2 - Compare Against Reference And Shared Authority completed an
audit-only helper-group comparison for the six source-idea high-residue owners
plus the clearest comparable owners recorded in Step 1.

Reference layout note: this checkout has no `src/backend/mir/arm/` tree, so no
ARM-specific file-by-file comparison was available. The available target
reference surfaces are `src/backend/mir/riscv/codegen/` and the prepared x86
notes/surfaces under `src/backend/mir/x86/`; shared authority lives primarily
under `src/backend/prealloc/` and `src/backend/bir/`.

Per-owner comparison notes:

| Owner | Comparable target layout evidence | Helper groups and likely authority source |
| --- | --- | --- |
| `calls.cpp` | RISC-V keeps outgoing ABI work in `calls.cpp`; x86 exposes a prepared query facade in `src/backend/mir/x86/prepared/prepared.hpp`. | Call-plan lookup and argument/result binding should consume `PreparedCallPlan`, `PreparedCallArgumentPlan`, `PreparedMoveBundle`, `PreparedAfterCallResultLaneBinding`, and call-boundary classification from `prealloc/call_plans.*`, `prealloc/calls.hpp`, and `prealloc/prepared_lookups.*`; edge/call-boundary republication should consume `PreparedEdgePublication*`, `PreparedEdgePublicationSourceProducer*`, `PreparedTypedStackSourcePublication`, and store-source publication plans from `prealloc/prepared_lookups.*` and `prealloc/publication_plans.*`; target-local residue is ABI register conversion, AArch64 call instruction records, scratch choice, direct/indirect branch spelling, and call-boundary move emission. |
| `memory.cpp` | RISC-V keeps load/store/GEP/dynamic stack in `memory.cpp`; x86 lowering notes describe prepared frame/addressing as a policy layer over shared prepared metadata. | Local/global/frame/pointer memory access should consume `PreparedAddressingFunction`, `PreparedMemoryAccess`, `PreparedAddressMaterialization`, frame-address offsets, value homes, and storage plans from `prealloc/addressing.*`, `prealloc/storage_plans.*`, `prealloc/value_locations.hpp`, and `prealloc/prepared_lookups.*`; aggregate stack-source and typed stack-source publication should consume `PreparedAggregateStackSourceAuthority` and `PreparedTypedStackSourcePublication` from `prealloc/prepared_lookups.*`; target-local residue is AArch64 encodable offset tests, scratch/base register selection, memory instruction records, and load/store opcode selection. |
| `alu.cpp` | RISC-V keeps scalar integer ALU in a small `alu.cpp`; x86 lowering notes split scalar lowering from prepared placement queries. | BIR opcode/type semantics should stay with `bir::BinaryOpcode`, `bir::TypeKind`, and BIR values; prepared home/register facts should consume `PreparedValueHomeLookups`, `PreparedValueLocationFunction`, storage plans, and decoded home storage rather than re-deriving placement; same-block scalar producer and computed-value facts have shared surfaces in `PreparedSameBlockScalarProducer`, `PreparedComputedValue`, and immediate-binary classification in `prealloc/control_flow.hpp` and `prealloc/prepared_lookups.*`; target-local residue is AArch64 ALU opcode selection, immediate admissibility, scalar record construction, fallback materialization, and emitted scalar register tracking. |
| `machine_printer.cpp` | RISC-V emits mostly text directly from owner files; x86 prepared surfaces intentionally keep final operand text target-local. | Printer validation, register spelling, relocation/address spelling, atomic mnemonic spelling, scalar/call-boundary mnemonic spelling, and selected machine-record text are target-emission; value identity and prepared provenance should come only through `InstructionRecord` operands and prepared ids from earlier lowering; repeated type/register-view helpers overlap conceptually with `abi::convert_prepared_register`, BIR type queries, and target register profile surfaces, but final AArch64 assembly text remains local. |
| `instruction.hpp` | RISC-V has a compact `riscv_codegen.hpp`; x86 prepared structs expose target-facing query/operand wrappers but not a full machine-record schema. | Operand kinds, AArch64 opcode/pseudo/family enums, memory/scalar/branch/call/f128/i128/atomic/variadic record structs, printer mnemonic kinds, machine effects, and status records are target-emission schema; prepared ids, banks/classes, value homes, call/move plans, i128/f128 helper policies, and control-flow facts should remain references to `prealloc`/`bir` authority rather than copied semantic ownership. |
| `instruction.cpp` | RISC-V spreads record construction through emitting owners; x86 prepared notes keep final target lowering thin around shared plans. | Name helpers, opcode-to-printer mapping, machine status derivation, operand wrapping, branch/scalar/frame/address/call record constructors, and unsupported-record construction are target-emission; duplicate scalar type/register-view and compare predicate helpers are candidates to fold back to ABI/BIR helper surfaces or one local AArch64 helper owner; aggregate copy chunk/lane helpers are AArch64 instruction-selection policy unless a shared aggregate-transport plan is introduced. |
| `i128_ops.cpp` | RISC-V has a separate `i128_ops.cpp` for i128 pairs, shifts, compares, and runtime helpers. | i128 carrier/runtime-helper ownership should consume `PreparedI128Carrier*`, `PreparedI128RuntimeHelper*`, lane bindings, ABI policy, call-preservation policy, and clobber policy from `prealloc/i128_runtime_helpers.*` plus value homes/regalloc facts; target-local residue is AArch64 pair/lane instruction selection, shift/count admissibility, compare sequence emission, runtime-helper boundary records, register conversion, and final printing. |
| `comparison.cpp` | RISC-V keeps integer/float compare, fused compare-and-branch, select, and f128 compare in `comparison.cpp`; x86 prepared dispatch notes consume `PreparedBranchCondition` and short-circuit/compare-join facts. | Branch-condition and compare-join authority should consume `PreparedControlFlowFunction`, `PreparedBranchCondition`, `PreparedShortCircuitBranchPlan`, `PreparedMaterializedCompareJoin*`, `PreparedFusedCompareOperandProducer`, and branch target labels from `prealloc/control_flow.hpp` and `prealloc/prepared_lookups.*`; target-local residue is AArch64 condition-code selection, compare operand materialization, branch record construction, and final compare/branch emission. |
| `f128.cpp` | RISC-V has a separate `f128.cpp` soft-float owner; x86 prepared operand notes treat f128 homes/carriers as prepared placement facts. | f128 carrier/runtime-helper ownership should consume `PreparedF128Carrier*`, `PreparedF128RuntimeHelper*`, ABI transition/result ownership/resource/clobber policy, scalar result ownership, and memory-backed carrier facts from `prealloc/f128_runtime_helpers.*`, `prealloc/special_carriers.*`, addressing, and value homes; target-local residue is AArch64 Q/vector register conversion, f128 memory address spelling, helper call boundary records, and final transport/helper emission. |
| `cast_ops.cpp` | RISC-V keeps conversions in `cast_ops.cpp`; x86 has scalar/cast lowering notes. | Cast opcode semantics are BIR-owned through `bir::CastOpcode` and type facts; prepared value homes and register classes should come from value-location/regalloc/storage surfaces; target-local residue is AArch64 conversion opcode selection, width/sign extension instruction choice, and scalar cast record/error production. |
| `dispatch_publication.cpp` | RISC-V has no directly comparable prepared-publication owner in the small layout; x86 prepared dispatch and operand notes consume edge-publication intent. | Edge publication, edge-copy source facts, current-block join sources, and source-producer classification should consume `PreparedEdgePublication*`, `PreparedEdgeCopySourceFacts`, `PreparedCurrentBlockJoinParallelCopySourceFacts`, and `PreparedEdgePublicationSourceProducerLookups` from `prealloc/prepared_lookups.*`; target-local residue is AArch64 move/copy instruction selection and local publication materialization. |
| `variadic.cpp` | RISC-V has a small `variadic.cpp`; shared prealloc has variadic entry plan surfaces. | Variadic entry/helper operand homes should consume `PreparedVariadicEntryPlanFunction` and `PreparedVariadicEntryHelperOperandHomes` from `prealloc/variadic_entry_plans.*` and `prealloc/variadic.hpp`; target-local residue is AArch64 va_list field addressing, GP/FP save-area instruction selection, and va_arg/va_copy record emission. |

## Suggested Next

Proceed to Step 3 by turning these helper-group notes into the classification
table: `target-emission`, `consume-shared`, `missing-shared-authority`,
`fold-back-or-split`, or `needs-more-evidence`.

## Watchouts

- Audit-only route: no implementation files were edited for this packet.
- `src/backend/mir/arm/` is absent in this checkout; Step 3 should classify
  from the shared prepared authority and available RISC-V/x86 reference
  surfaces unless the supervisor provides an ARM source surface.
- Machine records and printer spelling still look target-local unless a later
  probe finds duplicated semantic authority.
- The highest-value consume-shared candidates are call-boundary/publication
  facts, memory/address/value-home facts, control-flow/compare-join facts, and
  i128/f128 prepared helper policies.

## Proof

No build/test required for this audit-only `todo.md` update; no `test_after.log`
was produced for the delegated proof.
Proof command: `git diff --check -- todo.md`
Result: passed.
