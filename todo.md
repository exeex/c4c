Status: Active
Source Idea Path: ideas/open/167_whole_codebase_string_authority_final_audit.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Classify Route-Local and Generated Names

# Current Packet

## Just Finished

Step 2 - Classify Covered Structured Domains is complete enough to advance.
The covered parser, sema, HIR, LIR, HIR-to-LIR, BIR, prepared backend, and
x86 link-facing structured-domain packets are recorded in committed
todo-only slices:

- `b64711bbc`: frontend structured-domain classification for parser
  support/state, sema validate/consteval/type-utils/canonical-symbol, and HIR
  rendered-compatibility paths.
- `3f4f6643e`: LIR and HIR-to-LIR classification for `LinkNameId`,
  `StructNameId`, extern declarations, aggregate references, discardable
  reachability, and final-spelling bridges.
- `674a385a8`: backend/BIR classification for LIR-to-BIR transport, BIR
  validation, prepared-stage `LinkNameId` transfer, and x86 final rendering.

Step 2 consolidation:
- No covered frontend structured path was found that reopens rendered-name
  lookup after a complete structured/TextId/key miss. Remaining frontend
  rendered maps are classified as no-metadata, incomplete-key, explicit
  rendered-compatibility, route-local, display, or diagnostic bridges.
- No covered LIR/HIR-to-LIR path was found that reopens lookup after a complete
  `LinkNameId` miss or known declared `StructNameId` miss. Remaining LIR
  bridge candidates are `extern_decl_name_map`/extern fallback filtering,
  `discardable_by_name`/`scan_refs`, final-spelling payload scans, and legacy
  struct declaration/printer shadows.
- No covered backend/BIR path was found that reopens rendered-name lookup after
  a complete `LinkNameId` miss. Remaining backend bridge candidates are raw
  LIR function-symbol probes, textual initializer/global-address parsing,
  string-pool/private-label paths, raw-only BIR payloads, and final ABI
  rendering checks.
- Remaining classified semantic-authority follow-up is bridge retirement, not
  a Step 2 blocker: add typed initializer/global-address/symbol-reference
  carriers where needed, then retire the raw-name maps/scans in the matching
  bridge-retirement lane.

## Suggested Next

Begin Step 3 with a route-local/generated-name packet for backend-local value,
label, slot, and string-pool identity.

First Step 3 packet:
- Scope:
  - LIR route-local names and final IR spelling in `src/codegen/lir/ir.hpp`,
    `src/codegen/lir/verify.cpp`, and `src/codegen/lir/hir_to_lir/**`:
    locals, value names, block labels, temporary names, string constants,
    generated static/global labels, and final LLVM text payloads.
  - BIR route-local names in `src/backend/bir/bir.hpp`,
    `src/backend/bir/bir_validate.cpp`, `src/backend/bir/bir_printer.cpp`, and
    `src/backend/bir/lir_to_bir/**`: `Value::name`, local slots, block labels,
    phi incoming labels, memory-address base names, private string labels, and
    generated temporaries.
  - Prepared backend route-local maps in `src/backend/prealloc/**`:
    `ValueNameId`, `BlockLabelId`, local `defs_by_name`/slot maps, stack-layout
    labels, register-allocation names, and temporary addressing helpers.
- Classification output required in `todo.md`:
  - owner/domain, file/local symbol or path, class (`RL`, `GT`, `DO`, `CB`,
    or `SA` if a cross-structure lookup still owns semantics), evidence, and
    whether the family should feed idea 169 route-local identity cleanup.
  - for route-local lookups, state whether lookup stays inside one function,
    one module, one backend stage, or crosses a stage boundary.
  - for generated names, record the generator and collision domain.
- Required inspection focus:
  - separate route-local names from source/link-visible identity; do not use
    `TextId` or `LinkNameId` as the desired authority for SSA temps, block
    labels, slots, registers, or private string-pool labels.
  - identify any route-local string lookup that validates or synchronizes
    multiple structures and should eventually become a typed local id.
  - keep final IR/assembly rendering and diagnostics classified as output
    unless a call chain feeds them back into lookup.

## Watchouts

- Step 3 is not bridge retirement. Record route-local identity families and
  cleanup candidates; do not replace names during the classification packet.
- `ValueNameId` and `BlockLabelId` are local/backend-stage ids, not source
  semantic identity. Classify their raw string companions by local collision
  domain and boundary.
- String-pool/private labels are addressable generated data names. Treat them
  separately from source string literal text and from link-visible function or
  global authority.
- Do not count final LLVM/BIR/assembly text as semantic authority unless it is
  parsed back into a lookup or validation decision.

## Proof

Step 2 proof was recorded in the committed packets above. This lifecycle
advance edited only `todo.md`; no build or test proof was required, and
`test_after.log` was not updated.

Suggested audit replay for the first Step 3 packet:
`rg -n --glob '!build*/**' --glob '!ideas/closed/**' --glob '!review/**' -e 'ValueNameId' -e 'BlockLabelId' -e 'value_name' -e 'block_label' -e 'defs_by_name' -e 'blocks_by_label' -e 'slot' -e 'string_constant' -e 'string_pool' -e 'tmp' -e 'temporary' -e 'label' -e 'local' src/codegen/lir src/backend/bir src/backend/prealloc`

Suggested AST-backed inventory:
`c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/prealloc/prealloc.cpp build/compile_commands.json`
`c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/prealloc/regalloc.cpp build/compile_commands.json`
`c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/prealloc/out_of_ssa.cpp build/compile_commands.json`
