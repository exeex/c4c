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
4. Do not mechanically dump whole `.cpp`/`.hpp` files into `.md`; require the
   executor to keep only
   the important API and contract surfaces, using short fenced `cpp` blocks for
   the pieces that matter.
5. Do not delete the only working implementation before a replacement path
   exists and has absorbed the target ownership.
6. Force every fast path or special case into one label:
   - core lowering
   - optional fast path
   - legacy compatibility
   - overfit to reject
7. Prefer behavior-preserving migration slices over heroic all-at-once rewrites.
8. Every executor packet must answer: what artifact was produced, what
   responsibility moved or was documented, what still remains in legacy code,
   and what proof covers the packet.
9. The supervisor does not perform extraction, review, draft writing, or code
   conversion directly when an executor packet can own it.
10. Use parallel executor packets when the active stage can be split into
    disjoint owned outputs without overlap.
11. Keep write ownership disjoint when parallelizing. Do not assign two agents
    the same output markdown or source file.
12. Every migration packet must answer: what responsibility moved, what still
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

Ask `c4c-plan-owner` to create exactly these four source ideas:

1. extract the target `.cpp` and `.hpp` into `.md`
2. review the extracted `.md` and critique the current design
3. produce new corresponding `.cpp.md` / `.hpp.md` drafts and review them
4. convert the reviewed drafts into real `.cpp` / `.hpp` implementation

The first idea must explicitly require that the extraction:

- converts the `.cpp` and `.hpp` into `.md`
- keeps only the important APIs and contracts
- uses short fenced `cpp` blocks such as

```cpp
// some important code ....
```

The second idea must explicitly require a broad review of the extracted `.md`
and a reconstruction of how the current design works.

The third idea must explicitly require new `.cpp.md` / `.hpp.md` drafts plus
review of those drafts.

The fourth idea must explicitly require converting the reviewed drafts into
real `.cpp` / `.hpp` source.

Keep each idea narrow. Do not let one idea silently span the whole rebuild.

When asking `c4c-plan-owner` to create those ideas, require each idea to state:

- its stage in the sequence
- the exact artifacts it produces
- what it does not own yet
- what downstream stage it unlocks

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

- keep only important APIs, contracts, and representative code
- use fenced `cpp` blocks for essential surfaces
- summarize the rest in prose

Representative block shape:

```cpp
// some important code ....
```

Name the document after the subsystem, not after the current file if the file
name is already misleading.

### Step 5: Executor Review Rules

When the active idea is the markdown-review stage, require the executor to use
the `.md` artifact from idea 1 as the review target.

Review:

- what responsibilities are mixed together
- what interfaces are falsely coupled
- which APIs are truly stable
- what should survive into the redesign
- what should be deleted or downgraded to compatibility

Do not design replacement files before this review exists.

Review questions:

- what responsibilities are mixed together
- what interfaces are falsely coupled
- which APIs are truly stable
- what should survive into the redesign
- what should be deleted, downgraded, or isolated as compatibility
- how the current design actually works overall

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

- extraction idea: markdown artifact exists and is compressed correctly
- review idea: critique is concrete and reconstructs the current design
- draft idea: `.cpp.md` / `.hpp.md` drafts exist and were reviewed
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
- one extracted design artifact `.md` for the subsystem
- reviewed replacement `.cpp.md` / `.hpp.md` drafts
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
