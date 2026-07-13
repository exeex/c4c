# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 11
Current Step Title: Reconcile legacy and core dispositions

## Just Finished

- Plan Step 11 is complete: `LEGACY_COVERAGE.md` now uses 44 mechanically
  extractable full-match path regexes that cover all 164 current files under
  `src/backend/legacy/` exactly once, with no zero-match ledger entry.
- Nested `prealloc/prepared_printer/`, `prealloc/regalloc/`, and
  `prealloc/stack_layout/` files have explicit dispositions. Mixed regalloc
  behavior is split among E1 liveness/interference, E2 assignment, E3 spill
  state, D2 call transport, and D5 phi/copy behavior; legacy coordinators,
  caches, lookup authority, and special-case repair routes are rejected.
- Address preparation is C6 throughout the ledger/core map, while C5 remains
  the variadic owner and its product is the valid immediate input to C6. E4
  fixes exact frame placements and registered mapping-rule facts; E2 owns
  abstract homes, C2 supplies the abstract-to-concrete domain, and F1 chooses
  and applies concrete spelling only within the registered rule. Implementation
  status remains explicitly incomplete.

## Suggested Next

- Execute Plan Step 12: reconcile the root/index contracts and run the complete
  documentation proof before requesting the repeated independent review.

## Watchouts

- Keep the ledger checker synchronized with its documented table syntax:
  first-column Python regexes have their Markdown `\|` escapes removed, then
  are applied with `re.fullmatch` relative to `src/backend/legacy/`.
- Any new legacy file or stale path expression must fail the exact-one-match
  proof. Do not broaden a pattern across capabilities with different owners.
- Preserve the repaired E1-E4/F1 boundary and do not restore C5 address
  ownership or downstream MIR allocation/frame repair. The valid `C5 facts`
  input cell for C6 is pipeline adjacency, not address ownership.

## Proof

- Passed `git diff --check`; the refined negative proof rejecting only claims
  that C5 owns address planning/preparation; the positive C5-to-C6, E2/C2,
  E4-frame-plan, and constrained-F1 boundary checks; and the delegated positive
  nested-family/owner search.
- The refined owner proof was ``! rg -ni 'address
  (planning|preparation) (belongs to|is owned by) `?C5`?|`?C5`?
  (owns|is the owner of) address (planning|preparation)'`` over the ledger,
  core, and root contracts. It intentionally admits the root `C5 facts` input
  cell for C6.
- Passed this exact deterministic inventory command with result
  `legacy inventory PASS: 164 files, 44 nonempty dispositions, exactly one match per file`:

```bash
python3 - <<'PY'
from pathlib import Path
import re

root = Path('src/backend/legacy')
ledger = Path('src/backend/bir/LEGACY_COVERAGE.md')
bir_root = Path('src/backend/bir')
root_index = (bir_root / 'README.md').read_text()
files = sorted(str(path.relative_to(root)) for path in root.rglob('*') if path.is_file())
rows = []
row_re = re.compile(r'^\| `([^`]*)` \| (.*?) \| (.*?) \|$')
for line in ledger.read_text().splitlines():
    if not line.startswith('| `'):
        continue
    match = row_re.fullmatch(line)
    assert match, f'malformed ledger row: {line}'
    pattern, disposition, owner = match.groups()
    pattern = pattern.replace(r'\|', '|')
    re.compile(pattern)
    rows.append((pattern, disposition, owner))
assert rows, 'no ledger rows'
matched = {pattern: [] for pattern, _, _ in rows}
for path in files:
    hits = [pattern for pattern, _, _ in rows if re.fullmatch(pattern, path)]
    assert len(hits) == 1, f'{path}: expected one disposition, got {hits}'
    matched[hits[0]].append(path)
for pattern, hits in matched.items():
    assert hits, f'zero-match disposition: {pattern}'
for pattern, disposition, owner in rows:
    if 'Accepted' not in disposition:
        continue
    assert owner.count('Producer/owner:') == 1, f'{pattern}: Accepted row needs one Producer/owner:'
    docs = re.findall(r'`([^`]*README\.md)`', owner)
    assert docs, f'{pattern}: Accepted owner has no indexed README path'
    producer_doc = docs[0]
    assert (bir_root / producer_doc).is_file(), f'{pattern}: missing producer owner {producer_doc}'
    assert f'({producer_doc})' in root_index, f'{pattern}: producer owner is not root-indexed: {producer_doc}'
print(f'legacy inventory PASS: {len(files)} files, {len(rows)} nonempty dispositions, exactly one match per file')
PY
```

- No build or test subset applies to this docs-only packet. Per the delegated
  boundary, regression logs were not created or modified.
