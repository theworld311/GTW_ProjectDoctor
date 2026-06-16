# Project Doctor — Asset Quality Scanner for Unreal Engine 5

**Version:** 1.0.0  
**Engine:** Unreal Engine 5.4+  
**Category:** Editor Tools  

---

## What is Project Doctor?

Project Doctor is an **asset quality scanner** that runs directly inside the Unreal Editor. It automatically inspects your project's assets and reports naming violations, missing LODs, texture issues, Blueprint compile errors, broken references, and much more — all in a clean, dockable panel.

No external tools. No command line. Just open the panel, click **Scan /Game**, and get a full diagnostic report in seconds.

---

## Features

### Asset Scanners
| Asset Type | What is checked |
|---|---|
| **Texture2D / TextureCube** | Resolution (4K/8K warnings), non-power-of-two, sRGB mismatch, normal map naming, mip settings |
| **Static Mesh** | Missing simple collision, single LOD, lightmap UV on channel 0, Nanite vs LOD |
| **Skeletal Mesh** | Missing LODs, missing Physics Asset, missing Skeleton |
| **Material** | Two-sided flag, translucency, missing Material Instances |
| **Material Instance** | Missing parent material (broken asset) |
| **Blueprint / AnimBP / WidgetBP** | Compile errors, deprecated parent class, uncategorised variables |
| **Sound Wave / Sound Cue** | Naming prefix, long sounds without streaming enabled |
| **Niagara / Particle System** | Naming prefix, missing fixed bounds |
| **All asset types** | Naming convention validation (18 asset types covered) |
| **Redirectors** | Detects stale redirectors that should be fixed before packaging |

### Naming Conventions Enforced
| Prefix | Asset Type |
|---|---|
| `SM_` | Static Mesh |
| `SK_` | Skeletal Mesh |
| `T_` | Texture2D, TextureCube |
| `RT_` | Render Target |
| `M_` | Material |
| `MI_` | Material Instance |
| `BP_` | Blueprint |
| `ABP_` | Anim Blueprint |
| `WBP_` | Widget Blueprint |
| `A_` | Sound Wave, Sound Cue |
| `AS_` | Anim Sequence |
| `AM_` | Anim Montage |
| `BS_` | Blend Space |
| `PHYS_` | Physics Asset |
| `DT_` | Data Table |
| `DA_` | Data Asset |
| `E_` | User Defined Enum |
| `F_` | User Defined Struct |
| `NS_` | Niagara System |
| `NE_` | Niagara Emitter |

### UI Features
- **Dockable panel** — opens via Tools menu, docks anywhere in the editor layout
- **Summary cards** — live counters for Assets Scanned, Total Findings, Errors, Warnings, Info
- **Filter buttons** — filter findings by All / Errors / Warnings / Info instantly
- **Double-click to navigate** — double-click any finding to highlight the asset in the Content Browser
- **Custom path scan** — scan any sub-folder of /Game, not just the whole project
- **HTML report** — open a full formatted report in your browser with one click
- **CSV and JSON export** — integrate reports into your pipeline or spreadsheets

---

## Installation

1. Copy the `ProjectDoctor` folder into your project's `Plugins/` directory
2. Open your project in Unreal Engine 5
3. When prompted to compile, click **Yes**
4. Go to **Edit → Plugins** and confirm **Project Doctor** is enabled

---

## How to Use

### Opening the Panel
Go to **Tools → Project Doctor** in the main menu bar.  
The panel will open as a dockable tab — you can drag it anywhere in your editor layout.

### Running a Scan
- Click **Scan /Game** to scan your entire project
- Click **Scan Custom Path...** to scan only a specific folder (e.g. `/Game/Characters`)
- Click **Refresh** to reload the last report without rescanning

### Reading Results
Each finding shows:
- **Severity** — Error (must fix), Warning (should fix), Info (review before shipping)
- **Rule** — what category of check triggered
- **Asset** — full content path of the affected asset
- **Message** — clear explanation of what was found

### Navigating to an Asset
Double-click any row to instantly select the asset in the Content Browser.

### Opening the HTML Report
Click **Open HTML Report** to open a full formatted report in your browser.  
Reports are also saved as `.json` and `.csv` in:
```
[YourProject]/Saved/ProjectDoctorReports/
```

---

## Severity Guide

| Severity | Meaning | Action |
|---|---|---|
| **Error** | Asset is broken or will cause packaging failure | Fix before packaging |
| **Warning** | Issue that will likely affect performance or correctness | Fix before shipping |
| **Info** | Convention violation or item to review | Review before publishing to Fab/marketplace |

---

## Requirements

- Unreal Engine 5.4 or newer
- Python Script Plugin enabled (enabled by default in UE5)
- Visual Studio 2022 (only needed if the plugin needs to be recompiled)

---

## Frequently Asked Questions

**Q: Will this slow down my editor?**  
A: No. The scan runs once when you click the button. It does not run in the background or affect editor performance.

**Q: Can I scan only part of my project?**  
A: Yes. Use **Scan Custom Path...** and enter any `/Game/` sub-path.

**Q: Where are the reports saved?**  
A: In `[YourProject]/Saved/ProjectDoctorReports/`. Each scan creates a timestamped file plus a `latest_` copy.

**Q: Does this work with Fab/marketplace projects?**  
A: Yes. It scans any asset in the Content Browser regardless of where it came from.

**Q: Can I ignore certain rules?**  
A: Rule filtering is on the roadmap for v1.1. For now, use the Info filter to hide lower-priority items.

---

## Roadmap

- [ ] Per-rule suppression (ignore specific rules for specific folders)
- [ ] Scheduled auto-scan on editor startup
- [ ] Custom naming prefix configuration
- [ ] CI/CD integration via command-line mode

---

## Support

Found a bug or have a suggestion? Please leave a review on the Fab marketplace page or contact us through the Epic Games support portal.

---

## Changelog

### v1.0.0
- Initial release
- 8 asset type scanners
- 18 naming convention rules
- Dockable Slate UI with filters and Content Browser navigation
- HTML, JSON, CSV report export
