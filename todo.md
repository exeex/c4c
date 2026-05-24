Status: Active
Source Idea Path: ideas/open/entry-formal-publication-planning-prealloc.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Validate Behavior And Anti-Overfit Coverage

# Current Packet

## Just Finished

Step 5 validated the formal-publication planning route and anti-overfit
coverage.

The backend subset passes with the prealloc helper coverage, AArch64 consumer
coverage, and x86 reuse coverage all enabled.

Anti-overfit coverage summary:

- The prealloc helper test covers multiple formal-publication forms and
  statuses, including register-home, stack-home, missing-home/missing-field,
  unsupported, and no-publication cases.
- AArch64 backend coverage passes with entry-formal dispatch consuming the
  shared formal-publication plans.
- The x86 prepared query test proves reuse of the shared planning helper
  without rewriting x86 prologue/lowering.
- No tests were weakened or reclassified.

## Suggested Next

The current runbook packet is validation-complete. Supervisor should decide
whether to close, review, or activate the next plan state.

## Watchouts

AArch64 ABI source selection, register spelling, incoming stack/frame policy,
scalar-state recording, inline asm/copy emission, diagnostics, and prologue
behavior remain target-local.

The prealloc formal-publication plan remains fact/status-only. It does not
construct target operands, choose target registers, compute target stack/frame
policy, or emit entry-copy/prologue code.

## Proof

Ran the delegated proof exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed, 154/154 backend tests. Proof log: `test_after.log`.
