# GTW Editor Tools for Unreal Engine

Development home and support hub for the **GTW Dev** editor plugins for Unreal Engine 5.

| Plugin | What it does | Docs |
|--------|--------------|------|
| 🩺 **Project Doctor** | Asset quality scanner: naming violations, missing LODs, broken assets, texture issues, Blueprint compile errors and more — in a single dockable panel with HTML/CSV/JSON reports | [README](Plugins/ProjectDoctor/Documentation/README.md) |
| 🏷️ **AI Asset Namer** | Renames assets automatically using AI (Claude or OpenAI). Suggests correct UE5 prefixes (`SM_`, `T_`, `M_`, `BP_`...) for any selection or folder — review and apply in one click | [README](Plugins/AIAssetNamer/Documentation/README.md) |
| 🧩 **AI Blueprint Generator** | Generates Blueprints from natural language descriptions. Describe what you want, review the generated script, create it in your project | [README](Plugins/AIBlueprintGen/Documentation/README.md) |

---

## Requirements

- **Unreal Engine 5.4+** (built and tested on 5.7)
- **Python Script Plugin** enabled (ships with UE5, enabled by default)
- For the AI plugins: an API key from [Anthropic](https://platform.claude.com/) or [OpenAI](https://platform.openai.com/) — bring your own key, stored locally, never embedded in your project

## Installation

**From Fab (recommended):** install through the Epic Games Launcher and enable the plugin under **Edit → Plugins**.

**From source (this repository):**
1. Copy the plugin folder (e.g. `Plugins/ProjectDoctor`) into your project's `Plugins/` directory
2. Regenerate project files and build (C++ project required), or launch the editor and accept the rebuild prompt
3. Enable the plugin under **Edit → Plugins → Editor** and restart

## Privacy

The AI plugins send only **asset names/types or your text description** to the AI provider you configure — never your project files, assets or source. API keys are stored in your per-project user settings (`Saved/Config`), which are not part of your project or this repository.

## Support

Found a bug or have a feature request? [Open an issue](https://github.com/theworld311/GTW_ProjectDoctor/issues) — please include your engine version and, when relevant, the Output Log.

---

Copyright (c) 2026 GTW Dev. All Rights Reserved.
