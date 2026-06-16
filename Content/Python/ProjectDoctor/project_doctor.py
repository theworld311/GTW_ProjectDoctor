
import csv
import json
import os
import datetime
import unreal

SEVERITY_INFO = "Info"
SEVERITY_WARNING = "Warning"
SEVERITY_ERROR = "Error"

class Finding:
    def __init__(self, severity, rule, asset_path, message):
        self.severity = severity
        self.rule = rule
        self.asset_path = asset_path
        self.message = message

    def to_dict(self):
        return {
            "severity": self.severity,
            "rule": self.rule,
            "asset_path": self.asset_path,
            "message": self.message,
        }


def _asset_registry():
    return unreal.AssetRegistryHelpers.get_asset_registry()


def _load_asset(asset_data):
    try:
        return asset_data.get_asset()
    except Exception:
        return None


def _class_name(asset_data):
    try:
        return str(asset_data.asset_class_path.asset_name)
    except Exception:
        try:
            return str(asset_data.asset_class)
        except Exception:
            return "Unknown"


def _object_path(asset_data):
    try:
        return str(asset_data.get_soft_object_path())
    except Exception:
        return str(asset_data.package_name)


# ---------------------------------------------------------------------------
# Texture
# ---------------------------------------------------------------------------

def _scan_texture(asset_data, asset, findings):
    size_x = getattr(asset, "blueprint_get_size_x", lambda: 0)()
    size_y = getattr(asset, "blueprint_get_size_y", lambda: 0)()
    path = _object_path(asset_data)
    name = str(asset_data.asset_name)

    if size_x >= 8192 or size_y >= 8192:
        findings.append(Finding(SEVERITY_WARNING, "TextureResolution", path,
                                f"Texture is very large: {size_x}x{size_y}. Consider downsizing."))
    elif size_x >= 4096 or size_y >= 4096:
        findings.append(Finding(SEVERITY_INFO, "TextureResolution", path,
                                f"Texture is high resolution: {size_x}x{size_y}."))

    # Non-power-of-two
    def _is_pot(v):
        return v > 0 and (v & (v - 1)) == 0
    if size_x > 0 and size_y > 0 and (not _is_pot(size_x) or not _is_pot(size_y)):
        findings.append(Finding(SEVERITY_WARNING, "TextureNPOT", path,
                                f"Texture is not power-of-two ({size_x}x{size_y}). May cause streaming issues."))

    try:
        is_normalmap = asset.compression_settings == unreal.TextureCompressionSettings.TC_NORMALMAP
        if is_normalmap and not name.lower().endswith(("_n", "_normal", "_nrm", "_nm")):
            findings.append(Finding(SEVERITY_INFO, "Naming", path,
                                    "Normal map texture name does not end with _N / _Normal / _NRM."))
        if not is_normalmap and name.lower().endswith(("_n", "_normal", "_nrm", "_nm")):
            findings.append(Finding(SEVERITY_WARNING, "TextureCompressionMismatch", path,
                                    "Name suggests a normal map but compression is not TC_NormalMap."))
    except Exception:
        pass

    # sRGB check: colour textures should have sRGB on, data textures off
    try:
        srgb = asset.srgb
        compression = asset.compression_settings
        data_compressions = {
            unreal.TextureCompressionSettings.TC_NORMALMAP,
            unreal.TextureCompressionSettings.TC_MASKS,
            unreal.TextureCompressionSettings.TC_GRAYSCALE,
            unreal.TextureCompressionSettings.TC_DISPLACEMENTMAP,
            unreal.TextureCompressionSettings.TC_VECTOR_DISPLACEMENTMAP,
        }
        if compression in data_compressions and srgb:
            findings.append(Finding(SEVERITY_WARNING, "TexturesRGB", path,
                                    "Data/mask texture has sRGB enabled. This can cause incorrect values at runtime."))
    except Exception:
        pass

    # MipMaps
    try:
        mip_gen = asset.mip_gen_settings
        no_mip = unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
        if size_x >= 64 and size_y >= 64 and mip_gen == no_mip:
            findings.append(Finding(SEVERITY_INFO, "TextureMips", path,
                                    "Texture has mip generation disabled. UI textures are exempt, but verify this is intentional."))
    except Exception:
        pass


# ---------------------------------------------------------------------------
# Static Mesh
# ---------------------------------------------------------------------------

def _scan_static_mesh(asset_data, asset, findings):
    path = _object_path(asset_data)
    path_lower = path.lower()
    is_foliage = any(k in path_lower for k in ("/foliage/", "grass", "fern", "clover", "bush", "tree", "shrub"))

    try:
        body_setup = asset.get_editor_property("body_setup")
        agg_geom = body_setup.get_editor_property("agg_geom") if body_setup else None
        has_simple = False
        if agg_geom:
            for prop in ["box_elems", "sphere_elems", "sphyl_elems", "convex_elems", "tapered_capsule_elems"]:
                try:
                    if len(agg_geom.get_editor_property(prop)) > 0:
                        has_simple = True
                        break
                except Exception:
                    pass
        if not has_simple:
            sev = SEVERITY_INFO if is_foliage else SEVERITY_WARNING
            msg = ("Foliage-style mesh has no simple collision. Confirm this is intentional."
                   if is_foliage else "Static mesh has no simple collision.")
            findings.append(Finding(sev, "StaticMeshCollision", path, msg))
    except Exception:
        findings.append(Finding(SEVERITY_INFO, "StaticMeshCollision", path,
                                "Could not inspect static mesh collision."))

    try:
        lod_count = asset.get_num_lods()
        if lod_count <= 1:
            findings.append(Finding(SEVERITY_INFO, "StaticMeshLOD", path,
                                    "Mesh has only 1 LOD. Confirm Nanite is enabled or add LODs before shipping."))
    except Exception:
        pass

    # Lightmap UV
    try:
        lightmap_index = asset.get_editor_property("light_map_coordinate_index")
        if lightmap_index == 0:
            findings.append(Finding(SEVERITY_INFO, "StaticMeshLightmapUV", path,
                                    "Lightmap UV is on channel 0 (same as base UVs). Consider a dedicated channel 1."))
    except Exception:
        pass

    # Nanite
    try:
        nanite = asset.get_editor_property("nanite_settings")
        enabled = nanite.get_editor_property("enabled") if nanite else False
        lod_count = asset.get_num_lods()
        if not enabled and lod_count <= 1:
            findings.append(Finding(SEVERITY_INFO, "StaticMeshNanite", path,
                                    "Nanite is disabled and mesh has no LODs. One of the two is recommended."))
    except Exception:
        pass


# ---------------------------------------------------------------------------
# Skeletal Mesh
# ---------------------------------------------------------------------------

def _scan_skeletal_mesh(asset_data, asset, findings):
    path = _object_path(asset_data)

    try:
        lod_count = asset.get_num_lods()
        if lod_count <= 1:
            findings.append(Finding(SEVERITY_INFO, "SkeletalMeshLOD", path,
                                    "Skeletal mesh has only 1 LOD. Add LODs to reduce draw cost on distant characters."))
    except Exception:
        pass

    try:
        physics_asset = asset.get_editor_property("physics_asset")
        if physics_asset is None:
            findings.append(Finding(SEVERITY_INFO, "SkeletalMeshPhysics", path,
                                    "Skeletal mesh has no Physics Asset assigned. Required for ragdoll and cloth simulation."))
    except Exception:
        pass

    try:
        skeleton = asset.get_editor_property("skeleton")
        if skeleton is None:
            findings.append(Finding(SEVERITY_ERROR, "SkeletalMeshSkeleton", path,
                                    "Skeletal mesh has no Skeleton assigned. Asset may be broken."))
    except Exception:
        pass


# ---------------------------------------------------------------------------
# Material
# ---------------------------------------------------------------------------

def _scan_material(asset_data, asset, findings):
    path = _object_path(asset_data)

    try:
        blend_mode = asset.get_editor_property("blend_mode")
        two_sided = asset.get_editor_property("two_sided")
        if two_sided:
            findings.append(Finding(SEVERITY_INFO, "MaterialTwoSided", path,
                                    "Material is two-sided. This doubles rasterisation cost. Use only when needed."))
        translucent_modes = {
            unreal.BlendMode.BLEND_TRANSLUCENT,
            unreal.BlendMode.BLEND_ADDITIVE,
            unreal.BlendMode.BLEND_MODULATE,
        }
        if blend_mode in translucent_modes:
            findings.append(Finding(SEVERITY_INFO, "MaterialTranslucency", path,
                                    "Material uses translucency. Verify sorting and performance impact."))
    except Exception:
        pass

    try:
        shading_model = asset.get_editor_property("shading_model")
        if shading_model == unreal.MaterialShadingModel.MSM_SUBSURFACE:
            findings.append(Finding(SEVERITY_INFO, "MaterialShading", path,
                                    "Material uses Subsurface shading model which has extra cost. Confirm it is intentional."))
    except Exception:
        pass

    # Check if Material has any Material Instances referencing it
    registry = _asset_registry()
    referencers = registry.get_referencers(
        asset_data.package_name,
        unreal.AssetRegistryDependencyOptions()
    )
    has_instance = any("MI_" in str(r) or "MaterialInstance" in str(r) for r in referencers)
    if not has_instance:
        findings.append(Finding(SEVERITY_INFO, "MaterialNoInstance", path,
                                "Material has no Material Instance. Direct material usage prevents parameter overrides."))


# ---------------------------------------------------------------------------
# Material Instance
# ---------------------------------------------------------------------------

def _scan_material_instance(asset_data, asset, findings):
    path = _object_path(asset_data)
    try:
        parent = asset.get_editor_property("parent")
        if parent is None:
            findings.append(Finding(SEVERITY_ERROR, "MaterialInstanceNoParent", path,
                                    "Material Instance has no parent material. Asset is broken."))
    except Exception:
        pass


# ---------------------------------------------------------------------------
# Blueprint
# ---------------------------------------------------------------------------

def _scan_blueprint(asset_data, asset, findings):
    path = _object_path(asset_data)

    try:
        # Check for compile errors
        status = asset.get_editor_property("status")
        if str(status) in ("BS_Error", "BlueprintStatus.BS_ERROR"):
            findings.append(Finding(SEVERITY_ERROR, "BlueprintCompileError", path,
                                    "Blueprint has compile errors. Must be fixed before packaging."))
            return
    except Exception:
        pass

    try:
        # Check for deprecated parent class
        parent_class = asset.get_editor_property("parent_class")
        if parent_class is not None:
            parent_name = str(parent_class)
            if "deprecated" in parent_name.lower():
                findings.append(Finding(SEVERITY_WARNING, "BlueprintDeprecatedParent", path,
                                        f"Blueprint inherits from a deprecated class: {parent_name}."))
    except Exception:
        pass

    try:
        # Variables without categories (makes large Blueprints hard to maintain)
        variables = asset.get_editor_property("new_variables")
        uncategorised = [v for v in variables if not str(v.get_editor_property("category")).strip()
                         or str(v.get_editor_property("category")).strip() in ("", "Default")]
        if len(uncategorised) > 5:
            findings.append(Finding(SEVERITY_INFO, "BlueprintVariableCategories", path,
                                    f"{len(uncategorised)} variables have no category. Add categories to keep the Details panel organised."))
    except Exception:
        pass


# ---------------------------------------------------------------------------
# Sound
# ---------------------------------------------------------------------------

def _scan_sound(asset_data, asset, findings):
    path = _object_path(asset_data)
    name = str(asset_data.asset_name)
    class_name = _class_name(asset_data)

    # Naming: A_ prefix for all sound assets
    if not name.startswith("A_"):
        findings.append(Finding(SEVERITY_INFO, "Naming", path,
                                f"Sound asset does not start with A_ prefix (class: {class_name})."))

    try:
        # SoundWave specific checks
        if class_name == "SoundWave":
            duration = asset.get_editor_property("duration")
            if duration and duration > 30:
                findings.append(Finding(SEVERITY_WARNING, "SoundDuration", path,
                                        f"SoundWave is {duration:.1f}s long. Long sounds should be streamed or converted to SoundCue."))
            try:
                streaming = asset.get_editor_property("streaming")
                if duration and duration > 10 and not streaming:
                    findings.append(Finding(SEVERITY_INFO, "SoundStreaming", path,
                                            "Long SoundWave has streaming disabled. Enable streaming to reduce memory usage."))
            except Exception:
                pass
    except Exception:
        pass


# ---------------------------------------------------------------------------
# Niagara / Particle
# ---------------------------------------------------------------------------

def _scan_niagara(asset_data, asset, findings):
    path = _object_path(asset_data)
    name = str(asset_data.asset_name)

    if not name.startswith(("NS_", "FX_", "PS_")):
        findings.append(Finding(SEVERITY_INFO, "Naming", path,
                                "Niagara/Particle system does not start with NS_, FX_, or PS_ prefix."))

    try:
        # Check for fixed bounds (helps culling)
        fixed_bounds = asset.get_editor_property("fixed_bounds")
        if fixed_bounds and fixed_bounds.is_valid() and fixed_bounds == unreal.Box():
            findings.append(Finding(SEVERITY_INFO, "NiagaraBounds", path,
                                    "Niagara system has no fixed bounds set. This may hurt occlusion culling."))
    except Exception:
        pass


# ---------------------------------------------------------------------------
# Naming (generic)
# ---------------------------------------------------------------------------

PREFIXES = {
    "StaticMesh": "SM_",
    "SkeletalMesh": "SK_",
    "Texture2D": "T_",
    "TextureCube": "T_",
    "TextureRenderTarget2D": "RT_",
    "Material": "M_",
    "MaterialInstanceConstant": "MI_",
    "Blueprint": "BP_",
    "SoundWave": "A_",
    "SoundCue": "A_",
    "AnimSequence": "AS_",
    "AnimMontage": "AM_",
    "AnimBlueprint": "ABP_",
    "BlendSpace": "BS_",
    "BlendSpace1D": "BS_",
    "PhysicsAsset": "PHYS_",
    "DataTable": "DT_",
    "DataAsset": "DA_",
    "UserDefinedEnum": "E_",
    "UserDefinedStruct": "F_",
    "WidgetBlueprint": "WBP_",
    "NiagaraSystem": "NS_",
    "NiagaraEmitter": "NE_",
}


def _scan_naming(asset_data, findings):
    name = str(asset_data.asset_name)
    path = _object_path(asset_data)
    class_name = _class_name(asset_data)
    expected = PREFIXES.get(class_name)
    if expected and not name.startswith(expected):
        findings.append(Finding(SEVERITY_INFO, "Naming", path,
                                f"{class_name} name '{name}' should start with '{expected}'."))


# ---------------------------------------------------------------------------
# HTML report
# ---------------------------------------------------------------------------

def _escape_html(value):
    return str(value).replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace('"', "&quot;")


def _write_html_report(path, summary, findings):
    rows = []
    for finding in findings:
        severity_class = finding.severity.lower()
        rows.append(
            "<tr class='{sc}'>"
            "<td>{sev}</td><td>{rule}</td><td class='asset'>{asset}</td><td>{msg}</td>"
            "</tr>".format(
                sc=_escape_html(severity_class),
                sev=_escape_html(finding.severity),
                rule=_escape_html(finding.rule),
                asset=_escape_html(finding.asset_path),
                msg=_escape_html(finding.message),
            )
        )

    html = """<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>Project Doctor Report</title>
<style>
*{{box-sizing:border-box;margin:0;padding:0}}
body{{font-family:Segoe UI,Arial,sans-serif;background:#f0f2f5;color:#1a1a2e;padding:24px}}
h1{{font-size:26px;font-weight:700;margin-bottom:4px}}
.subtitle{{color:#666;font-size:13px;margin-bottom:20px}}
.summary{{display:flex;gap:12px;flex-wrap:wrap;margin-bottom:24px}}
.card{{background:#fff;border:1px solid #dde3ee;border-radius:8px;padding:14px 20px;min-width:130px}}
.card .label{{font-size:11px;text-transform:uppercase;color:#888;letter-spacing:.5px}}
.card .value{{font-size:28px;font-weight:700;margin-top:4px}}
.card.errors .value{{color:#c0392b}}
.card.warnings .value{{color:#e67e22}}
.card.info .value{{color:#2980b9}}
.filters{{display:flex;gap:8px;margin-bottom:12px;flex-wrap:wrap}}
.filters button{{padding:6px 14px;border:1px solid #ccd;border-radius:20px;background:#fff;cursor:pointer;font-size:13px;transition:all .15s}}
.filters button.active,.filters button:hover{{background:#2980b9;color:#fff;border-color:#2980b9}}
.filters button.all.active{{background:#444;border-color:#444}}
.filters button.error.active{{background:#c0392b;border-color:#c0392b}}
.filters button.warning.active{{background:#e67e22;border-color:#e67e22}}
.filters button.info.active{{background:#2980b9;border-color:#2980b9}}
.table-wrap{{background:#fff;border:1px solid #dde3ee;border-radius:8px;overflow:auto}}
table{{width:100%;border-collapse:collapse;font-size:13px}}
th{{padding:10px 12px;background:#f5f7fb;border-bottom:2px solid #dde3ee;text-align:left;font-size:11px;text-transform:uppercase;color:#777;letter-spacing:.4px}}
td{{padding:9px 12px;border-bottom:1px solid #edf0f7;vertical-align:top}}
tr:last-child td{{border-bottom:none}}
tr:hover td{{background:#f9fbff}}
.sev{{font-weight:700;white-space:nowrap}}
tr.error .sev{{color:#c0392b}}
tr.warning .sev{{color:#e67e22}}
tr.info .sev{{color:#2980b9}}
.asset{{color:#555;font-family:monospace;font-size:12px;word-break:break-all}}
.rule{{white-space:nowrap;color:#444}}
.hidden{{display:none}}
.count{{font-size:12px;color:#888;margin-bottom:8px}}
</style>
</head>
<body>
<h1>Project Doctor</h1>
<div class="subtitle">Root: {root} &nbsp;|&nbsp; Generated: {date}</div>
<div class="summary">
  <div class="card"><div class="label">Assets scanned</div><div class="value">{assets}</div></div>
  <div class="card"><div class="label">Total findings</div><div class="value">{total}</div></div>
  <div class="card errors"><div class="label">Errors</div><div class="value">{errors}</div></div>
  <div class="card warnings"><div class="label">Warnings</div><div class="value">{warnings}</div></div>
  <div class="card info"><div class="label">Info</div><div class="value">{info}</div></div>
</div>
<div class="filters">
  <button class="all active" onclick="filter(this,'all')">All ({total})</button>
  <button class="error" onclick="filter(this,'error')">Errors ({errors})</button>
  <button class="warning" onclick="filter(this,'warning')">Warnings ({warnings})</button>
  <button class="info" onclick="filter(this,'info')">Info ({info})</button>
</div>
<div class="count" id="count">{total} findings shown</div>
<div class="table-wrap">
<table id="findings-table">
<thead><tr><th>Severity</th><th>Rule</th><th>Asset</th><th>Message</th></tr></thead>
<tbody>
{rows}
</tbody>
</table>
</div>
<script>
function filter(btn, level) {{
  document.querySelectorAll('.filters button').forEach(b => b.classList.remove('active'));
  btn.classList.add('active');
  var rows = document.querySelectorAll('#findings-table tbody tr');
  var shown = 0;
  rows.forEach(function(r) {{
    if (level === 'all' || r.classList.contains(level)) {{
      r.classList.remove('hidden'); shown++;
    }} else {{
      r.classList.add('hidden');
    }}
  }});
  document.getElementById('count').textContent = shown + ' finding' + (shown !== 1 ? 's' : '') + ' shown';
}}
</script>
</body>
</html>""".format(
        root=_escape_html(summary.get("root_path", "-")),
        date=datetime.datetime.now().strftime("%Y-%m-%d %H:%M"),
        assets=_escape_html(summary.get("scanned_assets", 0)),
        total=_escape_html(summary.get("findings_total", 0)),
        errors=_escape_html(summary.get("errors", 0)),
        warnings=_escape_html(summary.get("warnings", 0)),
        info=_escape_html(summary.get("info", 0)),
        rows="\n".join(rows),
    )
    with open(path, "w", encoding="utf-8") as f:
        f.write(html)


# ---------------------------------------------------------------------------
# Main scan
# ---------------------------------------------------------------------------

SCANNABLE_CLASSES = {
    "Texture2D", "TextureCube",
    "StaticMesh",
    "SkeletalMesh",
    "Material", "MaterialInstanceConstant",
    "Blueprint", "AnimBlueprint", "WidgetBlueprint",
    "SoundWave", "SoundCue",
    "NiagaraSystem", "NiagaraEmitter",
    "ParticleSystem",
}


def scan_project(root_path="/Game", max_assets=5000, export=True):
    registry = _asset_registry()
    assets = registry.get_assets_by_path(root_path, recursive=True)
    findings = []

    for index, asset_data in enumerate(assets):
        if index >= max_assets:
            findings.append(Finding(SEVERITY_WARNING, "ScanLimit", root_path,
                                    f"Scan stopped at {max_assets} assets. Increase limit or narrow root path."))
            break

        class_name = _class_name(asset_data)
        path = _object_path(asset_data)

        if class_name == "ObjectRedirector":
            findings.append(Finding(SEVERITY_WARNING, "Redirector", path,
                                    "Redirector found. Run 'Fix Up Redirectors' before packaging."))
            continue

        # Naming check for all known asset types
        _scan_naming(asset_data, findings)

        # Deep inspection only for supported classes
        if class_name not in SCANNABLE_CLASSES:
            continue

        asset = _load_asset(asset_data)
        if asset is None:
            findings.append(Finding(SEVERITY_ERROR, "LoadAsset", path,
                                    "Asset could not be loaded for inspection."))
            continue

        if class_name in ("Texture2D", "TextureCube"):
            _scan_texture(asset_data, asset, findings)
        elif class_name == "StaticMesh":
            _scan_static_mesh(asset_data, asset, findings)
        elif class_name == "SkeletalMesh":
            _scan_skeletal_mesh(asset_data, asset, findings)
        elif class_name == "Material":
            _scan_material(asset_data, asset, findings)
        elif class_name == "MaterialInstanceConstant":
            _scan_material_instance(asset_data, asset, findings)
        elif class_name in ("Blueprint", "AnimBlueprint", "WidgetBlueprint"):
            _scan_blueprint(asset_data, asset, findings)
        elif class_name in ("SoundWave", "SoundCue"):
            _scan_sound(asset_data, asset, findings)
        elif class_name in ("NiagaraSystem", "NiagaraEmitter", "ParticleSystem"):
            _scan_niagara(asset_data, asset, findings)

    summary = {
        "project": unreal.Paths.get_project_file_path(),
        "root_path": root_path,
        "scanned_assets": min(len(assets), max_assets),
        "findings_total": len(findings),
        "errors": len([f for f in findings if f.severity == SEVERITY_ERROR]),
        "warnings": len([f for f in findings if f.severity == SEVERITY_WARNING]),
        "info": len([f for f in findings if f.severity == SEVERITY_INFO]),
    }

    report_paths = {}
    if export:
        saved_dir = os.path.join(unreal.Paths.project_saved_dir(), "ProjectDoctorReports")
        os.makedirs(saved_dir, exist_ok=True)
        stamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        json_path = os.path.join(saved_dir, f"project_doctor_report_{stamp}.json")
        csv_path = os.path.join(saved_dir, f"project_doctor_report_{stamp}.csv")
        html_path = os.path.join(saved_dir, f"project_doctor_report_{stamp}.html")
        latest_json_path = os.path.join(saved_dir, "latest_project_doctor_report.json")

        payload = {"summary": summary, "findings": [f.to_dict() for f in findings]}
        with open(json_path, "w", encoding="utf-8") as f:
            json.dump(payload, f, indent=2)
        with open(latest_json_path, "w", encoding="utf-8") as f:
            json.dump(payload, f, indent=2)
        with open(csv_path, "w", encoding="utf-8", newline="") as f:
            writer = csv.DictWriter(f, fieldnames=["severity", "rule", "asset_path", "message"])
            writer.writeheader()
            for finding in findings:
                writer.writerow(finding.to_dict())
        _write_html_report(html_path, summary, findings)
        report_paths = {
            "json": json_path,
            "csv": csv_path,
            "html": html_path,
            "latest_json": latest_json_path,
        }

    unreal.log("PROJECT_DOCTOR_SUMMARY " + json.dumps(summary))
    for finding in findings[:50]:
        unreal.log(f"PROJECT_DOCTOR_FINDING [{finding.severity}] {finding.rule}: {finding.asset_path} - {finding.message}")
    if len(findings) > 50:
        unreal.log(f"PROJECT_DOCTOR_FINDING Output truncated. Full report has {len(findings)} findings.")
    if report_paths:
        unreal.log("PROJECT_DOCTOR_REPORTS " + json.dumps(report_paths))

    return {"summary": summary, "reports": report_paths, "findings": [f.to_dict() for f in findings]}
