# AArch64 Markdown Classification Index

This index classifies the AArch64 markdown artifacts preserved under
`src/backend/mir/aarch64/`. It is the review entry point for deciding which old
facts may inform the markdown-first rebuild and which routes must stay out of
the new BIR / prepared-backend contract.

Classification terms:

- `salvageable design note`: useful behavior or constraints, but not a route to
  revive directly.
- `obsolete route`: stale implementation path that must not define the rebuild.
- `binary-utils candidate`: object, ELF, relocation, or binary-inspection facts
  that may belong in shared binary utilities.
- `target-ABI candidate`: AArch64 ABI facts that may shape target-local lowering.
- `assembler/linker candidate`: assembler, encoder, or linker facts that may
  inform a later explicit toolchain layer.
- `delete/defer`: artifact should not drive near-term backend contract work.

## Contract Exclusions

These legacy routes must not influence the new BIR / `PreparedBirModule`
backend entry contract:

- Rendered assembly string recovery from codegen artifacts.
- The old `ArmCodegen` object model and direct text-emitter entry points.
- Legacy assembler parser operand recovery as a substitute for structured BIR
  or target-local MIR facts.
- Treating printed `.s` output as the bridge from codegen to the built-in
  encoder or object writer.
- Built-in linker orchestration as an implied backend output requirement.
- Feature-gated `gcc_assembler` or `gcc_linker` paths as target-selection
  policy.
- Any markdown-only surface as proof that live AArch64 codegen, assembly, or
  linking behavior currently exists.

## Current Ownership Ledger

The active ownership map for AArch64 feature families lives in
`BIR_PREPARED_GAP_LEDGER.md`. This index remains a legacy-artifact
classification surface only; it should not be used as the owning file for
feature-family responsibility, target-local records, carrier status, or first
implementation routes.

The memory carrier facts named by that ledger are the shared typed fields
`PreparedMemoryAccess::address_space` and
`PreparedMemoryAccess::is_volatile`. Legacy memory, assembler, object, linker,
atomics, inline-asm, or alias-analysis markdown must not be used to infer or
reconstruct those facts for target lowering.

## Artifact Index

| Artifact | Classification | Contract influence | Legacy or deprecated note |
| --- | --- | --- | --- |
| `CLASSIFICATION_INDEX.md` | salvageable design note | Index only; it does not define backend semantics. | Owner: markdown-first AArch64 route. Limitation: classification aid only. Removal condition: replaced by a more current backend-path review index. |
| `BINARY_UTILS_CONTRACT.md` | binary-utils candidate | May inform binary utility boundaries, not the AArch64 BIR entry type. | Owner: binary-utils baseline. Limitation: belongs to shared object/relocation proof, not target lowering. Removal condition: superseded by a committed shared binary-utils contract. |
| `mod.md` | obsolete route | Must not define new target-selection, feature-gate, or public backend entry contracts. | Owner: legacy top-level AArch64 route. Limitation: records old module grouping and feature-gate assumptions. Removal condition: new backend module contract replaces old exports. |
| `codegen/mod.md` | obsolete route | Must not define the new codegen module shape or old `ArmCodegen` ownership model. | Owner: legacy codegen module. Limitation: describes removed module wiring. Removal condition: target-local MIR and instruction-selection modules are documented. |
| `codegen/emit.md` | obsolete route | Must not define the new BIR / prepared backend contract; use only as a warning about rejected direct text emission. | Owner: legacy public entry shim. Limitation: mixed BIR/LIR recognition with assembly output. Removal condition: Step 4 contract rejects rendered-name and direct text fallback routes. |
| `codegen/alu.md` | salvageable design note | May inform later integer operation coverage, not entry contract shape. | Owner: legacy ALU lowering. Limitation: old register and text patterns lack structured target MIR. Removal condition: replaced by instruction-selection coverage docs/tests. |
| `codegen/asm_emitter.md` | obsolete route | Must not influence the new backend contract; direct assembly rendering is excluded. | Owner: legacy assembly text emitter. Limitation: text output route bypasses structured backend state. Removal condition: target-local printer or assembler layer is explicitly designed. |
| `codegen/atomics.md` | target-ABI candidate | ABI and ordering facts may be reviewed later; old fixed-register route must not shape the entry contract. | Owner: legacy atomic lowering. Limitation: register conventions and LL/SC loops are unproven in current backend. Removal condition: atomic lowering is redesigned over structured MIR. |
| `codegen/calls.md` | target-ABI candidate | May inform AAPCS64 call facts after Step 4 chooses the accepted input type. | Owner: legacy call lowering. Limitation: old stack/register decisions are descriptive only. Removal condition: current target ABI ledger owns call lowering facts. |
| `codegen/cast_ops.md` | salvageable design note | May inform operation inventory only; does not define semantic identity or type facts. | Owner: legacy cast lowering. Limitation: pattern list depends on removed emitter state. Removal condition: covered by structured conversion lowering docs/tests. |
| `codegen/comparison.md` | salvageable design note | May inform compare/branch coverage, not the backend entry contract. | Owner: legacy comparison lowering. Limitation: fused branch behavior was tied to old emitter flow. Removal condition: target MIR branch/condition model replaces it. |
| `codegen/f128.md` | delete/defer | Must not influence current contract unless binary128 support is explicitly reopened. | Owner: legacy f128 lowering. Limitation: niche route with no current proof. Removal condition: remove or replace when f128 support policy is decided. |
| `codegen/float_ops.md` | salvageable design note | May inform floating operation inventory after structured type facts are available. | Owner: legacy FP lowering. Limitation: old text patterns and binary128 behavior are not current proof. Removal condition: floating lowering is re-specified over target MIR. |
| `codegen/globals.md` | target-ABI candidate | May inform relocation/addressing needs, not raw entry contract selection. | Owner: legacy global addressing. Limitation: old symbol/addressing assumptions may belong in binary utils or target ABI. Removal condition: new symbol and relocation ledger replaces it. |
| `codegen/i128_ops.md` | delete/defer | Must not influence Step 4 except as a deferred support-risk note. | Owner: legacy i128 lowering. Limitation: special-width lowering lacks current backend proof. Removal condition: remove or replace when i128 lowering is designed. |
| `codegen/inline_asm.md` | obsolete route | Must not define semantic fallback, register naming, or string recovery behavior. | Owner: legacy inline asm lowering. Limitation: text substitution route conflicts with structured backend contract. Removal condition: inline asm contract is explicitly redesigned or deferred. |
| `codegen/intrinsics.md` | salvageable design note | May inform future intrinsic inventory; must not expand Step 4 entry requirements. | Owner: legacy intrinsic lowering. Limitation: broad legacy inventory without current capability proof. Removal condition: intrinsic lowering table is rebuilt from current frontend/BIR facts. |
| `codegen/memory.md` | salvageable design note | May inform load/store coverage, not legacy register ownership or text emission. | Owner: legacy memory lowering. Limitation: centered on `ArmCodegen` helpers. Removal condition: target MIR memory ops and addressing modes are specified. |
| `codegen/peephole.md` | delete/defer | Must not influence the backend entry contract; optimization belongs after correct lowering. | Owner: legacy peephole pass. Limitation: line-oriented text rewrites are stale. Removal condition: replaced by structured MIR or assembler optimization policy. |
| `codegen/prologue.md` | target-ABI candidate | May inform stack-frame and callee-save facts after input contract selection. | Owner: legacy prologue/epilogue lowering. Limitation: old frame layout is descriptive only. Removal condition: target ABI frame contract replaces it. |
| `codegen/returns.md` | target-ABI candidate | May inform return-value ABI facts after Step 4. | Owner: legacy return lowering. Limitation: old register behavior is not current proof. Removal condition: target ABI return ledger replaces it. |
| `codegen/variadic.md` | target-ABI candidate | May inform AAPCS64 variadic handling; not needed to choose the entry type. | Owner: legacy variadic lowering. Limitation: support policy and current BIR facts are unresolved. Removal condition: variadic ABI contract is accepted or feature is deferred. |
| `assembler/mod.md` | assembler/linker candidate | Must not make assembler text the backend contract; it is an external-input or printer-consumer lane only. | Owner: legacy assembler lane. Limitation: local parser/literal-pool route is removed. Removal condition: assembler layer is rebuilt or delegated to external tooling. |
| `assembler/parser.md` | assembler/linker candidate | Must not substitute parsed assembly operands for structured BIR, target MIR, or machine-node facts. | Owner: legacy assembler parser. Limitation: incomplete local grammar. Removal condition: parser route is deleted or replaced by an explicit assembler contract. |
| `assembler/elf_writer.md` | binary-utils candidate | May inform ELF object writing after structured nodes/encoding records exist, not backend entry type. | Owner: legacy assembler ELF writer. Limitation: object emission was tied to local assembler staging. Removal condition: shared ELF writer contract replaces it. |
| `assembler/encoder/mod.md` | assembler/linker candidate | Must not imply instruction encoding is available to codegen yet. | Owner: legacy encoder module. Limitation: helper surface depends on removed parser operands. Removal condition: encoder API is rebuilt around machine instruction nodes or lower structured encoding records. |
| `assembler/encoder/bitfield.md` | assembler/linker candidate | Encoding facts may be reused later only behind structured node or encoding-record input. | Owner: legacy bitfield encoder. Limitation: parser-shaped operands. Removal condition: structured bitfield instruction coverage replaces it. |
| `assembler/encoder/compare_branch.md` | assembler/linker candidate | Encoding facts may be reused later only behind structured node or encoding-record input. | Owner: legacy compare/branch encoder. Limitation: alias and condition handling tied to old parser. Removal condition: structured branch instruction coverage replaces it. |
| `assembler/encoder/data_processing.md` | assembler/linker candidate | Encoding facts may be reused later only behind structured node or encoding-record input. | Owner: legacy data-processing encoder. Limitation: parser operand model. Removal condition: structured data-processing instruction coverage replaces it. |
| `assembler/encoder/fp_scalar.md` | assembler/linker candidate | Encoding facts may be reused later only behind structured node or encoding-record input. | Owner: legacy FP scalar encoder. Limitation: old register classes and parser operands. Removal condition: structured FP instruction coverage replaces it. |
| `assembler/encoder/load_store.md` | assembler/linker candidate | Encoding facts may be reused later only behind structured node or encoding-record input. | Owner: legacy load/store encoder. Limitation: parser operand model. Removal condition: structured load/store instruction coverage replaces it. |
| `assembler/encoder/neon.md` | delete/defer | Must not expand the current contract into NEON support. | Owner: obsolete NEON encoder surface. Limitation: broad specialized instruction set with no current proof. Removal condition: NEON support is explicitly prioritized and redesigned. |
| `assembler/encoder/system.md` | assembler/linker candidate | Encoding facts may be reused later only behind structured node or encoding-record input. | Owner: legacy system encoder. Limitation: parser-shaped operands and old system-register handling. Removal condition: structured system instruction coverage replaces it. |
| `linker/mod.md` | obsolete route | Must not define backend output or imply built-in linking is required. | Owner: legacy linker module. Limitation: removed module wiring and shared infrastructure assumptions. Removal condition: linker work is split behind binary-utils/toolchain contracts. |
| `linker/elf.md` | binary-utils candidate | May inform ELF constants and relocation names only. | Owner: legacy linker ELF surface. Limitation: shared constants need validation against current binary utilities. Removal condition: centralized ELF definitions replace it. |
| `linker/emit_dynamic.md` | assembler/linker candidate | Must not influence Step 4 backend input/output contract. | Owner: legacy dynamic linker emission. Limitation: full dynamic link image construction is out of backend-entry scope. Removal condition: dynamic linker plan accepts or deletes the route. |
| `linker/emit_shared.md` | assembler/linker candidate | Must not influence Step 4 backend input/output contract. | Owner: legacy shared-library emission. Limitation: GNU hash and dynamic symbol policy are linker-layer concerns. Removal condition: shared-library linker plan accepts or deletes the route. |
| `linker/emit_static.md` | assembler/linker candidate | Must not imply the backend must emit final static executables. | Owner: legacy first static executable emission. Limitation: executable layout belongs outside core codegen contract. Removal condition: first-static linker contract replaces it. |
| `linker/input.md` | assembler/linker candidate | Must not influence backend semantic identity or input type. | Owner: legacy linker input loading. Limitation: file and symbol loading are linker-layer concerns. Removal condition: linker input contract replaces it. |
| `linker/link.md` | assembler/linker candidate | Must not influence backend semantic identity or input type. | Owner: legacy static link orchestration. Limitation: orchestration route is separate from codegen. Removal condition: static-link plan replaces or deletes it. |
| `linker/plt_got.md` | assembler/linker candidate | Must not influence Step 4 backend input contract. | Owner: legacy PLT/GOT construction. Limitation: dynamic relocation support is linker-layer work. Removal condition: dynamic-linker plan accepts or deletes it. |
| `linker/reloc.md` | binary-utils candidate | May inform relocation families after validation, not BIR entry shape. | Owner: legacy static text relocation. Limitation: relocation application belongs to binary utilities/linker. Removal condition: shared relocation contract replaces it. |
| `linker/types.md` | binary-utils candidate | May inform symbol and relocation data structures after validation. | Owner: legacy linker types. Limitation: old type surface may not match current binary utilities. Removal condition: shared symbol/relocation model replaces it. |

## Step 4 Inputs

For Step 4, the only positive inputs from this index should be:

- Current structured backend interfaces and `PreparedBirModule` facts from live
  code.
- Target-ABI candidate notes for calls, returns, frames, globals, atomics, and
  variadic behavior, treated as hypotheses to verify.
- Binary-utils candidates only when deciding whether codegen output requires a
  later object/relocation boundary.
- Shared prepared memory carrier fields `PreparedMemoryAccess::address_space`
  and `PreparedMemoryAccess::is_volatile`, treated as the only accepted source
  for address-space and volatility facts before target-local memory operands
  exist.

Everything marked `obsolete route` or `delete/defer` is excluded from the new
backend entry contract unless a later plan explicitly reopens it.
