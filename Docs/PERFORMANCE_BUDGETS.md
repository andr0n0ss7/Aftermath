# Performance budgets

**Minimum PC:** 4-core Intel Core i5 (6th gen or newer), 16 GB RAM, 4 GB graphics card
(GTX 1650, GTX 1050 Ti 4 GB, RX 570 4 GB), SSD, Windows 10/11, DirectX 11.
**Goal:** 1080p at 60 fps on Medium on that PC; Low as a fallback for older cards.

## Rendering choices (all presets)

| Feature | Setting | Why |
| --- | --- | --- |
| Lighting | Baked (Lightmass), Stationary room lights | Looks good on any card; costs almost nothing at runtime |
| Dynamic lights | The flashlight only, plus at most 1 more per room | Each dynamic shadow-casting light costs a shadow pass |
| Lumen, Nanite, Virtual Shadow Maps, ray tracing | Off | Memory and GPU cost too high for 4 GB cards |
| Reflections | Reflection captures + screen-space reflections | Cheap; one sphere capture per room |
| Anti-aliasing | TAA | Cheaper than TSR |
| Graphics API | DirectX 11 default, DirectX 12 optional (`-dx12`) | DX11 runs smoothest on older cards |
| Preset on first launch | Automatic benchmark picks Low / Medium / High | Players never start on a setting their PC can't hold |

## Memory per preset

| Preset | Texture streaming pool | Shadow map max |
| --- | --- | --- |
| Low | 600 MB | 512 |
| Medium | 1,200 MB | 1,024 |
| High | 1,800 MB | 2,048 |
| Epic | 2,500 MB | 2,048 |

## Per-scene budgets (measured at Medium, worst view)

| Measure | Budget | How to check |
| --- | --- | --- |
| Frame time | under 16.6 ms on the 4 GB card | `stat unit` |
| Draw calls | under 1,500 | `stat rhi` |
| Visible triangles | under 1.5 million | `stat rhi` |
| Texture memory | under 1.2 GB | `stat streaming` |
| Unique materials per scene | under 40 (instances of a few master materials) | Statistics window |
| Lightmap memory | under 150 MB | World Settings → Lightmass |

## Asset rules

- Texture sizes: 2K only for defects, firearms and hero props seen up close; 1K for furniture; 512 for small clutter.
- Every mesh has LODs (auto-generated is fine) and simple collision.
- Materials are instances of a small set of master materials: Surface, Decal, Glass, Metal, Emissive.
- Packed textures: Base Colour, Normal, and one ORM map (occlusion, roughness, metallic).
