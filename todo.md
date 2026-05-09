Status: Active
Source Idea Path: ideas/open/158_sema_validate_string_authority_audit.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Consteval Handoff Boundary Audit

# Current Packet

## Just Finished

Step 5: Consteval Handoff Boundary Audit classified the validate-side consteval
handoff maps in `validate.cpp` and tightened consteval function lookup
precedence. Structured consteval misses now fail closed once the structured
handoff table is populated, so TextId/string compatibility remains limited to
references without complete structured metadata.

## Suggested Next

Supervisor can review and commit this Step 5 slice, then decide whether the
remaining active-plan work needs a plan-owner close/rewrite decision.

## Watchouts

- Do not broaden this audit into `src/frontend/sema/consteval.cpp` or HIR
  storage migration without a new packet.
- `consteval_funcs_` remains the rendered compatibility bridge required by the
  current `evaluate_consteval_call` interface.
- `consteval_funcs_by_text_` remains the unqualified TextId bridge for
  no-domain carriers; `consteval_funcs_by_key_` is authoritative when populated
  and the call reference has a structured function key.

## Proof

Passed: `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|positive_sema_|cpp_positive_sema_|llvm_gcc_c_torture_src_930603_1_c|llvm_gcc_c_torture_src_loop_2_c)') > test_after.log 2>&1`

Proof log: `test_after.log` reports 923/923 matching parser, positive sema, and
representative GCC torture tests passed.
