# Project Doctor editor menu
import sys, os, unreal
_project_doctor_path = os.path.join(unreal.Paths.project_dir(), 'Content', 'Python', 'ProjectDoctor')
if _project_doctor_path not in sys.path:
    sys.path.append(_project_doctor_path)
try:
    import project_doctor_menu
except Exception as exc:
    unreal.log_warning(f'Project Doctor menu failed to load: {exc}')
