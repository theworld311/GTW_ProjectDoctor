# AI Blueprint Generator

Generate Unreal Engine Blueprints from natural language. Describe what you want ("a pickup item with a mesh, rotation over time and an overlap sphere that destroys the actor"), review the generated script, and create the Blueprint in your project with one click.

Powered by Claude (Anthropic) or OpenAI — you choose the provider and use your own API key.

---

## Requirements

- Unreal Engine 5.4+ (built and tested on 5.7)
- **Python Script Plugin** enabled (ships with UE5, enabled by default)
- An API key from [Anthropic](https://platform.claude.com/) or [OpenAI](https://platform.openai.com/)

---

## Quick Start

1. Enable the plugin: **Edit → Plugins → AI Blueprint Generator** → restart the editor
2. Open the panel: **Tools → AI Blueprint Generator**
3. Choose your **Provider** (Claude or OpenAI) and paste your **API key** — stored locally in your per-project user settings, never embedded in the plugin or your project
4. Pick a **Parent Class** (Actor, Pawn, Character...) and a **Save Folder** (e.g. `/Game/Blueprints`)
5. Describe the Blueprint you want in plain language
6. Click **Generate** — the AI writes an Unreal Python script that creates the Blueprint
7. **Review the script** in the output panel (you can edit it before running)
8. Click **Create Blueprint in Editor** — the script runs and your Blueprint appears in the Content Browser

---

## Important — review before running

The generated script is shown to you **before** anything executes, and nothing runs until you click the create button. Always skim the script first — it is standard `unreal.*` Python API code (create asset, add variables, compile, save). You can edit it directly in the output panel, or **Copy** it to use elsewhere.

---

## Privacy & Cost

- Only your **description, parent class and folder path** are sent to the AI provider — never project files or assets
- You pay the provider directly through your own API key; a typical generation costs less than a cent
- Default models: `claude-opus-5` (Claude) / `gpt-4o` (OpenAI) — configurable in **Project Settings → Plugins → AI Blueprint Generator**

---

## Support

- Issues and questions: https://github.com/theworld311/GTW_ProjectDoctor/issues

---

Copyright (c) 2026 GTW Dev. All Rights Reserved.
