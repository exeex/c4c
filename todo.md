# Current Packet

Status: Active
Source Idea Path: ideas/open/736_bir_phase_b_canonical_document_convergence.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Converge dominance/publication analyses and B4 P04 SSA

## Just Finished

- Completed plan Step 3 in the required order: exact-revision CFG analysis
  first, then B3/P03 CFG canonicalization.
- Defined `AnalysisId::Cfg` as a function-scope, schema-1,
  `CanonicalSemantic` immutable analysis over the exact P02
  `ScalarsCanonical` checkpoint. Its closed key binds epoch, module/function
  revisions, empty dependencies/options and no target/preparation axes.
- Added exhaustive analysis input/output matrices for terminator-derived
  successor and incoming occurrences, exact `EdgeKey {source, role, index}`
  multiplicity, reachability, unreachable blocks, traversal products,
  optional empty forms, unsupported/malformed errors, stable-ID identity,
  stale rejection and precise invalidation/preservation.
- Converged P03 with exact matching P02/CFG inputs and 28 closed CFG-form rows:
  14 `Normalize`, nine `Preserve` and five `Reject`. Terminators remain the
  sole stored successor authority; parallel occurrences and phi incoming keys
  remain distinct.
- Defined deterministic full-wave private transactions, complete rollback,
  cumulative `RawVerified` + `TypesLegal` + `ScalarsCanonical` +
  `CfgCanonical` postconditions and mutation-derived transitive invalidation.
- Required CFG recomputation from every resulting P03 terminator set before
  exact-B3 dominance/publication/B4 use. The pre-planning B2 handle cannot
  substitute, even for an unchecked preservation claim.
- Confirmed implementation remains absent: both owner directories contain only
  their contracts and no checked-in C/C++ definition names the CFG analysis or
  pass. No shared-owner seam was found, so the packet advances to Step 4.

## Suggested Next

- Execute plan Step 4 in required order: converge exact-B3 CFG-dependent
  dominance and publication analyses, then B4/P04 SSA and its B5 handoff.

## Watchouts

- Step 4 must consume the freshly recomputed exact-B3 CFG result; stale
  pre-P03 facts, dense indices, predecessor-block sets, names, positions and
  rendered text cannot establish dominance, availability or phi identity.
- Preserve exact parallel `EdgeKey` multiplicity through dominance frontiers
  and SSA incoming coverage. P04 may construct SSA but cannot mutate topology
  or invent incoming values without its registered rule.
- CFG analysis and P03 implementations are absent. Keep target/profile/helper/
  ABI/preparation/allocation authority and runtime-coverage claims out.

## Proof

- Documentation-only packet. The delegated scope explicitly forbids code,
  tests, builds and log edits; canonical regression logs were not touched.
- Exact scope/diff-check, metadata/order/matrices, CFG key/multiplicity, closed
  dispositions, atomicity/invalidation, mandatory recomputation, B4 handoff,
  links and implementation-truth proof:

```bash
python3 - <<'PY'
from collections import Counter
from pathlib import Path
import re
import subprocess

analysis_path = Path('src/backend/bir/analysis/cfg/README.md')
pass_path = Path('src/backend/bir/passes/cfg/README.md')
analysis = analysis_path.read_text()
cfg_pass = pass_path.read_text()
framework = Path('src/backend/bir/analysis/README.md').read_text()
root = Path('src/backend/bir/README.md').read_text()
dominance = Path('src/backend/bir/analysis/dominance/README.md').read_text()
ssa = Path('src/backend/bir/passes/ssa/README.md').read_text()

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
                '## Exhaustive P03 CFG-Form Disposition Matrix',
                '## Verification and Publication',
                '## Failure and Diagnostics',
                '## Analysis and Invalidation',
                '## Target and ABI Rules', '## Implementation State',
                '## Proof Requirements', '## Open Questions',
                '## Review Checklist']
for text, kind, phase in (
        (analysis, 'analysis',
         'B3 / P03 earliest planning consumer and mandatory post-P03 recomputation'),
        (cfg_pass, 'pass', 'B3 / P03')):
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
ordered(cfg_pass, pass_details)

def rows(text, start, end):
    section = text.split(start, 1)[1].split(end, 1)[0]
    result = []
    for line in section.splitlines():
        if line.startswith('|'):
            cells = [cell.strip() for cell in line.split('|')[1:-1]]
            if cells and cells[0] and not set(cells[0]) <= {'-', ':'}:
                result.append(cells)
    return result[1:]

analysis_inputs = rows(analysis,
    '### Exact descriptor and input-key matrix', '## Outputs')
analysis_outputs = rows(analysis,
    '### Exhaustive CFG result-family matrix', '## Adjacent-Stage Contract')
pass_inputs = rows(cfg_pass,
    '### Exact input and dependency matrix', '## Outputs')
pass_outputs = rows(cfg_pass,
    '### Exact output handoff matrix', '## Adjacent-Stage Contract')
forms = rows(cfg_pass,
    '## Exhaustive P03 CFG-Form Disposition Matrix',
    '## Verification and Publication')
assert (len(analysis_inputs), len(analysis_outputs), len(pass_inputs),
        len(pass_outputs), len(forms)) == (8, 9, 8, 6, 28)
assert all(len(row) == 6 and all(row) for row in forms)
counts = Counter(row[2] for row in forms)
assert counts == {'Normalize': 14, 'Preserve': 9, 'Reject': 5}, counts
for row in forms:
    assert row[2] in ('Normalize', 'Preserve', 'Reject'), row
    assert '`' in row[3] or 'no failure' in row[3], row
    assert row[5] == 'absent', row

for phrase in (
    '`AnalysisId::Cfg`, schema version 1', '`CanonicalSemantic`',
    'exact `ModuleEpoch` plus checkpoint `ModuleRevision`',
    'exact `FunctionId` plus `FunctionRevision`',
    'empty ordered `dependencies` set',
    'canonical empty `AnalysisOptionsFingerprint`',
    'target-layout key `None`; preparation digest empty',
    'EdgeKey { source BlockId, SuccessorRole role, uint32_t index }',
    'parallel-edge multiplicity', 'does not use an `Unknown` graph answer',
    'returns `StaleAnalysis`; it never rebinds',
    'P03 deliberately requires post-mutation CFG recomputation'):
    assert phrase in analysis, phrase
for phrase in (
    '`PassId::CfgCanonicalize`', '`AnalysisId::Cfg`',
    '`RawVerified` + `TypesLegal` + `ScalarsCanonical`',
    'one private transaction per function in the occurrence wave',
    'Failure rolls back the\nentire wave',
    'establishes `CfgCanonical` only after',
    'pre-planning B2 handle or patched cache cannot substitute',
    'Recompute CFG from every resulting\n   terminator set',
    'B4 requires that\nsame immutable revision, `CfgCanonical`, exact-current CFG'):
    assert phrase in cfg_pass, phrase

# Read-only shared/downstream owners agree with the converged boundary.
assert ('before `B3/P03` planning; recomputed from the resulting terminators '
        'afterward') in root
assert 'dependencies: { AnalysisId::Cfg schema 1 }' in dominance
assert ('The input is one immutable function view with exact epoch/module/function\n'
        'revision and `PassProperty::CfgCanonical`') in ssa
assert 'Exact-revision CFG and dominance\nhandles are required' in ssa
assert 'one stable numeric `AnalysisId`, one schema\nversion' in framework
assert 'After any revision increment, every old handle is\nstale' in framework

for path, text in ((analysis_path, analysis), (pass_path, cfg_pass)):
    for link in re.findall(r'\[[^]]+\]\(([^)]+)\)', text):
        assert (path.parent / link).resolve().exists(), (path, link)
assert [p.name for p in analysis_path.parent.iterdir()] == ['README.md']
assert [p.name for p in pass_path.parent.iterdir()] == ['README.md']
cpp_hits = []
for path in Path('src').rglob('*'):
    if path.suffix in ('.cpp', '.hpp'):
        text = path.read_text(errors='ignore')
        if ('AnalysisId::Cfg' in text or
                'PassId::CfgCanonicalize' in text):
            cpp_hits.append(path.as_posix())
assert not cpp_hits, cpp_hits

changed = set(subprocess.check_output(
    ['git', 'diff', '--name-only'], text=True).splitlines())
expected = {
    'src/backend/bir/analysis/cfg/README.md',
    'src/backend/bir/passes/cfg/README.md',
    'todo.md',
}
assert changed == expected, (changed, expected)
print('PASS step=3 analysis_inputs=8 analysis_outputs=9 cfg_inputs=8 '
      'cfg_outputs=6 forms=28 normalize=14 preserve=9 reject=5 '
      'edge_identity=role-index multiplicity=exact transaction=atomic '
      'invalidation=transitive recompute=mandatory b4=exact '
      'implementation=absent scope=3 next=4')
PY
git diff --check
```

- Result:

```text
PASS step=3 analysis_inputs=8 analysis_outputs=9 cfg_inputs=8 cfg_outputs=6 forms=28 normalize=14 preserve=9 reject=5 edge_identity=role-index multiplicity=exact transaction=atomic invalidation=transitive recompute=mandatory b4=exact implementation=absent scope=3 next=4
git diff --check: PASS
```
