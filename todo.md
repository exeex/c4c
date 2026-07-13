# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 extern-declaration packet completed ordered receipt from
  `LirModule::extern_decls`: added epoch-owned `ExternalDeclId`, typed Raw-BIR
  storage, builder receipt, immutable name/link views, production import, and
  reachable foundation verification.
- Receipt preserves vector order, source name, structured return type,
  `None`/`SignExt`/`ZeroExt`, and either a resolved link-table identity or an
  explicit fallback name. The two extern maps are strict parity/dedup evidence;
  they do not supply declaration order.

## Suggested Next

- Execute only the next bounded Step 3 global object/symbol identity packet.
  Account explicitly for the current producer evidence: `LirGlobal.id` remains
  default/unpopulated, so it cannot yet be authoritative identity, and
  `init_text` plus `initializer_function_link_name_ids` require a typed receipt
  or an explicit unsupported boundary rather than being dropped or text-parsed.

## Watchouts

- Extern receipt requires exact vector/map cardinality and snapshot parity;
  vector position alone defines Raw-BIR order. Link-backed rows must resolve
  through the imported link table with identical spelling.
- Structured `LirTypeRef` is return-type authority; `return_type_str` is only
  exact compatibility evidence. Raw text, malformed types, void/extension
  incoherence, unknown attrs, and duplicate identities remain rejected.
- Globals, broader symbols, and initializers remain unsupported. In particular,
  the next packet must not promote the currently default `LirGlobal.id` or
  silently accept a global while dropping either initializer evidence field.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_interface$'`.
- The focused post-change run passed 1/1 tests. Canonical proof log:
  `test_after.log`.
