# New BIR Architecture

Status: target architecture contract with a partial bootstrap implementation.
The current implementation provides the initial BIR carrier and verification
pieces; target legalization, pseudo-physical register allocation, the complete
MIR-ready verifier, and downstream lowering are not all implemented yet.

BIR is a staged backend IR family. `RawBir` and `CanonicalBir` preserve source
semantics and are deliberately unallocated. The target input authority is
`c4c::TargetProfile` in
[`src/target_profile.hpp`](../../target_profile.hpp): its triple, `TargetArch`,
`TargetOs`, `BackendAbiKind`, relocation model, and current ABI capability flags
identify the applicable target contract. BIR uses that profile to select and
derive its own pseudo-register-layout descriptor; the target does not inject a
ready-made register allocator or a bag of physical slots.

Once that BIR-owned layout and the inline-assembly constraint rules are
available, BIR legalization lowers the canonical body to a closed set of
pseudo-instruction nodes. The shared BIR register allocator then assigns finite
pseudo-physical homes and makes spill state explicit. This publishes the clean,
verified allocated BIR view consumed by MIR; it does not retroactively make
`CanonicalBir` target-dependent.

## Required stage order

```text
LIR
  -> lir_to_bir
  -> RawBir verification
  -> target-independent canonical BIR pass pipeline
  -> CanonicalBir verification
  -> c4c::TargetProfile selection
  -> BIR-owned pseudo-register-layout derivation
  -> target constraint facts
  -> BIR legalization to the admitted pseudo-instruction set
  -> BIR liveness and pseudo-physical register allocation
  -> explicit pseudo Spill/Reload insertion
  -> allocated-BIR verification
  -> clean MIR-ready BIR view
  -> MIR pseudo-instruction lowering
  -> verified MIR
  -> target assembler and emission
```

The MIR-ready view contains only admitted pseudo-instruction nodes, including
explicit `Spill` and `Reload`, plus the admitted `InlineAsm` node. Every live
operand and result is already assigned a legal pseudo-physical home, or its
movement through spill state is represented by explicit pseudo instructions.
The verifier rejects unresolved pressure, an illegal class/group assignment,
an unavailable pseudo slot, or any non-admitted node before MIR construction.

The derived pseudo-register-layout descriptor is a BIR fact keyed by the
selected `TargetProfile`. It owns the finite caller-saved, callee-saved, and
temporary pools; register classes; aliased and reserved units; group width,
alignment, and contiguity rules; and ABI eligibility needed by allocation.
These are derived BIR layout facts, not fields this architecture assumes already
exist in `TargetProfile`.

RV64, AArch64, and x86 all use the same BIR implementation for liveness,
allocation, eviction and pressure decisions, spill-slot management, explicit
`Spill`/`Reload` insertion, and allocated-BIR verification. Their differences
are selected layout data and rules derived from `TargetProfile`, not separate
target allocators. A pseudo home is not a target register spelling such as
`a0`, `x3`, or `rax`; it is a verified category, class/group, and slot
assignment that the target calling convention can map.

MIR is a downstream consumer of this allocated BIR view. It lowers each pseudo
instruction to one machine instruction or to an explicitly bounded expansion,
and maps pseudo-physical homes to concrete RV64, AArch64, or x86 registers
according to the already-derived layout and mapping contract. MIR and the
target backend cannot replace the shared BIR allocator, perform ordinary
register allocation, repair liveness, or introduce routine pressure-driven
spill/reload. Failure to map an already verified pseudo home is a boundary
verification or lowering error, not permission to reallocate it. The
authoritative MIR owner and filesystem location remain to be frozen outside
this README; this document does not assign MIR ownership to a BIR subdirectory.

Future target-specific optimization is allowed only as a separately reviewed,
explicit pseudo-BIR pass with declared analysis invalidation and a required
reverification gate. Such passes are deferred; they may not hide a
target-specific register manager, replace the shared allocation authority, or
bypass allocated-BIR verification.

## Inline assembly at this boundary

`InlineAsm` is one ordinary BIR instruction. Its inputs and outputs use the
same ordered generic operands and results as every other instruction, so SSA,
phi construction, CFG reasoning, liveness, and register allocation use the
normal BIR rules. Its assembly instruction text is an opaque payload and stays
opaque through BIR and MIR; only the target assembler parses or substitutes
that text.

The constraint contract is separate from the opaque instruction text. During
BIR allocation, target-aware constraint handling interprets forms such as
`r`, `=r`, `VR`, and `VRM2` against the instruction's ordered operands and
results, then assigns legal pseudo-physical slots/classes/groups. Explicit
clobbers are compiler contracts and participate in allocation. If a user writes
concrete register names such as `a0` or `a1` directly inside the opaque assembly
text without expressing the corresponding constraints or clobbers, the
compiler neither inspects nor reserves those names; any collision is the
user's responsibility.

## Design areas

- [`core/`](core/README.md): stable IDs, ownership, generic instruction
  operands/results, and the source-semantic BIR carrier.
- [`lir_to_bir/`](lir_to_bir/README.md): LIR import into `RawBir`.
- [`verify/`](verify/README.md): verifier gates for published BIR stages.
- [`passes/`](passes/README.md): target-independent canonicalization and the
  target-aware legalization/allocation stages described above.
- [`analysis/`](analysis/README.md): immutable, revision-bound, recomputable
  facts used by passes and allocation.
- [`preparation/`](preparation/README.md): target context and typed planning
  facts; these facts cannot replace the allocated BIR verifier gate.
- [`pipeline/`](pipeline/README.md): detailed stage and pass contracts. Where a
  subordinate scaffold still describes the older MIR-allocation boundary, it
  must be synchronized to this frozen cross-stage contract before acceptance.
- [`diagnostics/`](diagnostics/README.md): read-only diagnostics and rendering.
- [`compatibility/`](compatibility/README.md): temporary legacy quarantine.
- [`LEGACY_COVERAGE.md`](LEGACY_COVERAGE.md): migration ledger for every legacy
  backend capability family.
- [`REVIEW_TEMPLATE.md`](REVIEW_TEMPLATE.md): mandatory questions for repeated
  architecture review.

## Non-negotiable authority rules

1. Terminators are the only persistent CFG successor authority.
2. Stable IDs, not names, pointers, vector positions, or route numbers, are
   semantic identity.
3. SSA operands/results remain the value authority for ordinary instructions
   and `InlineAsm`; inline assembly does not create a second value system.
4. Analyses are revision-bound derived facts, never persistent semantic side
   tables.
5. `RawBir` and `CanonicalBir` are target-independent, source-semantic, and
   unallocated; target facts cannot leak backward into either stage.
6. Target-aware legalization and allocation must publish a new verified BIR
   stage rather than silently changing the meaning of an earlier stage token.
7. `TargetProfile` is target-input authority; the pseudo-register layout is a
   derived, profile-keyed BIR fact and is not supplied as target allocator state.
8. All supported targets share one BIR register-management implementation;
   target variation is explicit derived layout data and rules.
9. Register assignment and explicit spill/reload are authoritative allocated
   BIR facts. Concrete target register spellings, frame encoding, and target
   opcodes are not.
10. MIR may realize verified pseudo homes and pseudo instructions, but may not
   redo normal allocation or silently repair an invalid BIR input.
11. Target-specific optimization passes must be explicit, separately reviewed,
    declare invalidation, and re-enter verification; they cannot become hidden
    allocation authorities.
12. Opaque inline-assembly text is parsed only by the assembler; structured
   constraints and explicit clobbers are enforced earlier by BIR allocation.
13. Each published stage has one authoritative output and one verifier gate.

## Review state vocabulary

Every design document uses one of these states:

- `scaffold`: responsibility and boundaries exist, detailed behavior is open.
- `under-review`: legacy coverage and adjacent contracts are being checked.
- `accepted`: inputs, outputs, invariants, failure behavior, and legacy coverage
  have been reviewed together.
- `implemented`: allowed only for contracts backed by the corresponding code
  and proof; the root target architecture is not thereby claimed complete.
