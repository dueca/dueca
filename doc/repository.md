# Repository set-up at Control & Simulation {#reposetup}

This documentation is specific to the Control and Simulation division
at the Aerospace Engineering department of the Delft University of
Techology, the place where DUECA was created. However, it might offer
some inspiration to others who want to organise their software
repositories for DUECA projects. It is currently (2021) a work in
progress, since we are just transitioning from old CVS-based projects
to git+CMake based projects.

## Facilitating module borrowing

Facilitating software re-use was one of the design goals for the DUECA
middleware. In DUECA, two types of re-use are dominant, the first is
by "borrowing" modules from other DUECA projects, and the second form
is by adopting DCO objects, often implicitly by borrowing a module.

Sharing and borrowing code led to the following good:

- Common modules that are re-used in many projects.

- Commonly used DCO objects are available that create a more or less
  stable interface between different borrowed modules (notably across
  joystick input and control loading devices).

Some less favorable aspects are:

- DCO objects are still being borrowed from otherwise long forgotten
  student projects, no longer maintained but ubiquitous across many
  newer projects.

- Hacks are sometimes needed to squeeze the data in the current DCO
  objects. Changes in DCO objects are limited, to not affect large
  numbers of projects.

- Some projects still use obsolete variants of generic support modules
  (e.g., the "joystick" and "multi-stick"), which are no longer maintained.

- Some developers tended to copy rather than borrow modules, resulting
  in projects having copied modules with no or little changes; leading
  also to problems in maintenance of these projects.

With the transition from a CVS-based repository, which combined all
projects in one, to git-based repositories, with information on the
git url's for each software module, the DUECA projects of our group no
longer need to be all concentrated in the same location. This leads to
some choices for the use of repository and the relation between
projects.

## Design and working methods

To promote the re-use of general, curated DUECA modules, modules that
are commonly used in different projects can be organised in a separate
group on the repository server, at
```git@gitlab.tudelft.nl:ae-cs-dueca-base/```.

Git also offers better means to keep software variants in different
branches, and merge the results together. This enables quicker
incorporation of student work into a "reference" version of a
project. The proposed working method for students who continue with a
project from a predecessor or a reference project, would then be to
clone the project to a repository under their own username, and
perform work on one or more branches that are clearly identifyable as
belonging to the student. After completion of the project, the
branches may be merged back into the reference project. When the
capabilities of the reference project can be enhanced by the student's
work, the student's branch may be merged into the main project,
otherwise the branch might be kept as a separate branch in the main
project.

That still leaves us, for the time being, with a number of projects
converted from the CVS repository to git, which are not "cleaned up"
yet. The following division in groups is proposed for the DUECA
software on `gitlab.tudelft.nl`. That implies that the software can be
found at a url
```https://gitlab.tudelft.nl/<<group>>/<<Project>>.git```, or as
```git@gitlab.tudelft.nl:<<group>>/<<Project>>.git``` (with write access)
where
`<<group>>` is the group's name, but all in lower case, and
`<<Project>>` is the DUECA project name; these may be mixed, lower or
upper case.

<table>
<tr><th>Group/Repository</th><th>Purpose and approach</th></tr>

<tr><td>AE-CS-DUECA-base </td><td> Generic base projects, providing
services like motion filtering, control device IO, outside visual
images, etc. These projects are generally maintained by Control and
Simulation staff. </td></tr>

<tr><td>AE-CS-DUECA-active </td><td> Working projects that are kept
active on the simulation facilities, such as simulations now also used
for demonstration or testing. </td></tr>

<tr><td>AE-CS-DUECA-ftis </td><td> Projects for the Flight Test
Instrumentation and fly-by-wire system.  </td></tr>

<tr><td>AE-CS-DUECA-archive </td><td> Finished student projects, kept
as reference for future projects, but generally not actively
maintained on the simulation facilities. </td></tr>

<tr><td>AE-CS-DUECA-yard </td><td> Older projects, resulting from the
current transition from CVS to git, that are needed to run some of the
active, older or current projects. The yard needs cleaning up, and
with time the DCO objects and sometimes modules borrowed from these
projects are moved into base projects, while the projects themselves
are moved to the archive. </td></tr>

<tr><td>Netid folders </td><td> When working on their graduation
projects, students can use a clone from one of the projects in the
above groups as reference, or start a new project, and store it under
their own netid at the gitlab server. After completion of the project
the result is merged back into the donating project or added to the
archive. </td></tr>

</table>

The page on [Policies](@ref policies) describes how policy support in `dueca-gproject` might support automated changes to project code.
