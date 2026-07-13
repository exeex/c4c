# Current Packet

Status: Active
Source Idea Path: ideas/open/741_lir_structured_operand_and_terminator_identity_decomposition.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement the narrowest generic carrier

## Just Finished

- Plan Step 4 binds each focused probe to exactly one contract row in
  `docs/lir_structured_identity/carrier_contract.md`.
- The selected design extends `LirOperand` with one closed authority variant
  over `LirValueId`, `LinkNameId`, and native integer immediate payload. It
  keeps display text non-authoritative, relocates/re-exports the single existing
  `LirValueId` definition through a shared LIR model header, uses the existing
  per-function value allocator, and introduces no identity mirrors.
- The contract records exact producer population seams, current-function and
  module-global ownership, reachable LIR-verifier obligations, nearby positive
  and malformed/conflicting rejection coverage, and the later importer
  boundary for store, load, GEP, and return.
- It now binds authority through `AssignableLValue.ptr`, the focused
  rval/set/store transport, and coordinator result transport. `fresh_value`
  obtains authority from the current function shell and display separately.

## Suggested Next

- Execute Plan Step 5 in the documented dependency order: generic carrier and
  function-owner foundation, then the bounded store, load, GEP, and return
  packets with their focused and neighboring verifier proof.

## Watchouts

- `StmtEmitter::fresh_tmp` is presentation only and shares its counter with
  labels. Step 5 must allocate semantic result IDs through the eventual
  `LirFunction::alloc_value()` owner via `fresh_value`, not a new `FnCtx`
  counter or an ID inferred from `tmp_idx`.
- Populate global authority from the already selected `GlobalVar` and integer
  authority while the HIR literal is native. Never parse `%tN`, `@name`, or
  typed index/return strings, and do not expand into remaining `C-gap` rows.
- A valid `LinkNameId` owned by another global denotes that global regardless
  of display. Reject only invalid/unresolved, non-global, ownerless, or
  ambiguously owned IDs; canonical display parity belongs to producer/printer
  tests, not verifier identity.

## Proof

- Documentation-only Step 4 required no build or test run.
- `git diff --check` passed, and each of the four focused authority-matrix rows
  links exactly once to its matching carrier-contract row. No root proof log
  was written.
