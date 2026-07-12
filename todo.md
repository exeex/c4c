Status: Active
Source Idea Path: ideas/open/715_pass_ready_bir_schema_and_legacy_quarantine_research.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Compare The Reference Backend

# Current Packet

## Just Finished

- Completed plan.md Step 4: traced the reference backend's ownership, IDs,
  values, dense CFG analysis, pass sharing, and phi elimination, then tied each
  adopted and rejected principle to the c4c contract.

## Suggested Next

- Execute plan.md Step 5 and produce the concrete target files, types, storage,
  APIs, and raw-to-canonical-to-prepared/MIR stage blueprint.

## Watchouts

- Adopt the reference's terminator-derived CFG and semantic-ID-to-dense-analysis
  conversion, but not its bare numeric IDs or positional mutation interfaces.
- Reference CFG reuse depends on manual pass knowledge; c4c still requires
  revision-bound analysis results and declared preservation/invalidation.

## Proof

- Supervisor-selected documentation proof passed:
  `git diff --check && test -f docs/backend/pass_ready_bir/04_reference_backend_comparison.md && rg -n "IrModule|IrFunction|BasicBlock|BlockId|Value|FlatAdj|CfgAnalysis|phi|adopt|reject|InstId|invalidation" docs/backend/pass_ready_bir/04_reference_backend_comparison.md`.
- This documentation-only packet does not produce `test_after.log`; no build or
  test subset was delegated.
