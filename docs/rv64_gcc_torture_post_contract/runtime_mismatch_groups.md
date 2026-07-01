# RV64 Runtime Mismatch Owner Groups

Status: Step 4 recurring-owner grouping complete from Step 1-3 triage
evidence.

## Evidence Scope

This grouping is based on the runtime-only inventory in
`runtime_mismatch_inventory.md`, the reproduced representative set in
`runtime_mismatch_representatives.md`, and first-wrong-edge evidence in
`runtime_first_wrong_edge.md`.

The Step 1 inventory contains 40 runtime mismatch rows:

| Runtime mode | Inventory rows | Still-reproducing representatives examined in Step 3 |
| --- | ---: | --- |
| c4c aborts under qemu | 32 | `src/pr38533.c` |
| c4c segfaults under qemu | 7 | `src/20080506-2.c` |
| c4c exits 255 under qemu | 1 | `src/pr81503.c` |

Only the three Step 3 representatives have enough first-wrong-edge evidence
for owner grouping. The counts below therefore distinguish proven grouped
representatives from wider inventory buckets. Do not treat these groups as
owning every row in the same runtime-exit bucket without more evidence.

## Ranked Groups

| Rank | Group | Owning layer | Proven rows | Representative | Risk/readiness summary |
| ---: | --- | --- | ---: | --- | --- |
| 1 | Binary operand/value materialization after prepared BIR | RV64 lowering consumption of prepared value graph | 1 | `src/pr81503.c` | High correctness risk and high ordinary C relevance; implementation-ready because prepared BIR records the intended producer and the emitted RV64 uses wrong operands. |
| 2 | Frame-slot call-argument publication/materialization | Prepared call-argument publication feeding RV64 call lowering | 1 | `src/20080506-2.c` | High correctness risk and high ordinary C relevance; implementation-ready because prepared BIR explicitly records `missing_frame_slot_arg_publication=yes`. |
| 3 | Inline asm tied-output/result materialization | Inline asm carrier/result materialization before final RV64 behavior | 1 | `src/pr38533.c` | High correctness risk for inline asm users, but lower ordinary C relevance; implementation-ready as a bounded inline-asm carrier repair. |

No F128-owned runtime mismatch group was identified in the Step 3
representatives. If later runtime rows prove F128-owned, quarantine them as
low priority unless they block broader non-F128 work.

## Group 1: Binary Operand/Value Materialization After Prepared BIR

Representative: `src/pr81503.c`

Observed mode: c4c exits 255 under qemu; clang exits 0.

Known count:

- Proven grouped representatives: 1.
- Inventory bucket: 1 exit-255 runtime row, so this group currently accounts
  for the full known exit-255 bucket.

First wrong edge:

- Semantic BIR and prepared BIR preserve the intended value graph through the
  final store to global `c`.
- Prepared BIR records the store source as available and produced by a binary
  operation.
- Emitted RV64 later consumes the wrong operand values in `foo`, and `main`
  compares using registers that do not hold the loaded `c` value.

Owning-layer classification:

- Lowering/materialization owner after prepared BIR.
- Not an expected-output, runtime-comparison, or pass/fail-accounting issue.

Risk ranking rationale:

- Correctness risk: high. Wrong operand materialization can silently compute
  incorrect ordinary C expressions.
- Row count: singleton, but this singleton is the only known exit-255 runtime
  row.
- Ordinary C relevance: high. The evidence is about binary/comparison value
  materialization, not extensions or inline asm.
- Implementation readiness: high. The prepared value graph gives a concrete
  expected source for a targeted lowering-consumption fix.

Step 5 recommendation:

- Create a follow-up idea for RV64 value/operand materialization from prepared
  BIR, starting with `src/pr81503.c`.
- Include nearby ordinary C expression cases in validation before accepting
  the fix, so this does not become a named-case shortcut.

## Group 2: Frame-Slot Call-Argument Publication/Materialization

Representative: `src/20080506-2.c`

Observed mode: c4c segfaults under qemu; clang exits 0.

Known count:

- Proven grouped representatives: 1.
- Inventory bucket: 7 segfault runtime rows.
- Wider segfault ownership remains unclear; this group does not own
  `src/pr43008.c`, `src/zerolen-1.c`, or other segfault rows without separate
  first-wrong-edge evidence.

First wrong edge:

- Semantic BIR passes both local pointer values to `foo`.
- Prepared BIR records arg 1 as a frame-slot address and marks
  `missing_frame_slot_arg_publication=yes`.
- The linked c4c binary passes an uninitialized or wrong register for the
  second pointer argument, causing the later runtime fault.

Owning-layer classification:

- Producer/prepared-BIR call-argument publication owner, consumed by RV64 call
  lowering.
- Not a standalone RV64 load/store instruction bug based on this evidence.

Risk ranking rationale:

- Correctness risk: high. Missing pointer argument materialization can corrupt
  calls and produce memory faults.
- Row count: one proven representative inside a seven-row segfault bucket.
- Ordinary C relevance: high. Passing addresses of locals is ordinary C.
- Implementation readiness: high. The explicit prepared-BIR marker gives a
  direct repair target.

Step 5 recommendation:

- Create a follow-up idea for frame-slot call-argument publication into RV64
  call lowering, starting with `src/20080506-2.c`.
- Park the remaining segfault rows until each has first-wrong-edge evidence or
  until the frame-slot fix can be tested against them as non-owned probes.

## Group 3: Inline Asm Tied-Output/Result Materialization

Representative: `src/pr38533.c`

Observed mode: c4c aborts under qemu; clang exits 0.

Known count:

- Proven grouped representatives: 1.
- Inventory bucket: 32 abort runtime rows.
- Wider abort ownership remains unclear; this group does not own
  `src/pr83298.c` or the other abort rows without separate first-wrong-edge
  evidence.

First wrong edge:

- Semantic BIR lowers empty inline asm with constraint `=r,0` and then uses
  the returned value.
- Prepared BIR records an inline-asm register output tied to an input with a
  home value.
- Emitted RV64 moves from an uninitialized register into the result instead of
  materializing the tied input value into the output register.

Owning-layer classification:

- Inline asm carrier/result materialization owner before final runtime abort.
- Not a general abort lowering, branch lowering, or runtime runner issue based
  on this representative alone.

Risk ranking rationale:

- Correctness risk: high for supported inline asm because result registers can
  receive stale values.
- Row count: one proven representative inside a 32-row abort bucket.
- Ordinary C relevance: lower than the other groups because the evidence
  depends on inline asm constraints.
- Implementation readiness: medium-high. The first wrong edge is clear, but
  the repair should be scoped to real inline-asm tied-output semantics rather
  than a `pr38533.c` pattern.

Step 5 recommendation:

- Create or park as a follow-up idea depending on supervisor priority: a
  bounded inline-asm tied-output materialization repair beginning with
  `src/pr38533.c`.
- Keep the rest of the abort bucket unresolved until more representatives have
  first-wrong-edge evidence.

## Unclear Or Parked Runtime Rows

The remaining runtime mismatch rows are not assigned to an owner group by this
Step 4 evidence:

- Abort rows other than `src/pr38533.c`: 31 inventory rows remain unresolved.
- Segfault rows other than `src/20080506-2.c`: 6 inventory rows remain
  unresolved.
- `src/20000314-2.c`: stale or nondeterministic for this triage because Step 2
  reproduced it as passing after it appeared as an abort row in the full-scan
  inventory.

These rows should be parked or receive new first-wrong-edge probes in Step 5.
They should not be folded into one of the groups above based only on the final
runtime exit mode.

## Step 5 Handoff

Recommended follow-up order:

1. Create an ordinary-C RV64 operand/value materialization idea from
   `src/pr81503.c`.
2. Create an ordinary-C frame-slot call-argument publication idea from
   `src/20080506-2.c`.
3. Park or defer the inline-asm tied-output group unless inline asm support is
   the current priority.
4. Park unresolved abort and segfault rows until additional first-wrong-edge
   evidence exists.

No implementation fixes, expectation changes, allowlist changes, unsupported
markers, runtime comparison changes, or pass/fail-accounting changes were made
for this grouping.
