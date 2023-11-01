# More Simple Simulation (#example2b)

## Introduction

The [A Simple Simulation](#example2) chapter explained how to create a
DUECA simulation with one "player", re-use existing modules, create new
modules, and configure a simulation for running on a multi-computer set-up.

Here we look at how to connect multiple simulations together. In this way,
a multiplayer simulation can be made. Each player has his/her own DUECA 
controls, and can start and run independently from the other players. 
However, data between players can be exchanged, so it is possible to 
"see" the other players, by, e.g., showing their avatars / graphical models
in the world visualisation. 

We will start by adding a simple module to our simulation to track whoever
is currently "in the game". Then we discuss ways of configuring the multi-
player simulation. 

## Overview module

To track whas is happening, we will create an overview module. Use the 
`dueca-gproject` command to get a new module called `monitor-teams`. Now 
enter the `monitor-teams` folder and 