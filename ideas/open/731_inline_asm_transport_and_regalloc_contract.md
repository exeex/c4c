# Inline-Assembly Transport And Allocation Integration

Status: Open
Type: inline-assembly feature and accepted-BIR integration
Depends On:
`ideas/open/733_accepted_bir_a1_f3_architecture_implementation.md`
Related Architecture History:
`ideas/open/732_bir_stage_document_convergence_umbrella.md`

## Intent

Carry inline-assembly payloads and original constraint authority unchanged
through frontend, LIR, and BIR; bind reviewed constraints to ordinary SSA
inputs/results; enforce them through the shared BIR allocation architecture;
and substitute and parse the payload only at late assembly.

Idea 731 is the feature/integration consumer. It does not own implementation of
the complete B1-F3 compiler pipeline.

## Lifecycle Scope Split

Historical idea-731 edits absorbed general A1-F3 architecture convergence and
implementation. Formal review found that scope materially separate from the
original inline-asm feature. The independently accepted architecture and all
generic landed/WIP implementation history now belong to idea 733.

This idea remains open. It depends on idea 733 for shared target layout,
preparation, constraint infrastructure, allocation, spill/reload, E4
publication, strict MIR mapping, and late consumer seams. Completion of idea
733 is not completion of this feature; idea 731 must still prove its own
payload, identities, constraint behavior, allocation integration, and late
parse timing.

## Completed Feature Checkpoints

- `ac2f344f2` established the bounded target-independent Raw/Canonical
  `InlineAsmNode` carrier.
- `0d55ee766` completed structured ordinary SSA-edge import.
- `24faa7516` recorded the structured-transport full-suite proof.
- `bf3234f45` reconciled the scoped transport documentation.
- The implemented non-goto path preserves ordinary inputs/results and distinct
  read/write identities; general LIR opcode import remains unrelated scope.

## In Scope

- preserve original asm template/instruction text, original constraint text,
  ordered clobbers, side-effect flags, and structured source identities
  independently of LLVM-compatible rendering
- keep inputs as ordinary SSA uses, outputs as ordinary results, and read/write
  operands as distinct incoming and produced identities
- integrate the accepted C7/C9 shared tables/binder with reviewed `r`, `=r`,
  `VR`, `VRM2`, `VRM4`, and `VRM8` forms; keep `VRM1` unsupported
- prove ties, register groups, alignment/contiguity, early-clobbers, clobbers,
  multi-output, and `UseDef` semantics through shared idea-733 allocation
- preserve opaque payload bytes through `MirReadyBirView` and strict machine
  mapping
- substitute allocated operands and parse/encode inline-asm text only at the
  late assembler seam
- direct transport, constraint, allocation-integration, late-failure, and
  smallest end-to-end feature tests

## Out Of Scope

- implementing the general A1-F3 pipeline, canonical passes, ordinary ABI/call
  planning, general pseudo lowering, shared allocator internals, frame system,
  or all-target machine mapping; those belong to idea 733
- restoring legacy BIR/prealloc/MIR/`c4c-as` sources
- parsing asm instruction text before late assembly
- inferring semantic values from `args_str` or other printer text
- general GCC/LLVM constraint compatibility beyond reviewed forms
- unrelated globals, object/linker/runtime, or broad backend bring-up
- target-specific allocation or MIR pressure repair

## Acceptance Criteria

- Original asm and constraint payloads remain authoritative and can be compared
  unchanged across every pre-assembly feature boundary.
- LLVM-compatible rendering never overwrites or reconstructs semantic payload,
  values, roles, or constraints.
- Ordinary input/result/read-write identities remain distinct through BIR,
  projection, allocation, and machine mapping; ties constrain homes without
  merging SSA identities.
- Reviewed scalar and vector forms, groups, ties, early-clobbers, and clobbers
  are enforced by structured C9/shared allocation facts, not raw-string scans.
- Every inline-asm allocatable identity reaching the machine boundary has a
  legal shared-BIR assignment or explicit supported spill state.
- The late assembler receives allocated substitutions and is the first stage
  to parse mnemonics, directives, placeholders, `.insn`, or encoding details.
- A negative case proves invalid payload survives all earlier stages and fails
  specifically at late assembly.
- `VRM1` and unreviewed alternatives fail with stable diagnostics.
- Feature tests include neighboring same-rule cases and the supervisor-selected
  broader regression; legacy sources remain excluded.

## Reviewer Reject Signals

- Reject any pre-assembler parsing of asm mnemonics, directives, placeholders,
  `.insn`, or encoding fields.
- Reject recovering operands/results from `args_str`, printer text, variable
  names, or a feature-specific value table instead of ordinary SSA identities.
- Reject collapsing incoming use, produced result, output destination, or
  multi-output identities because two operands have an allocation tie.
- Reject a raw-string scan or named spelling branch used as allocator meaning
  instead of verified shared C9 facts and general class/group rules.
- Reject accepting `VRM1`, guessing a group width/alignment, weakening an
  unsupported alternative, or dropping a clobber/early-clobber/tie.
- Reject a target-specific feature allocator, MIR pressure repair, or revival
  of legacy/prealloc/old-MIR/`c4c-as` code.
- Reject claiming idea-731 progress from generic idea-733 infrastructure alone
  without feature-specific interface and end-to-end proof.
- Reject testcase-shaped shortcuts, expectation downgrades, helper renames,
  diagnostic reclassification, or broad unrelated backend changes claimed as
  inline-asm capability.
