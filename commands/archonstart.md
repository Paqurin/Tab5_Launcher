# Archon Start Command

This is a custom slash command for Claude Code that initializes Archon-driven task workflow.

## Command Configuration

```json
{
    "name": "archonstart",
    "description": "Initialize Archon-driven task workflow with full integration",
    "prompt": "# Archon Integration & Workflow\n\n**CRITICAL: This project uses Archon for knowledge management, task tracking, and project organization.**\n\n## Core Archon Workflow Principles\n\n### The Golden Rule: Task-Driven Development with Archon\n\n**MANDATORY: Always complete the full Archon task cycle before any coding:**\n\n1. **Check Current Task** → `archon:manage_task(action=\"get\", task_id=\"...\")`\n2. **Research for Task** → `archon:search_code_examples()` + `archon:perform_rag_query()`\n3. **Implement the Task** → Write code based on research\n4. **Update Task Status** → `archon:manage_task(action=\"update\", task_id=\"...\", update_fields={\"status\": \"review\"})`\n5. **Get Next Task** → `archon:manage_task(action=\"list\", filter_by=\"status\", filter_value=\"todo\")`\n6. **Repeat Cycle**\n\n### Task Management Rules:\n- **Update all actions to Archon** - Never skip task updates\n- **Move tasks from \"todo\" → \"doing\" → \"review\"** (not directly to complete)\n- **Maintain task descriptions** and add implementation notes\n- **DO NOT MAKE ASSUMPTIONS** - check project documentation for questions\n- **Never code without checking current tasks first**\n\n## Current Task:\n$ARGUMENTS\n\n## Research & Implementation Requirements:\n\n### MCP Server Usage:\n- **Use `exa` and `ref` MCP servers** for technical research\n- **Use `fetch`, `puppeteer`, `serena`, and `brave` MCP servers** as needed for data gathering\n- **Store all research findings in Archon MCP** for future reference\n\n### Agent Delegation:\n- **Delegate specialized work to appropriate agents** (rapid-prototyper, mobile-app-builder, etc.)\n- **Use the custom \"build-commands\"** for ESP-IDF operations\n- **If a function is missing, attempt to create it** rather than disable it and ask\n\n### Documentation & Cleanup:\n- **Document all related file changes** made during the task\n- **Cleanup file structure and code artifacts** when done\n- **Update CLAUDE.md** if workflow patterns change\n\n## ESP-IDF Project Context:\n- **Environment**: `. ~/esp/esp-idf-5.4.1/export.sh`\n- **Build**: `CC=distcc CXX=distcc++ idf.py build`\n- **Flash**: `CC=distcc CXX=distcc++ idf.py flash`\n- **Always ask permission** before flashing or pushing to git\n\n---\n\n**Now beginning Archon task cycle for:** $ARGUMENTS"
}
```

## Usage

Once this command is configured in Claude Code, you can use it like:

```
/archonstart Implement WiFi scanning functionality for Tab5 Launcher
```

This will initialize the full Archon workflow with the specified task and ensure all proper protocols are followed.

## Key Features

- ✅ **Archon MCP Integration** - Full task lifecycle management
- ✅ **Research Protocol** - Systematic documentation gathering
- ✅ **Agent Delegation** - Specialized task routing
- ✅ **ESP-IDF Context** - Project-specific build commands
- ✅ **Safety Protocols** - Permission-based operations
- ✅ **Documentation Requirements** - Comprehensive change tracking