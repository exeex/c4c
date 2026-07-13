# Current Packet

Status: Active
Source Idea Path: ideas/open/735_bir_phase_a_import_raw_document_convergence.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Converge the Raw core receiving and ownership contract

## Just Finished

- Completed plan Step 2: converged
  `src/backend/bir/lir_to_bir/memory/README.md` as a truthful build-excluded
  A1 memory import sub-boundary with the exact metadata spine, core-first
  sections, and applicable detail-heading order.
- Bound all 11 current memory instruction rows—legacy/new load, store and GEP
  twins; memcpy/memset; stack save/restore; and alloca—to their unique
  same-named top-level matrix row and one typed Raw owner. Bound 11 applicable
  metadata families covering stack objects/hoisted allocas, identities,
  operands, types, values, globals, initializers, strings, requirement parity,
  source order and module publication.
- Added explicit input/output handoff matrices with stable identity/revision,
  deterministic order, optional/error forms, module-transactional failures,
  invalidation exclusions, and positive plus malformed/neighbor proof
  obligations. Aggregate/vector, variadic, call, general-intrinsic, inline-asm,
  indirect-branch and terminator rows remain with their actual parent owners.
- The document now states one parent dispatcher/private A1 transaction, one
  core storage owner, and one full A2 publication gate. It produces no
  standalone memory graph, verifier, stage token, partial draft, placeholder,
  fixup table, analysis cache or publication path.
- Build truth is explicit: nested `bir/lir_to_bir/*.cpp` files are excluded and
  the build-included importer at `10d70b872` rejects these families. Design
  presence and stable unsupported failures are not receipt.
- Target-independent memory facts are preserved without scalarization, GEP/
  address folding, target/profile/layout interpretation, ABI/frame/allocation,
  machine lowering, MIR/emission or text-derived identity. No coordinated
  adjacent-owner blocker was found.

## Suggested Next

- Execute plan Step 3 on `src/backend/bir/core/README.md` only: reconcile every
  phase-A destination with one typed core owner or truthful missing-container
  disposition, stable IDs/order/def-use/CFG, private draft freeze, and the sole
  full A2 publication gate without promoting scaffolded design to checked-in
  implementation.

## Watchouts

- Keep the parent importer as the sole 38/6 dispatcher and the memory document
  subordinate. Do not copy its row validation into a second core/import path.
- Core must not acquire target/profile/layout, ABI, constraint, allocation,
  frame, MIR or emission state. Validation-only metadata families publish no
  duplicate semantic owner.
- Do not edit the importer, memory, shared verifier or other adjacent owners in
  Step 3. Record an exact coordinated-boundary seam if core convergence cannot
  agree without such an edit.

## Proof

- Docs-only packet: no canonical regression log applies and neither
  `test_before.log` nor `test_after.log` was changed.
- Exact structural, binding, link, implementation-truth, authority and scope
  proof:

```bash
python3 - <<'PY'
from pathlib import Path
import re
import subprocess

doc_path = Path('src/backend/bir/lir_to_bir/memory/README.md')
doc = doc_path.read_text()

spine = [
    'Contract-Status:', 'Implementation-Status:', 'Kind:', 'Phase-ID:',
    'Upstream:', 'Downstream:', 'Owner-Path:', 'Last-Reconciled-Commit:',
]
core = [
    '## Purpose', '## Owns', '## Does Not Own', '## Inputs', '## Outputs',
    '## Adjacent-Stage Contract',
]
detail = [
    '## Ordered Behavior', '## Invariants',
    '## Verification and Publication', '## Failure and Diagnostics',
    '## Analysis and Invalidation', '## Target and ABI Rules',
    '## Implementation State', '## Proof Requirements',
    '## Open Questions', '## Review Checklist',
]
for ordered in (spine, core, detail):
    positions = [doc.index(item) for item in ordered]
    assert positions == sorted(positions), (ordered, positions)

input_section = doc.split('### Input handoff matrix', 1)[1].split(
    '## Outputs', 1)[0]
required_rows = [
    'LirInst::LirLoad', 'LirInst::LirStore', 'LirInst::LirGep',
    'LirInst::LirMemcpyOp', 'LirInst::LirStackSaveOp',
    'LirInst::LirStackRestoreOp', 'LirInst::LirLoadOp',
    'LirInst::LirStoreOp', 'LirInst::LirMemsetOp', 'LirInst::LirGepOp',
    'LirInst::LirAllocaOp',
]
rows = re.findall(r'^\| `(LirInst::Lir[A-Za-z0-9_]+)` \|.*$',
                  input_section, re.M)
assert rows == required_rows
assert len(rows) == len(set(rows)) == 11
for row in rows:
    line = next(line for line in input_section.splitlines()
                if line.startswith(f'| `{row}` |'))
    cells = [cell.strip() for cell in line.split('|')[1:-1]]
    assert f'Parent row `{row}`' in cells[2], (row, cells[2])
    assert 'one ' in cells[2] or 'same single ' in cells[2], (row, cells[2])

expected_metadata = [
    'stack-objects-allocas', 'stable-identities', 'operand-kinds',
    'type-system', 'values-def-use', 'globals-objects', 'initializers',
    'strings', 'intrinsic-requirements', 'source-order-origin',
    'module-publication',
]
metadata = re.findall(r'^\| `metadata::([a-z0-9-]+)` \|',
                      input_section, re.M)
assert metadata == expected_metadata
assert len(metadata) == len(set(metadata)) == 11

links = re.findall(r'\[[^]]+\]\(([^)#]+)(?:#[^)]*)?\)', doc)
assert links
for target in links:
    resolved = (doc_path.parent / target).resolve()
    assert resolved.exists(), (target, resolved)

required_truth = [
    'Implementation-Status: absent',
    'not a second importer',
    'owns no verifier profile',
    'cannot finish, freeze, return, verify, or publish the draft',
    'This boundary produces no standalone artifact',
    'excludes every nested',
    'no opaque placeholder',
    'no memory-only `RawBir`',
]
for statement in required_truth:
    assert statement in doc, statement
checklist = doc.split('## Review Checklist', 1)[1]
assert '- [ ]' not in checklist
assert checklist.count('- [x]') == 9

changed = {
    line for line in subprocess.check_output(
        ['git', 'diff', '--name-only'], text=True).splitlines() if line
}
assert changed == {
    'src/backend/bir/lir_to_bir/memory/README.md', 'todo.md'
}, changed
print('PASS spine=8 core=6 detail=10 memory_rows=11 metadata=11 '
      f'links={len(links)} checklist=9/9 changed=2')
PY
git diff --check
```

- Result:

```text
PASS spine=8 core=6 detail=10 memory_rows=11 metadata=11 links=8 checklist=9/9 changed=2
git diff --check: PASS
```
