Status: Active
Source Idea Path: ideas/open/587_prepared_value_freshness_authority_mvp.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Wire Representative RV64 Call-Argument Consumer

# Current Packet

## Just Finished

Step 3 - Wire Representative RV64 Call-Argument Consumer: routed the RV64
prepared simple-call argument consumer through the shared
PreparedValueFreshnessAuthority query before trusting prior-preservation and
published register-source authorities. The prior-preservation branch now fails
closed unless the selected authority is PriorPreservation; the register-source
branch accepts only selected DirectHome, ExplicitPublication, or
ProducerRematerialization authorities while preserving existing register-bank,
stack-slot, and materialization validation. Focused RV64 coverage proves
selected prior preservation is accepted, while missing, ambiguous, and fresher
producer authority blocks the prior-preservation fallback.

## Suggested Next

Start Step 4 from plan.md: wire the next consumer or broaden coverage according
to the supervisor-selected packet, keeping the shared freshness query as the
ordering authority.

## Watchouts

- Do not claim progress through expectation rewrites, unsupported-marker edits,
  allowlist changes, or named-testcase shortcuts.
- Only the RV64 prepared simple-call argument consumer is wired so far; other
  RV64 object-emission and non-RV64 consumers may still use existing local
  source-selection paths.
- Producer rematerialization and explicit publication rank above older
  PriorPreservation through the shared query; target consumers should follow
  the selected authority rather than reimplementing rank ordering.
- Equal-rank matching authorities intentionally fail closed as
  `ambiguous_candidate`.
- The delegated owned-file path `src/backend/mir/riscv/calls.cpp` does not
  exist in this checkout; the implemented consumer lives in
  `src/backend/mir/riscv/codegen/prepared_call_emit.cpp`, which is called by
  the owned prepared function emitter.

## Proof

Passed: `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

`test_after.log` contains the passing proof for Step 3.
