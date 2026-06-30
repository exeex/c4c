Status: Active
Source Idea Path: ideas/open/432_prepared_inline_asm_operand_home_carriers.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Inline-Asm Carrier Producer State

# Current Packet

## Just Finished

Lifecycle switch completed. The exhausted RV64 call-adjacent scalar and
inline-asm object-lowering runbook was parked, and this active plan now owns
the prepared inline-asm producer/carrier blockers surfaced by Step 5 probes.

## Suggested Next

Execute Step 1: audit `missing_operand0_home` and
`tied_input_output_home_mismatch` through BIR inline-asm metadata and prepared
carrier construction, then choose the first implementation packet.

## Watchouts

- Do not implement RV64 object-lowering inference for missing inline-asm homes.
- Do not key behavior to `pr38533`, `pr45695`, `pr49279`, or probe instruction
  indexes.
- Keep `=*m` memory constraints and clobber-only `~{memory}` carriers outside
  this scalar GPR producer/carrier plan unless the audit proves a shared
  invariant requires them.
- Do not change expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.

## Proof

Lifecycle-only transition. Required local check:

```sh
git diff --check
```
