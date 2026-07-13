# LIR-To-New-BIR Transport And Legacy-Coverage Handoff

Status: Closed
Closed: 2026-07-13
Historical Name: Inline-Assembly Transport And Regalloc Contract

## Authoritative Scope

Idea 731 owned one bounded migration seam from current typed LIR into the new
target-independent Raw/Canonical BIR. Its implementation packet proved only
that the structured non-goto inline-assembly path available in current LIR can
use the new BIR carrier, ordinary value model, builder/view interfaces, and
verifier without loss. It did not prove general legacy-BIR semantic coverage.

The following work was not part of idea 731:

- target preparation or constraint interpretation
- register allocation, spill/reload, or frame materialization
- allocated or MIR-ready publication
- MIR mapping, target emission, operand substitution, or late assembly
- general A1-F3 backend implementation
- expanding or modifying LIR opcode/metadata families

Earlier lifecycle edits expanded idea 731 into those downstream concerns. The
user's closure decision supersedes that expansion. General A1-F3 work is parked
in `ideas/draft/733_accepted_bir_a1_f3_architecture_implementation.md`; complete
LIR-to-new-BIR container/import coverage belongs to
`ideas/open/734_lir_to_new_bir_container_completeness.md`. Neither is a
dependency or acceptance condition for this closed bounded idea.

## Completed Work

- `ac2f344f2` added the target-independent Raw/Canonical `InlineAsmNode`
  carrier, generic ordinary operand/result edges, builder/view support,
  verifier coverage, and the bounded LIR-to-BIR importer seam.
- `0d55ee766` completed structured `LirInlineAsmOp` import: typed ordinary
  inputs/results, distinct incoming and produced identities for read/write
  operands, original opaque payload and constraints, ordered clobbers, and
  side-effect flags reach the new BIR without parsing printer text or target
  meaning.
- `24faa7516` recorded the matching full-suite acceptance proof with
  `3030/3030` tests passed.
- `bf3234f45` reconciled the scoped `core`, `lir_to_bir`, and `verify` README
  surfaces against the implementation.
- `d9558af0a` retired the completed five-step structured transport runbook and
  recorded that no accidental desynchronization remained in its owned README
  surfaces.

## Closure Decision

Closure is accepted for the authoritative idea-731 scope. The implemented path
proves only that the new BIR can receive the current structured inline-assembly
LIR carrier through ordinary SSA identities without reviving legacy structures
or inferring semantics from rendered strings.

The legacy-coverage documents identify many other typed opcode and metadata
families. Their `source gap` labels are assumptions to audit against the
existing complete LIR, not established LIR defects. Idea 734 owns the explicit
coverage matrix, missing new-BIR typed containers, and faithful importer paths;
they are not silently claimed complete here. Constraint binding, allocation,
MIR-ready publication, MIR/emission, and late parsing are also outside this
closure.

## Closure Proof

- Historical implementation acceptance: `24faa7516`, full suite
  `3030/3030` passed for the completed structured transport runbook.
- Close-time matching regression guard: `test_before.log` and
  `test_after.log`, both covering `backend_lir_to_bir_interface` plus the
  currently adjacent BIR foundation tests, pass `4/4` with no new failures.
- Close-time monotonic comparison passed with
  `--allow-non-decreasing-passed`, appropriate for this lifecycle-only archive
  change.

## Remaining Work Outside Idea 731

- Idea 734 owns remaining target-independent LIR-to-new-BIR container/schema
  and import completeness, including audit of the sole possible minimal LIR
  inline-asm constraint carrier. Inline-asm inputs/results remain ordinary SSA
  values; reviewed requirements attach only to their positions and roles, and
  asm text remains opaque and byte-exact.
- Draft idea 733 records deferred general A1-F3 implementation direction; it
  is not active implementation authority.
- Constraint interpretation, generic allocation integration, target
  preparation, projection machinery, and late inline-assembly parsing remain
  outside idea 734 and require separately authorized downstream work; none may
  be reopened implicitly under idea 731.
- General LIR-to-new-BIR opcode/metadata container expansion and importer
  wiring belong to idea 734 after the phase-A documentation seam is accepted
  through idea 732's docs-only lifecycle; LIR itself remains unchanged.
