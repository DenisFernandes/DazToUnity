# Strand Hair Blender Alembic Bridge Notes

Updated: 2026-07-09

## Current Evidence

The native Daz Studio Alembic exporter does not currently give this fork the curve data Unity needs for strand hair.

Observed Mohawk export:

- Raw diagnostic file: `D:\_Meus negocios\Sleaze Games\Bouncing Boobs\Unity Project\Assets\Daz3D\Genesis8Female22\Hair\Genesis8Female22_Mohawk.raw.abc`
- Fixed Unity file: `D:\_Meus negocios\Sleaze Games\Bouncing Boobs\Unity Project\Assets\Daz3D\Genesis8Female22\Hair\Genesis8Female22_Mohawk.abc`
- `dtuabcfix` output: `CopiedMeshes=0 CopiedCurves=0 ConvertedPointMeshes=1 DefaultXforms=1 SkippedRoots=1`

Schema inspection:

- Raw file:
  - core type: `HDF5`
  - writer: Daz Studio Alembic using Alembic 1.0.4
  - `/Genesis8Female/Genesis8FemaleShape`: `AbcGeom_PolyMesh_v1`
  - `/Mohawk/MohawkShape`: `AbcGeom_PolyMesh_v1`
  - no `AbcGeom_Curve_v2`
- Fixed file:
  - core type: `Ogawa`
  - `/Mohawk/MohawkShape`: `AbcGeom_Points_v1`
  - no `AbcGeom_Curve_v2`

Conclusion: Unity is correct to report no curves. The raw Daz Alembic file has no Alembic curve schema for `dtuabcfix` to preserve. The fixer converts the zero-face Mohawk polymesh into points, which is useful evidence but not usable Unity curve hair.

The `dtuabcfix` curve-preservation patch is still valid for future files that actually contain `ICurves`; a synthetic Alembic smoke test preserved `AbcGeom_Curve_v2` and reported `CopiedCurves=1`.

## Blender/Diffeomorphic Hypothesis

The more promising workflow is not `Daz Alembic -> Blender -> Unity Alembic`. By the time the native Daz Alembic exists, strand topology has already been lost or reduced to a mesh/point representation.

The better candidate is:

1. Export or expose the character and strand hair through the Diffeomorphic DAZ Importer workflow, not through native Daz Alembic.
2. Import the character and strand hair into Blender with Diffeomorphic.
3. Import hair guides or strand hair versions in Blender.
4. Convert guide/strand polyline data into Blender curve/hair curve objects when needed.
5. Export selected hair from Blender as Alembic with curves enabled and "curves as mesh" disabled.
6. Verify the output with `abcls`; Unity should only be expected to import curves if the file contains `AbcGeom_Curve_v2`.

Why this may work:

- Diffeomorphic documents strand-based hair as corresponding to Blender hair curves and supports importing hair guides, which are much lighter than full strand-based hair.
- Blender's Alembic exporter can export hair as zero-width curves.
- Unity's Alembic package can import Alembic curve data.

Why it may fail:

- Some Daz hair products may import into Blender as mesh cards, not strand curves or useful guide polylines.
- A mesh made only of unconnected points cannot be safely converted into strand curves without separate strand ordering/connectivity.
- The exact Blender export settings matter. Exporting curves "as mesh" would defeat the purpose.

## Manual Experiment

Goal: prove whether the same Mohawk can become a Unity-readable Alembic curve file through Blender.

Recommended manual test:

1. In Daz Studio, select the dForce strand-based hair object.
2. Set `Viewport Line Tessellation Sides` to `1` before export, so the hair stays as line/strand data where the upstream tools support it.
3. Export/import the character with the Diffeomorphic DAZ Importer workflow.
4. In Blender, enable/import all hair versions or hair guides if the Diffeomorphic option is available.
5. Prefer guide curves/polylines over full dense strand hair for the first test.
6. If the imported hair is a mesh made of two-vertex edges or polylines, select it and use `Object > Convert > Curve`.
7. Export only the hair object to Alembic.
8. In Blender Alembic export settings:
   - include curves/hair object types.
   - do not export curves as mesh.
   - keep the export static/current-frame for the first proof.
9. Inspect the result:

```powershell
& "D:\SDKs\vcpkg\installed\x64-windows\tools\alembic\abcls.exe" -r -l -m "path\to\blender_hair.abc"
& "D:\SDKs\vcpkg\installed\x64-windows\tools\alembic\abctree.exe" -m -a "path\to\blender_hair.abc"
```

Pass condition:

```text
AbcGeom_Curve_v2
```

Fail conditions:

```text
AbcGeom_PolyMesh_v1
AbcGeom_Points_v1
```

If the pass condition is met, test the file in Unity with `com.unity.formats.alembic` and `ImportCurves` enabled.

## Automation Direction

Only automate after the manual Blender proof succeeds.

Possible fork integration:

- Add a hair export mode such as `BlenderCurveBake` or `ExternalBlenderCurveBake`.
- Daz plugin writes a hair conversion job next to the DTU export, including:
  - source Daz asset path or Diffeomorphic export path.
  - hair node name/label.
  - target Unity-relative Alembic path.
  - parent hint and current frame.
- A headless Blender script performs the conversion:
  - loads Diffeomorphic.
  - imports the Daz/Diffeomorphic data.
  - finds the selected hair object or hair guide object.
  - converts mesh edge polylines to curves if required.
  - exports selected curves to Alembic.
  - writes a small JSON result with success/failure and schema summary.
- Daz plugin or Unity importer records the produced `.abc` in `HairAssets`.
- Existing Unity importer path can stay mostly unchanged if the final `.abc` is already `AbcGeom_Curve_v2`.

This path likely avoids the Daz HDF5/Ogawa issue because the final Unity-facing Alembic file is written by Blender, not Daz Studio's old Alembic 1.0.4 exporter. Still verify `core type` with `abcls` rather than assuming.

## Open Questions

- Does the Mohawk import into Blender through Diffeomorphic as true hair curves, guide polylines, two-vertex edge meshes, or dense mesh geometry?
- Does Blender's Alembic exporter write those converted curves as `AbcGeom_Curve_v2` in the target Blender version?
- Can Unity render the imported curves acceptably with its Alembic curve renderer, or do we need a custom curve/hair renderer?
- Is the desired game-runtime representation strand curves, hair cards, or a hybrid where Blender curves are only an intermediate format?
- Can this be made reliable in headless Blender, or should it remain an optional external conversion step?

## References

- Diffeomorphic: Strand-Based Hair and Hair Guides: https://diffeomorphic.blogspot.com/2025/01/strand-based-hair-and-hair-guides.html
- Blender Manual: Alembic import/export: https://docs.blender.org/manual/en/latest/files/import_export/alembic.html
- Blender Manual: Convert objects: https://docs.blender.org/manual/en/latest/scene_layout/object/editing/convert.html
- Unity Alembic: Work with Curves: https://docs.unity3d.com/Packages/com.unity.formats.alembic@2.4/manual/curves.html
