# Current Packet

Status: Active
Source Idea Path: ideas/open/732_bir_stage_document_convergence_umbrella.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Audit the queue and hand off to Child A

## Just Finished

- Completed plan Step 3: created exactly six open, documentation-only child
  ideas in strict phase order and the supervisor accepted and committed the
  queue in `9c28f521` (`[idea] add ordered BIR phase convergence ideas`).
- The durable queue is:
  - Child A: `ideas/open/735_bir_phase_a_import_raw_document_convergence.md`
  - Child B: `ideas/open/736_bir_phase_b_canonical_document_convergence.md`
  - Child C: `ideas/open/737_bir_phase_c_preparation_document_convergence.md`
  - Child D: `ideas/open/738_bir_phase_d_pseudo_document_convergence.md`
  - Child E: `ideas/open/739_bir_phase_e_allocation_document_convergence.md`
  - Child F: `ideas/open/740_bir_phase_f_mir_boundary_document_convergence.md`
- Child A durably owns the exact 38 individually named `LirInst` rows, six
  individually named terminator rows, 18 individually named metadata-family
  rows, the finding that no LIR inline-asm carrier edit is currently justified,
  and the strict deferred idea-734 implementation-consumer boundary.
- B through F each require acceptance of their immediate predecessor. Child B
  orders analyses immediately before their earliest real consumers and keeps
  pass-framework support before the pipeline/publication boundary. Child C
  orders external C1 profile selection before C2 layout derivation, C3-C8
  preparation, C8 sequencing support, and C9 constraint authority; rendered
  LIR layout/profile context never becomes Raw/Canonical semantics.
- Idea 734 remains open, deferred, and inactive; idea 733 remains parked under
  `ideas/draft/`; umbrella idea 732 remains open and is still the active source.
  No child authorizes implementation, activation of 734, or closure of 732.

## Suggested Next

- Execute plan Step 4 as a docs-only queue audit. Check all six children
  against the umbrella inventory, uniform matrix contract, exact A-through-F
  predecessor/successor order, shared/external-owner rules, implementation-
  status truth, and final umbrella-audit requirements.
- If the audit passes, ask plan-owner to deactivate this runbook and activate
  Child A (`ideas/open/735_bir_phase_a_import_raw_document_convergence.md`).
  Keep umbrella 732 open, idea 734 deferred/inactive, and draft 733 parked.

## Watchouts

- Child A is the only eligible next activation. Acceptance of the queue is not
  acceptance of Child A's documentation work and does not activate idea 734.
- Recheck the corrected B ordering around comparison/CFG/dominance/
  publication/memory/provenance/call-graph analyses and the corrected C order
  around C1, C2, the six preparation products, C8 support, and C9.
- Shared root, analysis, verifier, diagnostics, compatibility, coverage and
  review-template owners are audited at named touchpoints but are not silently
  reassigned. Missing external C1 and F2/F3 Markdown authorities remain exact
  audit questions, not permission to broaden a child into implementation.
- Step-4 handoff retires this runbook but does not close umbrella idea 732; the
  umbrella remains open through all six children and its final cross-phase
  audit.

## Proof

- Docs-only packet: no canonical regression log applies and neither
  `test_before.log` nor `test_after.log` was changed.
- Read-only structural proof for the exact six paths, statuses/types, required
  headings, A-through-F dependency chain, Child-A 38/6/18 contract, B/C order,
  lifecycle states, and accepting commit:

```bash
python3 - <<'PY'
from pathlib import Path
import re
import subprocess

children = [
    Path('ideas/open/735_bir_phase_a_import_raw_document_convergence.md'),
    Path('ideas/open/736_bir_phase_b_canonical_document_convergence.md'),
    Path('ideas/open/737_bir_phase_c_preparation_document_convergence.md'),
    Path('ideas/open/738_bir_phase_d_pseudo_document_convergence.md'),
    Path('ideas/open/739_bir_phase_e_allocation_document_convergence.md'),
    Path('ideas/open/740_bir_phase_f_mir_boundary_document_convergence.md'),
]
assert all(path.is_file() for path in children)
actual = sorted(
    path for path in Path('ideas/open').glob('*.md')
    if re.match(r'(735|736|737|738|739|740)_', path.name)
)
assert actual == children, (actual, children)

required_headings = {
    '## Goal', '## Why This Exists', '## Scope and Exact Owner Order',
    '## Non-Goals', '## Acceptance and Closure Criteria',
    '## Reviewer Reject Signals',
}
texts = [path.read_text() for path in children]
for path, source in zip(children, texts):
    assert 'Status: Open' in source, path
    assert 'Type: Documentation-only architecture convergence' in source, path
    headings = set(re.findall(r'^## .+$', source, re.M))
    assert required_headings <= headings, (path, required_headings - headings)

for index in range(1, 6):
    predecessor = children[index - 1].as_posix()
    assert f'Predecessor: accepted `{predecessor}`' in texts[index]
for index in range(5):
    successor = children[index + 1].as_posix()
    assert f'Successor: `{successor}`' in texts[index]
assert 'Predecessor: external typed LIR contract' in texts[0]
assert 'Deferred Implementation Consumer: `ideas/open/734_' in texts[0]

inst = re.findall(r'^\| `LirInst::(Lir[A-Za-z0-9_]+)` \|', texts[0], re.M)
term = re.findall(r'^\| `LirTerminator::(Lir[A-Za-z0-9_]+)` \|', texts[0], re.M)
metadata = re.findall(
    r'^\| `([a-z0-9-]+)` \|',
    texts[0].split('## Exhaustive Metadata-Family Intake Contract', 1)[1]
            .split('## Inline Assembly and Deferred Implementation Boundary', 1)[0],
    re.M,
)
assert len(inst) == len(set(inst)) == 38
assert len(term) == len(set(term)) == 6
assert len(metadata) == len(set(metadata)) == 18

def ordered(source, needles):
    positions = [source.index(needle) for needle in needles]
    assert positions == sorted(positions), (needles, positions)

ordered(texts[1], [
    'passes/legalize/README.md', 'comparison/select analysis',
    'passes/scalar/README.md', 'CFG analysis', 'passes/cfg/README.md',
    'dominance analysis', 'publication/value-flow analysis',
    'passes/ssa/README.md', 'memory-effects analysis', 'provenance',
    'passes/memory/README.md', 'passes/aggregate/README.md',
    'call-graph analysis', 'passes/intrinsics/README.md',
    'passes/README.md', 'pipeline/README.md', 'Canonical` publication gate',
])
ordered(texts[2], [
    'C1: audit the external `TargetProfile`', 'C2: `target_layout/README.md`',
    'C3: `preparation/abi/README.md`', 'C4: `preparation/calls/README.md`',
    'C5: `preparation/variadic/README.md`',
    'C6: `preparation/address/README.md`',
    'C7: `preparation/inline_asm/README.md`',
    'C8: `preparation/runtime_helpers/README.md`',
    '`preparation/README.md` as C8 sequencing',
    'C9: `regalloc/constraints/README.md`',
])

assert 'Status: Open (deferred; not active)' in Path(
    'ideas/open/734_lir_to_new_bir_container_completeness.md').read_text()
assert 'Status: Draft (parked; not active implementation authority)' in Path(
    'ideas/draft/733_accepted_bir_a1_f3_architecture_implementation.md').read_text()
assert 'Status: Open' in Path(
    'ideas/open/732_bir_stage_document_convergence_umbrella.md').read_text()
plan = Path('plan.md').read_text()
assert 'Status: Active' in plan
assert 'Source Idea: ideas/open/732_bir_stage_document_convergence_umbrella.md' in plan

names = subprocess.check_output(
    ['git', 'show', '--name-only', '--format=', '9c28f521'], text=True
).splitlines()
assert [name for name in names if name] == [path.as_posix() for path in children]
print('PASS children=6 chain=A-F headings=6-each ChildA=38/6/18 '
      'B-order=correct C-order=correct lifecycle=732/733/734 commit=9c28f521')
PY
git show --stat --oneline 9c28f521
git show --name-only --format= 9c28f521
git diff --check
```

- Result:

```text
PASS children=6 chain=A-F headings=6-each ChildA=38/6/18 B-order=correct C-order=correct lifecycle=732/733/734 commit=9c28f521
9c28f5213 [idea] add ordered BIR phase convergence ideas
6 files changed, 702 insertions(+)
git show --name-only: exactly ideas/open/735... through ideas/open/740...
git diff --check: PASS
```
