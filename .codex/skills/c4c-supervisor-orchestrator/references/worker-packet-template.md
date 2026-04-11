# Worker Packet Template

Use this format when delegating a slice to a subagent.

```md
# Worker Packet

Source Idea: ideas/open/<idea>.md
Current Step: <plan step>
Objective: <one-sentence concrete goal>

Owned Files:
- path

Do Not Touch:
- plan.md
- todo.md
- unrelated owners or tests

Proof:
- <narrow build or test command>

Done When:
- <observable completion condition>

If Blocked:
- stop immediately
- report the exact blocker
- do not pivot to another slice
```

Workers should return:

- files changed
- local validation run
- assumptions
- blockers or follow-up notes
