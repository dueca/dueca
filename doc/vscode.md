# Editor support for VSCode / Codium {#vscode}

## Why?

[Visual Studio Code](https://code.visualstudio.com/) is an increasingly popular editing and coding tool. When working on a DUECA project, it can give you visual code formatting, indicate syntax and coding errors, and support building and debugging your code. For those reasons (and because I am also using it), some support was added to the `dueca-gproject` script for optimally using vscode.

## Basic installation (Ubuntu/Debian)

You can head over to the [Visual Studio](https://code.visualstudio.com/) website, but I prefer to install the open-source version of this editor, [VSCodium](https://vscodium.com/). This variant does not include the tracking and "call home" functionality of Visual Studio. To install it on Ubuntu or Debian, run the following (see also instructions on the website):

    wget -qO - https://gitlab.com/paulcarroty/vscodium-deb-rpm-repo/raw/master/pub.gpg \
        | gpg --dearmor \
        | sudo dd of=/usr/share/keyrings/vscodium-archive-keyring.gpg

    echo 'deb [ signed-by=/usr/share/keyrings/vscodium-archive-keyring.gpg ] https://download.vscodium.com/debs vscodium main' \
        | sudo tee /etc/apt/sources.list.d/vscodium.list

    sudo apt update && sudo apt install codium

## Extensions

To get optimum functionality, some additional software and plugins in Codium are needed. First, for C++ formatting and error checking support, install:

    sudo apt install clang format

Then I recomment the following extensions, using the extension manager ("four blocks" icon in VSCode/Codium):

- "Native Debug", by webfreak
- "Python", by ms-python
- "Black Formatter", by ms-python
- "Markdown All in One", by yzhang
- "CMake", by twxs
- "clangd", by llvm-vs-code-extensions
- "Scheme", by jeandeaual

## Use

Before use of the extensions and settings, first do a one-time configure (and compile) of your project, and install default configuration files for your VSCode/Codium, in your project folder, run:

    # configure & compile
    dueca-gproject build --debug

    # default configuration files
    dueca-gproject build --vscode

If you have multiple projects, repeat this for each project. You should now have the following files there (do not enter into git!):

    compile_commands.json
    .clang-format
    .vscode/launch.json
    .vscode/tasks.json
	.vscode/settings.json

Run codium in your project by starting, from the project folder, with:

	codium .

### Formatting

With a C++ file in the editor, you can now press (Ctrl+Shift+I) to run the formatting.

### Build

From any file, you can run (Ctrl+Shift+B) to build your project.

### Debug run

Using the "Run and Debug" icon, or (Ctrl+Shift+D), you can start the debugger. Debugging will be done for the solo/solo node.
