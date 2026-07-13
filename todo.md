# Current Packet

Status: Active
Source Idea Path: ideas/open/736_bir_phase_b_canonical_document_convergence.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish the phase-B baseline and converge B1 P01 legalize

## Just Finished

- Began plan Step 1 with the required read-only phase-B baseline audit before
  editing B1/P01. Confirmed the accepted predecessor is one immutable,
  exact-revision, fully verified, target-independent and unallocated `RawBir`;
  P01 cannot repair A2 failure, and B2 expects the exact P01 `TypesLegal`
  successor revision.
- Confirmed current implementation truth: `passes/legalize/` contains only its
  README, no checked-in P01 implementation or build edge. The existing document
  is design-only and must not claim implementation when Step 1 resumes.
- Found one material shared-owner contradiction in the read-only pass-framework
  baseline. Per the delegated blocker rule, no legalize edit was made and Step
  1 remains active pending an explicitly coordinated documentation repair.

### Exact two-sided coordinated-boundary seam

| Side | Owner and exact clause | Required disposition |
|---|---|---|
| Accepted phase-A / B1 input authority | Closed Child A 735 and `core/README.md` make `LirModule::target_profile` and rendered `data_layout` validation/origin/parity-only non-destinations. `verify/README.md` states Raw and Canonical carry no semantic target profile/triple/rendered layout/language-ABI/pointer-width/address-space layout selection or other C1/C2 target context; C1 selects the exact `TargetProfile` and C2 derives layout. The active Child-B runbook therefore requires P01 to consume only that target-independent Raw revision. | Preserve this accepted authority unchanged. B1 may preserve genuinely target-independent typed source facts assigned their own Raw owners, but it cannot receive, inspect, derive or preserve target/layout context as semantic module state. |
| Conflicting shared framework owner | `src/backend/bir/passes/README.md`, **Allowed dependency direction**, says: “Target-independent BIR semantics may include an already-resolved module data layout where the core schema requires it.” This grants canonical passes semantic module-layout input that accepted A2 explicitly excludes and makes the B1 input boundary ambiguous. | In a separately authorized shared-doc packet, replace this allowance with the accepted distinction: source-semantic typed sizes/alignments/addresses already owned by Raw rows may be preserved, but `target_profile`, rendered `data_layout`, target triple, pointer-width/address-space layout selection and all C1/C2 context are absent from Raw/Canonical; C1/C2 alone select/derive target layout. Do not edit root/core/verifier/closed Child A unless a fresh audit identifies a separate conflict. |

This is a documentation authority conflict, not permission to add target state
to B1, weaken A2, change phase order, or edit implementation. After the shared
framework wording is repaired, rerun the Step-1 baseline and then normalize
`passes/legalize/README.md` to the uniform metadata/core-first/matrix contract.

## Suggested Next

- Supervisor should coordinate a narrowly authorized shared-document repair
  for the exact `passes/README.md` clause above. Then redispatch Step 1 to
  converge `passes/legalize/README.md` and prove its P01 form dispositions,
  transaction/invalidation and exact B2 handoff.

## Watchouts

- Do not resolve the seam by importing rendered layout or any target/profile/
  ABI/preparation fact into Raw, Canonical or P01.
- P01's later matrix must distinguish source-semantic object/type attributes
  from C1/C2 target-layout decisions and must label implementation absent.
- Keep B2, B8, analysis, root, closed Child A, plan and source idea read-only
  until the supervisor explicitly coordinates any shared-owner repair.

## Proof

- Docs-only blocker record; no canonical regression log applies and neither
  `test_before.log` nor `test_after.log` changed.
- Exact conflict, accepted-side, implementation-truth and diff-scope proof:

```bash
python3 - <<'PY'
from pathlib import Path
import subprocess

plan = Path('plan.md').read_text()
todo = Path('todo.md').read_text()
closed_a = Path(
    'ideas/closed/735_bir_phase_a_import_raw_document_convergence.md').read_text()
core = Path('src/backend/bir/core/README.md').read_text()
verify = Path('src/backend/bir/verify/README.md').read_text()
framework = Path('src/backend/bir/passes/README.md').read_text()
legalize_path = Path('src/backend/bir/passes/legalize/README.md')

assert '### Step 1 - Establish the phase-B baseline and converge B1 P01 legalize' in plan
assert 'Current Step ID: 1' in todo
assert 'Current Step Title: Establish the phase-B baseline and converge B1 P01 legalize' in todo
assert ('| `module-context` | `LirModule::target_profile`, rendered `data_layout` | '
        'validation/origin/parity-only; no semantic Raw destination |') in closed_a
assert '`target_profile` and rendered `data_layout` have no semantic core destination' in core
assert ('Raw and Canonical BIR carry no semantic `target_profile`, target triple,\n'
        'rendered `data_layout`, language-ABI mode, pointer-width/address-space layout\n'
        'selection, or other C1/C2 target context') in verify
conflict = ('Target-independent BIR semantics may include an already-resolved module data\n'
            'layout where the core schema requires it')
assert conflict in framework
assert legalize_path.is_file()
assert not any(path.is_file() for path in legalize_path.parent.iterdir()
               if path.name != 'README.md')
subprocess.run(['git', 'diff', '--quiet', '--', legalize_path.as_posix()],
               check=True)
changed = set(subprocess.check_output(
    ['git', 'diff', '--name-only'], text=True).splitlines())
assert changed == {'todo.md'}, changed
print('PASS step=1 status=blocked seam=phase-A-target-context-vs-pass-framework-layout '
      'implementation=absent legalize=unchanged scope=todo')
PY
git diff --check
```

- Result:

```text
PASS step=1 status=blocked seam=phase-A-target-context-vs-pass-framework-layout implementation=absent legalize=unchanged scope=todo
git diff --check: PASS
```
