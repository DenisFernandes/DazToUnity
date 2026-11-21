# Daz To Unity Export Tailoring Roadmap

Updated: 2026-07-08

## Purpose

This fork is intended to make the Daz To Unity export path better suited for game-runtime characters and props. The main goals are:

- Export strand-based hair in Alembic format.
- Exclude unwanted shell-style plugin geometry from exports, such as shells generated for genital, breast, wetness, makeup, or similar overlays.
- Reduce runtime cost by combining or reusing materials and meshes where practical.
- Add per-surface or per-UV-piece texture priority so important regions such as the face can remain high resolution while lower-priority body or accessory regions can be imported smaller.
- Replace the current clunky export dialog with a clearer, profile-driven workflow.

This document is a long-term tracker. It should be updated as each design decision, implementation slice, and verification result lands.

## Current Baseline From Static Inspection

The bridge is split between:

- `DazStudioPlugin/`: Unity-specific Daz Studio action and dialog.
- `dzbridge-common/`: shared bridge export flow used by `DzUnityAction`.
- `UnityPlugin/Scripts/Editor/`: Unity importer, material conversion, prefab generation, and bridge window.

Relevant current behavior:

- `DzUnityAction::writeConfiguration()` writes the `.dtu` file and includes materials, morphs, skeleton, subdivisions, pose data, and dForce metadata for mesh or animation exports.
- `DzBridgeAction::exportHD()` delegates the core mesh path to `exportAsset()`, optionally exports a base mesh for subdivision, exports geograft morph OBJ files, and runs FBX post-processing.
- `DzBridgeAction::exportNode()` uses Daz Studio's `DzFbxExporter` and currently sets `doSelected=true`, `doVisible=false`, `doMergeClothing=true`, `doCopyTextures=false`, `doEmbed=false`, and `doSubD=true`.
- `DzBridgeAction::postProcessFbx()` already removes duplicate geograft nodes listed in `m_aGeografts`, but that is not the same thing as excluding shell geometry.
- `writeAllMaterials()` recursively writes every material on the selected node and its children into the DTU file.
- `writeAllDforceInfo()` writes dForce material metadata. The Unity importer explicitly warns and returns for dForce hair, so existing dForce support does not solve strand hair.
- `writeMLDeformerData()` writes an `AlembicFile` DTU member, and `dzbridge-common` has CMake references to Alembic/Imath, but there is no active strand-hair Alembic export implementation in this checkout.
- Unity material import creates one material asset per DTU material and maps FBX materials by scrubbed material name. Duplicate material detection/merging is listed as a known missing feature in `UnityPlugin/README.md`.
- The Daz export dialog is built from the shared `DzBridgeDialog` plus Unity-specific rows in `DzUnityDialog`. It uses a fixed-size Qt 4 form layout with important options spread between the main form and a collapsible Advanced Settings group.

## Guiding Constraints

- Keep the default bridge behavior available. New behavior should live behind an export profile or explicit options.
- Preserve non-interactive/scripted export paths. QA scripts instantiate `DzUnityAction` and dialog objects directly, so new options need scriptable defaults.
- Keep changes scoped to this fork unless a shared bridge change is unavoidable.
- Avoid hand-editing Unity shader graphs unless a shader-specific task requires it.
- Do not make Unity runtime scripts depend on `UnityEditor`; importer and optimization work belongs under `UnityPlugin/Scripts/Editor/`.
- Prefer reversible Daz scene changes during export. Any visibility, parenting, or property mutation needs an undo/restore table.
- Treat mesh combining for skinned characters as high risk until bone weights, blendshapes, submeshes, and material assignments are proven in Unity.

## Feature Roadmap

### 1. Export Profiles And Configuration Schema

Status: Not started

Goal: Create a single source of truth for tailoring choices so UI, non-interactive Daz scripts, DTU metadata, and Unity import behavior agree.

Planned output:

- New `ExportProfile` or similarly named configuration object on the Daz side.
- Saved Qt settings for the selected profile and option values.
- DTU section for fork-specific metadata, for example `ExportOptions` or `SleazeExportProfile`.
- Unity-side parser support for the new DTU section.
- Presets for common flows:
  - `Default Bridge Compatible`
  - `Game Character`
  - `Cinematic Character`
  - `Static Prop`
  - `Debug Full Export`

Initial options:

- `excludeShells`
- `shellExclusionMode`
- `exportStrandHairAlembic`
- `materialMergeMode`
- `texturePriorityMode`
- `meshCombineMode`
- `showAdvancedFbxOptions`

Acceptance criteria:

- Default profile produces the same export shape as the current bridge.
- A scripted/non-interactive export can set profile options without using the UI.
- DTU metadata records the active profile and option values for Unity importer diagnostics.

### 2. Shell And Overlay Exclusion

Status: Not started

Goal: Exclude plugin-created shell or overlay geometry that should not enter Unity.

Important distinction:

- Existing geograft handling detects `DzFigure::isGraftFollowing()` and removes duplicate geograft FBX nodes after export.
- Shells and overlay geometry are likely different Daz node/object types or modifier patterns. They need their own detection and filtering path.

Investigation tasks:

- In Daz Studio, log selected character descendants with node name, label, class name, presentation type, object class, shape class, geometry stats, visibility, render visibility, follow target, and modifier list.
- Identify reliable SDK signals for geoshells or plugin shell nodes.
- Collect sample names/classes for the specific shell sources to exclude: gens, breast overlays, wetness shells, tattoo shells, makeup shells, and any other plugin families used in production.

Implementation options:

- Preferred: Daz-side exclusion before FBX export by temporarily hiding, disconnecting, or otherwise excluding matched shell nodes, then restoring the scene.
- Fallback: FBX post-process removal using a generated exclusion list of exported node/geometry names.
- DTU filter: skip excluded nodes in `writeAllMaterials()` and `writeAllDforceInfo()` so Unity does not create orphaned materials.

Acceptance criteria:

- Exporting with `excludeShells=true` removes matched shell geometry and shell materials from FBX and DTU.
- Exporting with `excludeShells=false` preserves current bridge output.
- Scene state is restored after export, including visibility and parent relationships.
- Logs include a concise list of excluded nodes and the rule that matched each node.

Open questions:

- Which Daz SDK class or property reliably identifies all target shell nodes?
- Are there shells that should be converted into material layers instead of removed?
- Should exclusion be name-pattern based, type based, or both?

### 3. Strand-Based Hair Alembic Export

Status: Not started

Goal: Export strand-based hair separately from the FBX mesh path as `.abc`, then record the Alembic asset in DTU so Unity can import or link it.

Current evidence:

- The Unity README says dForce strand-based hair is not yet supported.
- Unity importer dForce support explicitly skips hair-like materials and logs that dForce hair is unsupported.
- `dzbridge-common` has inactive-looking Alembic/Imath CMake references and `writeMLDeformerData()` writes an `AlembicFile` field, but no active strand hair export path is present.

Investigation tasks:

- Probe Daz Studio's export manager for an Alembic exporter class and the file IO settings it supports.
- Identify Daz SDK classes/properties for strand-based hair nodes, hair systems, generated curves, and simulated hair output.
- Determine whether Daz can export strand hair through a built-in Alembic exporter or whether this fork must link the Alembic SDK and write curves directly.
- Inspect Unity package requirements for Alembic import. Unity may need `com.unity.formats.alembic` in the target project, which should be treated as an optional importer dependency.

Implementation plan:

- Add Daz-side strand hair discovery and classification.
- Add an Alembic export toggle and destination path convention:
  - FBX remains `<asset>.fbx`.
  - DTU remains `<asset>.dtu`.
  - Hair Alembic files use `<asset>_<hairNode>.abc` or a `Hair/` subfolder if multiple systems are exported.
- Add a DTU `HairAssets` section with node name, label, source class, Alembic file path, frame range if animated, and intended Unity parent/bone anchor.
- Add Unity importer support to detect `HairAssets` and either:
  - import and parent Alembic assets when the Alembic package exists, or
  - log a clear action item when the Unity project lacks Alembic support.

Acceptance criteria:

- A character with strand hair exports FBX, DTU, and one or more `.abc` files.
- Unity importer reports the Alembic files and does not silently drop them.
- Missing Alembic package produces a clear warning, not a failed whole-character import.
- Disabling the feature keeps current FBX-only behavior.

Open questions:

- Which Daz hair systems must be supported first: dForce strand hair only, or also other plugin hair systems?
- Is animation/simulation export required immediately, or is static groom export enough for the first milestone?
- Should Unity instantiate Alembic directly, convert to renderable hair cards/meshes, or leave it as an imported cache?

### 4. Material Reuse, Material Merging, And Texture Priority

Status: Not started

Goal: Reduce material count, draw calls, and texture memory without breaking character fidelity.

Recommended order:

1. Material dedupe/reuse in Unity.
2. Texture import max-size policy by material/surface.
3. Optional material grouping and atlas work.
4. Optional mesh combining after the above is stable.

Material dedupe design:

- Generate a stable material signature from DTU material type, shader selection, scalar/color properties, texture source paths, tiling, offset, UV set, and transparency-relevant settings.
- If two DTU materials are identical or configured as merge-compatible, reuse one Unity material asset.
- Keep an audit log that maps original DTU material names to reused Unity material assets.

Texture priority design:

- Add a surface priority map in the export profile.
- Start with material/surface-name rules:
  - Face, lips, eyes: high priority.
  - Torso/body: medium priority.
  - Limbs, nails, inner mouth: lower or explicit priority.
  - Clothing/accessories: profile dependent.
- Use the priority to set Unity `TextureImporter.maxTextureSize`, compression, and possibly mip bias.
- Treat the requested "UV piece weight" as a surface or material weight first. True UV island analysis and atlas packing can come later.

Possible DTU metadata:

```json
"TexturePriority": [
  { "Asset": "Genesis8Female", "Material": "Face", "Weight": 1.0, "MaxSize": 4096 },
  { "Asset": "Genesis8Female", "Material": "Torso", "Weight": 0.7, "MaxSize": 2048 },
  { "Asset": "Genesis8Female", "Material": "Legs", "Weight": 0.45, "MaxSize": 1024 }
]
```

Mesh/material combining design:

- Static props: combine meshes with the same material signature first.
- Skinned characters: avoid whole-character combining until tests prove blendshapes, bone weights, bounds, and material slots survive.
- For skinned meshes, consider combining only safe accessory groups or leaving meshes intact while reducing material duplication and texture sizes.
- Texture atlasing is a separate milestone because it requires UV remapping and material property remapping.

Acceptance criteria:

- Unity importer can reuse identical materials without changing visual output.
- Per-material texture max size is applied automatically and visible in import settings.
- The user can keep face textures higher resolution than body textures through profile rules.
- Mesh combining is opt-in and never runs by default on skinned characters until validated.

Open questions:

- Which body material names and UV sets are standard in the user's content library?
- Should texture priority be configured by Daz material name, UV set name, UDIM tile, node name, or a combination?
- What Unity render pipeline should be optimized first: Built-in, URP, or HDRP?

### 5. Export UI Redesign

Status: Not started

Goal: Make the Daz export dialog clearer, faster, and better aligned with the fork's export profiles.

Current issues:

- The dialog is fixed-size and form-heavy.
- Core workflow choices and advanced/debug options are mixed together.
- The Unity plugin installer and export options compete for attention.
- There is no visible export summary explaining what will be included, excluded, or optimized.

Proposed structure:

- Header:
  - Asset name.
  - Asset type.
  - Export profile selector.
  - Destination Unity Assets folder.
- Main tabs or grouped sections:
  - `Content`: selected root, include children, shell exclusion, geografts, props/clothing handling.
  - `Hair`: strand hair detection, Alembic export, static vs animated hair, Unity Alembic package warning.
  - `Optimization`: material dedupe, texture priority, mesh combine mode.
  - `Morphs/SubD`: existing morph and subdivision controls.
  - `Advanced`: FBX version, FBX dialog, normal map generation, material CSV, plugin installer.
- Bottom summary:
  - Files to be written.
  - Nodes to exclude.
  - Hair assets to export.
  - Expected Unity importer actions.

Implementation notes:

- Keep Qt 4 compatibility.
- Prefer adding Unity-specific UI in `DzUnityDialog` before changing shared `DzBridgeDialog`.
- If shared base dialog changes are necessary, keep them backward-compatible for existing tests and scriptable properties.
- Save profile and option values through `QSettings`.
- Add clear tooltips, but do not rely on tooltips for essential workflow state.

Acceptance criteria:

- Existing default export still works with minimal clicks.
- Advanced/debug options are available but not visually dominant.
- New shell, hair, and optimization options are visible in predictable locations.
- Non-interactive export can set every new option without needing the dialog.

## Suggested Milestones

### Milestone A: Planning Harness And Diagnostic Export

Status: Not started

- Add export profile data model and DTU metadata.
- Add diagnostic logging for descendant node classes/materials/modifiers.
- Add a sample Daz Script test that exports with the default profile and confirms the DTU metadata exists.
- No behavior changes by default.

### Milestone B: Shell Exclusion MVP

Status: Not started

- Implement shell detection with a conservative ruleset.
- Add a visible "Exclude shell/overlay geometry" option.
- Exclude matching nodes from FBX and DTU.
- Restore scene state after export.
- Add QA case with known shell-producing content.

### Milestone C: Unity Material Dedupe And Texture Priority

Status: Not started

- Parse profile metadata in Unity.
- Add material signature dedupe.
- Add per-material texture max-size rules.
- Keep mesh combining disabled.
- Verify imported prefab material assignments and texture import settings.

### Milestone D: Strand Hair Alembic Probe

Status: Not started

- Add in-Daz probe code/script to identify available Alembic export support and strand hair nodes.
- Decide between built-in Daz Alembic exporter and direct Alembic SDK writer.
- Add DTU `HairAssets` schema.
- Export a first static strand hair Alembic file.

### Milestone E: Strand Hair Unity Import

Status: Not started

- Detect Unity Alembic package availability.
- Import/link `.abc` hair assets when possible.
- Parent hair assets to the imported character prefab.
- Gracefully warn when package support is missing.

### Milestone F: UI Redesign

Status: Not started

- Rework `DzUnityDialog` around profiles and grouped options.
- Add export summary and diagnostics.
- Keep old settings migration.
- Re-run Daz Script UI/unit tests.

### Milestone G: Optional Mesh Combining And Atlasing

Status: Deferred

- Implement only after material dedupe and texture priority have proven useful.
- Start with static props.
- Add skinned mesh combining only if bone weights, blendshapes, bounds, and animation survive in Unity validation.
- Treat atlas generation as a separate, opt-in pipeline with explicit UV remap output.

## Verification Matrix

Static checks:

- C++ compiles with Qt 4 and Daz SDK headers.
- Unity editor scripts compile in the target Unity version.
- DTU parser handles missing fork-specific metadata for old exports.

Daz-side checks:

- Default export matches current output shape.
- Shell exclusion restores scene state after success and failure.
- Strand hair export creates `.abc` files when enabled.
- Non-interactive export can set and use all new options.

Unity-side checks:

- FBX and DTU import still generate a prefab.
- Reused materials remain assigned correctly.
- Texture max sizes are applied as configured.
- Missing Alembic support produces a clear warning.
- Alembic hair imports and parents correctly when support is available.

Manual QA scenes to maintain:

- Plain Genesis figure.
- Genesis figure with clothing and polygon hair.
- Genesis figure with plugin-created shell overlays.
- Genesis figure with geografts.
- Genesis figure with strand-based hair.
- Static prop with repeated materials.
- Character with face/body texture priority rules.

## Risk Register

- Daz shell detection may vary by plugin. Mitigation: support both class/type detection and user-editable name rules.
- Strand hair Alembic support may require SDK work not currently wired into this repo. Mitigation: probe built-in exporter first, then decide whether direct Alembic SDK writing is justified.
- Unity Alembic import may require an optional package in the target project. Mitigation: warn and skip hair import without failing the whole character.
- Material merging can change render behavior if transparent, double-sided, shader, UV, or texture properties differ. Mitigation: signature-based reuse first, manual merge rules later.
- Skinned mesh combining can break blendshapes, bone weights, bounds, and animation. Mitigation: keep it opt-in and late.
- UI changes can break QA scripts. Mitigation: keep defaults and scriptable setters stable.

## Decision Log

- 2026-07-08: Prioritize export profile/schema before behavior changes so Daz UI, non-interactive scripts, DTU metadata, and Unity importer stay aligned.
- 2026-07-08: Treat texture priority as material/surface weighting first. True UV island weighting and atlasing are deferred until the simpler policy proves useful.
- 2026-07-08: Treat mesh combining as optional and late, especially for skinned characters.
- 2026-07-08: Prefer Unity-side material dedupe before native FBX mesh mutation.

## Backlog

- [ ] Add Daz diagnostic node/material dump command.
- [ ] Design fork-specific DTU metadata section.
- [ ] Add export profile object and default profile.
- [ ] Add profile persistence through `QSettings`.
- [ ] Add Unity parser support for profile metadata.
- [ ] Probe Daz SDK shell/geoshell identification.
- [ ] Implement shell exclusion restore table.
- [ ] Skip excluded nodes in DTU material/dForce recursion.
- [ ] Add shell exclusion QA scene and script.
- [ ] Probe Daz Alembic exporter availability.
- [ ] Probe strand hair node classes and data access.
- [ ] Choose built-in Alembic exporter vs direct Alembic SDK writer.
- [ ] Add `HairAssets` DTU schema.
- [ ] Export static strand hair Alembic.
- [ ] Add Unity Alembic package detection.
- [ ] Parent imported hair assets under generated prefab.
- [ ] Add material signature hashing in Unity importer.
- [ ] Reuse duplicate Unity material assets.
- [ ] Add texture priority rules and importer max-size handling.
- [ ] Add profile UI controls to `DzUnityDialog`.
- [ ] Redesign dialog sections around Content, Hair, Optimization, Morphs/SubD, and Advanced.
- [ ] Add export summary panel.
- [ ] Investigate static prop mesh combining.
- [ ] Investigate skinned mesh combining only after material/texture optimization is stable.
- [ ] Investigate optional atlas generation and UV remapping.
