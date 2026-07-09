import json
import gzip
import os
import shutil
import subprocess
import sys
import traceback
from collections import defaultdict, deque

try:
    import bpy
except ImportError as exc:
    raise SystemExit("This script must be run by Blender: %s" % exc)


def _argv_after_separator():
    if "--" not in sys.argv:
        return []
    return sys.argv[sys.argv.index("--") + 1:]


def _load_job():
    args = _argv_after_separator()
    if len(args) != 1:
        raise RuntimeError("Expected one job JSON path after --")
    job_path = os.path.abspath(args[0])
    with open(job_path, "r", encoding="utf-8-sig") as handle:
        job = json.load(handle)
    job["_JobPath"] = job_path
    return job


def _write_result(job, result):
    result_path = job.get("ResultJson") or os.path.splitext(job["_JobPath"])[0] + ".result.json"
    result_path = os.path.abspath(result_path)
    os.makedirs(os.path.dirname(result_path), exist_ok=True)
    with open(result_path, "w", encoding="utf-8") as handle:
        json.dump(result, handle, indent=2, sort_keys=True)
    print("Wrote result:", result_path)


def _clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete()


def _operator_property_names(operator):
    try:
        return {prop.identifier for prop in operator.get_rna_type().properties}
    except Exception:
        return set()


def _call_operator(operator, kwargs):
    props = _operator_property_names(operator)
    filtered = {key: value for key, value in kwargs.items() if not props or key in props}
    return operator(**filtered)


def _enable_diffeomorphic(warnings):
    try:
        bpy.ops.preferences.addon_enable(module="import_daz")
        return True
    except Exception as exc:
        warnings.append("Could not enable Diffeomorphic add-on module 'import_daz': %s" % exc)
        return False


def _configure_diffeomorphic(job, warnings):
    try:
        from import_daz import settings
        gs = getattr(settings, "GS", None)
        if gs is None:
            return

        for attr in ("useHairGuides", "useHairGuide", "useHair"):
            if hasattr(gs, attr):
                setattr(gs, attr, True)

        content_dirs = [os.path.abspath(path) for path in job.get("ContentDirs", []) if path]
        for attr in ("contentDirs", "rootPaths"):
            if hasattr(gs, attr) and content_dirs:
                current = list(getattr(gs, attr) or [])
                for path in content_dirs:
                    if path not in current:
                        current.append(path)
                setattr(gs, attr, current)
    except Exception as exc:
        warnings.append("Could not configure Diffeomorphic settings: %s" % exc)


def _import_with_diffeomorphic(job, warnings):
    if not _enable_diffeomorphic(warnings):
        return False

    _configure_diffeomorphic(job, warnings)

    source = job.get("SourceDuf") or job.get("SourceDbz")
    if not source:
        raise RuntimeError("Job must provide SourceDuf or SourceDbz")
    source = os.path.abspath(source)
    if not os.path.exists(source):
        raise RuntimeError("Diffeomorphic source file was not found: %s" % source)

    if not hasattr(bpy.ops, "daz"):
        raise RuntimeError("Diffeomorphic operators were not registered under bpy.ops.daz")

    directory = os.path.dirname(source)
    filename = os.path.basename(source)
    attempts = []
    for op_name in ("import_daz_manually", "easy_import_daz"):
        if not hasattr(bpy.ops.daz, op_name):
            continue
        operator = getattr(bpy.ops.daz, op_name)
        props = _operator_property_names(operator)
        base_kwargs = {
            "directory": directory,
            "files": [{"name": filename}],
            "filepath": source,
        }
        if "fitMeshes" in props and job.get("SourceDbz"):
            base_kwargs["fitMeshes"] = "DBZFILE"
        if "useHairGuides" in props:
            base_kwargs["useHairGuides"] = True
        if "useHairGuide" in props:
            base_kwargs["useHairGuide"] = True
        try:
            _call_operator(operator, base_kwargs)
            warnings.append("Imported source with Diffeomorphic operator daz.%s" % op_name)
            return True
        except Exception as exc:
            attempts.append("daz.%s: %s" % (op_name, exc))

    raise RuntimeError("Diffeomorphic import failed. Attempts: %s" % " | ".join(attempts))


def _create_synthetic_polyline(job):
    mesh = bpy.data.meshes.new("SyntheticHairPolylineMesh")
    vertices = [
        (-0.30, 0.00, 0.00),
        (-0.20, 0.00, 0.45),
        (-0.10, 0.00, 0.85),
        (0.05, 0.02, 1.15),
        (0.20, 0.03, 0.10),
        (0.25, 0.05, 0.50),
        (0.30, 0.04, 0.95),
    ]
    edges = [(0, 1), (1, 2), (2, 3), (4, 5), (5, 6)]
    mesh.from_pydata(vertices, edges, [])
    mesh.update()
    obj = bpy.data.objects.new(job.get("HairNodeName") or "SyntheticHair", mesh)
    bpy.context.collection.objects.link(obj)
    obj["DazHairType"] = "LINE"
    return obj


def _daz_to_blender_point(point, scale=0.01, zup=True):
    if zup:
        return (scale * point[0], -scale * point[2], scale * point[1])
    return (scale * point[0], scale * point[1], scale * point[2])


def _load_dbz_json(path):
    with open(path, "rb") as probe:
        signature = probe.read(2)
    if signature == b"\x1f\x8b":
        with gzip.open(path, "rt", encoding="utf-8-sig", errors="replace") as handle:
            return json.load(handle)
    with open(path, "r", encoding="utf-8-sig") as handle:
        return json.load(handle)


def _dbz_polyline_indices(polyline, vertex_count):
    if not isinstance(polyline, list):
        return []
    if polyline and isinstance(polyline[-1], list):
        values = polyline[-1]
    else:
        values = polyline
    indices = [int(value) for value in values if isinstance(value, int) or (isinstance(value, float) and value.is_integer())]
    return [index for index in indices if 0 <= index < vertex_count]


def _score_dbz_figure(figure, keywords):
    text = " ".join([
        str(figure.get("name", "")),
        str(figure.get("id", "")),
        str(figure.get("label", "")),
        str(figure.get("class", "")),
    ]).lower()
    score = 0
    if any(keyword and keyword in text for keyword in keywords):
        score += 100
    if any(token in text for token in ("hair", "strand", "mohawk", "beard", "mustache", "moustache")):
        score += 25
    if figure.get("polylines"):
        score += 50
    if figure.get("hd polylines"):
        score += 40
    return score


def _create_curves_from_dbz(job, warnings):
    source_dbz = job.get("SourceDbz")
    if not source_dbz:
        return []
    source_dbz = os.path.abspath(source_dbz)
    if not os.path.exists(source_dbz):
        warnings.append("DBZ polyline fallback skipped because SourceDbz was not found: %s" % source_dbz)
        return []

    data = _load_dbz_json(source_dbz)
    figures = data.get("figures", [])
    keywords = _object_keywords(job)
    scored = []
    for figure in figures:
        polylines = figure.get("polylines") or figure.get("hd polylines") or []
        vertices = figure.get("vertices") or figure.get("hd vertices") or []
        if not polylines or not vertices:
            continue
        score = _score_dbz_figure(figure, keywords)
        if score > 0:
            scored.append((score, figure))

    if not scored:
        warnings.append("DBZ polyline fallback found no figure with vertices and polylines")
        return []

    scored.sort(key=lambda item: -item[0])
    figure = scored[0][1]
    vertices = figure.get("vertices") or figure.get("hd vertices") or []
    polylines = figure.get("polylines") or figure.get("hd polylines") or []
    curve = bpy.data.curves.new("%s_DBZ_Curve" % (figure.get("name") or job.get("HairNodeName") or "Hair"), "CURVE")
    curve.dimensions = "3D"
    curve.resolution_u = 1
    curve.bevel_depth = 0.0

    scale = float(job.get("DazScale", 0.01))
    zup = bool(job.get("DazZUp", True))
    strand_count = 0
    point_count = 0
    for polyline in polylines:
        indices = _dbz_polyline_indices(polyline, len(vertices))
        if len(indices) < 2:
            continue
        spline = curve.splines.new("POLY")
        spline.points.add(len(indices) - 1)
        for point, vertex_index in zip(spline.points, indices):
            co = _daz_to_blender_point(vertices[vertex_index], scale, zup)
            point.co = (co[0], co[1], co[2], 1.0)
        strand_count += 1
        point_count += len(indices)

    if strand_count == 0:
        warnings.append("DBZ polyline fallback found polylines, but none had at least two valid vertices")
        return []

    obj = bpy.data.objects.new(curve.name, curve)
    bpy.context.collection.objects.link(obj)
    warnings.append(
        "Created Blender curve directly from DBZ polylines: %s strands, %s points, figure=%s"
        % (strand_count, point_count, figure.get("label") or figure.get("name"))
    )
    return [obj]


def _mesh_edge_chains(mesh):
    adjacency = defaultdict(set)
    for edge in mesh.edges:
        a, b = edge.vertices
        adjacency[a].add(b)
        adjacency[b].add(a)

    if not adjacency:
        return []

    for vertex, neighbors in adjacency.items():
        if len(neighbors) > 2:
            raise RuntimeError("Mesh polyline topology branches at vertex %s" % vertex)

    remaining = {tuple(sorted(edge.vertices)) for edge in mesh.edges}
    chains = []

    while remaining:
        component_edges = set()
        seed = next(iter(remaining))
        queue = deque([seed[0], seed[1]])
        component_vertices = set()
        while queue:
            vertex = queue.popleft()
            if vertex in component_vertices:
                continue
            component_vertices.add(vertex)
            for neighbor in adjacency[vertex]:
                edge_key = tuple(sorted((vertex, neighbor)))
                if edge_key in remaining:
                    component_edges.add(edge_key)
                    queue.append(neighbor)

        endpoints = [vertex for vertex in component_vertices if len(adjacency[vertex]) == 1]
        start = endpoints[0] if endpoints else next(iter(component_vertices))
        chain = [start]
        previous = None
        current = start
        while True:
            next_vertices = [
                vertex for vertex in adjacency[current]
                if vertex != previous and tuple(sorted((current, vertex))) in remaining
            ]
            if not next_vertices:
                break
            nxt = next_vertices[0]
            remaining.discard(tuple(sorted((current, nxt))))
            previous = current
            current = nxt
            chain.append(current)
            if current == start:
                break

        for edge_key in component_edges:
            remaining.discard(edge_key)
        if len(chain) >= 2:
            chains.append(chain)

    return chains


def _object_keywords(job):
    values = [
        job.get("HairNodeName", ""),
        job.get("HairNodeLabel", ""),
        job.get("TargetObjectName", ""),
    ]
    return [value.lower() for value in values if value]


def _score_object(obj, keywords):
    name = obj.name.lower()
    score = 0
    if any(keyword and keyword in name for keyword in keywords):
        score += 100
    if "guide" in name or name.endswith("_guides") or name.endswith("_guide"):
        score += 50
    if any(token in name for token in ("hair", "strand", "mohawk", "beard", "mustache", "moustache")):
        score += 25
    if obj.type == "CURVE":
        score += 40
    if obj.type == "MESH":
        mesh = obj.data
        if len(mesh.edges) > 0 and len(mesh.polygons) == 0:
            score += 60
        if str(obj.get("DazHairType", "")).upper() in ("LINE", "SBH", "GUIDE"):
            score += 40
    return score


def _find_hair_candidates(job, warnings):
    keywords = _object_keywords(job)
    scored = []
    for obj in bpy.context.scene.objects:
        if obj.type not in ("CURVE", "MESH"):
            continue
        score = _score_object(obj, keywords)
        if score <= 0:
            continue
        if obj.type == "MESH" and len(obj.data.edges) == 0:
            warnings.append("Rejected mesh candidate without ordered strand edges: %s" % obj.name)
            continue
        if obj.type == "MESH" and len(obj.data.polygons) > 0:
            warnings.append("Rejected mesh-card candidate with faces: %s" % obj.name)
            continue
        scored.append((score, obj.name, obj))

    scored.sort(key=lambda item: (-item[0], item[1]))
    return [item[2] for item in scored]


def _convert_mesh_to_curve(obj):
    chains = _mesh_edge_chains(obj.data)
    if not chains:
        raise RuntimeError("Mesh candidate has no usable edge polylines: %s" % obj.name)

    curve = bpy.data.curves.new(obj.name + "_Curve", "CURVE")
    curve.dimensions = "3D"
    curve.resolution_u = 1
    curve.bevel_depth = 0.0

    matrix = obj.matrix_world.copy()
    for chain in chains:
        spline = curve.splines.new("POLY")
        spline.points.add(len(chain) - 1)
        for point, vertex_index in zip(spline.points, chain):
            co = matrix @ obj.data.vertices[vertex_index].co
            point.co = (co.x, co.y, co.z, 1.0)

    curve_obj = bpy.data.objects.new(curve.name, curve)
    bpy.context.collection.objects.link(curve_obj)
    return curve_obj


def _prepare_curve_objects(job, warnings):
    if job.get("SyntheticPolyline"):
        _create_synthetic_polyline(job)

    candidates = _find_hair_candidates(job, warnings)
    if not candidates:
        dbz_curve_objects = _create_curves_from_dbz(job, warnings)
        if dbz_curve_objects:
            return dbz_curve_objects

    if not candidates:
        raise RuntimeError("No guide, curve, or mesh-polyline hair candidates were found")

    curve_objects = []
    for obj in candidates:
        if obj.type == "CURVE":
            curve_objects.append(obj)
        elif obj.type == "MESH":
            curve_objects.append(_convert_mesh_to_curve(obj))

    if not curve_objects:
        raise RuntimeError("Hair candidates were found, but none could be converted to Blender curves")
    return curve_objects


def _export_alembic(job, curve_objects):
    output = os.path.abspath(job["OutputAlembic"])
    os.makedirs(os.path.dirname(output), exist_ok=True)

    for obj in bpy.context.scene.objects:
        obj.select_set(False)
    for obj in curve_objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = curve_objects[0]

    frame = int(job.get("Frame", 1))
    kwargs = {
        "filepath": output,
        "selected": True,
        "curves_as_mesh": False,
        "export_hair": True,
        "export_particles": False,
        "flatten": False,
        "start": frame,
        "end": frame,
        "as_background_job": False,
    }
    _call_operator(bpy.ops.wm.alembic_export, kwargs)
    return output


def _find_abcls(job):
    candidates = [
        job.get("AbcInspectExe", ""),
        os.environ.get("DAZ_TO_UNITY_ABCLS", ""),
        r"D:\SDKs\vcpkg\installed\x64-windows\tools\alembic\abcls.exe",
        shutil.which("abcls") or "",
    ]
    for candidate in candidates:
        if candidate and os.path.exists(candidate):
            return candidate
    return ""


def _inspect_alembic(job, output, warnings):
    abcls = _find_abcls(job)
    if not abcls:
        warnings.append("abcls.exe was not found; schema verification was skipped")
        return "", False

    completed = subprocess.run(
        [abcls, "-r", "-l", "-m", output],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        check=False,
    )
    summary = completed.stdout.strip()
    if completed.returncode != 0:
        warnings.append("abcls failed with exit code %s" % completed.returncode)
    return summary, "AbcGeom_Curve_v2" in summary


def main():
    job = _load_job()
    warnings = []
    result = {
        "Success": False,
        "ContainsCurveV2": False,
        "OutputAlembic": os.path.abspath(job.get("OutputAlembic", "")) if job.get("OutputAlembic") else "",
        "ObjectNames": [],
        "SchemaSummary": "",
        "Warnings": warnings,
        "Error": "",
    }

    try:
        _clear_scene()
        if not job.get("SyntheticPolyline"):
            _import_with_diffeomorphic(job, warnings)
        curve_objects = _prepare_curve_objects(job, warnings)
        result["ObjectNames"] = [obj.name for obj in curve_objects]
        output = _export_alembic(job, curve_objects)
        result["OutputAlembic"] = output
        summary, contains_curve = _inspect_alembic(job, output, warnings)
        result["SchemaSummary"] = summary
        result["ContainsCurveV2"] = contains_curve
        result["Success"] = bool(contains_curve and os.path.exists(output) and os.path.getsize(output) > 0)
        if not result["Success"] and not result["Error"]:
            result["Error"] = "Alembic export completed, but AbcGeom_Curve_v2 was not verified"
    except Exception as exc:
        result["Error"] = "%s\n%s" % (exc, traceback.format_exc())
    finally:
        _write_result(job, result)

    if not result["Success"]:
        raise SystemExit(2)


if __name__ == "__main__":
    main()
