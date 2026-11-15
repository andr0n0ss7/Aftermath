# Firearm models

Real-world firearm models replace the grey placeholder blocks without code changes: import the meshes, point the
firearm data asset at them, and every case that uses that asset shows the model.

## Where models come from

| Source | Licence to check for | Notes |
| --- | --- | --- |
| Fab (Epic) | Standard licence: commercial use in games allowed | Easiest; UE-ready formats |
| Sketchfab | "Standard" or CC-BY (credit the author in the game's credits) | Avoid "Editorial" and non-commercial licences |
| TurboSquid | Standard licence; **not** "Editorial use only" | Many real-firearm models are editorial-only |
| Commissioned / own | Written transfer of rights | Best for the hero weapons |

**Names and logos:** in game text and textures use generic names ("9 mm striker-fired pistol", ".38 six-shot
revolver"). Remove manufacturer logos, names and serial-number-style markings from textures. Using real brand names
needs the trademark owner's permission. This is a precaution, not legal advice; check before release.

## Model requirements

| Item | Requirement |
| --- | --- |
| Scale | Real size in centimetres (a typical pistol is 18–20 cm long) |
| Orientation | Muzzle along +X, top of the gun along +Z, origin at the grip |
| Parts | Separate meshes: **Body**; **Magazine** (pistols); **Cylinder** (revolvers) |
| Sockets on Body | `Magazine` where the magazine seats; `Cylinder` at the cylinder axis; `Muzzle` at the muzzle |
| Triangles | 15,000–30,000 for the body at LOD0, with 2–3 LODs |
| Textures | One 2K set per gun: Base Colour, Normal, ORM (occlusion/roughness/metallic) |
| Collision | Simple box (it is only picked up by the inspection trace) |

## Import steps

1. Content Browser → `Content/Aftermath/Firearms/<GunName>/` → Import the FBX (or glTF).
   Untick "Combine Meshes" so Body, Magazine and Cylinder stay separate. Name them
   `SM_FA_<GunName>_Body`, `SM_FA_<GunName>_Magazine`, `SM_FA_<GunName>_Cylinder`.
2. Open the Body mesh → Socket Manager → add `Magazine` / `Cylinder` / `Muzzle` sockets at the right spots.
3. Open the firearm data asset (`DA_Firearm_Pistol9mm` or `DA_Firearm_Revolver38`, or duplicate one for a new gun)
   and set Body Mesh, Magazine Mesh or Cylinder Mesh. Adjust **Rest Rotation** until the gun lies flat on its side.
4. Press Play: the evidence actor now shows the model. Making the weapon safe moves the magazine out beside a pistol,
   or swings a revolver's cylinder out.
5. A new gun in a case: in the case JSON set `Weapon.FirearmAsset` to the asset's path, plus its facts
   (capacity, rounds, chamber state or cylinder chambers).
