#!/bin/sh
# Install the QA units for the current user, pointed at this checkout.
#
#   qa/install.sh
#
# User units, not system ones: nothing here needs root, and everything it writes lives
# under $HOME. The unit files in `systemd/` carry `@QADIR@` rather than a path, because a
# vendored unit that names one person's home directory only works on that machine. This
# substitutes the directory the checkout is actually in.
#
# Run state goes to ~/.local/state/weft-fdb-qa, never beside the scripts: a QA run makes
# clusters, wipes them, and leaves logs, and none of that belongs in a git tree.
set -eu

qadir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
units=${XDG_CONFIG_HOME:-$HOME/.config}/systemd/user
mkdir -p "$units"

for f in "$qadir"/systemd/*.service; do
	name=$(basename "$f")
	sed "s|@QADIR@|$qadir|g" "$f" >"$units/$name"
	echo "installed $units/$name"
done

systemctl --user daemon-reload
echo
echo "Run them with:"
echo "  systemctl --user restart --no-block fdb-consensus-qa"
echo "  systemctl --user restart --no-block fdb-dr-qa"
echo "  systemctl --user restart --no-block fdb-consensus-qa-negative   # must FAIL"
echo
echo "Watch:  journalctl --user -u fdb-consensus-qa -f"
echo
echo "restart, not start: RemainAfterExit=yes makes start a no-op after a green run."
