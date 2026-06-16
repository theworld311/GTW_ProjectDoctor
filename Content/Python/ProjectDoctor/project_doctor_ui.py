
import importlib
import json
import os
import sys
import unreal

_THIS_DIR = os.path.dirname(__file__)
if _THIS_DIR not in sys.path:
    sys.path.append(_THIS_DIR)

WINDOW_TITLE = "Project Doctor"
_LAST_RESULT = None


def _load_scanner():
    import project_doctor
    importlib.reload(project_doctor)
    return project_doctor


def _latest_report_path():
    return os.path.join(unreal.Paths.project_saved_dir(), "ProjectDoctorReports", "latest_project_doctor_report.json")


def _load_latest_result():
    global _LAST_RESULT
    if _LAST_RESULT:
        return _LAST_RESULT
    path = _latest_report_path()
    if os.path.exists(path):
        with open(path, "r", encoding="utf-8") as file:
            _LAST_RESULT = json.load(file)
            return _LAST_RESULT
    return {"summary": {}, "findings": [], "reports": {}}


def _run_scan(root_path, max_assets=2500):
    global _LAST_RESULT
    scanner = _load_scanner()
    _LAST_RESULT = scanner.scan_project(root_path=root_path, max_assets=max_assets, export=True)
    open_window()
    return _LAST_RESULT


def _summary_text(result):
    summary = result.get("summary", {})
    reports = result.get("reports", {})
    if not summary:
        return "No scan has been run yet."
    lines = [
        f"Root: {summary.get('root_path', '-')}",
        f"Assets scanned: {summary.get('scanned_assets', 0)}",
        f"Findings: {summary.get('findings_total', 0)} | Errors: {summary.get('errors', 0)} | Warnings: {summary.get('warnings', 0)} | Info: {summary.get('info', 0)}",
    ]
    if reports.get("html"):
        lines.append(f"HTML: {reports.get('html')}")
    return "\n".join(lines)


def _findings_text(result, limit=200):
    findings = result.get("findings", [])
    if not findings:
        return "No findings to show."
    lines = []
    for index, finding in enumerate(findings[:limit], start=1):
        lines.append(
            f"{index:03d}. [{finding.get('severity', '-')}] {finding.get('rule', '-')}\n"
            f"     {finding.get('asset_path', '-')}\n"
            f"     {finding.get('message', '-')}"
        )
    if len(findings) > limit:
        lines.append(f"\nShowing first {limit} of {len(findings)} findings. Open the HTML/CSV report for the full list.")
    return "\n\n".join(lines)


def _copy_report_path():
    result = _load_latest_result()
    reports = result.get("reports", {})
    path = reports.get("html") or reports.get("json") or reports.get("csv") or ""
    if path:
        unreal.SystemLibrary.clipboard_copy(path)
        unreal.EditorDialog.show_message("Project Doctor", "Report path copied to clipboard.", unreal.AppMsgType.OK)
    else:
        unreal.EditorDialog.show_message("Project Doctor", "No report path available yet.", unreal.AppMsgType.OK)


def _locate_first_error_or_warning():
    result = _load_latest_result()
    findings = result.get("findings", [])
    target = None
    for severity in ("Error", "Warning", "Info"):
        for finding in findings:
            if finding.get("severity") == severity:
                target = finding.get("asset_path")
                break
        if target:
            break
    if not target:
        unreal.EditorDialog.show_message("Project Doctor", "No finding asset to locate.", unreal.AppMsgType.OK)
        return

    asset = unreal.EditorAssetLibrary.load_asset(target)
    if asset:
        unreal.EditorAssetLibrary.sync_browser_to_objects([asset])
    else:
        unreal.EditorDialog.show_message("Project Doctor", f"Could not load asset:\n{target}", unreal.AppMsgType.OK)


def open_window():
    result = _load_latest_result()
    body = (
        "PROJECT DOCTOR\n"
        "==============\n\n"
        "Actions are available from Tools:\n"
        "- Project Doctor: Open Window\n"
        "- Project Doctor: Scan /Game\n"
        "- Project Doctor: Scan /Game/StylescapeHD\n\n"
        "SUMMARY\n"
        "-------\n"
        f"{_summary_text(result)}\n\n"
        "FINDINGS\n"
        "--------\n"
        f"{_findings_text(result)}"
    )

    # EditorDialog is intentionally simple for the first UI pass. It works in any editor layout
    # and gives us a stable UI surface before moving to a full dockable Slate tab.
    unreal.EditorDialog.show_message(WINDOW_TITLE, body, unreal.AppMsgType.OK)


def scan_game_and_open():
    return _run_scan("/Game", 5000)


def scan_stylescape_and_open():
    return _run_scan("/Game/StylescapeHD", 2500)


def copy_report_path():
    return _copy_report_path()


def locate_first_issue():
    return _locate_first_error_or_warning()
