# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 12
Current Step Title: Reconcile the root and run the complete documentation proof

## Just Finished

- Plan Step 12 reconciled the normative root and all subordinate contracts.
  The root now states the exact 44-file inventory (one root plus 43
  subordinate documents), links every subordinate exactly once, and retains
  the exact 31-row A1-F3 order.
- The remaining stale transcription surfaces now agree with their existing
  owners: pass-framework implementation prerequisites are no longer presented
  as undecided architecture; C9 is direct input only to D1's first projection;
  preparation cannot defer address/frame selection to MIR; and the review walk
  includes initial D5, E1-E3 retry, post-E3 D5 resolution, and E4.
- The complete structural proof found no missing local links, stale S-stage or
  G01 authority IDs, absent `analysis/alias`, copied-graph authority, unresolved
  acceptance choice, late MIR allocation/copy/spill repair authority, missing
  accepted legacy owner, or revision-key ambiguity. No acceptance marker was
  added.

## Suggested Next

- Execute Plan Step 13, "Repeat the independent full architecture review."

## Watchouts

- Step 13 must independently review all 44 BIR Markdown files against the root,
  review template, plan, and source idea; this Step 12 proof is structural and
  does not record architecture acceptance.
- Step 14 and implementation remain forbidden unless that independent review
  is blocker-free for authority, adjacency, revision keys, realizability, and
  legacy dispositions.

## Proof

- Passed the exact executable documentation proof:

  ```sh
  git diff --check
  test "$(rg --files src/backend/bir -g '*.md' | wc -l)" -eq 44
  test "$(rg -o 'passes/call_lowering/README\.md' src/backend/bir/README.md | wc -l)" -eq 1
  ! rg -n '\bS(0[0-9]|1[0-9]|2[0-9])\b|\bG01\b' src/backend/bir -g '*.md'
  ! rg -n 'analysis/alias|copied[- ]graph|MIR selection|frame planning|E1/E2 and boundary verification as immutable Canonical bindings' src/backend/bir -g '*.md'
  ! rg -ni '^#{1,6} .*open|open design question|decision (needed|required|pending)|must (choose|decide)|to be decided|TBD|TODO|FIXME' src/backend/bir -g '*.md'
  python3 -c 'from pathlib import Path; import re; files=sorted(Path("src/backend/bir").rglob("*.md")); pat=re.compile(r"\[[^\]]*\]\(([^)]+)\)"); pairs=[(f,x.strip().removeprefix("<").removesuffix(">").split("#",1)[0]) for f in files for x in pat.findall(f.read_text())]; bad=[(str(f),x) for f,x in pairs if x and not re.match(r"^[a-z]+:",x,re.I) and not (f.parent/x).resolve().exists()]; root=Path("src/backend/bir/README.md"); links=[x.split("#",1)[0] for x in pat.findall(root.read_text())]; subs=[str(f.relative_to(root.parent)) for f in files if f != root]; ids=re.findall(r"^\| `([A-F][0-9])` \|",root.read_text(),re.M); expected=["A1","A2","B1","B2","B3","B4","B5","B6","B7","B8","C1","C2","C3","C4","C5","C6","C7","C8","C9","D1","D2","D3","D4","D5","E1","E2","E3","E4","F1","F2","F3"]; ledger=[line for line in Path("src/backend/bir/LEGACY_COVERAGE.md").read_text().splitlines() if line.startswith("|") and "Accept" in line and "Legacy files or capability family" not in line]; assert len(files)==44 and not bad and all(links.count(x)==1 for x in subs) and ids==expected and all("Producer/owner:" in line for line in ledger)'
  rg -n 'BoundConstraintSet|ProjectedConstraintSet|ConstraintProjectionTransaction|CopyResolutionTransaction|CopyScratch|ParallelCopy|E1|E2|E3|E4|F1|MirReadyBirView' src/backend/bir/README.md src/backend/bir/regalloc/constraints/README.md src/backend/bir/passes/out_of_ssa/README.md src/backend/bir/regalloc/README.md src/backend/bir/regalloc/spill_reload/README.md src/backend/bir/allocated/README.md src/backend/bir/REVIEW_TEMPLATE.md src/backend/mir/README.md >/dev/null
  ```
- The inline structural checker validated 44 Markdown files, 86 link
  references, 43 exact-once root subordinate links, 31 ordered stages, and 26
  accepted legacy-owner rows.
- The supervisor-selected documentation proof was sufficient; this packet did
  not create or modify `test_after.log` because regression logs were explicitly
  outside packet ownership.
