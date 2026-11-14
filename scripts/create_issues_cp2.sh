#!/usr/bin/env bash
# Creates the six CP2 issues in the current repository using the GitHub CLI.
# Run this ONCE, before any CP2 pull request is opened, so the issue numbers
# come out as #12 to #17 (issues and pull requests share one number sequence).
#
# Needs the GitHub CLI: https://cli.github.com   then:  gh auth login
# No GitHub CLI? Create the same six issues by hand from docs/ISSUES_CP2.md.
set -e

gh issue create --title "GIF writer, bitmap text and animated minimap tour" --body "Palette quantiser, LZW compression and GIF file writer; a 5x7 bitmap font for labelling figures; an animated minimap that follows a steering player and shows the lit field of view. Owner: Nelly Nkatha."
gh issue create --title "Matrix transformations" --body "3x3 homogeneous matrices for translate, rotate, scale, shear and reflect, composition, inverse, and a demo showing every operator and the effect of order. Owner: Darrel Kariuki."
gh issue create --title "Animation: easing, keyframes and path following" --body "Easing curves including a Bezier ease, keyframe tracks, shortest-arc angle blending, an arc-length path sampler, the camera route, and a demo with an easing race. Owner: Ravine Riang'a."
gh issue create --title "Torch sprites" --body "Projection of torches into camera space with a rotation matrix, size and fog by distance, per-column hiding behind walls, and a demo scene with the camera walking toward torches and a pillar. Owner: Shawn Owiro."
gh issue create --title "First-person renderer and walkthrough" --body "Brick walls, fog, torch light with shadow rays, head bob, sprites, a walkthrough animation along the camera route, and a hall demo with approach and sidestep animations. Owner: Arthur Asava."
gh issue create --title "CP2 tests, documentation and evidence" --body "Tests for every CP2 module, README and technique documentation, the proposed-vs-actual write-up, and the CP2 evidence PDF. Owner: Anita Bichanga."
