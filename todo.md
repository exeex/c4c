# Current Packet

Status: Active
Source Idea Path: ideas/open/736_bir_phase_b_canonical_document_convergence.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Converge CFG analysis and B3 P03 CFG

## Just Finished

- Completed plan Step 2 by converging the comparison/select analysis before its
  earliest consumer, B2/P02 scalar canonicalization.
- Defined `ComparisonSelect` as an immutable, non-authoritative analysis over
  the exact P01 `TypesLegal` revision. Its complete key includes analysis and
  schema identity, module epoch/revision, function identity/revision, options,
  semantic inputs and an explicitly empty dependency key; it has no target,
  ABI, preparation or allocation axis.
- Added exhaustive analysis input/output matrices for known, absent and
  reasoned-unknown comparison/select facts, including empty functions,
  malformed-input failure, stale-handle rejection, exact-key preservation and
  mutation-derived invalidation. P02 is the earliest consumer.
- Converged P02 to exact matching P01 and analysis inputs with exhaustive
  scalar/comparison/cast/select disposition coverage: 14 `Normalize`, ten
  `Preserve` and three `Reject` rows. Every row records a stable result/failure,
  next-owner visibility and truthful absent implementation status.
- Defined one deterministic all-function atomic transaction, cumulative
  `RawVerified` + `TypesLegal` + `ScalarsCanonical` postconditions, complete
  rollback, mutation-derived invalidation and an exact immutable B3 handoff.
- Confirmed implementation remains absent: each owner directory contains only
  its contract and no checked-in C/C++ definition names the analysis or pass.
  No shared-owner edit or unresolved Step-2 seam was required.

## Suggested Next

- Execute plan Step 3 in required order: converge exact-revision CFG analysis,
  then B3/P03 CFG, and specify recomputation from the resulting terminators.

## Watchouts

- CFG facts remain derived and non-authoritative; terminators are the sole
  successor authority and stable semantic IDs cannot be replaced by dense
  indices, pointers, names, positions or rendered text.
- B3 must consume the exact P02 `ScalarsCanonical` revision and its matching CFG
  result, publish atomically, then require fresh CFG analysis for changed
  terminators.
- Keep target/profile/helper/ABI/preparation/allocation authority out of Step 3.
  P01, comparison analysis and P02 implementations are absent; the contracts
  do not claim runtime coverage.

## Proof

- Docs-only packet: no canonical regression log applies and neither
  `test_before.log` nor `test_after.log` changed.
- Metadata/core/matrix/order, exact keys, exhaustive dispositions, atomicity,
  invalidation, B3 handoff, links, implementation truth and exact scope proof:

```bash
python3 - <<'PY'
from collections import Counter
from pathlib import Path
import re
import subprocess

analysis_path = Path('src/backend/bir/analysis/comparison/README.md')
scalar_path = Path('src/backend/bir/passes/scalar/README.md')
analysis = analysis_path.read_text()
scalar = scalar_path.read_text()
framework = Path('src/backend/bir/analysis/README.md').read_text()
p01 = Path('src/backend/bir/passes/legalize/README.md').read_text()
b3 = Path('src/backend/bir/passes/cfg/README.md').read_text()

def ordered(text, headings):
    positions = [text.index(heading) for heading in headings]
    assert positions == sorted(positions), (headings, positions)

core = ['## Purpose', '## Owns', '## Does Not Own', '## Inputs',
        '## Outputs', '## Adjacent-Stage Contract']
analysis_details = ['## Ordered Behavior', '## Invariants',
                    '## Failure and Diagnostics',
                    '## Analysis and Invalidation',
                    '## Target and ABI Rules', '## Implementation State',
                    '## Proof Requirements', '## Open Questions',
                    '## Review Checklist']
pass_details = ['## Ordered Behavior', '## Invariants',
                '## Exhaustive P02 Scalar-Form Disposition Matrix',
                '## Verification and Publication',
                '## Failure and Diagnostics',
                '## Analysis and Invalidation',
                '## Target and ABI Rules', '## Implementation State',
                '## Proof Requirements', '## Open Questions',
                '## Review Checklist']
for text, kind, phase in ((analysis, 'analysis', 'B2 / P02 earliest consumer'),
                          (scalar, 'pass', 'B2 / P02')):
    head = '\n'.join(text.splitlines()[:11])
    for item in ('Contract-Status: under-review',
                 'Implementation-Status: absent', f'Kind: {kind}',
                 f'Phase-ID: {phase}' if kind == 'pass'
                 else f'Applies-To: {phase}',
                 'Upstream:', 'Downstream:', 'Owner-Path:',
                 'Last-Reconciled-Commit:'):
        assert item in head, (item, head)
    ordered(text, core)
    assert '- [ ]' not in text
ordered(analysis, analysis_details)
ordered(scalar, pass_details)

def rows(text, start, end):
    section = text.split(start, 1)[1].split(end, 1)[0]
    result = []
    for line in section.splitlines():
        if line.startswith('|'):
            cells = [cell.strip() for cell in line.split('|')[1:-1]]
            if cells and cells[0] and not set(cells[0]) <= {'-', ':'}:
                result.append(cells)
    return result[1:]  # header

analysis_inputs = rows(analysis,
    '### Exact descriptor and input-key matrix', '## Outputs')
analysis_outputs = rows(analysis,
    '### Exhaustive result-family matrix', '## Adjacent-Stage Contract')
scalar_inputs = rows(scalar,
    '### Exact input and dependency matrix', '## Outputs')
scalar_outputs = rows(scalar,
    '### Exact output handoff matrix', '## Adjacent-Stage Contract')
forms = rows(scalar,
    '## Exhaustive P02 Scalar-Form Disposition Matrix',
    '## Verification and Publication')
assert (len(analysis_inputs), len(analysis_outputs), len(scalar_inputs),
        len(scalar_outputs), len(forms)) == (8, 7, 8, 5, 27)
assert all(len(row) == 6 and all(row) for row in forms)
counts = Counter(row[2] for row in forms)
assert counts == {'Normalize': 14, 'Preserve': 10, 'Reject': 3}, counts
for row in forms:
    assert '`' in row[3], row
    assert row[5] == 'absent', row

for phrase in (
    '`AnalysisId::ComparisonSelect`, schema version 1',
    '`CanonicalSemantic` analysis', 'P01 `TypesLegal` revision',
    'exact `ModuleEpoch` plus `ModuleRevision`',
    'exact `FunctionId` plus `FunctionRevision`',
    'empty `dependencies` set', 'no target/preparation axes',
    'Emit `Known`, `Absent` or stable-reason `Unknown`',
    'requested immediately before B2/P02',
    'returns `StaleAnalysis`;\nit never rebinds',
    'install a semantically equal immutable result under the new\ncomplete key'):
    assert phrase in analysis, phrase
for phrase in (
    '`PassId::ScalarCanonicalize`', '`TypesLegal`',
    '`AnalysisId::ComparisonSelect`',
    'one private transaction per function in the occurrence wave',
    'rolls back the entire wave',
    'retains\n`RawVerified`/`TypesLegal`, and establishes `ScalarsCanonical`',
    'publishes no revision, property, partial fold/function prefix',
    'B3 requests fresh exact-B2 CFG facts in Step 3',
    "B3's exact input clause requires this same immutable function view"):
    assert phrase in scalar, phrase

# Framework and adjacent accepted-owner clauses agree on exact-revision keys.
assert 'one stable numeric `AnalysisId`, one schema\nversion' in framework
assert 'it never retargets' in framework
assert '`TypesLegal`' in p01
assert '`PassProperty::ScalarsCanonical`' in b3
assert ('P02\npostconditions and the configured input verifier must accept that same\n'
        'revision') in b3

# All relative links resolve and implementation storage/build truth remains absent.
for path, text in ((analysis_path, analysis), (scalar_path, scalar)):
    for link in re.findall(r'\[[^]]+\]\(([^)]+)\)', text):
        assert (path.parent / link).resolve().exists(), (path, link)
assert [p.name for p in analysis_path.parent.iterdir()] == ['README.md']
assert [p.name for p in scalar_path.parent.iterdir()] == ['README.md']
cpp_hits = []
for path in Path('src').rglob('*'):
    if path.suffix in ('.cpp', '.hpp'):
        text = path.read_text(errors='ignore')
        if ('AnalysisId::ComparisonSelect' in text or
                'PassId::ScalarCanonicalize' in text):
            cpp_hits.append(path.as_posix())
assert not cpp_hits, cpp_hits

changed = set(subprocess.check_output(
    ['git', 'diff', '--name-only'], text=True).splitlines())
expected = {
    'src/backend/bir/analysis/comparison/README.md',
    'src/backend/bir/passes/scalar/README.md',
    'todo.md',
}
assert changed == expected, (changed, expected)
print('PASS step=2 analysis_inputs=8 analysis_outputs=7 scalar_inputs=8 '
      'scalar_outputs=5 forms=27 normalize=14 preserve=10 reject=3 '
      'transaction=atomic invalidation=exact-key b3=exact '
      'implementation=absent scope=3 next=3')
PY
git diff --check
```

- Result:

```text
PASS step=2 analysis_inputs=8 analysis_outputs=7 scalar_inputs=8 scalar_outputs=5 forms=27 normalize=14 preserve=10 reject=3 transaction=atomic invalidation=exact-key b3=exact implementation=absent scope=3 next=3
git diff --check: PASS
```
