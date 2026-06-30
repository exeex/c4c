# Prepared Inline-Asm Operand Home Carriers

Status: Open
Type: Implementation idea
Parent: `ideas/open/428_rv64_call_adjacent_scalar_inline_asm_materialization.md`
Source Evidence: `build/agent_state/428_step5_probe/probe_summary.tsv`
Owning Layer: Prepared inline-asm producer and carrier facts
Handoff Directory: `build/agent_state/428_step5_probe/`

## Goal

Make prepared inline-asm carriers publish coherent operand homes for scalar GPR
input/output and tied operand forms so target object lowering can consume them
without reconstructing missing metadata.

## Why This Exists

The exhausted RV64 call-adjacent runbook proved coherent scalar object-lowering
packets but could not close its representative inline-asm rows. Fresh Step 5
probes classified the remaining blockers as prepared producer/carrier facts:
`pr38533` reports repeated `missing_operand0_home`, while `pr45695` and
`pr49279` report `tied_input_output_home_mismatch`.

The RV64 lowering boundary should remain a consumer of complete prepared
inline-asm carriers. These rows need producer-side home publication or an
explicit unsupported producer diagnostic before more target lowering work is
valid.

## In Scope

- Audit how BIR inline-asm metadata, prepared value homes, and prepared
  inline-asm carrier construction represent scalar GPR register outputs,
  register inputs, and tied inputs.
- Repair producer/carrier logic so supported scalar GPR operands have stable
  operand homes and tied inputs agree with their output home.
- Preserve complete-carrier behavior already consumed by RV64 `.insn r` and
  `.insn d` object lowering.
- Add focused prepared-contract tests that prove `missing_operand0_home` and
  `tied_input_output_home_mismatch` are replaced by coherent carrier facts for
  supported scalar GPR forms.
- Re-probe representative rows such as `pr38533`, `pr45695`, and `pr49279`
  after focused tests pass.

## Out Of Scope

- RV64 target reconstruction of missing inline-asm operand homes.
- Filename-specific handling for `pr38533`, `pr45695`, `pr49279`, or any other
  torture row.
- Memory constraints such as `=*m`, address-selection operands, clobber-only
  `~{memory}` carriers, aggregate operands, vector operands, and F128 operands
  unless the first step proves they are required to keep scalar GPR carrier
  invariants coherent.
- Expectation, allowlist, unsupported-marker, runtime-comparison, or pass/fail
  accounting changes.
- Reworking aggregate ABI, pointer/address publication, or RV64 object
  emission paths that already fail closed on incomplete carriers.

## Acceptance Criteria

- Supported scalar GPR inline-asm register outputs and register inputs produce
  complete prepared carrier operand homes.
- Supported tied scalar GPR input/output operands publish one coherent home
  contract rather than `missing_operand0_home` or
  `tied_input_output_home_mismatch`.
- Unsupported memory, address, clobber-only, aggregate, vector, and F128
  shapes remain explicitly rejected or classified outside this idea.
- Focused prepared-contract tests and the supervisor-delegated backend proof
  pass.
- Representative probes show `pr38533`, `pr45695`, and `pr49279` no longer
  fail for the old prepared carrier fact names, or the idea records a more
  precise producer-owned unsupported reason.

## Reviewer Reject Signals

- Reject changes that make RV64 object lowering infer operand homes from
  source shape, raw BIR, representative filenames, or asm text instead of
  consuming prepared carrier facts.
- Reject testcase-shaped fixes keyed to `pr38533`, `pr45695`, `pr49279`, exact
  instruction indexes, or one narrow dump layout.
- Reject expectation rewrites, unsupported downgrades, allowlist edits, or
  pass/fail accounting changes as evidence of carrier progress.
- Reject helper renames, printer-only changes, or classification-only changes
  claimed as capability progress when the prepared carrier still reports
  `missing_operand0_home` or `tied_input_output_home_mismatch`.
- Reject broad rewrites of RV64 object emission, aggregate ABI, F128,
  pointer/address publication, or memory-constraint lowering outside this
  producer/carrier scope.
- Reject retaining the exact old failure mode behind a renamed diagnostic
  unless the route intentionally records that shape as out of scope with a
  more precise producer-owned unsupported fact.
