
import importlib
import os
import sys
import unreal

_THIS_DIR = os.path.dirname(__file__)
if _THIS_DIR not in sys.path:
    sys.path.append(_THIS_DIR)

OWNER = "ProjectDoctor"


def _load_scanner():
    import project_doctor
    importlib.reload(project_doctor)
    return project_doctor


def _format_summary(result):
    summary = result.get("summary", {})
    reports = result.get("reports", {})
    return "\n".join([
        "Project Doctor scan complete.",
        "",
        f"Root: {summary.get('root_path', '-')}",
        f"Assets scanned: {summary.get('scanned_assets', 0)}",
        f"Findings: {summary.get('findings_total', 0)}",
        f"Errors: {summary.get('errors', 0)}",
        f"Warnings: {summary.get('warnings', 0)}",
        f"Info: {summary.get('info', 0)}",
        "",
        "Reports:",
        reports.get("html", "No HTML report"),
        reports.get("json", "No JSON report"),
        reports.get("csv", "No CSV report"),
    ])


def scan_root(root_path="/Game", max_assets=5000, show_dialog=True):
    scanner = _load_scanner()
    result = scanner.scan_project(root_path=root_path, max_assets=max_assets, export=True)
    if show_dialog:
        unreal.EditorDialog.show_message(
            "Project Doctor",
            _format_summary(result),
            unreal.AppMsgType.OK,
        )
    return result


def scan_game():
    return scan_root("/Game", 5000, True)


def scan_stylescape():
    return scan_root("/Game/StylescapeHD", 5000, True)


def scan_custom():
    result = unreal.EditorDialog.show_message(
        "Project Doctor",
        "Custom scan will use /Game as root with a limit of 10 000 assets.\n\nProceed?",
        unreal.AppMsgType.YES_NO,
    )
    if result == unreal.AppReturnType.YES:
        return scan_root("/Game", 10000, True)


def _python_command(function_name):
    return (
        "import sys, os, unreal; "
        "p=os.path.join(unreal.Paths.project_dir(), 'Content', 'Python', 'ProjectDoctor'); "
        "sys.path.append(p) if p not in sys.path else None; "
        "import project_doctor_menu; "
        f"project_doctor_menu.{function_name}()"
    )


def _add_entry(menu, section, name, label, tooltip, command):
    entry = unreal.ToolMenuEntry(
        name=name,
        type=unreal.MultiBlockType.MENU_ENTRY,
        insert_position=unreal.ToolMenuInsert("", unreal.ToolMenuInsertType.DEFAULT),
    )
    entry.set_label(label)
    entry.set_tool_tip(tooltip)
    entry.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", command)
    menu.add_menu_entry(section, entry)


def register_menus():
    menus = unreal.ToolMenus.get()
    tools_menu = menus.extend_menu("LevelEditor.MainMenu.Tools")

    try:
        tools_menu.add_section("ProjectDoctorSection", "Project Doctor")
    except Exception:
        pass


    _add_entry(
        tools_menu,
        "ProjectDoctorSection",
        "ProjectDoctor.OpenWindow",
        "Project Doctor: Open Window",
        "Open the Project Doctor report window.",
        "import sys, os, unreal; p=os.path.join(unreal.Paths.project_dir(), 'Content', 'Python', 'ProjectDoctor'); sys.path.append(p) if p not in sys.path else None; import project_doctor_ui; project_doctor_ui.open_window()",
    )

    _add_entry(
        tools_menu,
        "ProjectDoctorSection",
        "ProjectDoctor.LocateFirstIssue",
        "Project Doctor: Locate First Issue",
        "Locate the first issue from the latest Project Doctor report in the Content Browser.",
        "import sys, os, unreal; p=os.path.join(unreal.Paths.project_dir(), 'Content', 'Python', 'ProjectDoctor'); sys.path.append(p) if p not in sys.path else None; import project_doctor_ui; project_doctor_ui.locate_first_issue()",
    )

    _add_entry(
        tools_menu,
        "ProjectDoctorSection",
        "ProjectDoctor.CopyReportPath",
        "Project Doctor: Copy Report Path",
        "Copy the latest HTML/JSON/CSV report path to the clipboard.",
        "import sys, os, unreal; p=os.path.join(unreal.Paths.project_dir(), 'Content', 'Python', 'ProjectDoctor'); sys.path.append(p) if p not in sys.path else None; import project_doctor_ui; project_doctor_ui.copy_report_path()",
    )

    _add_entry(
        tools_menu,
        "ProjectDoctorSection",
        "ProjectDoctor.ScanGameDirect",
        "Project Doctor: Scan /Game",
        "Scan the whole /Game content folder and export Project Doctor reports.",
        _python_command("scan_game"),
    )

    _add_entry(
        tools_menu,
        "ProjectDoctorSection",
        "ProjectDoctor.ScanStylescapeDirect",
        "Project Doctor: Scan /Game/StylescapeHD",
        "Scan only the StylescapeHD test asset folder.",
        _python_command("scan_stylescape"),
    )

    menus.refresh_all_widgets()
    unreal.log("Project Doctor direct menu entries registered under Tools.")


register_menus()
