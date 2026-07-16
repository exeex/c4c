# 812 closure trace

Evidence revision: `d58b8d44c9b64d2005d2b3760a0592b1b47ebd03` (the
revision recorded by the committed 812 baseline and matrix at `b9f248d01`).

## Disposition reconciliation

The Step 2 matrix has 29 singularly classified rows:

| Disposition | Count |
| --- | ---: |
| `structured-authoritative` | 2 |
| `checked mirror` | 1 |
| `existing open owner` | 19 |
| `insufficient evidence` | 3 |
| `unowned semantic route` | 0 |
| `intentional opaque/render text` | 4 |

The qualifying handoff set is exactly the three `insufficient evidence` rows:
`global.policy-identity-evidence`, `instruction.intrinsic-binding-evidence`,
and `cfg.phi-raw-bindings-evidence`.  An omissions check against every matrix
row found no omitted qualifying row and no included `existing open owner` or
intentional-text row.

## Credited bounded closures

The following are credited only for the exact matrix rows stated in
[closure reconciliation](closure_reconciliation.md): 759 (builtin enum/id),
760 (closed-set literals and named dynamic boundaries), 761 (structured call
mirrors), 762 (module declaration/type shadows), 763 (selected type facts),
754 (five aggregate/vector operation rows), 811 (vector carrier), 814 (poison
shape), 815 (mask coherence), 832 (one aggregate-owner crash), 833 (direct
scalar truthiness), 834 (focused aggregate canonicalization), and 835 (HIR
aggregate-owner carrier).  No family-wide closure is inferred from these
credits.

## Current owners and return chains

Existing owners remain exactly as recorded in
[existing_owner_map.md](existing_owner_map.md): 734 retains typed BIR receiver
completeness, with 829 as its selected producer prerequisite; 795 returns its
bounded parameter publication to 810 Step 3; 796 returns selected forms to
801 Step 2 after a comparable baseline; and 797 awaits accepted producer
handoffs and 734 receipts.

The active call/aggregate chain remains 829 (blocked for native arg-1) -> 830
(parked at Step 3 by 831) -> 831 (its Step 4 awaits 836) -> 836 (parked at
Step 1 by the 837 priority switch), then returns 836 -> 831 Step 4 -> 830 Step
3 -> 829 Step 2.  821/822 remain separate selector routes.  838 is separate
module aggregate/store work and does not close that chain.  839 retains
nominal signature/call composition; 840 vector schema; 841 scalar/ABI leaves;
842 restricted value boundaries; 843 HIR/array producers; 844
global/extern/initializer facts; 845 reference collectors; 846
verifier/dispatch/printer; and 847 universal escape-hatch deletion after its
M1--M15 gates.  None is moved, completed, or assigned by this trace.

## Excluded intentional text families

All four intentional-text matrix rows remain excluded from semantic routing:

- String-pool/global/name/link-name display mirrors.
- Inline-assembly templates and constraints.
- Raw literal/data bytes and final global-initializer spelling.
- Final LLVM printer output, diagnostics, data-layout, and target spelling.

The intentional-text registry further names the constituent final-render,
link-name-presentation, display/mangled-name, and data-layout/target families
under these four rows.  Its compatibility-call-text entry is explicitly a
non-exemption: semantic parser and scanner consumers remain existing-owner
matrix rows, not excluded text.  All intentionally excluded text is forbidden
to become identity, lookup, type/owner recovery, verification, or dispatch
input.

## Scope trace

812 introduced no implementation, test, lifecycle, or successor change.  It
only records the evidence baseline, bounded credits, preserved ownership, the
matrix, and the exact unresolved evidence handoff.  The source and runbook are
therefore ready for a plan-owner evidence-only completion decision; this file
does not make that decision.
