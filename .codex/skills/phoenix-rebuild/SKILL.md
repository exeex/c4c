---
name: "phoenix-rebuild"
description: "Use when an oversized implementation file, subsystem, or tangled interface family should be rebuilt around cleaner seams instead of receiving another local patch. This skill starts by routing idea creation through `c4c-plan-owner`, requiring a fixed four-idea source-idea series before rebuild work proceeds: markdown extraction, markdown review, replacement draft generation, and draft-to-implementation conversion."
---

# Phoenix Rebuild

Use this skill as a supervisor-facing workflow when the honest fix is
structural replacement, not another incremental patch.

This skill is for repeated "tear down and rebuild" work on code that has
accreted mixed responsibilities, shape-specific fast paths, and unclear
ownership boundaries. The supervisor owns routing and stage boundaries. The
specialists do the actual artifact production.

Do not use this skill for a normal bounded packet, a small refactor, or a
single bug fix that fits inside the current design.

Keep this skill self-contained. Do not rely on companion reference markdown
files. Put the rebuild rules in this file.

## Scripted Entry Point

The extraction stage should be script-driven, not hand-run file by file.

Use:

`python .codex/skills/phoenix-rebuild/scripts/extract_legacy_to_markdown.py --output-root <dir> '<glob>' ...`

Legacy cleanup:

`python .codex/skills/phoenix-rebuild/scripts/delete_legacy_cpp.py --output-root <dir> '<glob>' ...`

The script hardcodes the extraction prompt, expands the requested glob set,
and launches one agent process per matched legacy `.cpp` / `.hpp` in parallel.
Use it as the default stage-1 driver when the task is "translate legacy code
into compressed markdown evidence."

The cleanup script is the default way to remove old `.cpp` files after stage-1
evidence exists. It refuses deletion unless the matching extracted `.md`
already exists, then deletes the original `.cpp` files with `rm`.

Header policy is strict:

- one directory gets exactly one non-helper `.hpp` as its formal index surface
- that non-helper `.hpp` is the only LLM index entry for the directory
- `helper.hpp` is the only allowed exception and does not count toward the
  single-index-header limit
- do not design rebuild layouts around one `.cpp` plus one `.hpp` siblings
- if a globbed extraction set contains more than one non-helper `.hpp` in a
  directory, stop and repair the layout before continuing

## Trigger Conditions

Use this skill when one or more of these are true:

- one implementation file or module has become the de facto home for several
  unrelated responsibilities
- the file name and the actual responsibility no longer match
- repeated fixes keep adding new special cases without improving the structural
  boundaries
- the current implementation is no longer a trustworthy interface reference,
  only a behavior reference
- prior "rewrite" attempts failed because the rebuild had no stable extraction
  or migration workflow
- the user explicitly wants a subsystem to be rebuilt or "reborn"

Do not trigger merely because a file is long. Length is a smell, not the
decision rule.

## Goal

Replace an oversized implementation with a staged rebuild that:

1. first routes durable source-idea creation through `c4c-plan-owner`
2. activates one rebuild stage at a time through normal plan lifecycle
3. assigns artifact production to executors instead of doing the work in the
   supervisor shell
4. uses review and proof gates between stages
5. converts reviewed drafts into implementation only after the design is stable

The result should reduce responsibility mixing, not just move the same mess
into more files.

## Core Rules

1. Treat the old implementation as executable evidence, not as the new design.
2. Begin by asking `c4c-plan-owner` to create `ideas/open/*.md`; do not jump
   directly into `plan.md`/`todo.md` or implementation edits when the rebuild
   itself is not yet shaped.
3. Use the idea-plan-todo layering honestly:
   - `ideas/open/*.md` defines the durable rebuild initiative
   - `plan.md` defines the active execution runbook for one selected idea
   - `todo.md` tracks packet-level execution once a plan is active
4. Do not mechanically dump whole `.cpp`/`.hpp` files into `.md`; use the
   extraction script and require the agent to keep only the important API and
   contract surfaces, using short fenced `cpp` blocks for the pieces that
   matter.
5. When a phoenix route explicitly chooses teardown, delete legacy `.cpp`
   through the cleanup script so removal is gated on extracted `.md` evidence.
6. Do not leave old `.cpp` files parked beside the markdown extraction once
   the teardown step has been accepted.
7. Force every fast path or special case into one label:
   - core lowering
   - optional fast path
   - legacy compatibility
   - overfit to reject
8. Prefer behavior-preserving migration slices over heroic all-at-once rewrites.
9. Every executor packet must answer: what artifact was produced, what
   responsibility moved or was documented, what still remains in legacy code,
   and what proof covers the packet.
10. The supervisor does not perform extraction, review, draft writing, or code
   conversion directly when an executor packet can own it.
11. Use parallel executor packets when the active stage can be split into
    disjoint owned outputs without overlap. Stage-1 extraction should normally
    use the script's one-agent-per-file parallel fanout.
12. Keep write ownership disjoint when parallelizing. Do not assign two agents
    the same output markdown or source file.
13. Every migration packet must answer: what responsibility moved, what still
   remains in legacy code, and what proof covers the moved seam.

## Start Here

1. Ask `c4c-plan-owner` to create the source ideas for the rebuild sequence.
2. Ask `c4c-plan-owner` to activate the first rebuild idea into `plan.md` and
   `todo.md` when the user chooses to start.
3. Delegate the active plan step to one or more executors with explicit owned
   files and proof expectations.
4. Review returned artifacts and decide whether to keep executing, request
   review, or move to the next idea.
5. Reject fake progress and overfit at every stage.

## Workflow

### Step 1: Ask Plan Owner To Create The Idea Series

Do not write the rebuild ideas ad hoc in the current turn. Hand the initial
idea creation to `c4c-plan-owner` so the source ideas remain consistent with
the repo's lifecycle rules.

Ask `c4c-plan-owner` to create exactly four source ideas and make the role
write them as explicit stage contracts, not loose summaries.

Every phoenix-rebuild idea must include, at minimum:

- `## Intent`
- `## Stage In Sequence`
- `## Produces`
- `## Does Not Yet Own`
- `## Unlocks`
- `## Scope Notes`
- `## Boundaries`
- `## Completion Signal`

Keep each idea narrow. Do not let one idea silently span the whole rebuild.

When asking `c4c-plan-owner` to create those ideas, require each idea to state:

- its stage number in the four-stage sequence
- the exact artifacts it produces
- what it does not own yet
- what downstream stage it unlocks
- a concrete close condition in `## Completion Signal`
- a short "read this first" pointer telling executors/reviewers to consult
  `.codex/skills/phoenix-rebuild/SKILL.md` before doing phoenix-stage work

Do not allow `Completion Signal` to say vague things like "artifact exists" or
"review is done". It must say what full artifact set or decision package must
exist before the idea can close.

#### Step 1 Produces Idea 1

Idea 1 is the extraction idea. In the current x86 rebuild sequence, this maps
to idea 78.

Idea 1 must say that it produces:

- one `.md` companion for every in-scope legacy `.cpp`
- one `.md` companion for the single in-scope non-helper directory index
  `.hpp`
- one directory-level index `.md` for the whole extracted scope

Idea 1 content must explicitly require:

- script-driven glob expansion over the in-scope legacy source set
- one markdown artifact for each matched legacy `.cpp`
- one markdown artifact for the single non-helper `.hpp` index surface in each
  directory
- `helper.hpp` may exist beside that index header, but it never replaces the
  directory index role
- compressed extraction rather than source dumping
- important APIs, contracts, dependency directions, hidden dependencies,
  responsibility buckets, and special-case classification
- short fenced `cpp` blocks only for essential surfaces, for example:

```cpp
// some important code ....
```

Idea 1 close condition must explicitly require:

- every in-scope legacy `.cpp` and the single directory-index `.hpp` have
  their corresponding `.md`
- the accepted teardown packet has removed the replaced legacy `.cpp` files
  with the cleanup script instead of leaving them parked beside the extraction
- no directory exceeds one non-helper `.hpp`; `helper.hpp` is the only allowed
  exception
- the directory-level index points at the full artifact set
- the extraction is compressed enough to be reviewable

#### Step 2 Produces Idea 2

Idea 2 is the extraction-review and replacement-layout idea. In the current
x86 rebuild sequence, this maps to idea 79.

Idea 2 must say that it produces:

- one stage-2 review and replacement-layout document
- one explicit handoff document from idea 2 to idea 3

Idea 2 content must explicitly require:

- a broad review of the extracted `.md` set from idea 1
- reconstruction of how the current design actually works
- judgment about whether the idea-1 extraction set itself needs correction,
  expansion, compression, splitting, merging, or reorganization
- a concrete replacement architecture layout for the rebuilt subsystem
- an explicit judgment about whether that layout addresses the motivating
  historical ideas or failure families
- the exact `.cpp.md` / `.hpp.md` draft-file layout that idea 3 must fill
- a handoff artifact that tells idea 3 which extracted artifacts are trustworthy,
  which require correction first, which replacement draft files are mandatory,
  and what route constraints must be preserved

Idea 2 close condition must explicitly require:

- the review/layout document explains the current subsystem shape and the
  replacement layout
- it explicitly states how the idea-1 extraction set must be improved before
  later stages rely on it
- the stage-2-to-stage-3 handoff document exists and is concrete enough that
  idea 3 can consume it as an intake contract

#### Step 3 Produces Idea 3

Idea 3 is the replacement-draft and draft-review idea. In the current x86
rebuild sequence, this maps to idea 80.

Idea 3 must say that it produces:

- one `.cpp.md` for every planned replacement implementation file declared by
  idea 2
- one directory-index non-helper `.hpp.md` for each replacement directory
  declared by idea 2
- one directory-level index `.md` for the replacement draft set when needed
- one review artifact for the replacement draft set

Idea 3 content must explicitly require:

- following the exact replacement file layout declared by idea 2
- following the explicit handoff contract from idea 2
- partitioning by responsibility and dependency direction rather than by
  arbitrary slices of legacy code
- per-file ownership statements: owned inputs, owned outputs, indirect
  queries, forbidden knowledge, and whether the file is core logic, dispatch,
  optional fast path, or compatibility

Idea 3 close condition must explicitly require:

- every replacement `.cpp` and each directory-index non-helper `.hpp`
  declared by idea 2 has its corresponding `.cpp.md` / `.hpp.md`
- no replacement directory introduces more than one non-helper `.hpp`;
  `helper.hpp` is the only allowed exception
- any required replacement index `.md` exists
- the draft-review artifact exists and says the draft set is coherent enough
  for implementation conversion

#### Step 4 Produces Idea 4

Idea 4 is the draft-to-implementation conversion idea. In the current x86
rebuild sequence, this maps to idea 81.

Idea 4 must say that it produces:

- real `.cpp` / `.hpp` implementation that matches the reviewed draft set
- dispatcher and ownership rewiring that routes behavior through the reviewed
  replacement seams
- legacy deletion or retirement only after the new owner is live and proved

Idea 4 content must explicitly require:

- staged migration rather than all-at-once rewrite
- explicit accounting of what responsibility moved and what remains legacy
- proof for each migrated capability family before claiming completion

Idea 4 close condition must explicitly require:

- the reviewed draft set has been converted into real implementation
- the new ownership seams are actually in use
- remaining legacy code is explicitly classified
- proof shows the migrated capability families still work

The supervisor's deliverable for this step is the plan-owner-generated four
ideas, not the extraction artifact itself.

### Step 2: Ask Plan Owner To Activate One Idea

Once the idea series exists, ask `c4c-plan-owner` to activate exactly one of
the four ideas into `plan.md` / `todo.md`.

Do not activate multiple rebuild ideas at once.

The supervisor should choose activation order conservatively:

1. extraction idea
2. markdown review idea
3. replacement draft idea
4. draft-to-implementation idea

### Step 3: Delegate Artifact Production To Executors

After plan activation, the supervisor assigns the active step to executors.

The executor, not the supervisor, performs extraction, design review, draft
authoring, or implementation conversion.

Use one executor when the active output is naturally single-owner.

Use multiple executors in parallel when the active stage can be split into
disjoint artifacts, for example:

- one executor extracts `.cpp` into `.md` while another extracts the paired
  `.hpp` into `.md`
- one executor drafts `.cpp.md` while another drafts `.hpp.md`
- one executor reviews one artifact family while another reviews a disjoint
  companion artifact

Parallelization rules:

- keep owned files disjoint
- keep packet scopes narrow
- do not duplicate review of the same artifact unless an independent reviewer
  is explicitly desired
- have the supervisor integrate the results

### Step 4: Executor Extraction Rules

When the active idea is the extraction stage, require the executor to read the
legacy implementation and compress it into a design artifact.

Capture:

- current entry points and call sites
- real responsibility buckets
- required inputs and hidden dependencies
- internal state carried across helpers
- categories of special cases
- proof surfaces already available in the repo

When extracting code into `.md`:

- write one `.md` per in-scope legacy `.cpp` / `.hpp`
- also write one directory-level index `.md` for the whole rebuild scope
- keep only important APIs, contracts, and representative code
- use fenced `cpp` blocks for essential surfaces
- summarize the rest in prose

Representative block shape:

```cpp
// some important code ....
```

Per-file extraction documents should keep a stable mapping to the original file
name so reviewers can tell coverage at a glance. The directory-level index
document should summarize the subsystem and point at every per-file artifact.

### Step 5: Executor Review Rules

When the active idea is the stage-2 layout/review stage, require the executor
to use the `.md` artifact from idea 1 as the review target.

Review:

- whether the stage-1 extraction set is shaped well enough for redesign work
- what extraction artifacts must be corrected, expanded, compressed, split,
  merged, or reorganized before later stages should trust them
- what responsibilities are mixed together
- what interfaces are falsely coupled
- which APIs are truly stable
- what should survive into the redesign
- what should be deleted or downgraded to compatibility
- what the replacement architecture layout should be
- whether that layout actually addresses the historical motivating failure
  families
- what exact `.cpp.md` / `.hpp.md` artifact layout stage 3 should produce
- what explicit handoff stage 3 must consume

Do not draft replacement file contents before this stage exists. This stage is
allowed to define the replacement architecture layout and the planned stage-3
artifact map.

Review questions:

- is the idea-1 extraction set itself good enough, and if not, what must be
  corrected before stage 3 should trust it
- what responsibilities are mixed together
- what interfaces are falsely coupled
- which APIs are truly stable
- what should survive into the redesign
- what should be deleted, downgraded, or isolated as compatibility
- how the current design actually works overall
- what the new subsystem layout should be
- whether that layout addresses the earlier idea family that exposed the
  rebuild pressure
- what exact per-file `.cpp.md` / `.hpp.md` draft set stage 3 should create
- what stage-2-to-stage-3 handoff contract must exist

### Step 6: Executor Draft Rules

When the active idea is the replacement-draft stage, require the executor to
partition the behavior into owned seams such as:

- layout or addressing resolution
- value-home or operand resolution
- instruction lowering
- branch lowering
- call lowering
- function or module dispatch
- optional pattern fast paths

Do not split by arbitrary line ranges. Split by responsibility and dependency
direction.

Define interfaces before rewriting implementation.

For each replacement component, state:

- owned inputs
- owned outputs
- what it may query indirectly
- what it must not know
- whether it is core lowering, dispatch, or a fast-path layer

If two proposed components still require each other's full internal context,
the seam is not clean enough yet.

Draft these interfaces as `.cpp.md` / `.hpp.md` artifacts in idea 3 and review
them before converting them into real source files.

Do not collapse multiple planned replacement files into one catch-all draft.
Each planned `.cpp` / `.hpp` needs its own corresponding `.cpp.md` / `.hpp.md`
plus any directory-level index needed to explain how the replacement pieces fit
together.

Treat the stage-2 layout artifact as the contract for which per-file drafts
must exist in stage 3. If stage 3 wants to add or remove planned files, that is
a stage-2 contract repair first, not silent drift.

Treat the stage-2 handoff artifact as the intake contract for stage 3. If the
handoff says some extraction artifact must be repaired before a specific draft
is trustworthy, do not silently ignore that requirement.

For each replacement component, define:

- owned responsibility
- owned inputs
- owned outputs
- what it may query indirectly
- what it must not know
- whether it is core logic, dispatch, or a fast-path layer

### Step 7: Executor Implementation Rules

When the active idea is the draft-to-implementation stage, require the
executor to convert the reviewed drafts into real implementation through a
staged migration.

Typical order:

1. shared layout and operand helpers
2. value or home resolution helpers
3. one lowering family
4. dispatch rewiring
5. legacy deletion and cleanup

Keep the old implementation in place until the new dispatcher can choose the
replacement path for the migrated seam.

Typical migration order:

1. shared helpers or foundational state
2. one coherent lowering or transformation family
3. dispatch rewiring
4. deletion of dead compatibility code

### Step 8: Supervisor Acceptance And Review Gates

Before moving to the next idea, the supervisor checks that the current stage's
artifacts exist and are good enough.

Use a reviewer when route quality or draft quality is unclear.

Typical gates:

- extraction idea: the full per-file markdown artifact set exists, the index
  points at the complete set, and the extraction is compressed correctly
- review idea: the review/layout artifact is concrete, judges whether the
  idea-1 extraction set itself needs improvement, defines the new structure,
  addresses the motivating failure families, and includes the stage-2-to-stage-3
  handoff artifact
- draft idea: `.cpp.md` / `.hpp.md` drafts exist, were reviewed, and the old
  replacement ownership map is concrete enough to drive implementation
- implementation idea: real source exists, ownership moved, and proof passes

For each accepted packet, require the executor record:

- which files now own the seam
- which legacy helpers became unreachable or redundant
- which tests, build commands, or narrow proofs cover the slice

Deletion is allowed only when the seam has a clear new owner and the legacy
path no longer supplies unique behavior.

### Step 9: Delete Legacy Code

Delete the old file or old route only after:

- the redesign artifact still matches reality
- replacement interfaces are in use
- the remaining legacy logic is empty or strictly obsolete
- proof covers the migrated capability family

If a deletion packet still depends on "we think nothing uses this", stop and
 collect stronger proof first.

## Red Flags

Stop and redesign again if one or more of these appear:

- new interfaces are just old globals or giant contexts with different names
- the rewrite copies old special cases without classifying them
- the migration packet moves code but not ownership
- the design artifact reads like annotated source code instead of a subsystem
  model
- the new module graph still funnels everything through one god-object or one
  catch-all `.cpp`
- the only proof is one previously failing testcase

Classify every special case as exactly one of:

- core logic
- optional fast path
- legacy compatibility
- overfit to reject

## Deliverables

Produce these artifacts during the rebuild:

- four ordered source ideas under `ideas/open/`, created by `c4c-plan-owner`
- one extracted `.md` per in-scope legacy `.cpp` / `.hpp`
- one directory-level index `.md` for the extracted scope
- one stage-2 review/layout artifact
- one explicit stage-2-to-stage-3 handoff artifact
- reviewed replacement `.cpp.md` / `.hpp.md` drafts for every planned new
  implementation/header file
- one directory-level index `.md` for the replacement draft set when needed
- new or revised headers that define the replacement seams
- staged implementation files that take ownership incrementally
- proof notes or command logs appropriate to the repo workflow

## Output

When using this skill, report:

- whether phoenix rebuild is justified
- the plan-owner-generated four-idea sequence
- which of the four ideas should be activated first
- the current active idea and active packet owner
- whether the current stage can be parallelized safely
- the artifact paths produced by executors
- the replacement interface families, if the stage reached drafting
- the next migration slice
- the remaining legacy responsibilities
- the deletion gate that still must be satisfied
