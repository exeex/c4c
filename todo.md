# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 packet 3A completed the current LIR string-pool family: added
  epoch-owned `StringDataId`, ordered Raw-BIR storage, builder receipt, immutable
  ID/name views, production import, and reachable foundation verification.
- Import preserves every `pool_name`, opaque `raw_bytes` byte, `byte_length`, and
  vector position exactly. It validates counter parity and cache values only as
  structured evidence, without using cache keys or payload parsing for semantic
  identity or order.

## Suggested Next

- Execute only the next bounded Step 3 global object/symbol identity packet.
  Preserve initializer state as an explicit unsupported boundary until its own
  typed packet; do not drop it or parse rendered initializer text.

## Watchouts

- Current producer evidence has two structured string forms: nonnegative lengths
  are cache-backed ordinary rows; `byte_length == -1` is the cacheless wide-row
  sentinel. The counter covers both forms, and lengths below `-1` are malformed.
- `str_pool_map` keys are source-byte dedup evidence while ordered rows carry an
  opaque escaped/preformatted carrier. Never compare them by decoding text;
  cache values alone reconcile ordinary row names.
- Globals, externs, broader symbols, and initializers remain unsupported. The
  next packet must not silently accept a global while dropping initializer state.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_interface$'`.
- The focused post-change run passed 1/1 tests. Canonical proof log:
  `test_after.log`.
