name: Bug Report
description: Create a report to help us improve
title: "[Bug]: "
labels: ["Bug"]
body:
  - type: markdown
    attributes:
      value: |
        Thanks for taking the time to fill out this bug report! Please make sure you provide all the requested information to help us address the issue efficiently.

  - type: checkboxes
    id: prerequisites
    attributes:
      label: Prerequisites
      description: Please confirm these before submitting your issue
      options:
        - label: I have checked that my issue doesn't exist yet in the [issue tracker](https://github.com/alliedmodders/sourcemod/issues)
          required: true

  - type: input
    id: os-version
    attributes:
      label: Operating System and Version
      description: What operating system and version are you using?
      placeholder: "e.g., Windows 10 21H2, Ubuntu 22.04"
    validations:
      required: true

  - type: input
    id: game-version
    attributes:
      label: Game / AppID and Version
      description: Which game and version are you using?
      placeholder: "e.g., CS:GO (730) version 1.38.2.5"
    validations:
      required: true

  - type: input
    id: sourcemod-version
    attributes:
      label: SourceMod Version
      description: On which version of SourceMod did you first observe this?
      placeholder: "e.g., 1.11.0.6936"
    validations:
      required: true

  - type: input
    id: metamod-version
    attributes:
      label: Metamod:Source Version
      description: On which version of Metamod:Source did you first observe this?
      placeholder: "e.g., 1.11.0-git1145"

  - type: checkboxes
    id: version-checks
    attributes:
      label: Version Verification
      description: Please confirm you've tested with the latest versions
      options:
        - label: I have updated SourceMod to the [latest version](https://www.sourcemod.net/downloads.php) and the issue persists
        - label: I have updated SourceMod to the [latest snapshot](https://www.sourcemod.net/downloads.php?branch=dev) and the issue persists
        - label: I have updated Metamod:Source to the [latest snapshot](https://sourcemm.net/downloads.php?branch=dev) and the issue persists

  - type: input
    id: sourcemod-version-latest
    attributes:
      label: Updated SourceMod Version
      description: If you updated SourceMod to the latest version to confirm the issue persisted, which version was that?
      placeholder: "e.g., 1.11.0.6936"

  - type: input
    id: metamod-version-latest
    attributes:
      label: Updated Metamod:Source Version
      description: If you updated Metamod:Source to the latest version to confirm the issue persisted, which version was that?
      placeholder: "e.g., 1.11.0-git1145"  

  - type: textarea
    id: description
    attributes:
      label: Description
      description: Please provide a clear and concise description of the bug
      placeholder: What happened? What did you expect to happen?
    validations:
      required: true

  - type: textarea
    id: reproduction
    attributes:
      label: Steps to Reproduce
      description: Please provide the code or steps needed to reproduce the issue
      placeholder: |
        *If possible, please include the smallest possible test-case to reproduce the issue.*

        *Also, please make it clear whether or not you can consistently reproduce this issue with the provided description*

        1. First step...
        2. Second step...
        3. ...
        
        ```sourcepawn
        // Add your code here if applicable
        ```

  - type: textarea
    id: logs
    attributes:
      label: Relevant Log Output
      description: |
        Please provide any relevant log output. This will be automatically formatted into code, so no need for backticks.
        Include:
        - Game output
        - Library logs
        - Kernel logs
        - Minidump or dump analyze output (in case of crashes)
      render: shell
