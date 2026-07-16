# Idea 803 Step 3: `asm goto` Instruction-Point SSA Snapshots

Status: converged Markdown design contract; implementation absent

## Authority split

[B3 CFG](../../../src/backend/bir/passes/cfg/README.md) is the sole topology
owner. It canonicalizes the typed optional fallthrough and zero-or-more label
successor slots, preserves duplicate occurrences, and rewrites exact edge keys
through critical-edge normalization. [CFG analysis](../../../src/backend/bir/analysis/cfg/README.md)
only observes those terminators.

[B4 SSA](../../../src/backend/bir/passes/ssa/README.md) is the sole value
visibility owner. It combines exact instruction-point observations from
[publication/value flow](../../../src/backend/bir/analysis/publication/README.md)
with the post-B3 occurrence set and publishes exactly one exact-revision
snapshot per registered asm-goto pair. The [verifier](../../../src/backend/bir/verify/README.md)
checks both authorities without repairing either. [D5](../../../src/backend/bir/passes/out_of_ssa/README.md)
later consumes exact phi occurrences; it does not reinterpret snapshots.

## Closed shape matrix

| Label targets | Fallthrough | Outputs / clobbers | Required disposition |
| --- | --- | --- | --- |
| zero | present | independently absent or present | one fallthrough occurrence; outputs are visible there; clobbers remain explicit facts |
| one | present | independently absent or present | distinct label and fallthrough entries; label sees pre-asm definitions, fallthrough sees declared outputs |
| multiple, including duplicate destinations | present | independently absent or present | one entry per exact role/index; no destination-based collapse |
| one or multiple | absent | independently absent or present | only label entries; outputs are not visible on any outgoing occurrence |
| zero | absent | any | invalid B3 topology; B4 publishes no snapshot |

Outputs never create topology. Clobbers never act as SSA definitions. Every
legal combination retains the instruction's complete ordinary input, output,
and clobber roles even when a particular edge cannot observe an output.

## Complete scenario

Block `B10` defines `v0`, then executes asm-goto instruction `N40`. `N40`
declares input `v0`, output `v1`, and a clobber. Its paired terminator has:

```text
EdgeKey{B10, GotoLabel, 0} -> B20
EdgeKey{B10, GotoLabel, 1} -> B20
EdgeKey{B10, Fallthrough, 0} -> B30
```

The two label occurrences are different even though both target `B20`. Their
snapshot maps select `v0` and exclude `v1`. The fallthrough map includes the
declared `v1`; an instruction after `N40` is not visible on either label edge.
If B3 splits either critical edge, its live rewritten `EdgeKey`, not `B20` or
the rendered label, keys the corresponding B4 entry.

At a join, B4 creates one phi incoming per exact live occurrence. The two
`B20` occurrences therefore remain two inputs and may both select the same
pre-asm value without being merged. D5 eventually emits one edge-local transfer
per incoming occurrence. Any missing duplicate, label-edge use of `v1`, use of
a later definition, stale revision, or block-end reconstruction rejects the
private candidate and publishes no B4 checkpoint or snapshot.

## Failure and invalidation closure

- B3 rejects malformed instruction/terminator pairing, mismatched label slots,
  an implicit layout fallthrough, an empty successor set, and incomplete edge
  rewriting. It publishes no partial topology.
- B4 rejects missing, duplicate, stale, or incomplete snapshots; wrong
  occurrence classification; output or later-definition leakage to a label;
  missing explicit fallthrough output visibility; and phi/snapshot mismatch.
- A topology edit invalidates CFG, dominance, publication flow, SSA, and every
  snapshot. An instruction-order, input, output, clobber, definition, use, or
  phi edit invalidates publication flow, snapshots, SSA, and later consumers.
- No pointer, dense index, label text, block name, rendered form, destination
  equality, or layout position may replace stable instruction identity plus
  exact successor occurrence identity.

All product and diagnostic names are prospective documentation vocabulary, not
claims of landed APIs, implementation, or new `NodeKind` entries.
