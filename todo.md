# Current Packet

Status: Active
Source Idea Path: ideas/open/546_rv64_instruction_fragment_current_classification.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Produce Follow-Up Routing

## Just Finished

Step 5 (`Produce Follow-Up Routing`) routed all `265` screened
instruction-fragment rows from
`build/agent_state/546_step4_instruction_fragment_screening.tsv`.

Generated artifacts:

- Routing notes:
  `build/agent_state/546_step5_followup_routing.md`.
- Route-level count table:
  `build/agent_state/546_step5_followup_routing.tsv`.

Routing count check, summing to `265`:

- `implementation_ready_rv64`: `87`
- `f128_quarantine`: `28`
- `producer_prepared_boundary`: `143`
- `abi_or_call_boundary`: `5`
- `evidence_gap`: `2`

Ranked ordinary RV64 follow-up buckets:

1. `integer_div_rem`: `30` rows, first owner `rv64_object_lowering`;
   recommended first lifecycle route because it has the best count and the
   clearest direct BIR operation family (`sdiv`, `udiv`, `srem`, `urem`).
   Representative rows: `src/20001026-1.c`, `src/20050215-1.c`,
   `src/20090113-2.c`, `src/20090113-3.c`, `src/20101013-1.c`.
2. `integer_arithmetic_shift_right`: `12` rows, first owner
   `rv64_object_lowering`; smaller but high proof clarity around `ashr`.
   Representative rows: `src/pr78438.c`, `src/pr79737-2.c`,
   `src/20000815-1.c`, `src/920501-9.c`, `src/931110-1.c`.
3. `scalar_fp_cast_or_op`: `18` rows, first owner
   `rv64_object_lowering`; route through the existing scalar FPR residual lane
   rather than mixing with integer lowering. Representative rows:
   `src/20040709-3.c`, `src/930603-1.c`, `src/20000605-1.c`,
   `src/20010118-1.c`, `src/20020321-1.c`.
4. `scalar_integer_binary`: `15` rows, first owner
   `rv64_object_lowering`; implementation-ready but heterogeneous enough to
   need a follow-up triage split before code. Representative rows:
   `src/20000819-1.c`, `src/20020107-1.c`, `src/20071018-1.c`,
   `src/921123-1.c`, `src/921208-1.c`.
5. `pointer_integer_cast`: `12` rows, first owner
   `rv64_object_lowering`; coherent `inttoptr`/`ptrtoint` evidence but lower
   confidence because pointer/address-width behavior needs careful proof.
   Representative rows: `src/20000622-1.c`, `src/20001101.c`,
   `src/940115-1.c`, `src/950710-1.c`, `src/loop-2c.c`.

Screened-out routing:

- `28` F128/long-double rows route to existing
  `ideas/open/426_f128_quarantine_and_external_softfloat_policy.md`.
- `143` producer/prepared rows route to producer/prepared authority follow-up;
  likely existing lanes include
  `ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md`
  and `ideas/open/547_bir_local_memory_call_metadata_boundary_review.md`, but
  plan-owner should decide exact lifecycle placement.
- `5` ABI/call rows route to ABI/call boundary follow-up.
- `2` evidence-gap rows route to evidence recovery.

Reject signals recorded for follow-up ideas:

- no testcase-name dispatch or named-case constants
- no raw diagnostic/opcode-text-only matching as a lowering strategy
- no expectation rewrites, unsupported downgrades, or allowlist filtering
- no folding F128, producer/prepared, ABI/call, or evidence-gap rows into
  ordinary RV64 implementation work without first-owner proof

Closure readiness: idea 546 appears complete for executor purposes. The
coherent current row set is traceable to the refreshed scan, high-frequency
sub-buckets have first-owner/readiness decisions, F128 rows are routed to
quarantine, and implementation follow-up recommendations include concrete row
evidence plus overfit reject signals. Supervisor/plan-owner should decide
source-idea creation and lifecycle closure.

## Suggested Next

Plan-owner lifecycle packet: close or transition idea 546, and decide whether
to create/activate a narrow follow-up idea for `30` RV64 integer div/rem
instruction-fragment rows.

## Watchouts

- Leave `review/557_step13_vector_local_memory_review.md` untouched.
- The refreshed coherent current count is `265`, not the source-expected `137`
  and not the Step 1 mixed-time `179`.
- Step 5 wrote draft follow-up payloads into
  `build/agent_state/546_step5_followup_routing.md` but did not create
  `ideas/open/` files.
- Keep screened-out rows out of ordinary RV64 implementation queues unless a
  later plan proves first-owner authority.

## Proof

- Evidence-only packet; no CTest proof required.
- Routing input:
  `build/agent_state/546_step4_instruction_fragment_screening.tsv`.
- Routing outputs:
  `build/agent_state/546_step5_followup_routing.md` and
  `build/agent_state/546_step5_followup_routing.tsv`.
- Validation:
  - `git diff --check -- todo.md`
  - routing TSV route-count sum is `265`
- No `test_after.log` update was required by this evidence-only packet.
