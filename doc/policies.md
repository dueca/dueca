# Coding policies support and enforcement {#policies}

DUECA policy support can detect code patterns and suggest or implement
improvements in projects.

## Introduction

DUECA projects can use parts of other DUECA projects. This promotes
code re-use, and modular design. However, project interdependency can
also grow in uncontrolled ways, resulting in odd connections where
parts of obsolete projects (DCO files or modules) are being used. This
became clear in the effort to transport DUECA code at Control and
Simulation from a CVS repository to a more modern GIT repository.

Policy support is added to the new dueca-gproject project script for
configuring git+cmake -based DUECA projects. Policies can specify
changes to use DCO files from a better base project, substitute
obsolete borrowed modules with more up-to-date and modern equivalents,
replace certain old code patterns and the like. The support is aimed
at keeping code across projects streamlined.

## Invocation

The dueca-gproject script, which provides support for DUECA projects
that use GIT as a software repository, and CMAKE for the build
configuration, has an option for policy checking and
implementation. When run from within a project folder:

    dueca-gproject policies

The code and configuration in the project folder is checked against
configured policies. These policies are described in XML files; by
default, policies can be installed in several locations:

- The `dueca-gproject policies` command can be invoked with one or
  more `--policiesurl` arguments, to indicate the locations of policy
  files.

- If no argument for policy is given, or the `--include-default`
  argument is given, the following default locations are searched:

  * In the user's home folder; ${HOME}/.config/dueca/policies.xml
  * From the environment variable DUECA_POLICIES

## Background

The policy checking system is expandable, so the capabilities might
change in the future, but the following is roughly possible:

- Check whether a project uses (either borrowed or 'own') a specific
  module.

- Check whether a module uses a specific dco file.

- Check for a certain pattern in files.

These checks can be logically combined. For example, for streamlining
the use of projects with common modules for outside visual (WorldView)
and 3D audio (WorldListener), that use a set of common DCO files, all
non-audio related DCO files should be defined in WorldListener. A
historic situation had evolved in which 3D motion would use a mix of
DCO files -- linked with inheritance -- from both projects.

The policy checks for a project:

- Whether it uses either WorldView or WorldListener

- Whether it uses the old mixture of DCO files from both these projects.

A set of actions is defined which correct this situation:

- DCO objects borrowed from WorldListener are now modified to DCO
  objects borrowed from WorldView

- The implementation of this policy, and which files were modified, is
  recorded in the file policies.xml in the &lt;projectdir&gt;/.config
  folder.

Particularly for cases where many projects use an old, mistaken
approach, it is advantageous to implement the checks and policies in
policy files.

## Writing policies

A policy consists of three parts:

- A description for the policy

- A condition, or test, which, when it evaluates to true, indicates
  that the policy should be applied. The condition typically has
  by-products that can be used later, in the action stage, to
  implement changes to the files.

- One or more actions that describe how files are to be modified for
  the implementation of the policy.

Note that "evaluates to true" is a complex concept here. Suppose you
modified a dco file, so that it now inherits from second dco
object. You need to add that second dco object to the comm-objects.lst
files in all of the modules that use the first dco object. To do this,
you would need to:

- Figure out which comm-objects.lst file use your dco file, let's call
  it BaseProject/comm-objects/Venerable.dco

- Figure out which comm-objects.lst files do not yet include the new
  parent, BaseProject/comm-objects/NewParent.dco

- Combine that information, to find a list of comm-objects.lst files
  that need correcting.

- Use the list to be corrected, and implement the changes.

To do this logic on lists, you can supply which variables are used to
match two lists of information on dco files.

These policies are described in xml files. The main structure of the
file is given as:

    <policies xmlns="https://dueca.tudelft.nl"
              xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
              xsi:schemaLocation="https://dueca.tudelft.nl policies.xsd">
      <policy name="a name" id="a unique id">
        ...
      </policy>
    </policies>

A policy file may contain multiple policies. It also may contain
include statements to import policies from other files, using the
`policyfile` tag.

Here is an example of a condition:

    <condition type="and">
      <param name="inputvar">
	    <!-- These are two variables that contain the result of embedded
		     policy tests -->
        files_that_include,cmake_locations
      </param>
	  <!-- for the resulting list produced by this "and" condition, we
	       will use the filename data from the cmake_locations inputs.
		   That means the combined result will contain the filenames of
		   the cmake files, not from the files with the include statements -->
      <param name="result-filename">
        cmake_locations
      </param>
	  <!-- likwise for the indexes into the text indicating a match -->
      <param name="result-matches">
        cmake_locations
      </param>
	  <!-- combine the incoming lists (from grep-ing the cxx|hxx files for
		   includes, and the CMakeLists.txt) on the module level, so form
		   groups of files that are from the same module -->
      <param name="match">
        module
      </param>
	  <!-- produce the result in include_spots. Should be a list indicating
	       the CMakeLists.txt files in the filename, and indicating where to
		   modify in matches -->
      <param name="resultvar">
        include_spots
      </param>
	  <!-- This excludes the CMakeLists.txt that already seem to have the
		   modification -->
      <condition type="not">
        <param name="inputvar">
          cmakelists_with_include
        </param>
        <param name="outputvar">
          not_included_yet
        </param>
		<!-- detect the right include -->
        <condition type="find-pattern">
          <param name="fileglob">
            */CMakeLists.txt
          </param>
          <param name="pattern">
            SRSMotion/motion-common
          </param>
          <param name="outputvar">
            cmakelists_with_include
          </param>
        </condition>
      </condition>
	  <!-- check all cxx and hxx files that include GenericMotionFilter -->
      <condition type="find-pattern">
        <param name="fileglob">
          */*.?xx
        </param>
        <param name="pattern">
          #include "GenericMotionFilter\.hxx"
        </param>
		<!-- result in "files_that_include" -->
        <param name="resultvar">
          files_that_include
        </param>
      </condition>
	  <!-- find where the INCLUDEDIRS word is -->
      <condition type="find-pattern">
        <param name="fileglob">
          */CMakeLists.txt
        </param>
        <param name="pattern">
          \n[ \t]*INCLUDEDIRS[^\n]*\n
        </param>
        <param name="resultvar">
          cmake_locations
        </param>
      </condition>
    </condition>

As you can see, it is a compound "and" condition. Parameters control
the behaviour of the conditions. To communicate between conditions and
between conditions and actions, result variables are used. Each result
variable describes the result of a specific test, typically in the
form of the file name of the file that matches the test, locations
where in the file the match is found, etc.

In the example above, regular expression patterns are searched in
files. The "fileglob" parameter indicates which files to check, the
"pattern" indicates what text to check for.

The resultvar will be a list of files that match the pattern, together
with information on which line or lines are matched. After running its
two subordinate condition, the "and" condition has access to the
`cmake_locations` and `files_that_include` variables.

In this particular example, the "and" condition matches these two
variables on the module level, so that a single "positive and" is
generated per module, where at least one file includes
"GenericMotionFilter.hxx", and there is a `CMakeLists.txt` file with
the INCLUDEDIRS keyword. This produces per matching module an
`include_spots` result, that points to the filename from the
`cmake_locations` variable and the line numbers (matches), also of the
`cmake_locations` variable.

The action here is to add an include line to the `CMakeLists.txt` file:

    <action type="insert-text">
      <param name="inputvar">
        include_spots
      </param>
      <param name="substitutevar">
        found_module
      </param>
      <param name="text">
    # policy 21-005
    $\{CMAKE_SOURCE_DIR\}/../SRSMotion/motion-common
      </param>
    </action>
    <action type="change-module">
      <param name="new_project">
        SRSMotion
      </param>
      <param name="new_module">
        motion-common
      </param>
    </action>

This uses the `include_spots` variable, that indicates all
`CMakeLists.txt` files that need the include, together with the
appropriate locations. The second action globally adds a module to the
project.
