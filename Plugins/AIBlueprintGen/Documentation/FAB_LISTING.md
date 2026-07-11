# AI Blueprint Generator — Fab Marketplace Listing

---

## Short Description (150 chars max)
Generate Blueprints from plain language using AI. Describe it, review the generated script, create it in your project — all inside the editor.

---

## Long Description

**Describe it. Review it. Create it.**

AI Blueprint Generator turns natural language descriptions into Unreal Engine Blueprints. Type what you want — "a health pickup that rotates, plays a sound on overlap and destroys itself" — and the AI (Claude or OpenAI, your choice, your API key) writes an Unreal Python script that creates the Blueprint. You review the script, edit it if you want, and run it with one click.

Great for scaffolding: parent class set up, variables added and typed, asset saved in the right folder with the right prefix — the boilerplate is done, you add the gameplay logic.

---

### How it works

1. Open **AI Blueprint Generator** from the Tools menu
2. Pick your provider and paste your API key
3. Choose a parent class (Actor, Pawn, Character...) and target folder
4. Describe the Blueprint in plain language
5. Click **Generate**, review the script in the output panel
6. Click **Create Blueprint in Editor** — done, it's in your Content Browser

**You always see the script before it runs.** Nothing executes without your click, and you can edit the script directly in the panel or copy it out.

---

### What it generates

- Blueprints with the correct parent class (Actor, Pawn, Character, ActorComponent...)
- Typed member variables (float, int, bool, vectors, object references)
- Correct UE5 naming (BP_, ABP_, WBP_ prefixes)
- Compiled and saved assets, ready to open and extend

---

### Privacy & Cost

- Only your **description, parent class and folder path** are sent to the AI — never project files
- Bring your own API key (Anthropic or OpenAI); keys are stored locally in your user settings, never in the project
- A typical generation costs less than a cent

---

### Technical Details

- **Engine:** Unreal Engine 5.4, 5.5, 5.7, 5.8
- **Type:** Editor Plugin (does not ship with your game)
- **Language:** C++ (Slate UI), generates Unreal Python scripts
- **Dependencies:** Python Script Plugin (included with UE5, enabled by default)
- **Network:** Internet connection + API key required for generation only

---

### Perfect for

- Rapid prototyping and game jams
- Teaching/learning Blueprint structure
- Scaffolding repetitive Blueprint setups
- Technical designers automating asset creation

---

## Tags
AI tool, blueprint generator, automation, natural language, editor tool, productivity, prototyping, scaffolding, code plugin, python

---

## Category
Code Plugins → Editor Extensions
