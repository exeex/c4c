# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 12
Current Step Title: Reconcile the root and run the complete documentation proof

## Just Finished

- Plan Step 12 is complete: all 44 BIR Markdown contracts and the normative MIR
  root were re-audited against the repaired architecture. The root has 43
  subordinate links exactly once, the ordered 31-row A1-F3 registry, one D2
  owner, and no stale S00-S29/G01 authority.
- The repeated proof found and corrected one bounded D5 transcription mismatch:
  `passes/out_of_ssa/README.md` now names the E4-owned non-mutating frame
  transaction between exact-current E3 spill validation and final target
  realizability. Its atomic closure now explicitly installs six products:
  projection, E1, E2, E3, frame realization, and target realizability.
- The four repeated-review blockers are synchronized across their named owners:
  exact-current resolved products, frame-aware one-record closure, distinct
  allocation-free/public and assigned/private verifier intervals, and the
  exact 164-file/44-disposition legacy inventory with C6 address ownership.
  Implementation remains explicitly deferred/scaffolded; no acceptance marker,
  implementation change, test-expectation change, or testcase-shaped exception
  was introduced.

## Suggested Next

- Execute repeated Plan Step 13: independently review the exact current 44-file
  architecture plus normative MIR boundary and record the judgment under
  `review/`. Do not enter Step 14 unless that review reports no blocker.

## Watchouts

- The existing transient `review/731_full_architecture_review_repeat.md` is the
  failed prior review and remains outside this packet. The new reviewer must
  review the post-Step-12 document state, including the corrected six-product
  D5 closure, rather than treating the prior report as current acceptance.
- Preserve the exact closure order: projection -> E1 -> E2 -> E3 -> E4-owned
  frame realization -> target realizability. The final target key incorporates
  the exact frame key; no predecessor product can be relabeled current.
- Preserve `PseudoPublicationGate` as allocation-free and
  `AssignedAllocationCandidateGate` as the private assigned retry/resolved
  interval. F1 applies the fixed frame/mapping plan and cannot choose or repair.

## Proof

- Passed `git diff --check`.
- Passed the deterministic structure/link checker with:
  `structure PASS: 44 files, 43 root links exactly once, 31 ordered A1-F3 rows,
  D2 cardinality 1, 45-doc local-link audit`.
- Passed the exact legacy checker with:
  `legacy inventory PASS: 164 files, 44 nonempty dispositions, exactly one
  match per file; Accepted owners present and indexed`.
- Passed focused positive and negative semantic assertions with:
  `semantic PASS: four repaired blockers, strict F1 boundary, implementation
  honesty, and anti-overfit wording are synchronized; all focused negatives
  absent`.
- The executable structure proof was:

```bash
python3 - <<'PY'
from pathlib import Path
import re
root = Path('src/backend/bir')
docs = sorted(root.rglob('*.md'))
assert len(docs) == 44
index = (root / 'README.md').read_text()
sub = [p.relative_to(root).as_posix() for p in docs if p != root / 'README.md']
for rel in sub:
    assert len(re.findall(r'\]\(' + re.escape(rel) + r'(?:#[^)]*)?\)', index)) == 1, rel
expected = [*(f'A{i}' for i in range(1, 3)), *(f'B{i}' for i in range(1, 9)),
            *(f'C{i}' for i in range(1, 10)), *(f'D{i}' for i in range(1, 6)),
            *(f'E{i}' for i in range(1, 5)), *(f'F{i}' for i in range(1, 4))]
assert re.findall(r'^\| `([A-F]\d+)` \|', index, re.M) == expected
assert index.count('(passes/call_lowering/README.md)') == 1
checked = docs + [Path('src/backend/mir/README.md')]
for doc in checked:
    for target in re.findall(r'(?<!!)\[[^\]\n]+\]\(([^)\n]+)\)', doc.read_text()):
        target = target.split('#', 1)[0]
        if not target or '://' in target or target.startswith(('mailto:', '/', '?:')):
            continue
        assert (doc.parent / target).resolve().exists(), (doc, target)
print('structure PASS: 44 files, 43 root links exactly once, 31 ordered A1-F3 rows, D2 cardinality 1, 45-doc local-link audit')
PY
```

- The executable legacy proof was the Step 11 exact checker, rerun unchanged:

```bash
python3 - <<'PY'
from pathlib import Path
import re
root = Path('src/backend/legacy')
ledger = Path('src/backend/bir/LEGACY_COVERAGE.md')
bir = Path('src/backend/bir')
index = (bir / 'README.md').read_text()
files = sorted(str(p.relative_to(root)) for p in root.rglob('*') if p.is_file())
row_re = re.compile(r'^\| `([^`]*)` \| (.*?) \| (.*?) \|$')
rows = []
for line in ledger.read_text().splitlines():
    if line.startswith('| `'):
        match = row_re.fullmatch(line)
        assert match, line
        pattern, disposition, owner = match.groups()
        pattern = pattern.replace(r'\|', '|')
        re.compile(pattern)
        rows.append((pattern, disposition, owner))
matched = {pattern: [] for pattern, _, _ in rows}
for path in files:
    hits = [pattern for pattern, _, _ in rows if re.fullmatch(pattern, path)]
    assert len(hits) == 1, (path, hits)
    matched[hits[0]].append(path)
for pattern, hits in matched.items():
    assert hits, pattern
for pattern, disposition, owner in rows:
    if 'Accepted' in disposition:
        assert owner.count('Producer/owner:') == 1, pattern
        docs = re.findall(r'`([^`]*README\.md)`', owner)
        assert docs and (bir / docs[0]).is_file(), pattern
        assert f'({docs[0]})' in index, pattern
assert len(files) == 164 and len(rows) == 44
print('legacy inventory PASS: 164 files, 44 nonempty dispositions, exactly one match per file; Accepted owners present and indexed')
PY
```

- Focused positive assertions covered `ProjectedConstraintSet`, exact-current
  E1/E2/E3 keys, `FrameRealizationTransaction`/plan/key, final
  `TargetRealizabilityKey`, both verifier gates, nested legacy families, C6,
  and the F1 apply-only boundary. Focused negative assertions covered
  predecessor-keyed current products, F1/MIR frame or allocation repair, stale
  undifferentiated D1-D5 allocation rejection, C5 address ownership,
  `analysis/alias`, stale IDs, unresolved-design headings, and an architecture
  acceptance marker.
- No build or test subset applies to this docs-only packet. Per the delegated
  boundary, `test_before.log` and `test_after.log` were not created or modified.
