# Select-Edge Source Producer Move-Bundle Placement Authority

Status: Open
Type: Prepared producer/placement metadata idea
Parent: `ideas/closed/457_before_instruction_stack_to_register_move_materialization.md`
Source Evidence: `build/agent_state/457_step4_residual_disposition/`
Owning Layer: Prepared placement authority for select-edge source-producer
move bundles

## Goal

Publish explicit producer/prepared placement authority for before-instruction
register-destination move bundles that are tied to select-edge source
producers, so later RV64 consumers can distinguish safe same-block
materialization, edge-owned materialization, and suppression when
predecessor-edge publication already consumes the source producer.

## Why This Exists

Idea 457 found the `20010329-1` `block_index=4 instruction_index=2`
before-instruction register-destination bundle cannot be consumed safely from
current facts. The bundle has moves into `%t18`/`t0`, source producer
`%t18 = bir.ule ptr %t15, %t17`, select carrier `%t22`, and edge transfer
`logic.rhs.end.13 -> logic.end.14 incoming=%t18 destination=%t22`, but it does
not say whether it belongs at the join instruction, on the predecessor edge, or
should be suppressed because predecessor-edge publication already materializes
the source producer.

## In Scope

- Audit prepared move-bundle, select-chain, source-producer, and edge-transfer
  facts for the `20010329-1` target bundle.
- Define explicit placement authority for select-edge source-producer
  before-instruction register-destination bundles.
- Include one or more durable facts for same-block compare-operand setup,
  edge-owned materialization with predecessor/successor labels, or suppression
  when predecessor-edge publication already consumes the source producer.
- Expose placement/source-producer identity in prepared output or focused
  producer tests.
- Add fail-closed coverage for missing, ambiguous, mismatched, raw-inferred,
  unsafe same-block, and unrelated bundle cases.

## Out Of Scope

- RV64 lowering for register-destination move bundles before placement
  authority exists.
- Generic stack-to-register or register-to-register move lowering.
- Reopening explicit cast-dependency authority consumption closed by idea 456.
- Consuming `load_from_stack_slot missing_stack_freshness`.
- Generic stack-home branch operand/condition materialization, pointer-value
  provenance, local/global store publication, and generic instruction-side
  lowering.
- Inferring edge placement from raw BIR shape, value ids, block indexes,
  instruction indexes, filenames, function names, or one prepared dump.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, or `test_baseline.new.log` churn.

## Acceptance Criteria

- The target before-instruction register-destination bundle is classified with
  explicit placement authority or exact fail-closed reason.
- Prepared evidence can distinguish same-block materialization, edge-owned
  materialization with predecessor/successor identity, and suppression because
  predecessor-edge publication already consumes the source producer.
- Focused tests cover accepted placement facts and fail-closed missing,
  ambiguous, mismatched, raw-inferred, unsafe, and unrelated bundles.
- No RV64 consumer packet is selected until producer-owned placement authority
  exists and is tested.

## Reviewer Reject Signals

- Reject RV64 object-lowering changes that consume the target bundle before
  producer/prepared placement authority exists.
- Reject inferring predecessor/successor identity or suppression authority from
  value ids, block indexes, instruction indexes, raw BIR shape, or testcase
  layout.
- Reject broad generic move-bundle placement metadata unrelated to
  select-edge source producers.
- Reject testcase-shaped fixes keyed to `20010329-1`, `logic.rhs.end.13`,
  `logic.end.14`, `%t18`, `%t22`, value ids 7/9/10, or one prepared dump.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, or baseline/log churn.
