# Content

There have compare [select/poll/epoll](COMPARE.md)

## sample
  - [select](select.c),
  - [pselect](pselect.c)
  - [poll](poll.c)
  - [epoll](epoll.c)

But we should know that:
  - select/poll have [signal race condition](RACE.md) issue, [sample here](select_issue.c)
  - which can be solved by [pselect](pselect.c)/ppoll.
