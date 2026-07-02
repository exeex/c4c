Status: Active
Source Idea Path: ideas/open/556_prepared_move_bundle_ambiguous_stack_destination_src_960209_1.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Or Justify The Prepared Contract

# Current Packet

## Just Finished

Step 2 - Repair Or Justify The Prepared Contract completed by justification;
no test churn or semantic repair was needed.

The captured `src/960209-1.c` row is already covered by the focused prepared
contract test
`verify_move_bundle_consumer_rejects_ambiguous_multi_source_stack_destination`.
That test constructs the semantic shape the classifier rejects:

- a `BeforeInstruction` move bundle with `authority_kind=None`
- no parallel-copy owner on the traversal event
- two ordered moves with per-move `authority_kind=None`
- default-empty `source_parallel_copy_step_index`
- two register-backed source value homes
- stack-slot destinations targeting the same prepared destination value
- the exact fail-closed status
  `AmbiguousNonParallelMultiSourceStackDestination`
- the exact diagnostic category and message
  `prepared move-bundle classifier rejected ambiguous non-parallel multi-source stack-destination authority`

The same test also proves the nearby boundaries: distinct prepared destination
values backed by the same stack slot still fail closed, while a single
register-source stack-destination move remains available. This matches the
captured row's authority=none, non-parallel, multi-register-source bundle into
one stack value, so RV64 must not consume or materialize it by guessing source
ownership or sequencing.

## Suggested Next

Execute Step 3 from `plan.md`: prove the retained prepared contract against
the `src/960209-1.c` row-level outcome without weakening pass/fail accounting.

## Watchouts

- Do not continue this work as RV64 local-memory addressing.
- Do not weaken gcc_torture expectations, unsupported markers, allowlists, or
  runtime comparison behavior.
- Do not special-case `src/960209-1.c` or materialize ambiguous bundles in
  RV64 by guessing source ownership.
- Treat any acceptance route that picks `%t43` or `%t44`, drops one move, or
  infers ordering from dump order as testcase-overfit.
- The retained contract is semantic, not row-shaped: it does not depend on
  function name `f`, block `tern.end.38`, slot #21, or `%t43`/`%t44`/`%t45`.

## Proof

Proof command run exactly as delegated, with combined output recorded in
`test_after.log`:

```sh
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: build succeeded (`ninja: no work to do.`) and all 345 selected
`backend_` tests passed. The supervisor-selected proof was sufficient for this
no-code-change contract justification slice.
