# Some hints for using git with dueca-gproject {#githints}

## Introduction

The files for a DUECA project are in most cases arranged in a specific
structure, so that standard build tools supplied with DUECA can
compile and link the code, and (distributed) simulations can easily be
set-up and run. The structure of these projects, and the use of the
`dueca-gproject` script to create and update such a structure, is
described in a [chapter on application development](appdevelg). To
keep track of changes in a project, the `git` version control system
is used by `dueca-gproject`. In the development work on a project, the
developer will use a few `dueca-gproject` commands that interact with
the version control system, for further version control interaction,
`git` will be directly used. This chapter gives some recipes for
managing the version control of a DUECA project.



## Git interaction through dueca-gproject

Only a few of the git manipulations are done through
the `dueca-gproject` script itself, notably:

- Creating a new project, and adding needed project files. Example:

      dueca-gproject new --name MyNewProject \
          --remote git@gitlab.tudelft.nl:<folder>/MyNewProject.git

  Note that this needs an empty repository. This command creates and
  adapts the necessary files for a new project, and then checks these in
  to the given repository.

- Cloning a project from the repository, including pulling *borrowed*
  modules from that repository.

       dueca-gproject clone --remote \
           git@gitlab.tudelft.nl:<folder>/MyNewProject.git

  This command clones the given project from the repository, and inspects
  the appropriate `modules.xml` and all `comm-objects.lst` to find out
  whether there are borrowed DCO objects and borrowed modules from other
  repositories and creates a (sparse) clone of these as well.

- Updating the borrowed modules in an existing project. This inspects
  and updates the borrowed modules, to reflect any changes in those
  modules that have been committed to the repository since the
  previous clone or fetch action.

      dueca-gproject refresh

- Adding a new 'own' module in a project:

      dueca-gproject new-module --name MyNewModule

  This adds the files for the new module from their templates, and
  add these to the version control.

- Borrowing a module from another project:

      dueca-gproject borrow-module --name BorrowedModule \
          --remote git@gitlab.tudelft.nl:<folder>/LendingProject.git

  In this case the configuration files are adapted, and the borrowed
  module is cloned from the repository.

For all other operations, the user is expected to directly use the git
version control system itself. This is possible because the work done
by a developer will be on the code of the cloned project, which can be
completely controlled by the `git` version control system.

## Branches, repositories, development tree

Git is a very powerful tool for version control, having a lot of
flexibity in its set-up. This can make using git for version control
daunting at first. As a developer, you interact with a number of
different copies of your file:

- The files you edit, in the project tree. These files are "checked
  out". Note that you may also have files there that are not known to
  git, if you want their version controlled, use `git add <filenam>`
  to tell git.

- The files in your "local repository", a local "state of the version
  controlled files" maintained by git.

- A local copy of one (or more, or zero) remote repositories.

- The remote repository, typically on a server.

Each repository can have several branches; a branch is usually used to
separate different paths in the development of software. The main
branch is commonly called "master".

There are some git steps to take to keep all these software versions
aligned, first we discuss the interaction between your local
repository and the files in your project tree:

- To update the files in your project tree (the ones you actually
  edit), with a specific branch of development, use:

      git checkout <some branch>

  If you want to start developing in a new branch, use:

      git checkout -b <my new branch>

  That starts a new branch from the point that you had currently checked
  out, for example from default branch `master`.

- To mark a new file for inclusion in the branch, do:

      git add <the new file>

- To record all changes you had into your local reposotory, use:

      git commit -a

  Git will ask you for a commit message, if you want to give that on
  the command line, add an option: `-m "some commit
  message"`. Properly read git's output, it might inform you of files
  that it sees in your local file tree, but that are not included in
  the branch. 


Note that this is by no means complete, and there is a wealth of
better git documentation available to properly learn git, however,
we'll make do with these basics for now.

Now for the communication between your local repository, and the
remote repositories (and implicitly also with your local version of
the remote repositories).

Suppose you created a new branch `deploy-on-simona`, for recording all
your actions for deployment on the SIMONA Research Simulator. You
would want to have all that work also on the remote repository. After
doing your commits of this work, use the following:

    git push -u origin

With `origin` the shorthand name for the remote repository. This will
create the `deploy-on-simona` branch on the remote repository, link
your local `deploy-on-simona` with the `origin/deploy-on-simona`, and
transfer the commits to the remote server. Note that the link between
the branches is essential to make your life easier; git now knows that
any pushes and pulls (see below), from your local branch use this
remote branch.

You would have typically started this deployment on the SRS on one of
the computers, and created the branch there. You now need these
developments on the other computers in SIMONA as well.

On the next computer, you need this software and branch as well. If
you did not yet have the project there, use:

    dueca-gproject clone \
        --remote git@gitlab.tudelft.nl:<repository>/MyProject.git \
        --version deploy-on-simona

If the project is already there, use git, from the project folder:

    git fetch origin

This will retrieve the latest changes from the remote repository. Git
will tell you that there is a new branch since you last compared your
copy and the remote repository, now ensure that you also have this
branch in your checked-out folder:

    git checkout --track origin/deploy-on-simona

Now you have a local branch checked out, and linked to the branch on
the remote repository. Any pushes will again end up on the remote branch.

It is important to keep these changes synchronized, so `commit` and
`push` after completing edits, and `pull` before starting your work.

### Pull versus fetch and merge

A `git fetch` will only affect your copy of a remote repository. Your
checked out files, and your local branches, will remain the same. To
bring your local branches and checked-out copy in line with the
remote, you would need to combine those changes, typically with a `git
merge`, or with a `git rebase`; from your checked-out local branch,
and using the previous example:

    git merge origin/deploy-on-simona

Since this set of operations is practically always needed when you
continue work, this is combined in the `git pull` command. Note that
by default a git pull works for the currently checked-out branch;
after switching branches, it does not hurt to run a `git pull`.

## Development scenarios

### New student project

Note that starting with a completely new project is rare, but it
happens. Steps:

- Ask a staff member to create an repository for you.

- Use the `dueca-gproject new` command to create the initial project.

- Create a branch for your development, do not work on master

- Use `git add <file>` when adding new files to your project

- Use `git commit` to record significant steps in development.

- When continuing development on another computer, or when you want to
  share the code with your supervisors or collaborators, or to have a
  back-up after a day of work, use a `git push` to update the software
  in the repository.

- After (known or suspected) changes in the remote repository, use a
  `git pull` to get these in your working files.

### Branching of an existing project

When you are continuing with an existing project, you can create a
branch in that project and work on that branch. This is useful if
after your work, your changes can be fed back into the existing
project, e.g., to implement a new experiment with an existing project,
or add a particular module.

- Ask a staff member to give you access to the project

- Use the `dueca-project clone` command to get your copy of the software.

- Create one or more branches for your development, in the name of the
  branch, indicate which project/variant you are working on.

The remainder of the steps are common with the previous case.

### Archiving a (student) project

The git@gitlab.tudelft.nl:ae-cs-dueca-archive folder is intended for
keeping completed DUECA projects. To archive a project there, or
anywhere else, take the following steps:

- Ensure you have the required version cloned

- Add a remote:

      git remote add archive \
          git@gitlab.tudelft.nl:ae-cs-dueca-archive/MyProject.git

- Checkout and push all branches

      git push --all archive
