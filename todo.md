# Current Packet

Status: Active
Source Idea Path: ideas/open/170_string_authority_regression_guard.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Guard Candidates

## Just Finished

Step 1 inventory scanned the requested compiler domains for suspicious
string-authority surfaces:

- string-keyed containers and aliases:
  `unordered_map<std::string...>`, `unordered_map<std::string_view...>`,
  `std::map<std::string...>`, and aliases wrapping those forms
- broad name helpers: `*_by_name`, `*_name_map`, `find_*name*`,
  `lookup_*name*`, `find(name)`-style calls, `raw_symbol`, `rendered`,
  `legacy`, `compatibility`, and `fallback`
- source roots scanned: `src/frontend/parser/`, `src/frontend/sema/`,
  `src/frontend/hir/`, `src/codegen/lir/`, `src/backend/`, and
  `src/codegen/shared/`

Initial classification buckets from ideas 167, 168, and 169:

- `structured-authority`: typed IDs or structured keys are the authority, e.g.
  parser `TextId`/`QualifiedNameKey`, sema `SemaStructuredNameKey`, HIR
  owner/member keys, LIR `LinkNameId`/`StructNameId`, BIR/prepared name ids.
- `compatibility-bridge`: explicitly named or commented rendered/raw fallback
  for no-metadata carriers, e.g. parser tag-map compatibility, sema rendered
  enum/type compatibility, HIR rendered owner mirrors, LIR
  `extern_decl_name_map`, BIR `fallback_by_name` and
  `raw_symbol_link_name_ids`.
- `display-output`: final spelling, dumps, printers, emitted symbol spelling,
  string pools, and output text where lookup is not semantic authority.
- `diagnostic-debug`: error/reporting/debug-route strings and test messages.
- `route-local-identity`: function-local SSA values, block labels, local
  slots, prealloc/regalloc handles, stack-layout maps, BIR local-slot maps, and
  x86/aarch64 route-local lowering caches.
- `generated-temporary-name`: mangled names, synthesized SSA/block/label names,
  name counters, and generated parser/HIR temporary binding names.
- `abi-spelling`: assembler/linker/relocation symbol spellings, section labels,
  GOT/PLT names, and target ABI-visible strings.
- `false-positive`: includes docs/`.md`, tests, source-profile lexical
  classification text, comments-only hits, include-only hits, and ordinary
  containers keyed by typed ids but storing strings.

Chosen initial guard surface:

- Guard declaration-level authority surfaces in the scanned source roots, not
  every string expression: string-keyed map/set declarations or aliases,
  `*_by_name`/`*_name_map` members, and helper declarations/definitions whose
  names contain `find|lookup` plus `name|raw_symbol|rendered|legacy|fallback`.
- Require each hit to be either classified or rejected; do not add directory
  skips for whole frontend/backend areas.
- Start with exact file + symbol/pattern classifications for current hits.
  This catches new semantic maps and broad fallback helpers while avoiding
  ordinary diagnostics, output assembly text, and local `std::string` values.

Reviewable data shape for Step 2:

```json
{
  "version": 1,
  "roots": [
    "src/frontend/parser",
    "src/frontend/sema",
    "src/frontend/hir",
    "src/codegen/lir",
    "src/backend",
    "src/codegen/shared"
  ],
  "classifications": [
    {
      "path": "src/frontend/sema/validate.cpp",
      "symbol": "structured_record_keys_by_tag_",
      "pattern": "string-keyed-container",
      "owner": "frontend-sema",
      "domain": "record lookup compatibility",
      "category": "compatibility-bridge",
      "reason": "Rendered mirror for no-metadata parser carriers; structured record keys remain authoritative.",
      "removal_condition": "Remove when record TypeSpec/Node carriers always preserve structured owner metadata.",
      "evidence": "ideas/closed/167..., ideas/closed/168..., adjacent source comment"
    }
  ]
}
```

Expected false positives to classify, not suppress globally:

- backend linker/assembler maps keyed by ABI spelling or emitted symbol names
- x86/aarch64 route-local codegen caches keyed by SSA, slot, label, or
  generated temporary names
- parser/sema/HIR compatibility mirrors with adjacent comments proving typed
  metadata is authoritative
- output-only string pools, dump/printer state, and diagnostic/debug helpers
- docs and tests if the script is accidentally aimed outside source roots

## Suggested Next

Step 2 should add `scripts/string_authority_classifications.json` using the
shape above and seed exact entries for the current declaration-level hits. Use
a small Python stdlib JSON reader for the later guard; repo scripts already use
Python and JSON, so this avoids a new dependency.

## Watchouts

- Idea 171 depends on this guard; do not skip directly to migration closure.
- Idea 170 still mentions idea 168 as an open dependency, but the dependency is
  satisfied by `ideas/closed/168_compatibility_bridge_retirement.md`.
- The guard must not reject ordinary diagnostic, output, ABI spelling, or
  route-local string use.
- Do not claim progress from broad allowlist exceptions without owner, domain,
  category, and reason.
- Do not scan every `.find(name)` call in Step 2; start with declarations and
  named helper surfaces, then add call-site scanning only after the first guard
  has a stable classification file.
- First implementation proof direction: add a script self-test or focused test
  proving an unclassified `std::unordered_map<std::string, ...>` under a
  covered source root fails, while representative classified compatibility,
  display/output, route-local, diagnostic/debug, generated-temporary, and ABI
  spelling entries pass.

## Proof

Step 1 inventory-only proof:
- `git diff --check -- todo.md`
- proof log: `test_after.log`
