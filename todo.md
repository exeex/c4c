# Current Packet

Status: Active
Source Idea Path: ideas/open/736_bir_phase_b_canonical_document_convergence.md
Source Plan Path: plan.md
Current Step ID: 9
Current Step Title: Prove phase-C acceptance and Child-B completion

## Just Finished

- Completed plan Step 8 in exact required order: pass framework, ordered
  pipeline, then read-only Canonical verifier audit.
- Converged the pass framework to uniform metadata/core-first structure with
  eight-row input/output matrices and a seven-row closed P01-P07 descriptor
  matrix. It owns invocation transactions, occurrence support, analysis access/
  invalidation, verification hooks and deterministic execution support, but
  cannot choose order, define verifier semantics or mint `CanonicalBir`.
- Corrected and proof-locked the accepted descriptor kind vector to `Module`,
  `Function`, `Function`, `Function`, `Module`, `Module`, `Module`. P01 is a
  module pass because globals, declarations/definitions and shared type/symbol
  tables publish through one whole-module transaction.
- Retained the previously accepted Raw/Canonical target-context paragraph
  byte-identically while making target/profile/layout/ABI/helper/preparation/
  allocation exclusion explicit throughout the framework contract.
- Converged the pipeline to uniform metadata/core-first structure with eight
  input rows, six output rows, the exact eight-row P01-P07+B8 occurrence table
  and seven exact stamp/fingerprint axes. The pipeline alone owns configured
  order, lineage, checkpoints, re-entry, rollback and capability transitions.
- Defined exact move-only checkpoint/candidate behavior, resume strictly after
  the completed ordinal, unconditional B8, atomic last-good rollback and the
  same-owner frozen B7-to-B8 boundary. No partial/mixed state or earlier report
  can become Canonical success.
- Audited the read-only Canonical verifier and found no required change. It
  already checks exact canonical-v1 lineage, plan/options, ordinal 7,
  `IntrinsicsCanonical`, epoch/module/function digest and all cumulative
  P01-P07 obligations on the same frozen candidate; it rejects target/
  preparation/allocation/MIR facts and alone produces the private token from
  which the pipeline mints exactly one immutable `CanonicalBir`.
- Corrected implementation truth to `partial-foundation`: build-included
  cancellation/budget, revision/digest/stamp, consume-Raw, private fork/view/
  discard foundations exist; pass registry/sessions/analysis integration,
  runner/resume/serialization, B7 freeze, B8 integration and Canonical minting
  remain absent. No verifier seam remains, so the packet advances to Step 9.

## Suggested Next

- Execute plan Step 9: build the exhaustive phase-B input/output and adjacency
  proof, prove exact phase-C acceptance, run final documentation/implementation-
  truth checks, then request plan-owner completion judgment.

## Watchouts

- Step 9 must prove phase C receives only the verified move-only
  target-independent unallocated `CanonicalBir`, with no Raw alias, stale
  analysis, target fact, ABI placement, home or spill state.
- Do not treat runbook exhaustion or the green Step-8 verifier audit as source-
  idea completion; plan-owner must make the completion judgment.
- Framework/pipeline implementation is only partial foundation. Documentation
  convergence must not be reported as callable P01-P07/B8 implementation.

## Proof

- Documentation-only packet. The delegated scope forbids code, tests, builds
  and log edits; canonical regression logs were not touched.
- Exact scope/diff-check, metadata/order/matrices, authority separation,
  P01-P07 order/stamps/rollback/re-entry, same-revision verifier gate, links and
  implementation-truth proof:

```bash
python3 - <<'PY'
from pathlib import Path
import re
import subprocess

framework_path = Path('src/backend/bir/passes/README.md')
pipeline_path = Path('src/backend/bir/pipeline/README.md')
verify_path = Path('src/backend/bir/verify/README.md')
framework = framework_path.read_text()
pipeline = pipeline_path.read_text()
verify = verify_path.read_text()
root = Path('src/backend/bir/README.md').read_text()
analysis = Path('src/backend/bir/analysis/README.md').read_text()

def ordered(text, headings):
    positions = [text.index(heading) for heading in headings]
    assert positions == sorted(positions), (headings, positions)

core = ['## Purpose', '## Owns', '## Does Not Own', '## Inputs',
        '## Outputs', '## Adjacent-Stage Contract']
framework_details = ['## Closed Canonical Descriptor Matrix',
    '## Ordered Behavior', '## Invariants',
    '## Transaction, Failure, and Rollback',
    '## Analysis Access and Invalidation',
    '## Verification and Publication Support', '## Target and ABI Rules',
    '## Re-entry, Repetition, and Orchestration Support',
    '## Implementation State', '## Proof Requirements',
    '## Open Questions', '## Review Checklist']
pipeline_details = ['## Exact Built-In Occurrence Sequence',
    '## Stage Stamp and Fingerprint Matrix', '## Ordered Behavior',
    '## Invariants', '## Failure, Rollback, and Re-entry',
    '## Verification and Canonical Publication', '## Target and ABI Rules',
    '## Implementation State', '## Proof Requirements',
    '## Open Questions', '## Review Checklist']
for text, kind, phase in (
        (framework, 'framework',
         'Applies-To: B1 / P01 through B8 orchestration support'),
        (pipeline, 'pipeline', 'Phase-ID: B1 through B8')):
    head = '\n'.join(text.splitlines()[:11])
    for item in ('Contract-Status: under-review',
                 'Implementation-Status: partial-foundation', f'Kind: {kind}',
                 phase, 'Upstream:', 'Downstream:', 'Owner-Path:',
                 'Last-Reconciled-Commit:'):
        assert item in head, (item, head)
    ordered(text, core)
    assert '- [ ]' not in text
ordered(framework, framework_details)
ordered(pipeline, pipeline_details)

def rows(text, start, end):
    section = text.split(start, 1)[1].split(end, 1)[0]
    result = []
    for line in section.splitlines():
        if line.startswith('|'):
            cells = [cell.strip() for cell in line.split('|')[1:-1]]
            if cells and cells[0] and not set(cells[0]) <= {'-', ':'}:
                result.append(cells)
    return result[1:]

fin = rows(framework, '### Exact framework input matrix', '## Outputs')
fout = rows(framework, '### Exact framework output matrix',
            '## Adjacent-Stage Contract')
descriptors = rows(framework, '## Closed Canonical Descriptor Matrix',
                   '## Ordered Behavior')
pin = rows(pipeline, '### Exact pipeline input matrix', '## Outputs')
pout = rows(pipeline, '### Exact pipeline output matrix',
            '## Adjacent-Stage Contract')
sequence = rows(pipeline, '## Exact Built-In Occurrence Sequence',
                '## Stage Stamp and Fingerprint Matrix')
stamps = rows(pipeline, '## Stage Stamp and Fingerprint Matrix',
              '## Ordered Behavior')
assert tuple(map(len, (fin, fout, descriptors, pin, pout, sequence, stamps))) == (
    8, 8, 7, 8, 6, 8, 7)

expected_ids = [
    'B1 / P01 `Legalize`', 'B2 / P02 `ScalarCanonicalize`',
    'B3 / P03 `CfgCanonicalize`', 'B4 / P04 `SsaCanonicalize`',
    'B5 / P05 `MemoryCanonicalize`',
    'B6 / P06 `AggregateCanonicalize`',
    'B7 / P07 `IntrinsicCanonicalize`']
assert [row[0] for row in descriptors] == expected_ids
expected_kinds = [
    'Module', 'Function', 'Function', 'Function',
    'Module', 'Module', 'Module']
assert [row[1] for row in descriptors] == expected_kinds
# The accepted pre-convergence scope table and local owner transaction shapes
# independently agree with the descriptor kind vector.
old_pipeline = subprocess.check_output(
    ['git', 'show', 'HEAD:src/backend/bir/pipeline/README.md'], text=True)
scope_section = old_pipeline.split('The initial scope plan is:', 1)[1].split(
    'No function pass may', 1)[0]
scope_kinds = []
for name in ('legalize', 'scalar', 'cfg', 'ssa', 'memory', 'aggregate',
             'intrinsics'):
    match = re.search(rf'^\| `{name}` \| `(Module|Function)` \|',
                      scope_section, re.M)
    assert match, name
    scope_kinds.append(match.group(1))
assert scope_kinds == expected_kinds, scope_kinds
local_owner_paths = [
    'legalize', 'scalar', 'cfg', 'ssa', 'memory', 'aggregate', 'intrinsics']
local_owners = {
    name: Path(f'src/backend/bir/passes/{name}/README.md').read_text()
    for name in local_owner_paths}
assert 'one private module occurrence transaction' in local_owners['legalize']
for name in ('scalar', 'cfg', 'ssa'):
    assert 'function wave' in local_owners[name], name
for name in ('memory', 'aggregate', 'intrinsics'):
    assert 'whole-module occurrence transaction' in local_owners[name], name
expected_sequence = [
    'B1 / P01', 'B2 / P02', 'B3 / P03', 'B4 / P04',
    'B5 / P05', 'B6 / P06', 'B7 / P07', 'B8']
assert [row[0] for row in sequence] == expected_sequence

for phrase in (
    'provides orchestration support but never chooses the\nordered pipeline',
    'one private transaction per invocation',
    'atomic module occurrence\n  barriers',
    'Every revision increment stales old handles',
    'cannot reorder P01-P07, skip B8',
    'cannot call that object `CanonicalBir`',
    'Implementation is partial foundation only'):
    assert phrase in framework, phrase
for phrase in (
    'sole configured order/capability owner',
    'resume_bir_pipeline',
    'Resume starts strictly\nafter the completed ordinal',
    'always runs B8',
    'Only the completely green\nprivate token permits the pipeline to mint exactly one immutable',
    'same owner frozen by P07',
    'Implementation is partial foundation only'):
    assert phrase in pipeline, phrase

# Preserve the previously accepted target-context paragraph byte-identically.
old = subprocess.check_output(
    ['git', 'show', 'HEAD:src/backend/bir/passes/README.md'], text=True)
start = ('Raw and Canonical BIR may preserve source-semantic typed sizes, '
         'alignments, and\n')
accepted = start + old.split(start, 1)[1].split('\n\n', 1)[0]
assert accepted in framework

# Framework, pipeline and verifier authorities are disjoint and adjacent.
assert 'owns verifier semantics' in framework
assert 'pipeline alone defines last-good retention' in framework
assert 'pass framework | choose P01-P07 order' in pipeline
assert 'verifier | schedule passes' in pipeline
assert ('The `Canonical` profile is the exact cumulative profile of the '
        'immutable B7 /\nP07 output') in verify
for obligation in (
    '1. P01 legal semantic types', '2. P02 scalar expressions',
    '3. P03 terminators', '4. P04 explicit-`Phi`',
    '5. P05 memory', '6. P06 aggregate values',
    '7. P07 intrinsic registry identity'):
    assert obligation in verify, obligation
for phrase in (
    'same owning revision frozen by P07',
    '`ModuleEpoch`/`ModuleRevision`',
    '(FunctionId, FunctionRevision)',
    'rejects every stage-forbidden\nprepared, calling-placement, allocation',
    'verifier-private token to mint exactly one immutable `CanonicalBir`',
    'No\nearlier green report can be cached across a P07 edit'):
    assert phrase in verify, phrase
assert ('B8` | Canonical verification and publication | stamped `P07` output'
        ) in root
assert 'After any revision increment, every old handle is\nstale' in analysis

# Links resolve and checked-in/build-included foundation truth is exact.
for path, text in ((framework_path, framework), (pipeline_path, pipeline)):
    for link in re.findall(r'\[[^]]+\]\(([^)]+)\)', text):
        assert (path.parent / link).resolve().exists(), (path, link)
required_files = [
    'src/backend/bir/passes/execution_control.cpp',
    'src/backend/bir/passes/execution_control.hpp',
    'src/backend/bir/pipeline/identity.cpp',
    'src/backend/bir/pipeline/identity.hpp',
    'src/backend/bir/pipeline/checkpoint_internal.cpp',
    'src/backend/bir/pipeline/checkpoint_internal.hpp']
assert all(Path(path).exists() for path in required_files)
cmake = Path('src/backend/CMakeLists.txt').read_text()
assert 'file(GLOB_RECURSE C4C_BACKEND_BIR_SOURCES' in cmake
cpp = '\n'.join(Path(path).read_text(errors='ignore')
                for path in Path('src/backend/bir').rglob('*.cpp'))
assert 'compute_function_revision_digest' in cpp
assert 'CancellationToken::checkpoint' in cpp
assert 'consume_verified_raw' in cpp and 'fork_occurrence' in cpp
assert 'run_bir_pipeline' not in cpp
assert 'PassId::Legalize' not in cpp

changed = set(subprocess.check_output(
    ['git', 'diff', '--name-only'], text=True).splitlines())
expected = {
    'src/backend/bir/passes/README.md',
    'src/backend/bir/pipeline/README.md',
    'todo.md',
}
assert changed == expected, (changed, expected)
assert not subprocess.check_output(
    ['git', 'diff', '--name-only', '--', verify_path.as_posix()], text=True)
print('PASS step=8 framework=8/8/7 pipeline=8/6 sequence=8 stamps=7 '
      'kinds=M,F,F,F,M,M,M '
      'order=P01-P07-B8 rollback=atomic reentry=exact authorities=separate '
      'verifier=same-revision-cumulative canonical=one '
      'implementation=partial-foundation scope=3 verifier_edits=0 next=9')
PY
git diff --check
```

- Result:

```text
PASS step=8 framework=8/8/7 pipeline=8/6 sequence=8 stamps=7 kinds=M,F,F,F,M,M,M order=P01-P07-B8 rollback=atomic reentry=exact authorities=separate verifier=same-revision-cumulative canonical=one implementation=partial-foundation scope=3 verifier_edits=0 next=9
git diff --check: PASS
```
