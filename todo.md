Status: Active
Source Idea Path: ideas/open/34_aarch64_store_source_publication_planning.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Convert AArch64 To Consume Shared Authority

# Current Packet

## Just Finished

Step 3 converted the selected AArch64 store-local cast publication path to
consume the shared prepared store-source producer fact. The AArch64
store-source plan now attaches only
`PreparedEdgePublicationSourceProducerKind::Cast` authority from
`PreparedFunctionLookups`, and the cast lowering branch uses the plan's cast
pointer plus producer block/instruction index instead of same-block cast
rediscovery.

The old AArch64 `store_local_value_cast_producer` rediscovery helper and public
declaration were removed. Missing or incomplete prepared cast producer facts
remain fail-closed, and focused AArch64 coverage proves both the prepared-fact
success path and the missing-fact rejection path. The broader AArch64
instruction-dispatch positive now attaches `PreparedFunctionLookups` before
dispatch so it also consumes the shared authority instead of relying on
same-block rediscovery.

## Suggested Next

Execute Step 4 proof handoff for the converted AArch64 cast producer path:
review the Step 3 diff against the source idea, then run the supervisor-chosen
broader validation or regression guard before commit.

## Watchouts

- Do not merge `memory_store_sources.*` into `memory.cpp` during this semantic
  migration.
- Do not claim progress through expectation rewrites, unsupported downgrades,
  or fixture-specific matching.
- Step 3 intentionally ignored non-cast prepared source-producer kinds in the
  AArch64 store-source attachment; keep Step 4 focused on proof/review unless
  the supervisor explicitly delegates another producer family.

## Proof

Baseline command:
`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Baseline result: passed. `test_before.log` records 163/163 backend tests
passed.

After command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log`

After result: passed. Build completed, and `test_after.log` records 163/163
backend tests passed.

Regression guard:
`check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Regression result: PASS.
