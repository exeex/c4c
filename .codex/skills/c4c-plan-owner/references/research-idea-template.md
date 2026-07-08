# Research Idea Template

Use this reference when creating an `ideas/open/*.md` research idea.

Research ideas produce documentation or evidence artifacts only. They should
not perform implementation, test expectation changes, unsupported-marker
changes, allowlist changes, runtime behavior changes, or lifecycle activation
unless the supervisor explicitly delegates that separately.

## Required Shape

Prefer this section order:

```markdown
# <Title>

Status: Open
Type: Research and architecture documentation
Parent: `<source path or none>`
Related:
- `<related idea/doc/code path>`
Owning Layer: <layers being researched>

## Goal

Produce concrete research documents under `docs/<topic>/` that answer <main
question>.

## Why This Exists

Explain the architectural uncertainty, evidence gap, or decision that must be
answered before implementation.

## Research Questions And Required Answer Files

There are <N> research questions. The delivery must contain exactly <N>
question-answer Markdown files, one for each question, plus one `index.md`.
Each answer file must answer only its assigned question and may link to the
other answer files for supporting context.

1. `01_<question_slug>.md`

   Question: <specific question>

   Required answer shape:
   - <required evidence or trace>
   - <required classification/table/decision>
   - <required conclusion>

## Required Documentation Output

Create the research documents in:

```text
docs/<topic>/
```

Required files:

- `docs/<topic>/index.md`
- `docs/<topic>/01_<question_slug>.md`

`index.md` must link to all numbered answer files and summarize the overall
result. It must not replace any required answer file.

## In Scope

- <research/evidence/documentation work>

## Out Of Scope

- Implementation changes.
- Test expectation, unsupported-marker, allowlist, or runtime behavior changes.
- Activating the idea into `plan.md` unless explicitly requested later.

## Acceptance Criteria

- `docs/<topic>/` contains one `index.md` plus exactly one `.md` answer file
  for each numbered research question.
- Each answer file answers its assigned question directly and follows its
  required answer shape.
- The documents cite concrete code surfaces, logs, docs, or closed ideas
  rather than relying on generic architecture claims.
- No implementation files, test expectations, unsupported markers, allowlists,
  runtime behavior, active plan state, or lifecycle history are changed.

## Reviewer Reject Signals

- Reject answer-file count mismatches.
- Reject collapsed or split numbered questions.
- Reject implementation or expectation changes under a research idea.
- Reject claims without concrete evidence.
- Reject broad follow-up recommendations that do not separate documentation,
  narrow implementation ideas, and discussion-required architecture work.
```

## Style Rules

- Use one `docs/<topic>/` directory per research idea.
- Make every required output path explicit.
- Prefer numbered answer files when there is more than one research question.
- Keep each question answerable and reviewable on its own.
- Acceptance criteria must enforce the output count and filenames.
- Reviewer reject signals must block implementation work disguised as research.
