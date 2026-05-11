# Current Packet

Status: Active
Source Idea Path: ideas/open/170_string_authority_regression_guard.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement The Local Guard

## Just Finished

Step 3 implemented and tightened the local string-authority guard that consumes
`scripts/string_authority_classifications.json` and scans only the configured
roots for the Step 1 declaration-level surface.

Changed files:

- `scripts/string_authority_guard.py`
- `scripts/test_string_authority_guard.py`
- `scripts/string_authority_classifications.json`
- `todo.md`

Implementation notes:

- the guard validates required classification fields, duplicate exact entries,
  and every detected current hit by exact `path` + `symbol`
- the scanner is intentionally local and text-based, using Python stdlib only
- unclassified declaration-level string-keyed containers and aliases now fail
  even when their names are generic, such as `unexpected_cache`
- unclassified `*_by_name` / `*_name_map` members and selected raw, rendered,
  legacy, fallback, or explicit by-name lookup helpers fail unless classified
- diagnostics report file, line, symbol, pattern, and the instruction to
  classify the exact path+symbol or replace the use with structured authority
- the self-test builds a temporary covered root and proves both semantic and
  generic unclassified `std::unordered_map<std::string, ...>` members fail,
  while exact classified entries pass
- the classification inventory was expanded with exact current hits exposed by
  the stricter declaration-level scanner so the repo guard stays green

## Suggested Next

Step 4 should integrate `python3 scripts/string_authority_guard.py` into the
developer workflow without widening the scanner surface. A good next packet is
to add a documented local command or CTest/script hook, plus reviewer guidance
for adding a new exact classification entry with owner, domain, category,
reason, removal condition, and evidence.

## Watchouts

- The current guard reports 235 classified declaration-level hits after the
  stricter Step 3 inventory expansion.
- The first implementation deliberately avoids ordinary `.find(name)` call-site
  scanning; widening into expression use sites should be a separate
  re-inventory packet with new exact classifications.
- The scanner is still a local text scanner, not a full C++ parser; it now uses
  accumulated statement text for multi-line function/type scope detection so
  function-local scratch declarations are not treated as new top-level hits.
- Keep fixtures out of production roots; the self-test uses a temporary tree.

## Proof

Step 3 proof:

- `python3 scripts/string_authority_guard.py > test_after.log 2>&1`
- `python3 scripts/test_string_authority_guard.py`
- `python3 -m py_compile scripts/string_authority_guard.py scripts/test_string_authority_guard.py`
- `git diff --check`

All commands passed. The guard proof writes the canonical executor proof log to
`test_after.log`.
