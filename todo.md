Status: Active
Source Idea Path: ideas/open/623_rv64_cast_instruction_fragment_consumers.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Split ownership by first missing fact

# Current Packet

## Just Finished

Step 2 classified the 28 refreshed CastInst residual rows from
`build/agent_state/623_step1_cast_residuals.tsv` by first missing or
responsible owner without implementation, test, unsupported-marker, allowlist,
runtime/accounting, plan, or idea changes.

Artifacts written:
- `build/agent_state/623_step2_cast_owner_buckets.tsv`: row-level ownership
  classification, prepared cast line, source kind, and producer/move evidence.
- `build/agent_state/623_step2_cast_owner_notes.md`: summary counts,
  multi-row RV64 consumer sub-families, outside-route guardrails, and row
  details.

Ownership result:
- `RV64 consumer`: 26 ordinary i32 rows. BIR producer facts are complete for
  these rows: each row has a typed prepared `CastInst` line and a matching
  before-instruction prepared move bundle. The first missing fact is RV64
  object-route consumption of the CastInst.
- `policy`: 2 f128/floating rows, `src/930622-2.c` and
  `src/ieee/pr29302-1.c`, kept outside the ordinary RV64 GPR cast-consumer
  route.

Multi-row RV64 consumer sub-families with complete producer facts:
- `rv64-consumer:width-preserving-zext-i32-to-i32`: 18 rows.
- `rv64-consumer:width-preserving-trunc-i32-to-i32`: 4 rows.
- `rv64-consumer:ptrtoint-*-ptr-to-i32`: 4 rows split by value, global, and
  local-memory pointer source kind.

No refreshed CastInst row was assigned to BIR producer, semantic cast, ABI,
global, local-memory, select, branch, move-bundle, runtime, width, signedness,
source-kind, or authority as the first missing owner. Source kind only refines
the ptrtoint RV64-consumer sub-family. The nearby 60 non-cast guard rows remain
outside this route.

## Suggested Next

Execute the first implementation packet for the ordinary RV64 consumer family:
add semantic RV64 object-route consumption for width-preserving i32-to-i32
CastInst rows, starting with the 18-row `zext i32 ... to i32` sub-family and
keeping f128/floating-policy rows out of scope.

## Watchouts

- Do not include the 2 `sitofp -> f128` rows in the ordinary GPR cast-consumer
  packet; they remain policy/floating work.
- Do not use the 60 Step 1 non-cast guard rows as acceptance for this route.
- Keep ptrtoint source-kind differences as a separate RV64 consumer sub-family
  if width-preserving i32 cast consumption is split first.
- Reject any route that rewrites expectations or unsupported markers instead
  of lowering the RV64 CastInst consumer path.

## Proof

Supervisor-selected proof command:

```sh
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1
```

Result: passed. `test_after.log` is the canonical proof artifact; the backend
CTest subset completed successfully.
