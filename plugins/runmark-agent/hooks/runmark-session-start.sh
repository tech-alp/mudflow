#!/bin/sh
# Hook'lar dar bir PATH ile calisabiliyor: Codex'te `env node` bulunamayip
# exit 127 verdi, `rmk` de gorunmuyordu. Kurulum yerlerini PATH'e ekle.
# ponytail: sabit liste; node baska yere kuruluysa buraya eklenir.
PATH="$PATH:/usr/bin:/bin:/opt/homebrew/bin:/usr/local/bin:$HOME/.local/bin"
export PATH
# Dis komut yok: dar PATH'te `dirname` de bulunamayabiliyor.
case "$0" in */*) dir=${0%/*} ;; *) dir=. ;; esac
node=$(command -v node) || exit 0
[ -n "$node" ] || exit 0
exec "$node" "$dir/runmark-session-start.js"
