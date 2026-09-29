#!/usr/bin/env bash
# Render standard review views of an OpenSCAD file or a mesh.
#
# Usage: render_views.sh input.(scad|stl) out_dir [extra openscad args...]
#
# Writes iso, front, side, rear and under PNGs. .scad files render in preview
# mode, so %-modified ghosts and keep-outs appear. Meshes are wrapped in an
# import(). --viewall --autocenter frames the part, and the camera angles are
# fixed so the same views stay comparable between revisions.
#
# If a file is included with `use <...>`, its top-level $fn does not apply.
# Set $fn in the wrapper, or pass -D '$fn=160'.
set -euo pipefail
in="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
out="$2"; shift 2
mkdir -p "$out"
src="$in"
case "$in" in
  *.stl|*.3mf|*.obj|*.off)
    src="$out/.wrap.scad"
    printf 'import("%s");\n' "$in" > "$src" ;;
esac
# name:rotx,roty,rotz (the camera orbits the auto-centred part)
for v in iso:62,0,28 front:80,0,0 side:80,0,90 rear:70,0,200 under:130,0,30; do
  name=${v%%:*}; rot=${v#*:}
  openscad --preview --camera=0,0,0,$rot,0 --viewall --autocenter \
    --imgsize=1000,760 --colorscheme=Tomorrow "$@" -o "$out/$name.png" "$src" >/dev/null 2>&1
  echo "$out/$name.png"
done
