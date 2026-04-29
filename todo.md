Status: Active
Source Idea Path: ideas/open/126_bir_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory BIR String Lookup Surfaces

# Current Packet

## Just Finished

Step 1 inventory completed for `src/backend/bir`.

Major BIR string surfaces by category:

- Pure text lookup: `collect_lowered_string_constants` keeps `bytes_by_name`
  as `std::unordered_map<std::string, std::string>` for decoded string-pool
  byte lookup by stripped pool spelling; `collect_string_pointer_aliases` keeps
  SSA-to-string-pool-name aliases as `std::unordered_map<std::string,
  std::string>` before direct-call string pointer argument rewrite.
- Semantic lookup: `GlobalTypes`, `FunctionSymbolSet`, `ValueMap`,
  `CompareMap`, `BlockLookup`, `PhiBlockPlanMap`, and the memory/provenance
  maps still use raw LIR/BIR names as lookup keys for globals, functions,
  SSA values, compare aliases, labels, local slots, pointer aliases, aggregate
  slots, dynamic array accesses, and pointer provenance. Several global,
  call, and initializer paths already carry `LinkNameId`; block labels already
  get `BlockLabelId` after lowering; structured layouts already have a
  structured table plus legacy text table.
- Display: BIR record fields such as `Value::name`, function/global/param/local
  slot names, block labels, call callee text, load/store global and local slot
  names, memory-address base names, structured type spellings, and string
  constant names remain printed spelling.
- Selector: canonical select lowering uses `BlockLookup`,
  `follow_canonical_select_chain`, `CompareMap`, and SSA value aliases keyed by
  LIR block/value strings to recognize a control-flow shape and synthesize
  `bir.select`.
- Dump text: `bir_printer.cpp` renders names, labels, structured return type
  names, inline asm metadata, memory-address text, and `semantic_phi` comments;
  `render_block_label` prefers `BlockLabelId` spelling and falls back to the
  retained label string.
- Diagnostics: verifier/lowering failure messages include raw function,
  global, block, slot, callee, family, and structured-layout text for user
  reporting; these are not conversion targets by themselves.
- Final spelling: `Function::name`, `Global::name`, `CallInst::callee`,
  `LoadGlobalInst::global_name`, `StoreGlobalInst::global_name`,
  `Global::initializer_symbol_name`, and block/slot/value names still provide
  output spelling even where a semantic id exists.
- Compatibility bridge: `rewrite_direct_call_string_pointer_args` maps LIR
  direct calls back to lowered BIR calls by resolved function name and call
  order, then rewrites pointer args to string-pool globals; pointer initializer
  parsing keeps `initializer_symbol_name` text while module lowering resolves
  `initializer_symbol_name_id`; string constants intentionally remain
  name-only because LIR string-pool entries lack `LinkNameId`.
- Unresolved boundary: string-pool constants do not carry `LinkNameId`;
  structured type spelling is still text-backed despite structured layout
  metadata; many SSA/local-slot/pointer-provenance maps are function-local LIR
  value identity rather than interned text identity; BIR verification still
  validates block membership and local slots by retained strings even though
  block labels have `BlockLabelId`.

First bounded conversion target identified: convert the pure text
string-pool byte lookup/alias path in `src/backend/bir/lir_to_bir.cpp` from
raw pool-name string keys to interned text identity. The next packet should
keep string-pool final spelling intact while replacing `bytes_by_name` and
`StringPointerAliasMap` lookup authority with stable text ids already
available through the lowered BIR module name tables or a small local
interning table.

## Suggested Next

Execute Step 2 for the string-pool direct-call rewrite path only: convert
`LoweredStringConstantMetadata::bytes_by_name`,
`StringPointerAliasMap`, and their lookups in
`rewrite_direct_call_string_pointer_args` to interned text identity while
preserving `StringConstant::name` and printed `@pool` spelling.

## Watchouts

Do not start with verifier/global/call semantic maps; those already have a
mixture of `LinkNameId`, final spelling, and compatibility fallbacks and need
a separate Step 3 packet. Do not convert string constants to `LinkNameId` in
this route because the LIR string pool currently lacks semantic link-name
metadata.

## Proof

Passed delegated proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log`

Result: `ninja: no work to do`; `ctest` reported 100% tests passed, 0 failed
out of 108 run in the `^backend_` subset, with 12 disabled tests not run.
Proof log: `test_after.log`.
