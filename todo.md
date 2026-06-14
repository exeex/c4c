Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Design the Value-Name Agreement Boundary

# Current Packet

## Just Finished

Completed Step 2: designed the value-name agreement boundary for idea 260's
`names` same-block lookup candidate.

Chosen helper placement:

- Put the shared boundary in `src/backend/prealloc/lookup_agreement.hpp` and
  `src/backend/prealloc/lookup_agreement.cpp`, beside the existing prepared/BIR
  function and block agreement helpers.
- Use the shared helper from both `prepared_lookups.cpp` and
  `select_chain_lookups.cpp` so the prepared scalar/integer lookup path and the
  select-chain source-producer path accept the same named-value facts.
- Keep each lookup file responsible for its current public return type and
  recursive behavior; the shared helper should only answer whether one BIR
  value, prepared `ValueNameId`, producer row, block, type, and cutoff agree.

Accepted agreement rows:

- The queried BIR value must be named and have non-empty spelling.
- The caller-supplied or resolved prepared `ValueNameId` must be valid and
  must match `PreparedNameTables::value_names.find(value.name)`.
- The producer lookup map must exist and contain exactly the queried prepared
  value-name row; `Unknown` duplicate rows are not agreement.
- The producer row must stay in the queried prepared block label and its
  instruction index must be before the caller's cutoff.
- The producer index must address the supplied BIR block and the indexed BIR
  instruction must be the producer's stored instruction pointer/kind.
- The producer result and indexed instruction result must be the same named BIR
  value, with the same prepared value-name id and the expected result type.
- Integer-constant lookup may fold only after every named operand reaches this
  same agreement boundary; immediate operands remain directly foldable.

Rejected or compatibility rows:

- Unnamed BIR values, empty value names, missing prepared value ids, invalid
  value-name ids, missing producer maps, and missing producer rows fail closed.
- Stale producer rows, future producer rows at or after the cutoff, wrong
  block labels, out-of-range instruction indexes, wrong instruction kinds,
  wrong result pointers, and wrong result types fail closed.
- Duplicate spelling rows published as `Unknown` fail closed instead of being
  treated as agreement.
- Prepared/BIR name drift fails closed when a BIR spelling resolves to a
  different prepared value-name id than the row being queried.
- Existing raw-spelling lookup behavior is retained only as public
  compatibility where a caller already resolves spelling through prepared
  tables; it must not be used to claim structured prepared/BIR agreement.

Candidate implementation and test files:

- `src/backend/prealloc/lookup_agreement.hpp`
- `src/backend/prealloc/lookup_agreement.cpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/select_chain_lookups.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

## Suggested Next

Execute Step 3: implement the shared value-name agreement helper in
`lookup_agreement.*`, wire it into `prepared_same_block_source_producer(...)`,
`evaluate_prepared_same_block_integer_constant(...)`, and
`find_prepared_select_chain_source_producer(...)`, and add focused positive and
fail-closed rows in `backend_prepared_lookup_helper_test.cpp`.

## Watchouts

- Keep this runbook limited to the selected `names` same-block value-name
  lookup candidate.
- Do not treat this activation as approval for `PreparedBirModule` deletion,
  privatization, wrapping, broad aggregate retirement, or another idea 260
  candidate.
- Preserve prepared name tables, existing lookup maps, public aggregate
  compatibility, and current null or unavailable fallback behavior.
- Preserve raw-spelling lookup only as compatibility behavior; do not treat it
  as structured prepared/BIR value-name agreement.
- The Step 3 implementation packet should not change route-debug, target
  output, baselines, unsupported expectations, helper/oracle status names,
  printer/debug strings, value-home lookup, semantic resolver API,
  control-flow, or store-source publication behavior.
- Step 3/4 proof should add nearby rows for positive agreed scalar/integer
  lookups plus unnamed, empty, missing prepared id, stale producer, wrong type,
  duplicate spelling, missing maps, and prepared/BIR name-drift rejection.
- Do not rewrite route-debug, target output, baselines, unsupported
  expectations, helper/oracle status names, printer/debug strings, value-home
  lookup, semantic resolver API, control-flow, or store-source publication
  behavior to claim progress.

## Proof

Concrete Step 3 implementation packet:

```text
to_subagent: c4c-executor
Objective: implement the shared value-name agreement helper for idea 260 `names` same-block lookup candidate.
Plan Step: Step 3: Implement Narrow Value-Name Bridge
Owned Files: src/backend/prealloc/lookup_agreement.hpp, src/backend/prealloc/lookup_agreement.cpp, src/backend/prealloc/prepared_lookups.cpp, src/backend/prealloc/select_chain_lookups.cpp, tests/backend/bir/backend_prepared_lookup_helper_test.cpp, todo.md
Do Not Touch: plan.md, ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md, ideas/closed/*, review/*
Tooling: no clang-tools needed; this is a narrow prealloc helper wiring packet with focused local tests.
Proof: (cmake --build --preset default --target backend_prepared_lookup_helper_test && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure) > test_after.log 2>&1
Done When: the shared agreement helper accepts only non-empty named BIR values whose prepared ValueNameId, same block, before-cutoff producer, result type, and integer-constant operands agree; the selected prepared and select-chain paths use it; tests prove positive rows and fail-closed rows for unnamed/empty/missing prepared id/stale producer/wrong type/duplicate spelling/missing maps/prepared-BIR drift without changing unrelated candidates.
If Blocked: stop and report the exact ambiguity; do not edit plan.md or source idea.
```

Supervisor-selected focused proof for Step 3:

```bash
(cmake --build --preset default --target backend_prepared_lookup_helper_test && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure) > test_after.log 2>&1
```

Step 2 design proof:

```bash
git diff --check -- todo.md
```
