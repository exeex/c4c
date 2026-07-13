Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Freeze inventory, constraint table, and schema checkpoint

# Current Packet

## Just Finished

- Plan Step 1 review repair completed: the checkpoint now freezes SSA-correct
  ordered multi-output results/writebacks, distinct `UseDef` incoming and
  produced identities, numeric assignment ties, and atomic BIR construction
  without caller-supplied result IDs.
- `LirModule::target_profile` is now the normalization authority; BIR retains
  only target-arch/constraint-contract provenance and BIR-to-MIR requires a
  matching external target before physical-clobber resolution.

## Suggested Next

- Re-review `docs/inline_asm_transport/` against
  `review/inline_asm_checkpoint_review.md`; do not authorize or begin Step 2
  until the repaired Step 1 checkpoint is accepted.

## Watchouts

- Active HIR currently rewrites `%N` placeholders and parses `.insn r`; active
  HIR-to-LIR rewrites/escapes the only payload and erases `+` into LLVM
  output-plus-tie spelling. Step 2 must introduce separate opaque/source and
  compatibility carriers rather than blessing those rendered strings.
- Explicit blockers remain: named operands are not retained by the parser;
  `VRM1` has only quarantined alias evidence; fixed-register, memory,
  immediate, alternatives, combined constraints, non-RV64 rules, and exact
  reserved-register pools require separate reviewed tables.
- Multi-output work must preserve source operand ordinal, output ordinal, and
  produced result index as separate coordinates.
- BIR provenance is only an interpretation guard; register pools, reserved
  sets, names, and candidate order stay external.
- Do not apply `stash@{0}` or revive legacy BIR, prealloc, MIR, or `c4c-as`.

## Proof

- Docs-only packet: `git diff --check` passed.
- Evidence-path/symbol audit used AST-backed function inventories for
  `src/codegen/lir/hir_to_lir/stmt.cpp`, `src/codegen/lir/verify.cpp`,
  `src/codegen/lir/lir_printer.cpp`, and active
  `src/backend/bir/lir_to_bir.cpp`, plus targeted source/reference reading.
- Review-repair cross-reference audit covers every blocking/high and medium
  finding in `review/inline_asm_checkpoint_review.md`.
- No build/test command or `test_after.log` was required by this docs-only
  packet.
