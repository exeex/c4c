# LIR Native Vaarg Operand/Result Seam Decomposition

Status: Open
Type: focused frontend-LIR decomposition blocker
Blocked Parent: `ideas/open/782_lir_vaarg_phi_input_result_identity_publication.md`
Downstream Consumer: `ideas/open/751_lir_phi_incoming_value_and_predecessor_identity.md`

## Goal

Establish and bind the narrowest viable native, current-function operand/result
contracts for the three vaarg helper chains that feed 782, before any helper
field implementation is attempted.

## Why This Exists

782 Step 1 reached `LirGepOp.ptr`: publishing authority for the AArch64
`reg_addr` helper GEP needs an authoritative raw SSA base (`gr_top`), which
recursively exposes the generic vaarg expression/operand chain. Earlier 751
also could not give its PHI carrier these IDs without forbidden text recovery.
The first bad fact has moved upstream across two PHI-facing routes without
shrinking the failure family. This initiative changes the execution shape into
three independently observable frontend-LIR seams rather than retrying either
PHI-facing route.

## In Scope

- Establish a matching baseline for focused frontend-LIR structural probes.
- Enumerate and bind one native operand/result contract for each seam:
  - AArch64 GP propagation from the authoritative `gr_top`-family input into
    the `reg_addr` GEP address path.
  - AArch64 FP/alignment helper operands, including the chain that yields the
    aligned helper values used by the FP path.
  - AMD64 register and stack helper result paths that feed its vaarg join.
- Add or refine focused frontend-LIR structural probes with one primary
  contract per seam, and record the exact producer/consumer boundary each
  probe proves.
- Accept one narrowest viable native vaarg operand/result contract covering all
  three chains, then return control to 782 Step 1.

## Probe Location

The default divide-and-conquer location `tests/backend/case/` is inapplicable:
these are frontend LIR construction and structural-authority seams, not backend
lowering or emitted-code behavior. Place the focused probes in the repository's
existing frontend-LIR test location selected by the initial inventory; retain
any larger vaarg case only as integration context, not as the primary probe.

## Out Of Scope

- Implementing 782's PHI-helper input fields before the three contracts are
  accepted.
- `LirPhiOp`, its carrier or verifier, predecessor/edge authority, and any
  claim of PHI completion.
- Raw-BIR/importer, backend, target lowering, MIR, emission, generic migration,
  or a broad generic-expression API redesign.
- Recovering identity from names, labels, rendered text, instruction order, or
  testcase text; side tables and result-name maps are excluded.

## Acceptance Criteria

- A baseline command and matching focused frontend-LIR probe set are recorded
  before implementation work begins.
- Each of the AArch64 GP, AArch64 FP/alignment, and AMD64 reg/stack chains has
  one independently named structural seam and focused probe contract.
- The accepted contract states the authoritative source, required propagation,
  and consumer boundary for every chain without relying on textual recovery.
- The outcome explicitly identifies whether 782 Step 1 can resume; it does not
  claim generic migration, PHI-carrier completion, or backend progress.

## Reviewer Reject Signals

- Reject a retry of the old PHI-carrier or PHI-verification route presented as
  decomposition progress.
- Reject a GP-only, FP-only, or AMD64-only probe/contract claimed to unblock
  the entire vaarg family.
- Reject backend `tests/backend/case/` probes or rendered-text assertions as
  proof of a frontend-LIR structural contract.
- Reject names, labels, printer output, instruction order, testcase-specific
  branches, side tables, or result-name maps as authority sources.
- Reject Raw-BIR/importer, backend, target lowering, MIR, emission, generic
  migration, expectation downgrades, or helper-only renames claimed as this
  capability's progress.
- Reject retaining the `LirGepOp.ptr` raw-base failure behind a renamed helper
  abstraction rather than binding its authoritative operand seam.
