Status: Active
Source Idea Path: ideas/open/68_aarch64_large_owner_residue_audit.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Classify Each Major Helper Group

# Current Packet

## Just Finished

Step 3 - Classify Each Major Helper Group completed an audit-only
classification table from the Step 2 helper-group comparison notes for the
six source-idea high-residue owners plus comparable audited owners.

Classification table:

| Audited owner(s) | Major helper group | Classification | Authority, boundary, or probe | Next action |
| --- | --- | --- | --- | --- |
| `calls.cpp` | Call-plan lookup, argument/result binding, after-call result lanes, and call-boundary move bundles | `consume-shared` | Existing shared facts: `PreparedCallPlan`, `PreparedCallArgumentPlan`, `PreparedMoveBundle`, `PreparedAfterCallResultLaneBinding`, call-boundary classification from `prealloc/call_plans.*`, `prealloc/calls.hpp`, and `prealloc/prepared_lookups.*`. | Follow-up should replace local re-derivation with prepared call-plan queries, leaving only AArch64 register conversion and instruction records local. |
| `calls.cpp`, `dispatch_publication.cpp` | Edge publication, edge-copy source facts, current-block join sources, typed stack-source and store-source publication | `consume-shared` | Existing shared facts/queries: `PreparedEdgePublication*`, `PreparedEdgeCopySourceFacts`, `PreparedCurrentBlockJoinParallelCopySourceFacts`, `PreparedEdgePublicationSourceProducer*`, `PreparedTypedStackSourcePublication`, store-source publication plans from `prealloc/prepared_lookups.*` and `prealloc/publication_plans.*`. | Follow-up should route publication source selection through shared prepared-publication queries and keep AArch64 copy/move instruction selection local. |
| `calls.cpp` | AArch64 ABI register conversion, scratch choice, direct/indirect call spelling, and call-boundary machine records | `target-emission` | Target-local emission responsibility; shared authority stops at prepared call, move, and publication facts. | Keep in AArch64 calls owner or a more focused local calls helper; do not move to shared prealloc. |
| `memory.cpp` | Local/global/frame/pointer memory addressing, address materialization, value homes, storage plans, and frame offsets | `consume-shared` | Existing shared facts/queries: `PreparedAddressingFunction`, `PreparedMemoryAccess`, `PreparedAddressMaterialization`, frame-address offset queries, `PreparedValueHomeLookups`, `PreparedValueLocationFunction`, storage plans, and decoded home storage from `prealloc/addressing.*`, `prealloc/storage_plans.*`, `prealloc/value_locations.hpp`, and `prealloc/prepared_lookups.*`. | Follow-up should make memory lowering consume prepared address/value-home/storage facts directly before selecting AArch64 load/store forms. |
| `memory.cpp` | Aggregate stack-source and typed stack-source publication for memory-backed transfers | `consume-shared` | Existing shared facts/queries: `PreparedAggregateStackSourceAuthority` and `PreparedTypedStackSourcePublication` from `prealloc/prepared_lookups.*`. | Follow-up should remove duplicated source-authority decisions from AArch64 memory helpers while preserving local memory instruction choice. |
| `memory.cpp`, `f128.cpp`, `variadic.cpp` | AArch64 encodable offset checks, base/scratch register selection, memory opcode selection, address spelling, and va_list field addressing | `target-emission` | Target-local emission responsibility because legality and spelling are AArch64 instruction-selection details. | Keep local; any cleanup should be a local helper split rather than shared-authority migration. |
| `alu.cpp`, `cast_ops.cpp` | BIR scalar opcode, cast opcode, and scalar type semantics | `consume-shared` | Existing shared authority: `bir::BinaryOpcode`, `bir::CastOpcode`, `bir::TypeKind`, and BIR value/type queries. | Follow-up should remove local semantic duplication and use BIR opcode/type facts as the source of truth before AArch64 opcode selection. |
| `alu.cpp`, `cast_ops.cpp` | Prepared scalar homes, register classes, storage, and decoded home storage | `consume-shared` | Existing shared facts/queries: `PreparedValueHomeLookups`, `PreparedValueLocationFunction`, storage plans, regalloc facts, and decoded home storage. | Follow-up should consume prepared placement facts instead of re-deciding placement in scalar and cast lowering. |
| `alu.cpp` | Same-block scalar producer, computed-value, and immediate-binary classification | `consume-shared` | Existing shared facts/queries: `PreparedSameBlockScalarProducer`, `PreparedComputedValue`, and immediate-binary classification from `prealloc/control_flow.hpp` and `prealloc/prepared_lookups.*`. | Follow-up should use prepared scalar producer/computed-value queries for operand provenance, then keep only AArch64 immediate admissibility local. |
| `alu.cpp`, `cast_ops.cpp` | AArch64 ALU opcode selection, immediate admissibility, fallback materialization, width/sign extension instruction choice, and scalar/cast record production | `target-emission` | Target-local emission responsibility; these decide concrete AArch64 instructions and records. | Keep local, with possible local owner cleanup only if repeated across scalar owners. |
| `machine_printer.cpp` | Register spelling, relocation/address spelling, atomic/scalar/call-boundary mnemonic spelling, printer validation, and final assembly text | `target-emission` | Target-local emission responsibility; value provenance should already be encoded in `InstructionRecord` operands and prepared ids. | Keep local in the printer; only consume earlier prepared ids through records. |
| `machine_printer.cpp`, `instruction.cpp`, `instruction.hpp`, `alu.cpp`, `comparison.cpp` | Repeated scalar type, register-view, and compare predicate helper logic | `fold-back-or-split` | Better local owner boundary: one focused AArch64 helper surface, likely beside instruction construction or ABI/register-profile helpers, with BIR type facts and `abi::convert_prepared_register` as inputs. | Follow-up should first inventory exact duplicate helpers, then fold them into one AArch64-local helper instead of spreading copies across printer, instruction, ALU, and comparison owners. |
| `instruction.hpp` | Operand kinds, AArch64 opcodes/pseudos/families, memory/scalar/branch/call/i128/f128/atomic/variadic record structs, printer mnemonic kinds, machine effects, and status records | `target-emission` | Target-local machine-record schema; shared authority should be referenced by ids/facts, not own the AArch64 schema. | Keep local; future cleanup can split schema by local family only if it improves ownership without moving semantics to shared code. |
| `instruction.cpp` | Name helpers, opcode-to-printer mapping, machine status derivation, operand wrapping, branch/scalar/frame/address/call record constructors, and unsupported-record construction | `target-emission` | Target-local record-construction and printer-bridge responsibility. | Keep local, except duplicate semantic helpers should be folded into the focused AArch64 helper boundary named above. |
| `instruction.cpp`, `memory.cpp` | Aggregate copy chunk/lane planning and transport decomposition | `missing-shared-authority` | Missing shared fact/query shape: a prepared aggregate-transport plan that names chunks, lanes, source/destination homes, scratch needs, and whether the transport is stack-backed, register-backed, or mixed. | Do not move this yet; draft a follow-up evidence/proposal idea only if local aggregate-copy duplication blocks cleanup. |
| `i128_ops.cpp` | i128 carrier selection, runtime-helper policy, lane bindings, ABI policy, call-preservation, and clobber policy | `consume-shared` | Existing shared facts/queries: `PreparedI128Carrier*`, `PreparedI128RuntimeHelper*`, lane bindings, ABI policy, call-preservation policy, clobber policy from `prealloc/i128_runtime_helpers.*`, plus value-home/regalloc facts. | Follow-up should consume prepared i128 helper/carrier policy and keep concrete AArch64 pair/lane emission local. |
| `i128_ops.cpp` | AArch64 i128 pair/lane instruction selection, shift/count admissibility, compare sequence emission, runtime-helper boundary records, and register conversion | `target-emission` | Target-local emission responsibility for AArch64 pair and helper-boundary records. | Keep local; split only within AArch64 i128 owner if a later cleanup needs narrower helpers. |
| `comparison.cpp` | Branch-condition authority, short-circuit branch plans, compare joins, fused compare operand producers, and branch target labels | `consume-shared` | Existing shared facts/queries: `PreparedControlFlowFunction`, `PreparedBranchCondition`, `PreparedShortCircuitBranchPlan`, `PreparedMaterializedCompareJoin*`, `PreparedFusedCompareOperandProducer`, and branch labels from `prealloc/control_flow.hpp` and `prealloc/prepared_lookups.*`. | Follow-up should route condition and compare-join decisions through prepared control-flow queries before constructing AArch64 compare/branch records. |
| `comparison.cpp` | AArch64 condition-code selection, compare operand materialization, compare/branch record construction, and final compare/branch emission | `target-emission` | Target-local emission responsibility; shared authority describes control-flow intent but not AArch64 condition-code spelling. | Keep local in comparison lowering. |
| `f128.cpp` | f128 carrier selection, runtime-helper policy, ABI transition/result ownership, resource/clobber policy, scalar result ownership, and memory-backed carrier facts | `consume-shared` | Existing shared facts/queries: `PreparedF128Carrier*`, `PreparedF128RuntimeHelper*`, ABI transition/result ownership/resource/clobber policy, scalar result ownership, memory-backed carrier facts from `prealloc/f128_runtime_helpers.*`, `prealloc/special_carriers.*`, addressing, and value homes. | Follow-up should consume prepared f128 carrier/helper policy before local AArch64 transport and helper-call emission. |
| `f128.cpp` | AArch64 Q/vector register conversion, f128 memory address spelling, helper call boundary records, and final f128 transport/helper emission | `target-emission` | Target-local emission responsibility for AArch64 vector/register and helper-boundary records. | Keep local, with any cleanup scoped to AArch64 f128 transport helpers. |
| `variadic.cpp` | Variadic entry plan and helper operand homes | `consume-shared` | Existing shared facts/queries: `PreparedVariadicEntryPlanFunction` and `PreparedVariadicEntryHelperOperandHomes` from `prealloc/variadic_entry_plans.*` and `prealloc/variadic.hpp`. | Follow-up should consume prepared variadic entry facts directly before AArch64 va_list/save-area emission. |
| `variadic.cpp` | AArch64 GP/FP save-area instruction selection, va_arg/va_copy record emission, and va_list layout spelling | `target-emission` | Target-local emission responsibility for AArch64 ABI layout and instruction records. | Keep local in variadic lowering. |
| `dispatch_publication.cpp`, `calls.cpp`, `memory.cpp` | Publication materialization ordering across call edges, memory sources, and dispatch edges | `needs-more-evidence` | Narrow probe: trace one edge-copy publication, one call-boundary publication, and one typed stack-source publication from prepared facts to AArch64 records, then identify whether ordering is already shared or locally re-derived. | Before drafting implementation cleanup, run the probe and decide whether this is pure `consume-shared` or needs a new publication-order query. |
| `machine_printer.cpp`, `instruction.cpp` | Machine status and printer validation overlap | `needs-more-evidence` | Narrow probe: compare status derivation helpers against printer validation checks for the same record families and list any semantic checks repeated in both owners. | Keep target-local by default; only fold into a local helper if the probe shows duplicated semantic validation instead of final text validation. |

## Suggested Next

Proceed to Step 4 by grouping the classification rows into scoped follow-up
idea drafts: one shared-authority migration for call/publication facts, one
memory/address/value-home migration, one scalar/control-flow migration, one
i128/f128/variadic helper-policy migration, and separate local AArch64 helper
or evidence-probe ideas for fold-back and needs-more-evidence rows.

## Watchouts

- Audit-only route: no implementation files were edited for this packet.
- `src/backend/mir/arm/` is absent in this checkout, so the classification is
  based on Step 2's RISC-V/x86 reference notes plus shared prepared authority.
- Machine records and printer spelling remain `target-emission` unless a
  narrow probe proves duplicated semantic validation.
- Aggregate transport is the only clear `missing-shared-authority` entry from
  the Step 2 notes; it should not be converted into implementation work until a
  follow-up idea names the exact prepared aggregate-transport query shape.

## Proof

No build/test required for this audit-only `todo.md` update; no
`test_after.log` was produced for the delegated proof.
Proof command: `git diff --check -- todo.md`
Result: passed.
