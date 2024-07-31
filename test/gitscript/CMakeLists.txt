find_package(Python3 COMPONENTS Interpreter)
find_package(Git)
find_program(XMLLINT xmllint)

if(NOT Python3_Interpreter_FOUND)
  message(STATUS "No python3 interpreter, old cmake?")
  find_program(Python3_Interpreter python3)
  if(Python3_Interpreter)
    set(Python3_Interpreter_FOUND TRUE)
  endif()
endif()

# to run the tests
# ninja test/gitscripts/test
#
# to check the results
# more test/gitscripts/test/Testing/Temporary/LastTest.log


if(Python3_Interpreter_FOUND)

  set(DUECA_GPROJECT
    ${CMAKE_SOURCE_DIR}/gitscripts/dueca-gproject.py --verbose)

  # set up two empty git repositories; clean them out if they already
  # exist
  add_test(NAME prepare_clean
    COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/testarea
    ${CMAKE_CURRENT_BINARY_DIR}/testarea2
    ${CMAKE_CURRENT_BINARY_DIR}/testarea3
    ${CMAKE_CURRENT_BINARY_DIR}/TestProject1.git
    ${CMAKE_CURRENT_BINARY_DIR}/TestProject2.git
    ${CMAKE_CURRENT_BINARY_DIR}/otherrepo)

  # make the folders
  add_test(NAME prepare_folders
    COMMAND mkdir ${CMAKE_CURRENT_BINARY_DIR}/testarea
    ${CMAKE_CURRENT_BINARY_DIR}/testarea2
    ${CMAKE_CURRENT_BINARY_DIR}/testarea3
    ${CMAKE_CURRENT_BINARY_DIR}/TestProject1.git
    ${CMAKE_CURRENT_BINARY_DIR}/TestProject2.git
    ${CMAKE_CURRENT_BINARY_DIR}/otherrepo
    ${CMAKE_CURRENT_BINARY_DIR}/otherrepo/TestProject3.git)

  # let git init empty repos there
  add_test(NAME prepare_repo1
    COMMAND ${GIT_EXECUTABLE} init --bare
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/TestProject1.git")
  set(REPO1 "file://${CMAKE_CURRENT_BINARY_DIR}/TestProject1.git")

  add_test(NAME prepare_repo2
    COMMAND ${GIT_EXECUTABLE} init --bare
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/TestProject2.git")
  set(REPO2 "file://${CMAKE_CURRENT_BINARY_DIR}/TestProject2.git")

  add_test(NAME prepare_repo3
    COMMAND ${GIT_EXECUTABLE} init --bare
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/otherrepo/TestProject3.git")
  set(REPO3 "file://${CMAKE_CURRENT_BINARY_DIR}/otherrepo/TestProject3.git")

  # environment variable to be used with repo3
  set(ENV{DAPPS_GITROOT} file://${CMAKE_CURRENT_BINARY_DIR}/otherrepo/)
  # and a named one
  set(ENV{DAPPS_GITROOT_try} file://${CMAKE_CURRENT_BINARY_DIR}/)

  # create a new project with the dueca-gproject script
  add_test(NAME gitscript_new
    COMMAND ${DUECA_GPROJECT} new --name TestProject1 --remote "${REPO1}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/testarea)

  # create a second new project
  add_test(NAME gitscript_new2
    COMMAND ${DUECA_GPROJECT} new --name TestProject2 --remote "${REPO2}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/testarea)

  # create a module in the project
  add_test(NAME gitscript_newmodule
    COMMAND ${DUECA_GPROJECT} new-module --name TestModule1
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1)

  # verify that the module has been added to the .config
  add_test(NAME verify_newmodule
    COMMAND grep TestModule1
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1/.config/class/solo/modules.xml)

  # commit the changes
  add_test(NAME git_commit1
    COMMAND git commit -a -m "ADDED MODULE"
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1)

  # now try to git push it
  add_test(NAME git_push1
    COMMAND git push origin
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1)

  # borrow this module in the other project
  add_test(NAME gitscript_borrowmodule
    COMMAND ${DUECA_GPROJECT}
    borrow-module --name TestModule1
    --remote "${REPO1}"
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject2/TestProject2)

  # verify the module has been added to the .config
  add_test(NAME verify_borrowmodule1
    COMMAND grep "TestModule1"
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject2/TestProject2/.config/class/solo/modules.xml)

  # verify the module has been checked out
  add_test(NAME verify_borrowmodule2
    COMMAND test -d
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject2/TestProject1/TestModule1)

  # copy a module, go to a new name
  add_test(NAME gitscript_copymodule1
    COMMAND ${DUECA_GPROJECT}
    copy-module --name TestModule1
    --newname TestModule1Copy
    --remote "${REPO1}"
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject2/TestProject2)

  # verify the presence of the module
  add_test(NAME verify_copymodule1
    COMMAND grep "TestModule1Copy"
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject2/TestProject2/.config/class/solo/modules.xml)

  # check the presence of files
  add_test(NAME verify_copymodule2
    COMMAND test -f
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject2/TestProject2/TestModule1Copy/comm-objects.lst)

  # create a new module
  add_test(NAME prepare_addcode1
    COMMAND new-module dueca SomeModule "do something"
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1/TestModule1)

  # add the files through git
  add_test(NAME prepare_gitaddcode1
    COMMAND git add SomeModule.?xx
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1/TestModule1)

  # modify its code
  add_test(NAME modify_newmodule
    COMMAND ${Python_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/testcoder.py
    --target SomeModule.cxx
    --mods ${CMAKE_CURRENT_SOURCE_DIR}/SomeModule.cxx.txt
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1/TestModule1)

  # add a dco file
  add_test(NAME prepare_adddco
    COMMAND ${Python_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/testcoder.py
    --target Some.dco
    --source ${CMAKE_CURRENT_SOURCE_DIR}/Some.dco
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1/comm-objects)

  add_test(NAME prepare_adddco1
    COMMAND git add Some.dco
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1/comm-objects)

  add_test(NAME prepare_adddco2
    COMMAND bash -c
    "echo TestProject1/comm-objects/Some.dco >> comm-objects.lst"
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1/TestModule1)
  # add a dco file

  add_test(NAME prepare_adddco_
    COMMAND ${Python_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/testcoder.py
    --target Other.dco
    --source ${CMAKE_CURRENT_SOURCE_DIR}/Other.dco
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject2/TestProject2/comm-objects)

  add_test(NAME prepare_adddco1_
    COMMAND git add Other.dco
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject2/TestProject2/comm-objects)

  add_test(NAME prepare_adddco2_
    COMMAND bash -c
    "echo TestProject2/comm-objects/Other.dco >> comm-objects.lst"
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1/TestModule1)

  add_test(NAME prepare_gitcommit2
    COMMAND git commit -a -m "added some dco"
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject2/TestProject2)

  # now try to git push it
  add_test(NAME git_push2
    COMMAND git push origin
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject2/TestProject2)

  add_test(NAME prepare_gitcommit
    COMMAND git commit -a -m "added some module"
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1/TestModule1)

  add_test(NAME pepare_gitpush1b
    COMMAND git push
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1/TestModule1)

  add_test(NAME gitscript_refresh
    COMMAND
    ${DUECA_GPROJECT} refresh --auto-find-url
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject2/TestProject2)

  add_test(NAME gitscript_borrowproject
    COMMAND
    ${DUECA_GPROJECT} borrow-project --remote ${REPO2}
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1)

  add_test(NAME gitscript_refreshb
    COMMAND
    ${DUECA_GPROJECT} refresh --auto-borrow-for-dco --auto-find-url
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1)

  add_test(NAME gitscript_newplatform
    COMMAND
    ${DUECA_GPROJECT} new-platform --name SRS
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1)

  add_test(NAME gitscript_trycompile
    COMMAND cmake ..
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1/build)

  add_test(NAME gitscript_trycompile2
    COMMAND make
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1/build)

  add_test(NAME gitscript_newnode0
    COMMAND
    ${DUECA_GPROJECT} new-node --name srsecs
    --platform SRS --num-nodes 2 --node-number 0 --cmaster srsctrlecat
    --gui gtk3
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1)

  add_test(NAME gitscript_newnode1
    COMMAND
    ${DUECA_GPROJECT} new-node --name srsctrlecat
    --platform SRS --num-nodes 2 --node-number 1
    --gui none
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1)

  add_test(NAME gitscript_newmachineclass
    COMMAND
    ${DUECA_GPROJECT} new-machine-class --name ig
    --gui none
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1)

  add_test(NAME verify_newmachineclass
    COMMAND
    test -d
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1/.config/class/ig)

  add_test(NAME gitscript_configure2
    COMMAND cmake ..
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject2/TestProject2/build)

  add_test(NAME gitscript_prepareplatform
    COMMAND ${DUECA_GPROJECT} prepare-platform --name srs
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject2/TestProject2)


  # check a node added to machineclass
  add_test(NAME gitscript_prepareplatform_check_mapping
    COMMAND grep srsctrlecat .config/machinemapping.xml
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject2/TestProject2)

  # check appropriate modules added
  add_test(NAME gitscript_prepareplatform_check_modules
    COMMAND grep IOController .config/class/srs-io/modules.xml
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject2/TestProject2)

  # check config modified
  add_test(NAME gitscript_prepareplatform_check_config
    COMMAND grep HardwareInterface .config/class/srs-io/config.cmake
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject2/TestProject2)

  add_test(NAME prepare_gitcommit1c
    COMMAND git commit -a -m "compiles now"
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1/TestModule1)

  add_test(NAME gitscript_push1c
    COMMAND git push origin
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1)

  add_test(NAME gitscript_tryinforun
    COMMAND bash -c "DUECA_SCRIPTINSTRUCTIONS=Environment ./dueca_run.x"
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1/build)

  # check out a cloned project
  add_test(NAME gitscript_clone
    COMMAND ${DUECA_GPROJECT}
    clone --remote ${REPO1} --node srsecs
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea2)

  # verify the clone
  add_test(NAME gitscript_clone_diff
    COMMAND diff -r --exclude=build --exclude=.git --exclude=*~ --exclude=*.bak??? --exclude=comm-objects.h
    testarea/TestProject1 testarea2/TestProject1
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR})

  # create a project using a dgr shorthand url
  add_test(NAME gitscript_new3
    COMMAND
    ${DUECA_GPROJECT} new --name TestProject3 --remote dgr:///TestProject3.git
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea)
  set_property(TEST gitscript_new3 PROPERTY ENVIRONMENT
    "DAPPS_GITROOT=file://${CMAKE_CURRENT_BINARY_DIR}/otherrepo/")

  # first a borrow-project, will need for DCO
  add_test(NAME gitscript_borrowproject3
    COMMAND
    ${DUECA_GPROJECT} borrow-project --remote dgrtry:///TestProject2.git
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject3/TestProject3)
  set_property(TEST gitscript_borrowproject3 PROPERTY ENVIRONMENT
    "DAPPS_GITROOT_try=file://${CMAKE_CURRENT_BINARY_DIR}/")

  # and borrow with another shorthand url
  add_test(NAME gitscript_borrowmodule3
    COMMAND
    ${DUECA_GPROJECT} borrow-module --name TestModule1
    --remote dgrtry:///TestProject1.git
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject3/TestProject3)
  set_property(TEST gitscript_borrowmodule3 PROPERTY ENVIRONMENT
    "DAPPS_GITROOT_try=file://${CMAKE_CURRENT_BINARY_DIR}/")

  # verify that the module has been added to the .config
  add_test(NAME verify_newmodule3
    COMMAND grep TestModule1
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject3/TestProject3/.config/class/solo/modules.xml)

  # check the base functionality for reading&interpreting comm-objects
  # files
  add_test(NAME scriptchange_readcommobjects
    COMMAND ${Python_EXECUTABLE} -c
    "from duecautils.commobjects import CommObjectsList; a = CommObjectsList('${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1/TestModule1')")

  # check for policies
  add_test(NAME gitscript_checkpolicies
    COMMAND ${DUECA_GPROJECT}
    policies --policiesurl
    file://${CMAKE_CURRENT_SOURCE_DIR}/example-policies.xml
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1)

  # do this on a branch
  add_test(NAME gitscript_tobranch
    COMMAND ${GIT_EXECUTABLE} checkout -b policies
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1)

  add_test(NAME gitscript_applypolicies
    COMMAND ${DUECA_GPROJECT}
    policies --policiesurl
    file://${CMAKE_CURRENT_SOURCE_DIR}/example-policies.xml
    --apply 21-001
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1)

  add_test(NAME gitscript_applypolicies2
    COMMAND ${DUECA_GPROJECT}
    policies --policiesurl
    file://${CMAKE_CURRENT_SOURCE_DIR}/example-policies.xml
    --apply 21-001
    --force
    --include-default
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1)

  add_test(NAME gitscript_ignorepolicies
    COMMAND ${DUECA_GPROJECT}
    policies --policiesurl
    file://${CMAKE_CURRENT_SOURCE_DIR}/example-policies.xml
    --skip 21-002
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1)

  add_test(NAME gitscript_remainingpolicies
    COMMAND ${DUECA_GPROJECT}
    policies --policiesurl
    file://${CMAKE_CURRENT_SOURCE_DIR}/example-policies.xml
    --apply-all
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1)

  add_test(NAME gitscript_commitbranch
    COMMAND ${GIT_EXECUTABLE} commit -a -m "applied policies on policies branch"
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1
  )

  add_test(NAME gitscript_pushbranch
    COMMAND ${GIT_EXECUTABLE} push --set-upstream origin policies
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1
  )

  # clone the versioned/branched
  add_test(NAME gitscript_clone_branch
    COMMAND ${DUECA_GPROJECT}
    clone --remote ${REPO1} --version policies
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/testarea3)

  # and check that it matches
  add_test(NAME gitscript_clone_diffbranched
    COMMAND diff -r --exclude=build --exclude=.git --exclude=*~ --exclude=*.bak??? --exclude=comm-objects.h
    testarea/TestProject1 testarea3/TestProject1
    WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR})

  add_test(NAME lint_modules
    COMMAND ${XMLLINT} --schema
    ${CMAKE_SOURCE_DIR}/gitscripts/default/modules.xsd
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1/.config/class/solo/modules.xml)

  add_test(NAME lint_machinemapping
    COMMAND ${XMLLINT} --schema
    ${CMAKE_SOURCE_DIR}/gitscripts/default/machinemapping.xsd
    ${CMAKE_CURRENT_BINARY_DIR}/testarea/TestProject1/TestProject1/.config/machinemapping.xml)

endif()
