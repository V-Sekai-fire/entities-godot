# ParallelCommits.tla, and why it may be here

`spec/ParallelCommit.lean` names `ParallelCommits.tla` as the authority for the protocol and
then says the mapping onto this layout "is an argument rather than an observation". An
authority nothing here can read is an authority in name only, so the file is vendored rather
than cited, and this note records where it came from and under what terms.

## Provenance

| field        | value                                                              |
| ------------ | ------------------------------------------------------------------ |
| source       | `v-sekai/cockroach`, branch `release-22.1-v-sekai`                 |
| path         | `docs/tla-plus/ParallelCommits/ParallelCommits.tla`                |
| sha256       | `2ecc5f7fa6b2a575d12d8ad79a572ef78b8729b2326191b04dafa0ebb6bf15bd` |
| lines        | 919                                                                |
| PlusCal hash | `PCal-7847e3ffca2156d2f95a169911409dbb` (the file's own)           |

`BSL.txt` beside it is that fork's licence file, copied at the same time, because the
argument below is about its parameters and a citation to a file that can be edited is not
evidence.

## Why this is Apache-2.0 and not BSL

The Business Source License is time-limited by construction. `BSL.txt` carries:

    Licensed Work:  CockroachDB 22.1
    Change Date:    2025-04-01
    Change License: Apache License, Version 2.0

The Change Date has passed — it is 2026-08-20 as this is written, sixteen months after — so
the licence on this work **is** Apache-2.0 now, by the terms of the BSL itself rather than by
anyone's permission. No relicensing happened and none was needed.

That matters because this repository is `Apache-2.0 OR MIT`, and a use-restricted licence
inside it would propagate a restriction into everything that reads the file. Under BSL it
would have been the same hazard the workspace's OpenRAIL-M and CC-BY-SA entries exist for.
After the Change Date it is not a hazard at all, and the distinction is a date, which is why
the date is written here rather than left for a reader to look up.

The BSL's Additional Use Grant — the "Database Service" restriction — expires with the rest
of it on the Change Date, so it does not reach this repository either. It is quoted in
`BSL.txt` regardless, since a reader checking this argument should not have to fetch the
file to check it.

`thirdparty/` keeps the terms its contents arrived with, per the repository's licence scope
note. Here those terms are Apache-2.0, which is one of the two this repository already
offers.

## Why the 22.1 copy and not the 20.2 one

The v20.2.0 copy was fetched first and is not the one kept. The two differ in one line of
the algorithm, and the difference is a fix rather than a style change:

    20.2:  to_check := to_check \union {key}
    22.1:  to_check := to_check \ {key}

This is in `QueryPipelinedWrite`, on the branch where the intent **was** found. The loop
condition is `while to_check /= {} \/ ...`, so adding the key back to the set it was drawn
from makes a found intent no progress at all. The 22.1 line removes it, which is what the
surrounding comment says the step does.

A model checked against the 20.2 copy would explore a state space this one does not, and the
line sits directly on the pipelined-write path — which is the path `spec/ParallelCommit.lean`
maps onto staged pages. So the choice of copy is not bookkeeping.

Both files also carry a `Last modified` trailer naming the same author and date. Only the
20.2 copy's PlusCal translation is unhashed; the 22.1 copy carries `PCal-` and `TLA-` hashes,
so a future edit that forgets to re-run the translator is reported by the tools rather than
discovered by a reader.
