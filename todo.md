# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 global object/symbol identity packet completed ordered receipt
  from `LirModule::globals`: added epoch-owned `GlobalObjectId`, typed Raw-BIR
  storage, builder receipt, immutable ordered/name/link views, production
  import, and reachable foundation verification.
- The admitted initializer-free external subset preserves source name,
  authoritative structured type, resolved link-table or explicit fallback
  identity, alignment, and all semantic flags. `LirGlobal.id` does not supply
  identity; source vector order alone assigns append-only Raw-BIR IDs.

## Suggested Next

- Execute only the next bounded Step 3 initialized-global/initializer packet.
  Add a typed lossless initializer receipt before admitting definitions; keep
  both initializer text and structured function-link evidence explicit at the
  import boundary until that representation exists.

## Watchouts

- The current coherent global subset is deliberately restricted to
  initializer-free `external ` / `global ` scalar rows whose structured
  `TypeSpec` parity is exact. Weak/visibility forms and non-scalar global type
  parity need typed authority before admission; do not infer them from text.
- Initialized definitions remain unsupported. Neither `init_text` nor
  `initializer_function_link_name_ids` may be dropped or parsed as semantic
  authority, and `LirGlobal.id` remains producer-default compatibility state.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_interface$'`.
- The focused post-change run passed 1/1 tests. The supervisor then captured a
  clean-HEAD baseline and matching post-change `^backend_` checkpoint; both
  passed 4/4 tests and the monotonic regression guard passed. Canonical proof
  logs: `test_before.log` and `test_after.log`.
