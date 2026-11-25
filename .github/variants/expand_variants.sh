#!/usr/bin/env bash
# Expand variants.json into the job matrix: spec x arch x target x extra x precision.
# Prints {"include":[...]} on stdout. Usage: expand_variants.sh [host] (filter by host).
set -euo pipefail
here=$(cd "$(dirname "$0")" && pwd)
jq -c --arg host "${1:-}" '
  def slug: gsub("[^a-z0-9]+"; "-") | sub("^-";"") | sub("-$";"");
  {include: [
    .[] | select($host == "" or .host == $host)
    | . as $s
    | .arch[] as $a | .targets[] as $t
    | (($s.extras // [""]) | map(if . == "" then ($s.extra // "") else ($s.extra // "") + " " + . end | ltrimstr(" "))) [] as $x
    | ("single","double") as $p
    | {
        name: ([$s.platform,$a,$t,($x|slug),$p] | map(select(. != "")) | join("-")),
        platform: $s.platform, host: $s.host, image: ($s.image // ""),
        arch: $a, target: $t, precision: $p,
        flags: ([($s.opts // ""), $x, (if $p == "double" then "precision=double" else "" end)] | map(select(. != "")) | join(" "))
      }
  ]}' "$here/variants.json"
