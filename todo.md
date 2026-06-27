Status: Active
Source Idea Path: ideas/open/418_prepared_target_consumer_boundary_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Target Consumer Recovery Sites

# Current Packet

## Just Finished

Completed Step 1 inventory for RV64/RISC-V and AArch64 prepared target
consumer recovery sites. First-pass classifications:

| Finding ID | Target/helper group | File-reference evidence | Taxonomy rows | First-pass owner classification | Downstream owner |
| --- | --- | --- | --- | --- | --- |
| `418-INV-RV64-CALL-COHERENT-001` | RV64 prepared simple call lowering over explicit prepared call plans, argument/result moves, and preservation effects. | `src/backend/mir/riscv/codegen/prepared_call_emit.cpp:390` finds the prepared call plan; `:430` and `:584` consume before/after move bundles; `:474`-`:482` rejects non-GPR or aggregate routes; `:557`-`:580` consumes an explicit result move. | `TAX-FAM-CALL-BOUNDARY-ARG-RESULT-001`, `TAX-FAM-CALL-ARG-TYPED-ROUTES-PLACEHOLDER-001` | Acceptable coherent-fact lowering for simple GPR direct/indirect calls. Unsupported-but-coherent when route is outside the selected RV64 subset. | Idea 414 |
| `418-INV-RV64-CALL-RECOVERY-001` | RV64 frame-slot address call argument materialization recovers stack offsets from prepared address-materialization lookups and emits target stack arithmetic. | `src/backend/mir/riscv/codegen/prepared_call_emit.cpp:31`-`:72` scans indexed prepared address materializations for a matching value; `:75`-`:105` turns the selected offset into `addi`; `:518`-`:524` tries this path before falling back to register move emission. | `TAX-FAM-CALL-ARG-TYPED-ROUTES-PLACEHOLDER-001`, `TAX-FAM-MEMORY-ACCESS-PLACEHOLDER-001`, storage-relevant `TAX-FAM-VALUE-HOME-TYPED-STORAGE-001` | Suspected producer-gap/consumer recovery boundary: the offset comes from prepared facts, but the target reconstructs the final call source route locally rather than consuming a single typed call-argument fact. Missing/incoherent route fields should be producer-owned, not guessed. | Ideas 414 and 417 |
| `418-INV-RV64-BYVAL-COHERENT-001` | RV64 byval aggregate call argument lowering validates explicit aggregate transport chunks and copies prepared stack payload. | `src/backend/mir/riscv/codegen/prepared_call_emit.cpp:257`-`:310` requires `LocalFrameAddressMaterialization` plus `PreparedAggregateTransport`; `:328`-`:364` proves required payload chunk coverage before emitting copy lines. | `TAX-FAM-CALL-ARG-TYPED-ROUTES-PLACEHOLDER-001`, `TAX-FAM-MEMORY-ACCESS-PLACEHOLDER-001` | Acceptable coherent-fact lowering when all aggregate transport fields are complete; unsupported-but-coherent for unlaned/large or non-stack-copy variants. | Ideas 414 and 417 |
| `418-INV-RV64-GLOBAL-RECOVERY-001` | RV64 assembly global storage/load/store helpers inspect raw BIR globals, initializers, string constants, and source/fallback names. | `src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp:75`-`:120` reconstructs i32 words from `bir::Global` initializers; `:183`-`:218` matches string constants by `global.name`; `:248`-`:271` falls back from link-name spelling to raw names; `:290`-`:294` strips `@` from raw value names; `:505`-`:563` and `:565`-`:630` combine prepared memory access facts with BIR load/store fields and fallback global names. | `TAX-FAM-GLOBAL-INITIALIZER-STORAGE-PLACEHOLDER-001`, `TAX-FAM-PUBLICATION-FACTS-PLACEHOLDER-001`, `TAX-FAM-MEMORY-ACCESS-PLACEHOLDER-001`, `TAX-FAM-VALUE-MATERIALIZATION-PLACEHOLDER-001` | Suspected target recovery/producer-gap boundary. Prepared memory access facts are present, but global initializer bytes, labels, and symbol identity are still reconstructed from target-side BIR/source spelling. Missing or contradictory global/object facts should classify as `producer_missing` or `producer_incoherent`; coherent unsupported data forms should classify as `target_unsupported_but_coherent`. | Idea 417, with Idea 415 for symbol/value materialization handoff |
| `418-INV-RV64-OBJECT-GLOBAL-RECOVERY-001` | RV64 object emission reconstructs labels and data bytes from BIR globals/string constants. | `src/backend/mir/riscv/codegen/object_emission.cpp:8409`-`:8432` falls back to raw global/text names; `:8463`-`:8541` derives initializer bytes from BIR immediate values/elements; `:8550`-`:8635` emits sections from those reconstructed bytes and rejects unsupported forms. | `TAX-FAM-GLOBAL-INITIALIZER-STORAGE-PLACEHOLDER-001`, `TAX-FAM-PUBLICATION-FACTS-PLACEHOLDER-001` | Suspected target recovery/producer-gap boundary for object-data publication: byte/section facts should be prepared global facts, while RV64 owns only emission of coherent bytes/relocations/zero-fill. | Idea 417 |
| `418-INV-RV64-INLINEASM-COHERENT-001` | RV64 prepared inline asm object helpers substitute operands from complete prepared inline-asm carriers. | `src/backend/mir/riscv/codegen/object_emission.cpp:8089`-`:8132` rejects incomplete carriers and maps immediates/homes; `:8135`-`:8145` classifies `.insn.d` only after requiring a complete carrier. | `TAX-FAM-HELPER-OPERAND-TYPED-PLACEHOLDER-001`, related `TAX-FAM-VALUE-HOME-TYPED-STORAGE-001` | Acceptable coherent-fact lowering for a narrow helper/inline-asm surface; missing operand homes should remain producer-owned. | Idea 416 |
| `418-INV-A64-VALUE-COHERENT-001` | AArch64 value operand resolution consumes decoded prepared home/storage facts and attaches verifier reports on fail-closed paths. | `src/backend/mir/aarch64/codegen/operands.cpp:88`-`:157` lowers decoded register/frame/immediate/symbol homes; `:174`-`:187` attaches the contract report; `:205`-`:227` verifies before materializing operands. | `TAX-FAM-VALUE-HOME-TYPED-STORAGE-001` | Acceptable coherent-fact lowering; this is the reference target-consumer pattern from idea 413. | Idea 415 and storage-relevant Idea 417 rows |
| `418-INV-A64-CALL-COHERENT-001` | AArch64 call plan diagnostics and boundary moves consume explicit call plans and carry contract reports. | `src/backend/mir/aarch64/codegen/calls.cpp:84`-`:122` finds call plans and converts unavailable classifications to prepared contract reports; `:9125`-`:9305` materializes missing frame-slot call arguments from prepared publication needs, value homes, move bundles, and source selections. | `TAX-FAM-CALL-BOUNDARY-ARG-RESULT-001`, `TAX-FAM-CALL-ARG-TYPED-ROUTES-PLACEHOLDER-001`, `TAX-FAM-PUBLICATION-FACTS-PLACEHOLDER-001` | Mostly coherent-fact lowering. The `materialize_missing_frame_slot_call_arguments` helper is a selected follow-up boundary because it bridges a missing publication need and may need a stronger typed route/diagnostic instead of silent `continue` when facts are absent. | Idea 414, with publication handoff to Ideas 415/417 |
| `418-INV-A64-PRESERVE-RECOVERY-001` | AArch64 stack-preserved call value publication can recover source registers from emitted scalar state or value homes and emits a store to a frame slot. | `src/backend/mir/aarch64/codegen/calls.cpp:9309`-`:9330` finds stack-preserved values; `:9346`-`:9375` recovers a source register from emitted state or value homes; `:9384`-`:9390` publishes a target store with `frame_slot_address`. | `TAX-FAM-PUBLICATION-FACTS-PLACEHOLDER-001`, `TAX-FAM-CALL-ARG-TYPED-ROUTES-PLACEHOLDER-001`, storage-relevant `TAX-FAM-VALUE-HOME-TYPED-STORAGE-001` | Suspected target recovery/producer-gap boundary: target-side republication is over prepared facts, but the fallback from emitted state/value home to a store should be audited for producer-owned publication completeness and fail-closed diagnostics. | Ideas 414 and 417 |
| `418-INV-A64-VARIADIC-COHERENT-001` | AArch64 variadic helpers require complete entry/helper operand homes and access plans before producing records. | `src/backend/mir/aarch64/codegen/variadic.cpp:65`-`:79` names missing helper facts; `:82`-`:140` builds `va_start` only from complete entry facts and homes; `:143`-`:150` requires a complete scalar `va_arg` access plan. | `TAX-FAM-VARIADIC-HELPER-OPERAND-HOMES-001`, `TAX-FAM-HELPER-OPERAND-TYPED-PLACEHOLDER-001` | Acceptable coherent-fact lowering with existing missing/incoherent owner surface from idea 413. | Idea 416 |
| `418-INV-A64-INLINEASM-COHERENT-001` | AArch64 inline-asm carrier lowering rejects missing/incomplete prepared carriers and uses prepared operand homes, memory addresses, and immediates. | `src/backend/mir/aarch64/codegen/inline_asm.cpp:766`-`:783` rejects missing or incomplete carriers; `:829`-`:899` requires register homes/tied-home agreement; `:901`-`:963` consumes immediate and memory/address selections. | `TAX-FAM-HELPER-OPERAND-TYPED-PLACEHOLDER-001`, `TAX-FAM-VALUE-HOME-TYPED-STORAGE-001`, `TAX-FAM-MEMORY-ACCESS-PLACEHOLDER-001` | Acceptable coherent-fact lowering for inline-asm helper operands; missing carrier/home/address facts are producer gaps for Idea 416 rather than target inference. | Idea 416 |

## Suggested Next

Execute Step 2 by drafting
`docs/prepared_fact_contracts/target_consumer_boundary_audit.md` from the
inventory above. Suggested first audit rows: RV64 global/object-data recovery,
RV64 frame-slot call argument recovery, AArch64 stack-preserved republication,
and coherent-reference rows for AArch64 value operands/variadic helpers plus
RV64 simple call/byval/inline-asm lowering.

## Watchouts

- The RV64 target is under `src/backend/mir/riscv`; no
  `src/backend/mir/rv64` directory exists in this checkout.
- RV64 global/object helpers are the clearest first cleanup/owner-decision
  candidates: they mix prepared memory access with raw BIR globals,
  initializer bytes, and fallback spellings.
- AArch64 already has strong coherent examples from idea 413; avoid treating
  those as defects unless Step 2 needs reference rows.
- The AArch64 stack-preserved republication helper should be audited carefully
  before code changes because it may be intentional target scheduling over
  prepared facts, but missing publication completeness should not silently
  degrade into target recovery.

## Proof

Inventory-only packet. No build or test command was required or run. Proof is
the file-reference evidence and taxonomy row mapping recorded above; no
`test_after.log` was created or modified.
