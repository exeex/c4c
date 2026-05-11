# Current Packet

Status: Active
Source Idea Path: ideas/open/170_string_authority_regression_guard.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Classification Data

## Just Finished

Step 2 added `scripts/string_authority_classifications.json` as the
reviewable classification artifact for the Step 1 declaration-level guard
surface.

Changed files:

- `scripts/string_authority_classifications.json`
- `todo.md`

Data shape:

- top-level `version`
- top-level `roots` matching the Step 1 source roots:
  `src/frontend/parser`, `src/frontend/sema`, `src/frontend/hir`,
  `src/codegen/lir`, `src/backend`, and `src/codegen/shared`
- top-level `classifications` array
- each classification entry has `path`, `symbol`, `pattern`, `owner`,
  `domain`, `category`, `reason`, `removal_condition`, and `evidence`

Seeded coverage summary:

- parser: parser record and named-constant compatibility helpers plus
  parser-local rendered record tag state
- sema: rendered enum/consteval/template binding compatibility maps,
  rendered-to-TextId/key bridge helpers, record-key mirrors, and a structured
  field TextId bridge partitioned by `SemaStructuredNameKey`
- HIR: template binding/NTTP compatibility aliases, function-local lowering
  maps, rendered compatibility names, labels, static globals, and record
  definition mirrors kept beside owner-key tables
- LIR/shared: extern declaration rendered-name fallback, string-pool output,
  legacy verifier shadow maps, shared function-local labels, no-stable-id
  dedup fallbacks, and generated temporary name counters
- BIR/backend: route-local SSA/slot/provenance maps, raw-symbol compatibility,
  LinkNameId fallback lookup, regalloc/prealloc local lookup helpers, BIR
  validation diagnostics, target assembler/linker ABI spelling maps, and
  target route-local stack/value maps

The seed intentionally uses exact file + symbol entries. It does not add broad
directory allowlists, and every entry records a category, owner, domain, reason,
removal condition, and evidence for review.

## Suggested Next

Step 3 should implement the narrow local guard script that reads
`scripts/string_authority_classifications.json`, scans only the configured
`roots`, and reports declaration-level hits matching the Step 1 surface:
string-keyed map/set declarations or aliases, `*_by_name`/`*_name_map`
members, and helper declarations/definitions whose names contain
`find|lookup` plus `name|raw_symbol|rendered|legacy|fallback`.

Expected Step 3 behavior:

- accept current hits only when their `path` + `symbol` are classified and the
  required fields are present
- fail an unclassified suspicious `std::unordered_map<std::string, ...>` or
  broad rendered-name lookup helper under a covered root
- avoid scanning implementation expressions such as ordinary `.find(name)`
  call sites in the first guard
- treat this JSON as reviewable classification data, not as a directory-level
  suppressions file
- add a focused script self-test or fixture proving one unclassified semantic
  map fails while representative classified categories pass

## Watchouts

- This Step 2 seed is representative and reviewable; Step 3 should keep its
  first scanner narrow enough that these declaration-level classifications are
  the expected accepted surface.
- If Step 3 broadens beyond declaration-level hits, it should first add more
  exact classifications or split the expansion into a separate packet.
- Do not classify new semantic string authority as display/output or
  diagnostic/debug just to make the guard green.
- Keep generated files, docs, `.md` compatibility notes, and tests out of the
  initial root scan unless the supervisor explicitly widens scope.

## Proof

Step 2 data proof:

- `python3 -m json.tool scripts/string_authority_classifications.json >/tmp/string_authority_classifications.pretty.json`
- `git diff --check -- scripts/string_authority_classifications.json todo.md`

Both commands passed. The delegated proof commands write to `/tmp` or stdout
and do not themselves create `test_after.log`; no root-level proof log was
created for this data-only packet.
