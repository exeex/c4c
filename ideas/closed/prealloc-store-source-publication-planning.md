# Prealloc Store-Source Publication Planning

## Intent

Move store-source publication planning, recovered-source lookup, stack-object
identity resolution, and pointer-store writeback intent into prealloc while
leaving target-specific address modes and store instructions in target codegen.

## Why This Exists

AArch64 store-source lowering still owns source-provenance and Prepared-home
reasoning that is not specific to AArch64 instruction spelling. x86 and RISC-V
stores need the same logical facts for local/global stores, narrow-store to
wide-load recovery, pending publication, and pointer-store writeback, even
though each target has different memory operand legality and store mnemonics.

This idea is intentionally narrower than full store lowering.

## In Scope

- Add a prealloc-owned store-source planning record for logical source,
  destination, recovered wide-load source, stack-object identity, and pending
  publication requirements.
- Move target-independent source recovery and pointer-store writeback intent
  out of AArch64 codegen.
- Keep AArch64 lowering responsible for legal memory operands, load/store
  mnemonics, scratch use, and final machine instruction construction.
- Preserve existing AArch64 behavior for local stores, global stores,
  narrow-store to wide-load recovery, and pointer-store writeback paths touched
  by the migration.
- Add an x86 or RISC-V fixture or adapter that can inspect the same store plan
  without inheriting AArch64 addressing rules.

## Out Of Scope

- AArch64 instruction spelling, ABI lane policy, stack-pointer sequences,
  memory operand spelling, and final machine instruction construction.
- Target-specific address-mode legality, store width opcode selection, scratch
  register selection, or global address materialization.
- Scalar publication planning unrelated to store-source requirements.
- Branch fusion, dynamic-stack operations, or call-boundary republication.

## Acceptance Criteria

- Store-source planning facts are produced before AArch64 final emission and
  are not encoded as AArch64 memory operands or instructions.
- AArch64 codegen consumes the plan through target-local address/emission
  handling.
- Existing AArch64 local/global store, recovered-source, and pointer-writeback
  behavior remains equivalent for touched paths.
- The plan data has an explicit x86 or RISC-V reuse path.

## Proof Expectations

- Build proof for affected C++ targets.
- Focused AArch64 store tests or fixtures covering narrow-store to wide-load
  recovery and pointer-store writeback behavior touched by the migration.
- One x86 or RISC-V compile/unit fixture proving the store plan can be consumed
  without AArch64 memory operand spelling.

## Reviewer Reject Signals

- The patch moves AArch64 memory operand spelling, load/store opcode selection,
  global address materialization, stack-pointer sequences, or final machine
  instruction construction into prealloc.
- The implementation special-cases one known store testcase or one named stack
  object instead of exposing a reusable store-source plan.
- Existing store expectations are weakened, marked unsupported, or rewritten to
  avoid recovered-source or pointer-writeback behavior.
- The new prealloc record is only a renamed AArch64 operand bundle.
- The x86/RISC-V reuse proof is absent or only theoretical.

## Closure Summary

Closed after Step 5 review. The implementation added the prealloc-owned
`PreparedStoreSourcePublicationPlan` API, routed touched AArch64 local store
publication, recovered-source, and pointer-store writeback paths through
target-local adapters over that neutral plan, and added the x86-labeled
`backend_x86_store_source_publication_plan_reuse` fixture to prove record-level
reuse without AArch64 memory operands or instruction records.

The remaining AArch64 address legality, store mnemonic, scratch register,
global materialization, stack-pointer sequence, and final instruction details
remain target-local as intended; no separate follow-up initiative was identified
for this source idea.

Close proof: `test_before.log` records the latest backend proof with `162/162`
backend tests passing. The close-time regression guard was run read-only against
that current no-code-change backend proof with non-decreasing results allowed:
`162/162` before and after, no new failures.
