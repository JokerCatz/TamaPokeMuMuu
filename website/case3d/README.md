# The 3D board on the website

`public/models/tamapoke_case.glb` is the printed case of the original project
([MakerWorld](https://makerworld.com/es/models/2937822-tamapoke-a-pokemon-pokeball-tamagotchi)),
itself a remix of [“Pokeball” by yoyothechicken](https://makerworld.com/models/839922-pokeball).
Both are CC BY-NC-SA, and so is this GLB.

To rebuild it, download the case's 3MF (`PokeWithPowerButtonRound.3mf`), unzip it
into `extracted/` next to these two scripts and run

    blender -b --factory-startup -P build_case.py -- export

`build_case.py` puts the white half under the red one with a black band between,
fits the front button into the round hole the two half-notches make, finds the
slanted screen opening (the plane most top-edge vertices lie on, about 23.5°),
puts a 44 mm round screen in it (mesh `Screen`, with UVs) and writes
`tamapoke_case.glb` turned so the screen faces glTF +Z. The site swaps the
screen's material for the current screenshot (`components/DeviceCanvas.tsx`).
Without `export` it only renders previews into `renders/`.
