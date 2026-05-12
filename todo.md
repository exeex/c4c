# Current Packet

Status: Active
Source Idea Path: ideas/open/171_identity_authority_migration_closure_gate.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Verify Guard Usability

## Just Finished

Step 2: Verify Guard Usability checkpoint completed for idea 171.

Developer-facing guard commands identified from
`scripts/string_authority_guard.py --help` and `tests/CMakeLists.txt`:
- Direct guard: `python3 scripts/string_authority_guard.py`
- Focused self-test: `python3 scripts/test_string_authority_guard.py`
- CTest workflow proof:
  `ctest --test-dir build -R string_authority_guard --output-on-failure`
- CMake workflow target also exists as `string_authority_guard`, running the
  direct guard followed by the focused self-test from the source tree.

Classification behavior verified:
- `scripts/string_authority_guard.py` requires each retained classification to
  provide `path`, `symbol`, `pattern`, `owner`, `domain`, `category`,
  `reason`, `removal_condition`, and `evidence`.
- Accepted hits are keyed by exact path plus symbol in
  `scripts/string_authority_classifications.json`; unclassified hits report
  the source location and ask for classification of that exact path+symbol or
  replacement with structured authority.
- Current classification categories present are `abi-spelling`,
  `compatibility-bridge`, `diagnostic-debug`, `display-output`,
  `generated-temporary-name`, `route-local-identity`, and
  `structured-authority`.
- The guard is declaration-level: it scans configured source roots for
  string-keyed declarations, suspicious by-name/raw-symbol/rendered/legacy
  declaration surfaces, and lookup helper declarations. Ordinary `.find(name)`
  call-site scanning is explicitly out of scope in the help guidance and idea
  170 closure notes.
- `scripts/test_string_authority_guard.py` verifies unclassified semantic and
  generic string maps fail, classified exact path+symbol entries pass, and
  accepted entries can carry the expected category metadata.

Blocker status:
- No missing usability artifact found for Step 2. The guard has a direct CLI
  command, a focused self-test, CTest registration, required classification
  metadata, exact path+symbol acceptance behavior, and documented out-of-scope
  limits.

## Suggested Next

Proceed to Step 3 by reconciling the remaining identity inventory against the
idea 167 audit, idea 168 compatibility bridge notes, idea 169 route-local
cleanup notes, and idea 170 guard classifications.

## Watchouts

- Idea 172 depends on this closure gate; do not start type-identity audit work
  from this plan.
- This is a checkpoint idea, not a broad cleanup pass.
- Do not claim closure if any semantic string-authority item remains
  unclassified or lacks a follow-up decision.
- Do not weaken tests or downgrade expectations to make the closure gate pass.
- No Step 2 guard-usability blocker found. Keep Step 3 evidence-only unless an
  unclassified semantic string-authority item requires a separate follow-up
  idea.

## Proof

Supervisor-selected guard proof:
- `python3 scripts/string_authority_guard.py > test_after.log 2>&1`
  - Result: passed.
  - Proof log path: `test_after.log`
  - Log summary: `string authority guard passed: 235 classified declaration-level hits`
- `python3 scripts/test_string_authority_guard.py`
  - Result: passed, `Ran 3 tests`, `OK`.
- `ctest --test-dir build -R string_authority_guard --output-on-failure`
  - Result: passed, `100% tests passed, 0 tests failed out of 2`.
- `git diff --check -- todo.md`
  - Result: passed.
