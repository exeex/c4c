# LIR-to-BIR Carrier Bootstrap Runbook

Status: Active
Source Idea: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Supersedes: the paused idea-730 globals packet; its WIP remains in `stash@{0}`

## Purpose

Finish the smallest usable LIR-to-BIR boundary before resuming allocation or
MIR architecture work.

## Goal

Make the current LIR importer publish verified, target-independent Raw and
Canonical BIR whose structs can carry the imported abstract semantic nodes,
including opaque inline asm.

## Core Rule

This runbook ends at verified Canonical BIR. Copy source-semantic LIR facts
into typed BIR carriers without interpreting target constraints, assigning
registers, inserting spills, or designing MIR.

## Read First

- `ideas/open/731_inline_asm_transport_and_regalloc_contract.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/core/ids.hpp`
- `src/backend/bir/core/ir.hpp`
- `src/backend/bir/core/builder.hpp`
- `src/backend/bir/core/view.hpp`
- `src/backend/bir/verify/verifier.hpp`
- `src/backend/bir/lir_to_bir.hpp`
- `src/backend/bir/lir_to_bir.cpp`
- `src/codegen/lir/ir.hpp`
- `tests/backend/bir/backend_lir_to_bir_interface_test.cpp`

## Current Targets

- stable abstract BIR node and operand/result carriers required by the direct
  importer
- builder and read-only view support for those carriers
- verifier-issued `RawBir` and `CanonicalBir` publication
- lossless `InlineAsm` carriage of opaque payload, ordered structured
  operands, original constraint tokens/text available at the LIR boundary,
  and clobber syntax
- direct LIR-to-BIR interface tests only

## Non-Goals

- Do not edit or depend on `docs/**`, architecture READMEs, or transient
  `review/**` reports for this packet.
- Do not implement register allocation, target pool budgets, ABI register
  mapping, `MirReadyBirView`, MIR, spill/reload insertion, or late assembly.
- Do not parse or normalize inline-asm mnemonics, templates, constraints,
  clobbers, register classes, or target metadata.
- Do not add target opcodes, concrete registers, frame offsets, or target
  profiles to Raw/Canonical BIR.
- Do not restore legacy/prealloc/old-MIR sources or apply `stash@{0}`.
- Do not expand the accepted test surface beyond the direct
  `backend_lir_to_bir_interface` target.

## Working Model

1. `ModuleBuilder` constructs unpublished typed BIR storage with stable IDs.
2. The Raw verifier validates ownership, ordering, operands/results, CFG, and
   carrier shape before it can issue `RawBir`.
3. A minimal Canonical publication gate verifies and exposes the same
   target-independent semantic graph as `CanonicalBir`; it does not allocate
   registers or rewrite nodes into machine form.
4. `InlineAsm` is one abstract instruction node. Its payload stays opaque;
   ordered operands and source constraint/clobber syntax stay typed and
   separately inspectable.
5. A later allocation attachment may have a typed reserved seam, but
   Raw/Canonical publication requires it to be absent/unassigned.

## Execution Rules

- Implement only node kinds that the bounded importer tests exercise plus the
  `InlineAsm` carrier required by the active idea. Do not attempt a complete
  compiler-wide opcode catalog.
- Keep instruction payloads in a closed typed variant; do not introduce a
  stringly target opcode escape hatch.
- Preserve source operand ordinal, result index, input value, output value,
  and destination identity as separate fields wherever the available LIR
  facts distinguish them. Do not use allocation ties to collapse SSA identity.
- Copy inline-asm payload and constraint/clobber syntax byte-for-byte from the
  LIR carrier. Unsupported or insufficiently structured LIR forms must return
  a structured importer error and publish no partial BIR.
- If an allocation seam is added, keep it target-neutral and verifier-enforced
  as absent in Raw/Canonical BIR. Do not invent incomplete `Spill`/`Reload`
  nodes in this packet.
- Keep publication transactional: a build/import/verification error produces
  no `RawBir` or `CanonicalBir` token.
- Use only the supervisor-delegated build command and direct interface-test
  command as proof.

## Step 1: Implement the verified LIR-to-BIR carrier boundary

Goal: make one direct LIR module import into inspectable, verified Raw and
Canonical BIR without target interpretation.

Primary targets:

- `src/backend/bir/core/{ir,builder,view}.{hpp,cpp}`
- `src/backend/bir/verify/verifier.{hpp,cpp}`
- `src/backend/bir/{bir,lir_to_bir}.hpp`
- `src/backend/bir/lir_to_bir.cpp`
- only the minimum LIR carrier field additions that direct import requires
- `tests/backend/bir/backend_lir_to_bir_interface_test.cpp`

Concrete actions:

- Replace the empty bootstrap opcode/unsupported-instruction placeholder with
  the smallest closed abstract semantic-node variant required by the direct
  importer, including `InlineAsm`.
- Give instruction views typed access to opcode/payload, ordered operands,
  ordered results, and stable `InstId`/`ValueId` ownership.
- Extend builders and verification so malformed ownership, arity, result
  definitions, CFG references, or inline-asm carrier shape fail before
  publication.
- Connect current LIR instructions in the admitted subset to those builders;
  retain structured rejection for unsupported LIR rather than silently
  dropping it.
- Carry inline-asm payload opaquely and keep ordered structured operands plus
  original constraint/clobber syntax separately inspectable. Do not carry or
  derive early parsed instruction metadata as BIR authority.
- Add the minimal verifier-issued `CanonicalBir` stage/view over verified
  target-independent storage. It must not be a second rewritten graph and must
  not contain allocation state.
- Add only a reserved typed allocation attachment seam if compilation or API
  shape requires it; assert in verification/tests that Raw and Canonical
  publication leave it absent.
- Extend the direct interface test with positive view assertions and negative
  transactional rejection for the admitted ordinary-node and inline-asm
  carrier shapes.

Completion check:

- The supervisor-selected backend-enabled build succeeds.
- `backend_lir_to_bir_interface` passes and directly proves stable views,
  admitted abstract nodes, opaque inline-asm payload equality, separate
  constraint/clobber carriage, no target/allocation facts, and no publication
  on malformed or unsupported input.
- No docs, broad backend tests, regalloc, spill/reload, MIR, target-capacity,
  or legacy files are required for acceptance.

## Deferred After This Runbook

The source idea remains open after Step 1. Target preparation, abstract
physical-register categories and capacities, allocation, spill/reload,
MIR-ready publication, target calling-convention mapping, and late assembly
must receive a later reviewed runbook. They are not blockers for this
LIR-to-BIR bootstrap and are not authorized here.
