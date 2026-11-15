# Level design rules

Scenes are multi-room, believable and readable. They are deliberately sparser than dense tactical shooters such as
Ready or Not: every object either tells the story, takes a bullet, or hides something. Clutter that does none of these
stays out.

## Scene size

| Scene type | Rooms | Example |
| --- | --- | --- |
| Small | 2–3 | Flat: kitchen + hallway (Kitchen 02:14) |
| Medium | 4–7 | Family house ground floor, small shop with back office, motel corridor + two rooms |
| Large | 8–12, on one or two floors | Bar with cellar, office floor, house with garage and yard |

One scene = one Unreal level. Two-floor scenes load each floor as a sub-level.

## Content per scene

| Item | Budget |
| --- | --- |
| Unique meshes | 40–80 (modular walls, doors, windows, furniture, a few hero props) |
| Placed props per room | 15–30 |
| Evidence traces | 12–30 per case, including 3–6 decoys |
| Surface materials | 6–12 from the library below |
| Dynamic lights | flashlight + 1 per room at most |

Build rooms from a **modular kit**: wall pieces on a 50 cm grid, door and window frames, skirting, ceiling.
Walls that can be shot through are real two-sided pieces (front face, core, back face) so through-holes and rods work.

## Surface material library

Defined in `AftermathData/Data/SurfaceProfiles.json` and imported into `DA_SurfaceLibrary`. Every trace in a case
names the surface it is on (`SurfaceId`). Values are tunable design values, not laboratory data.

| Id | Material | Category | Handgun bullet typically | Skips below (deg) |
| --- | --- | --- | --- | --- |
| drywall_painted | Painted drywall (gypsum board) | Wall | passes through | 8 |
| plaster_on_masonry | Plaster on brick or block | Wall | stops | 12 |
| brick_exposed | Exposed brick | Wall | stops | 15 |
| concrete | Concrete (walls, floors, stairs) | Wall | deflects | 15 |
| wood_door_hollow | Hollow-core interior door | Wall | passes through | 6 |
| wood_solid | Solid wood (frames, exterior doors, tables) | Furniture | passes through | 8 |
| mdf_particleboard | MDF or particleboard furniture | Furniture | passes through | 8 |
| laminate_floor | Laminate or vinyl floor | Floor | deflects | 10 |
| ceramic_tile | Ceramic floor or wall tile | Floor | deflects | 12 |
| stone_worktop | Polished stone worktop | Furniture | deflects | 15 |
| carpet | Carpet over subfloor | Floor | stops | 10 |
| upholstery | Sofa, chair, mattress | Fabric | stops | 5 |
| fabric_curtain | Curtain or hanging fabric | Fabric | passes through | 2 |
| sheet_steel_vehicle | Vehicle body panel | Vehicle | passes through | 8 |
| steel_appliance | Fridge, washing machine panel | Furniture | passes through, stops inside | 8 |
| glass_float | Window glass | Glass | passes through, cracks | 5 |
| glass_tempered | Car side window, shower door | Glass | shatters | 5 |
| glass_laminated | Windscreen | Glass | passes through, deflects | 8 |
| mirror | Mirror | Glass | shatters | 5 |
| plastic_hard | Hard plastic | Furniture | passes through | 6 |
| cardboard | Cardboard boxes | Furniture | passes through | 3 |
| ceiling_tile | Mineral-fibre ceiling tile | Wall | passes through | 5 |
| asphalt | Asphalt (exterior) | Exterior | deflects | 12 |

Defect textures exist so far for drywall entry and exit, plaster crater, steel hole and ricochet smear. Each new
surface needs its own entry, exit and ricochet textures before it appears in a case.

## Readability rules

- A trace must be findable with the flashlight from 1.5 m; small ones (cases, smears) may need crouching, never pixel-hunting.
- Lighting contrast: rooms lit dimly enough that the flashlight matters, never so dark that the room can't be read.
- Every decoy has a test that rules it out (no lead, no exit, dust or paint over it, tarnish).
